#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3

void fisheye_square_half_mask(double * mask, int width, double r, double m) {
    geometry_t g = {
        (int) ceil(2. * r),
        (int) ceil(2. * r),
        {r, r}
    };
    point_t *c = point_new(0., 0.), *nc;
    polar_t *p, *np;
    int x, y, x2, y2, ycol, xcol;
    double dx, dy;
    for (y=0; y < width; y++) {
        c->y = (double) y;
        ycol = y * width << 1;
        y2 = y << 1;
        for (x = y; x < width; x++) {
            c->x = (double) x;
            p = geometry_polar_from_point(&g, c);

            if (p->r < r) {
                np = unmagnify(p, r, m);
                nc = geometry_point_from_polar(&g, np);
                dx = nc->x - c->x; // nc->x = x + dx
                dy = nc->y - c->y; // nc->y = y + dy

                free(nc);
                free(np);
                xcol = x * width << 1;
                x2 = x << 1;

                mask[ycol + x2] = dx;
                mask[ycol + x2 + 1] = dy;
                if (x != y) {
                    mask[xcol + y2] = dy;
                    mask[xcol + y2 + 1] = dx;
                }
            }
            free(p);
        }
    }
    free(c);
}

void
fisheye_inplace_sub(Bitmap* img, const point_t *c, const double* dv) {
    double dx, dy, idx, idy, r0, g0, b0, r1, g1, b1;
    unsigned int nx, ny, mx, my,
        x = c->x * COLORS,
        width = img->width * COLORS;
    unsigned char *data0, *data1, *to, r, g, b;
    double cx = c->x + dv[0],
        cy = c->y + dv[1];

    nx = floor(cx);
    ny = floor(cy);
    mx = ceil(cx);
    my = ceil(cy);
    dx = cx - nx;
    dy = cy - ny;
    idx = 1 - dx;
    idy = 1 - dy;
    nx *= COLORS;
    mx *= COLORS;
    // rows
    data0 = &(img->data[ny * width]);
    data1 = &(img->data[my * width]);
    to = &(img->data[(int)(c->y * width)]);
    // intermediary points
    r0 = idx * data0[nx] + dx * data0[mx + 0];
    g0 = idx * data0[nx + 1] + dx * data0[mx + 1];
    b0 = idx * data0[nx + 2] + dx * data0[mx + 2];
    r1 = idx * data1[nx] + dx * data1[mx + 0];
    g1 = idx * data1[nx + 1] + dx * data1[mx + 1];
    b1 = idx * data1[nx + 2] + dx * data1[mx + 2];
    // final points
    r = idy * r0 + dy * r1;
    g = idy * g0 + dy * g1;
    b = idy * b0 + dy * b1;
    to[x] = r;
    to[x + 1] = g;
    to[x + 2] = b;
}

