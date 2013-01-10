#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3

#if defined WIN32
inline double round(double x) { return floor(x + 0.5); }
#endif

static void
fisheye_chunk(unsigned char* dst, const Bitmap* src, double radius, double m, unsigned int rank, unsigned int chunk_height) {
    geometry_t geometry = {
        src->width, src->height,
        {round(src->width/2.), round(src->height/2.)}};
    point_t *c = point_new(0., 0.);
    unsigned int x, y, nx, ny,
        y0 = rank * chunk_height,
        y1 = std::min(src->height, (int)((rank+1) * chunk_height)),
        width = src->width * COLORS;
    double dx, dy, r0, g0, b0, r1, g1, b1, r, g, b;
    const unsigned char *data, *data0, *data1;
    unsigned char *to;
    for (y = y0; y < y1; y++) {
        c->y = (double) y;
        to = &(dst[(y - y0) * width]);
        data = &(src->data[y * width]);
        for (x=0; x < width; x+=COLORS) {
            c->x = x / (double) COLORS;
            polar_t *p = geometry_polar_from_point(&geometry, c);
            if (p->r < radius) {
                polar_t *np = unmagnify(p, radius, m);
                point_t *nc = geometry_point_from_polar(&geometry, np);
                nx = floor(nc->x);
                ny = floor(nc->y);
                dx = nc->x - nx;
                dy = nc->y - ny;
                nx *= COLORS;
                // rows
                data0 = &(src->data[ny * width]);
                data1 = &(src->data[(ny + 1) * width]);
                // intermediary points
                r0 = (1 - dx) * data0[nx] + dx * data0[nx + 3];
                g0 = (1 - dx) * data0[nx + 1] + dx * data0[nx + 4];
                b0 = (1 - dx) * data0[nx + 2] + dx * data0[nx + 5];
                r1 = (1 - dx) * data1[nx] + dx * data1[nx + 3];
                g1 = (1 - dx) * data1[nx + 1] + dx * data1[nx + 4];
                b1 = (1 - dx) * data1[nx + 2] + dx * data1[nx + 5];
                // final points
                r = (1 - dy) * r0 + dy * r1;
                g = (1 - dy) * g0 + dy * g1;
                b = (1 - dy) * b0 + dy * b1;
                to[x] = r;
                to[x+1] = g;
                to[x+2] = b;
                free(nc);
                free(np);
            } else {
                to[x] = data[x];
                to[x+1] = data[x+1];
                to[x+2] = data[x+2];
            }
            free(p);
        }
    }
    free(c);
}

int
main(int argc, char** argv) {
    int rank, gsize, saved=1;
    MPI_Status status;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &gsize);

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
        MPI_Abort(comm, 1);
        return 1;
    }

    double t0, t1, t2, t3, t4;
    Bitmap *src, *dst;

    t0 = MPI_Wtime();
    src = loadBitmap(argv[1]);
    if (src == NULL) {
        std::cerr << "Cannot open " << argv[1] << std::endl;
        MPI_Abort(comm, 1);
        return 1;
    }
    unsigned int width = src->width,
                 height = src->height;
    double radius = std::min(height, width) * .45,
           magnify_factor = 5.0;
    if (argc > 3) {
        sscanf(argv[3], "%lf", &radius);
        if (radius <= 0) {
            std::cerr << "Radius cannot be null or negative" << std::endl;
            MPI_Abort(1, comm);
        }
    }
    if (argc > 4) {
        sscanf(argv[4], "%lf", &magnify_factor);
        if (magnify_factor < 1) {
            std::cerr << "Less than 1 magnify lens are not supported" << std::endl;
            MPI_Abort(comm, 1);
            return 1;
        }
    }
    // End of configuration

    unsigned int chunk_height = (unsigned int) ceil(height / (double) gsize),
        chunk_size = width * COLORS * chunk_height;
    unsigned char* chunk = (unsigned char*) calloc(sizeof(unsigned char), chunk_size);
    if (chunk == NULL) {
        std::cerr << "Cannot allocate chunk." << std::endl;
        MPI_Abort(comm, 1);
        return 1;
    }

    // Wait for all the participants to be ready computing.
    MPI_Barrier(comm);
    t1 = MPI_Wtime();
    if (rank > 0) {
        fisheye_chunk(chunk, src, radius, magnify_factor, rank, chunk_height);
        MPI_Send(chunk, chunk_size, MPI_CHAR, 0, 0, comm);
        free(chunk);
    } else {
        dst = createBitmap(width, height, 24);
        fisheye_chunk(dst->data, src, radius, magnify_factor, rank, chunk_height);
    }
    t2 = MPI_Wtime();

    if (rank == 0) {
        unsigned int r, copy_size, last = gsize - 1,
            full_size = width * COLORS * height;
        for (int i=1; i<gsize; i++) {
            MPI_Recv(chunk, chunk_size, MPI_CHAR, MPI_ANY_SOURCE, 0, comm, &status);
            r = status.MPI_SOURCE;
            // The last chunk might be smaller.
            copy_size = chunk_size;
            if (r == last) {
                copy_size -= gsize * chunk_size - full_size;
            }
            std::copy(
                chunk,
                chunk + copy_size,
                dst->data + r * chunk_size);
        }
        free(chunk);
        if (dst == NULL) {
            std::cerr << "Cannot allocate destination picture " << argv[2] << std::endl;
            free(src);
            MPI_Abort(comm, 1);
            return 1;
        }
        t3 = MPI_Wtime();
        saved = saveBitmap(argv[2], dst);
        destroyBitmap(src);
        destroyBitmap(dst);
        t4 = MPI_Wtime();

        if (!saved) {
            std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
        } else {
            // Could us asprintf but it's a GNU thing.
            char *cout = (char *) calloc(sizeof(char), 100);
            sprintf(cout, "%s\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d", argv[1],
                width * height, t4-t0, t1-t0, t2-t1, t3-t2, t4-t3, gsize);
            cout[99] = '\0';
            std::cerr << cout << std::endl;
        }
    } else {
        destroyBitmap(src);
    }
    MPI_Finalize();
    return !saved;
}
