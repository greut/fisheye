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
        x = (int) c->x * COLORS,
        width = img->width * COLORS,
        height = img->height;
    unsigned char *data0, *data1, *to, r, g, b;
    double cx = c->x + dv[0],
        cy = c->y + dv[1];

    nx = (unsigned int) floor(cx);
    ny = (unsigned int) floor(cy);
    mx = (unsigned int) ceil(cx);
    my = (unsigned int) ceil(cy);
    dx = cx - nx;
    dy = cy - ny;
    idx = 1 - dx;
    idy = 1 - dy;
    nx *= COLORS;
    mx *= COLORS;
    // rows
    // FIXME
    if (ny == height) {
        ny--;
    }
    if (my == height) {
        my--;
    }
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
    r = (unsigned char) (idy * r0 + dy * r1);
    g = (unsigned char) (idy * g0 + dy * g1);
    b = (unsigned char) (idy * b0 + dy * b1);
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
    unsigned int mask_width, mask_size, width, height, depth, x0, y0, zero=0;
    unsigned int quadrants[TOTAL];
    unsigned char *buf[TOTAL];

    double t0, t1, t2, t3, t4;

    if (rank == 0) {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
            return 1;
        }

        t0 = MPI_Wtime();
        img = loadBitmapHeaderOnly(argv[1]);
        if (img == NULL) {
            std::cerr << "Cannot open " << argv[1] << std::endl;
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

        mask_width = (unsigned int) ceil(std::min(std::min(width, height)/2., radius));
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
        unsigned int y, w2, h2, to, req=0;
        buf[0] = (unsigned char *) calloc(sizeof(unsigned char), len);
        buf[1] = (unsigned char *) calloc(sizeof(unsigned char), len);
        quadrants[0] = 0;
        quadrants[1] = 3;

        w2 = (int) ceil(width / 2.);
        h2 = (int) ceil(height / 2.);

        x0 = std::max(zero, w2 - mask_width);
        y0 = std::max(zero, h2 - mask_width);

        for(y = 0; y < mask_width; y++) {
            std::copy(
                &(img->data[((y + y0) * width + x0) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + mask_width + 1) * COLORS - 1]),
                &(buf[0][y * mask_width * COLORS]));
            std::copy(
                &(img->data[((y + y0) * width + x0 + mask_width) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + 2 * mask_width + 1) * COLORS - 1]),
                &(buf[1][y * mask_width * COLORS]));
        }
        for (int i=0; i<TOTAL/2; i++) {
            to = (quadrants[i] % (gsize - 1)) + 1;
            MPI_Isend(&(quadrants[i]), 1, MPI_INT, to, 0, comm, &(requests[req++]));
            MPI_Isend(buf[i], len, MPI_CHAR, to, 1, comm, &(requests[req++]));
        }

        buf[2] = (unsigned char *) calloc(sizeof(unsigned char), len);
        buf[3] = (unsigned char *) calloc(sizeof(unsigned char), len);
        quadrants[2] = 1;
        quadrants[3] = 2;
        for(y = mask_width; y < 2 * mask_width; y++) {
            std::copy(
                &(img->data[((y + y0) * width + x0) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + mask_width + 1) * COLORS - 1]),
                &(buf[2][(y - mask_width) * mask_width * COLORS]));
            std::copy(
                &(img->data[((y + y0) * width + x0 + mask_width) * COLORS]),
                &(img->data[(((y + y0) * width) + x0 + 2 * mask_width + 1) * COLORS - 1]),
                &(buf[3][(y - mask_width) * mask_width * COLORS]));
        }
        for (int i=TOTAL/2; i < TOTAL; i++) {
            to = (quadrants[i] % (gsize - 1)) + 1;
            MPI_Isend(&(quadrants[i]), 1, MPI_INT, to, 0, comm, &requests[req++]);
            MPI_Isend(buf[i], len, MPI_CHAR, to, 1, comm, &requests[req++]);
        }

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

        Bitmap *imgs[TOTAL];

        for (int i=rank; i <= TOTAL; i += (gsize - 1)) {
            imgs[i] = createBitmap(mask_width, mask_width, depth);
            MPI_Recv(&(quadrants[i]), 1, MPI_INT, 0, 0, comm, &status);
            MPI_Recv(imgs[i]->data, len, MPI_CHAR, 0, 1, comm, &status);

            fisheye_inplace_from_square_half_mask_with_quadrant(imgs[i], mask, mask_width, quadrants[i]);

            // DEBUG
            // -----
            // Saves the intermediary picture.
            //char *foo = (char *)calloc(sizeof(char), 20);
            //sprintf(foo, "tmp%d.bmp", quadrants[i]);
            //saveBitmap(foo, imgs[i]);

            MPI_Isend(&(quadrants[i]), 1, MPI_INT, 0, 0, comm, &(requests[2*i]));
            MPI_Isend(imgs[i]->data, len, MPI_CHAR, 0, 1, comm, &(requests[2*i+1]));
        }

        // Because of the asynchronous calls, we have to wait before freeing
        // the send data.
        free(mask);
        for (int i=rank; i <= TOTAL; i += (gsize - 1)) {
            MPI_Wait(&(requests[2*i]), &status);
            MPI_Wait(&(requests[2*i+1]), &status);
            destroyBitmap(imgs[i]);
        }
    }

    if (rank == 0) {
        unsigned char *buffer = (unsigned char *) calloc(sizeof(unsigned char), len);
        for (int i=0; i < TOTAL; i++) {
            MPI_Recv(&(quadrants[i]), 1, MPI_INT, MPI_ANY_SOURCE, 0, comm, &status);
            MPI_Recv(buffer, len, MPI_CHAR, MPI_ANY_SOURCE, 1, comm, &status);

            for(unsigned int y = y0; y < y0 + mask_width; y++) {
                if (quadrants[i] == 0) {
                    std::copy(
                        &(buffer[((y - y0) * mask_width) * COLORS]),
                        &(buffer[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[(y * width + x0) * COLORS]));
                } else if (quadrants[i] == 1) {
                    std::copy(
                        &(buffer[((y - y0) * mask_width) * COLORS]),
                        &(buffer[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[((y + mask_width) * width + x0) * COLORS]));
                } else if (quadrants[i] == 2) {
                    std::copy(
                        &(buffer[((y - y0) * mask_width) * COLORS]),
                        &(buffer[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[((y + mask_width) * width + x0 + mask_width) * COLORS]));
                } else if (quadrants[i] == 3) {
                    std::copy(
                        &(buffer[((y - y0) * mask_width) * COLORS]),
                        &(buffer[((y - y0 + 1) * mask_width - 1) * COLORS]),
                        &(img->data[(y * width + x0 + mask_width) * COLORS]));
                }
            }
        }
        free(buffer);
        for (int i=0; i < TOTAL; i++) {
            MPI_Wait(&(requests[2*i]), MPI_STATUS_IGNORE);
            MPI_Wait(&(requests[2*i+1]), MPI_STATUS_IGNORE);
            free(buf[i]);
        }
        t3 = MPI_Wtime();
        saved = saveBitmap(argv[2], img);
        destroyBitmap(img);
        t4 = MPI_Wtime();

        if (!saved) {
            std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
        } else {
            char *cout = (char *) calloc(sizeof(char), LEN);
            cout[LEN-1] = '\0';
            sprintf(cout, "%s\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d", argv[1],
                width * height, t4-t0, t1-t0, t2-t1, t3-t2, t4-t3, gsize);
            std::cerr << cout << std::endl;
        }
    }

    // Wait and kill everybody!!
    MPI_Barrier(comm);
    MPI_Abort(comm, 0);
    //if (rank == 0) {
    //    std::cout << "Done! Press Ctrl+C if it doesn't end gracefully" << std::endl;
    //}
    //MPI_Abort(comm, 0);
    // This could hang!
    MPI_Finalize();
    return !saved;
}