void
fisheye_inplace_from_square_half_mask_with_quadrant(Bitmap* img, double* mask,
        unsigned int mask_width, unsigned int quadrant) {
    point_t *c = point_new(0, 0), *r = point_new(0, 0);
    unsigned int x, y, yy,
        width = img->width,
        height = img->height;

    double* dv;
    for (y = 0; y < height; y++) {
        c->y = y;
        r->y = height - 1 - y;
        yy = y * mask_width << 1;
        for (x = 0; x < width; x++) {
            dv = &(mask[yy + (x << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                // FIXME
                //if (quadrant == 0) {
                    c->x = x;
                    fisheye_inplace_sub(img, c, dv);
                //}
                // North-East
                /*if (quadrant == 3) {
                    dv[0] = -dv[0];
                    c->x = width - 1 - x;
                    fisheye_inplace_sub(img, c, dv);
                }
                // South-East
                if (quadrant == 2) {
                    dv[0] = -dv[0];
                    dv[1] = -dv[1];
                    r->x = width - 1 - x;
                    fisheye_inplace_sub(img, r, dv);
                }
                // South-West
                if (quadrant == 1) {
                    dv[1] = -dv[1];
                    r->x = x;
                    fisheye_inplace_sub(img, r, dv);
                }
                */
            }
        }
    }

    free(c);
    free(r);
}

#define TOTAL 4 // number of quadrant
#define LEN 100 // size of the buffer

int
main(int argc, char** argv) {
    int rank, gsize, saved=1;
    MPI_Status status;
    MPI_Request requests[2 * TOTAL];
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &gsize);

    char packbuf[LEN];
    int packsize, position, len;

    Bitmap *img;
    double radius, magnify_factor;
    double *mask;
    unsigned int mask_width, mask_size, width, height, depth, x0, y0,
        quadrant, zero=0;
    unsigned char *buf;

    double t0, t1, t2, t3, t4;

    if (rank == 0) {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
            return 1;
        }

        t0 = MPI_Wtime();
        img = loadBitmapHeaderOnly(argv[1]);
        if (img == NULL) {
            MPI_Abort(comm, 1);
            return 1;
        }
        width = img->width,
        height = img->height;
        depth = img->depth;
        destroyBitmap(img);

        radius = std::min(height, width) * .45;
        magnify_factor = 5.0;
        if (argc > 3) {
            sscanf(argv[3], "%lf", &radius);
            if (radius <= 0) {
                std::cerr << "Radius cannot be null or negative" << std::endl;
                return 1;
            }
        }
        if (argc > 4) {
            sscanf(argv[4], "%lf", &magnify_factor);
            if (magnify_factor < 1) {
                std::cerr << "Less than 1 magnify lens are not supported" << std::endl;
                return 1;
            }
        }

        mask_width = ceil(std::min(std::min(width, height)/2., radius));
        mask_size = mask_width * mask_width << 1;

        packsize = 0;
        MPI_Pack(&width, 1, MPI_INT, packbuf, LEN, &packsize, comm);
        MPI_Pack(&height, 1, MPI_INT, packbuf, LEN, &packsize, comm);
        MPI_Pack(&depth, 1, MPI_INT, packbuf, LEN, &packsize, comm);
        MPI_Pack(&mask_width, 1, MPI_INT, packbuf, LEN, &packsize, comm);
        MPI_Pack(&radius, 1, MPI_DOUBLE, packbuf, LEN, &packsize, comm);
        MPI_Pack(&magnify_factor, 1, MPI_DOUBLE, packbuf, LEN, &packsize, comm);
    }

    MPI_Bcast(&packsize, 1, MPI_INT, 0, comm);
    MPI_Bcast(packbuf, packsize, MPI_PACKED, 0, comm);

    if (rank == 0) {
        Bitmap* img = loadBitmap(argv[1]);
        if (img == NULL) {
            MPI_Abort(comm, 1);
            return 1;
        }
        //saveBitmap("2.bmp", img);
        t1 = MPI_Wtime();
        // Divide the picture in 4.
        // 0 | 3
        // --+--
        // 1 | 2
        len = mask_width * mask_width * COLORS;
        unsigned int y, w2, h2;
        unsigned char *buf0, *buf1;
        buf0 = (unsigned char *) calloc(sizeof(unsigned char), len);
        buf1 = (unsigned char *) calloc(sizeof(unsigned char), len);

        w2 = (int) ceil(width / 2.);
        h2 = (int) ceil(height / 2.);

        x0 = std::max(zero, w2 - mask_width);
        y0 = std::max(zero, h2 - mask_width);

        for(y = 0; y < mask_width; y++) {
            std::copy(
                &(img->data[((y + y0) * width + x0) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + mask_width + 1) * COLORS - 1]),
                &(buf0[y * mask_width * COLORS]));
            std::copy(
                &(img->data[((y + y0) * width + x0 + mask_width) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + 2 * mask_width + 1) * COLORS - 1]),
                &(buf1[y * mask_width * COLORS]));
        }
        int to, req = 0;
        quadrant = 0;
        to = (quadrant % (gsize - 1)) + 1;
        //std::cerr << rank << " sending to " << to << " data: " << len << std::endl;
        MPI_Isend(&quadrant, 1, MPI_INT, to, 0, comm, &(requests[req++]));
        MPI_Isend(buf0, len, MPI_CHAR, to, 1, comm, &(requests[req++]));
        quadrant = 3;
        to = (quadrant % (gsize - 1)) + 1;
        //std::cerr << rank << " sending to " << to << std::endl;
        MPI_Isend(&quadrant, 1, MPI_INT, to, 0, comm, &requests[req++]);
        MPI_Isend(buf1, len, MPI_CHAR, to, 1, comm, &requests[req++]);
        for(y = mask_width; y < 2 * mask_width; y++) {
            std::copy(
                &(img->data[((y + y0) * width + x0) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + mask_width + 1) * COLORS - 1]),
                &(buf0[(y - mask_width) * mask_width * COLORS]));
            std::copy(
                &(img->data[((y + y0) * width + x0 + mask_width) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + 2 * mask_width + 1) * COLORS - 1]),
                &(buf1[(y - mask_width) * mask_width * COLORS]));
        }

        quadrant = 1;
        to = (quadrant % (gsize - 1)) + 1;
        //std::cerr << rank << " sending to " << to << std::endl;
        MPI_Isend(&quadrant, 1, MPI_INT, to, 0, comm, &requests[req++]);
        MPI_Isend(buf0, len, MPI_CHAR, to, 1, comm, &requests[req++]);
        quadrant = 2;
        to = (quadrant % (gsize - 1)) + 1;
        //std::cerr << rank << " sending to " << to << std::endl;
        MPI_Isend(&quadrant, 1, MPI_INT, to, 0, comm, &requests[req++]);
        MPI_Isend(buf1, len, MPI_CHAR, to, 1, comm, &requests[req++]);

        free(buf0);
        free(buf1);
        t2 = MPI_Wtime();
    }

    if (rank > 0 && rank <= TOTAL) {
        position = 0;
        MPI_Unpack(packbuf, packsize, &position, &width, 1, MPI_INT, comm);
        MPI_Unpack(packbuf, packsize, &position, &height, 1, MPI_INT, comm);
        MPI_Unpack(packbuf, packsize, &position, &depth, 1, MPI_INT, comm);
        MPI_Unpack(packbuf, packsize, &position, &mask_width, 1, MPI_INT, comm);
        MPI_Unpack(packbuf, packsize, &position, &radius, 1, MPI_DOUBLE, comm);
        MPI_Unpack(packbuf, packsize, &position, &magnify_factor, 1, MPI_DOUBLE, comm);

        len = mask_width * mask_width * COLORS;
        mask_size = mask_width * mask_width << 1;
        mask = (double *) calloc(sizeof(double), mask_size);

        fisheye_square_half_mask(mask, mask_width, radius, magnify_factor);

        img = createBitmap(mask_width, mask_width, depth);

        for (int i=0; i < TOTAL; i += (gsize - 1)) {
            //std::cerr << rank << ": ready" << std::endl;
            MPI_Recv(&quadrant, 1, MPI_INT, 0, 0, comm, &status);
            MPI_Recv(img->data, len, MPI_CHAR, 0, 1, comm, &status);
            //std::cerr << rank << ": got " << quadrant << " data: " << len << std::endl;

            fisheye_inplace_from_square_half_mask_with_quadrant(img, mask, mask_width, quadrant);
            // DEBUG
            //char *foo = (char *)calloc(sizeof(char), 20);
            //sprintf(foo, "tmp%d.bmp", quadrant);
            //saveBitmap(foo, img);

            MPI_Isend(&quadrant, 1, MPI_INT, 0, 0, comm, &(requests[2*i]));
            MPI_Isend(img->data, len, MPI_CHAR, 0, 1, comm, &(requests[2*i+1]));
        }
        for (int i=0; i < TOTAL; i += (gsize - 1)) {
            MPI_Wait(&(requests[2*i]), &status);
            MPI_Wait(&(requests[2*i+1]), &status);
        }
        free(mask);
        destroyBitmap(img);
    }

    if (rank == 0) {
        buf = (unsigned char *) calloc(sizeof(unsigned char), len);
        for (int i=0; i < TOTAL; i++) {
            MPI_Recv(&quadrant, 1, MPI_INT, MPI_ANY_SOURCE, 0, comm, &status);
            //std::cerr << rank << " got: " << quadrant << std::endl;
            MPI_Recv(buf, len, MPI_CHAR, MPI_ANY_SOURCE, 1, comm, &status);
            for(unsigned int y = y0; y < y0 + mask_width; y++) {
                if (quadrant == 0) {
                    std::copy(
                        &(buf[((y - y0) * mask_width) * COLORS]),
                        &(buf[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[(y * width + x0) * COLORS]));
                } else if (quadrant == 1) {
                    std::copy(
                        &(buf[((y - y0) * mask_width) * COLORS]),
                        &(buf[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[((y + mask_width) * width + x0) * COLORS]));
                } else if (quadrant == 2) {
                    std::copy(
                        &(buf[((y - y0) * mask_width) * COLORS]),
                        &(buf[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[((y + mask_width) * width + x0 + mask_width) * COLORS]));
                } else if (quadrant == 3) {
                    std::copy(
                        &(buf[((y - y0) * mask_width) * COLORS]),
                        &(buf[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[(y * width + x0 + mask_width) * COLORS]));
                }
            }
        }
        free(buf);
        t3 = MPI_Wtime();
        saved = saveBitmap(argv[2], img);
        destroyBitmap(img);
        t4 = MPI_Wtime();

        if (!saved) {
            std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
        } else {
            printf("%s\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\n", argv[1],
                width * height, t4-t0, t1-t0, t2-t1, t3-t2, t4-t3, gsize);
        }
    }

    MPI_Finalize();
    return !saved;
}
