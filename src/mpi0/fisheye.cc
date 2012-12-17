#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3
#define LEN 100

void fisheye_square_half_mask_rank(double * mask, int width, double r, double m, int rank, int size) {
    geometry_t g = {
        (int) ceil(2. * r),
        (int) ceil(2. * r),
        {r, r}
    };
    point_t *c = point_new(0., 0.), *nc;
    polar_t *p, *np;
    int x, y, x2, ycol, y0, y1, chunksize;
    double dx, dy,
        shift = r - width;
    chunksize = (int) ceil(width / (double) (size - 1));
    y0 = (rank - 1) * chunksize;
    y1 = std::min(width, rank * chunksize);
    for (y=y0; y < y1; y++) {
        c->y = (double) y + shift;
        ycol = (y - y0) * width << 1;
        for (x = 0; x < width; x++) {
            c->x = (double) x + shift;
            p = geometry_polar_from_point(&g, c);
            if (p->r < r) {
                np = unmagnify(p, r, m);
                nc = geometry_point_from_polar(&g, np);
                dx = nc->x - c->x; // nc->x = x + dx
                dy = nc->y - c->y; // nc->y = y + dy

                free(nc);
                free(np);
                x2 = x << 1;

                mask[ycol + x2] = dx;
                mask[ycol + x2 + 1] = dy;
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
        width = img->width * COLORS;
    unsigned char *data0, *data1, *to, r, g, b;
    double cx = c->x + dv[0],
        cy = c->y + dv[1];

    nx = (int) floor(cx);
    ny = (int) floor(cy);
    mx = (int) ceil(cx);
    my = (int) ceil(cy);
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
    r = (unsigned char) (idy * r0 + dy * r1);
    g = (unsigned char) (idy * g0 + dy * g1);
    b = (unsigned char) (idy * b0 + dy * b1);
    to[x] = r;
    to[x + 1] = g;
    to[x + 2] = b;
}

void
fisheye_inplace_from_square_half_mask(Bitmap* img, double* mask, unsigned int mask_width) {
    point_t *c = point_new(0, 0), *r = point_new(0, 0);
    unsigned int x, y, x0 = 0, y0 = 0, yy, zero = 0,
        width = img->width,
        height = img->height,
        ycorr = height % 2 ? 0 : 1,
        xcorr = width % 2 ? 0 : 1;

    double* dv;
    if (width / 2 > mask_width) {
        x0 = std::max(zero, ((int) ceil(width / 2.) - mask_width));
    }
    if (height / 2 > mask_width) {
        y0 = std::max(zero, ((int) ceil(height / 2.) - mask_width));
    }

    for (y = 0; y < mask_width; y++) {
        c->y = y + y0;
        r->y = height - ycorr - (y + y0);
        yy = y * mask_width << 1;
        for (x = 0; x < mask_width; x++) {
            dv = &(mask[yy + (x << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                // North-West
                c->x = x + x0;
                fisheye_inplace_sub(img, c, dv);

                // North-East
                dv[0] = -dv[0];
                c->x = width - xcorr - (x + x0);
                fisheye_inplace_sub(img, c, dv);

                // South-East
                dv[1] = -dv[1];
                r->x = width - xcorr - (x + x0);
                fisheye_inplace_sub(img, r, dv);
                dv[0] = -dv[0];

                // South-West
                r->x = x + x0;
                fisheye_inplace_sub(img, r, dv);
                dv[1] = -dv[1];
            }
        }
    }

    free(c);
    free(r);
}

int
main(int argc, char** argv) {
    int rank, gsize, provided, saved = 1;
    MPI_Status status;
    MPI_Comm comm = MPI_COMM_WORLD;
    if (MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided)) {
		std::cerr << "MPI Initialization error" << std::cerr;
		return 1;
	}
	if (provided != MPI_THREAD_MULTIPLE) {
		std::cerr << "MPI must be multithreaded" << std::cerr;
		return 1;
	}
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &gsize);

    unsigned int mask_width, mask_size, submask_size;
    double *mask = NULL, *submask = NULL;
    double radius, magnify_factor;

    char packbuf[LEN];
    int packsize, position;

    Bitmap *img;
    int width, height;
    double t0, t1, t2, t3, t4;
    if (rank == 0) {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
			MPI_Abort(comm, 1);
            return 1;
        }
        t0 = MPI_Wtime();
        img = loadBitmapHeaderOnly(argv[1]);
		if (img == NULL){
			std::cerr << "Cannot load " << argv[1] << std::endl;
			MPI_Abort(comm, 2);
			return 1;
		}
        width = img->width;
        height = img->height;
        radius = std::min(height, width) * .45;
        magnify_factor = 5.0;
        if (argc > 3) {
            sscanf(argv[3], "%lf", &radius);
            if (radius <= 0) {
                std::cerr << "Radius cannot be null or negative" << std::endl;
				MPI_Abort(comm, 1);
                return 1;
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
        mask_width = (int) ceil(std::min(std::min(width, height)/2., radius));
        packsize = 0;
        MPI_Pack(&mask_width, 1, MPI_INT, packbuf, LEN, &packsize, comm);
        MPI_Pack(&radius, 1, MPI_DOUBLE, packbuf, LEN, &packsize, comm);
        MPI_Pack(&magnify_factor, 1, MPI_DOUBLE, packbuf, LEN, &packsize, comm);
    }

    MPI_Bcast(&packsize, 1, MPI_INT, 0, comm);
    MPI_Bcast(packbuf, packsize, MPI_PACKED, 0, comm);
	
    if (rank > 0) {
        position = 0;
        MPI_Unpack(packbuf, packsize, &position, &mask_width, 1, MPI_INT, comm);
        MPI_Unpack(packbuf, packsize, &position, &radius, 1, MPI_DOUBLE, comm);
        MPI_Unpack(packbuf, packsize, &position, &magnify_factor, 1, MPI_DOUBLE, comm);
        mask_size = mask_width * mask_width << 1;
        submask_size = (int) ceil(mask_width / (double) (gsize - 1)) * mask_width << 1;
        submask = (double *) calloc(sizeof(double), submask_size);
        fisheye_square_half_mask_rank(submask, mask_width, radius, magnify_factor, rank, gsize);
        MPI_Send(submask, submask_size, MPI_DOUBLE, 0, 0, comm);
        free(submask);
    }
	
    if (rank == 0) {
        // load the whole file this time.
        destroyBitmap(img);
        img = loadBitmap(argv[1]);
		if (img == NULL) {
			free(mask);
			std::cerr << "Cannot load " << argv[1] << std::endl;
			MPI_Abort(MPI_COMM_WORLD, 1);
			return 1;
		}
        t1 = MPI_Wtime();

		// Joining the mask parts
        mask_size = mask_width * mask_width << 1;
        submask_size = (int) ceil(mask_width / (double) (gsize - 1)) * mask_width << 1;
        // Over provision the mask
        mask = (double *) calloc(sizeof(double), submask_size * (gsize - 1));
        submask = (double *) calloc(sizeof(double), submask_size);
        // We could use Gather here but we want to leverage the loadBitmap time
        for (int i = 1; i < gsize; i++) {
            MPI_Recv(submask, submask_size, MPI_DOUBLE, MPI_ANY_SOURCE, 0, comm, &status);
            std::copy(submask, &submask[submask_size - 1], &mask[(status.MPI_SOURCE - 1) * submask_size]);
        }
        free(submask);
        t2 = MPI_Wtime();

		// Applying the mask
        fisheye_inplace_from_square_half_mask(img, mask, mask_width);
        t3 = MPI_Wtime();

		// Saving the file
        saved = saveBitmap(argv[2], img);
        free(mask);
        destroyBitmap(img);
        t4 = MPI_Wtime();

        if (!saved) {
            std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
        } else {
			char *cout = (char *) calloc(sizeof(char), LEN);
			cout[LEN-1] = '\0';
			sprintf(cout, "%s\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\n", argv[1],
                width * height, t4-t0, t1-t0, t2-t1, t3-t2, t4-t3, gsize);
			std::cout << cout << std::endl;
        }
    }
	// Wait and kill everybody!!
	MPI_Barrier(comm);
	if (rank == 0) {
		std::cout << "Done! Press Ctrl+C if it doesn't end gracefully" << std::endl;
	}
	//MPI_Abort(comm, 0);
	// This would hang!
    MPI_Finalize();
    return !saved;
}
