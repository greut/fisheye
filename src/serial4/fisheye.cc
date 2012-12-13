#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
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
    double dx, dy,
        shift = r - width;
    for (y=0; y < width; y++) {
        c->y = (double) y + shift;
        ycol = y * width << 1;
        y2 = y << 1;
        for (x = y; x < width; x++) {
            c->x = (double) x + shift;
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
fisheye_inplace_from_square_half_mask(Bitmap* img, double* mask, unsigned int mask_width) {
    point_t *c = point_new(0, 0), *r = point_new(0, 0);
    unsigned int x, y, x0 = 0, y0 = 0, yy, zero = 0,
        width = img->width,
        height = img->height,
        ycorr = height % 2 ? 0 : 1,
        xcorr = width % 2 ? 0 : 1;

    double* dv;
    if (width / 2 > mask_width) {
        x0 = std::max(zero, ((width / 2) - mask_width));
    }
    if (height / 2 > mask_width) {
        y0 = std::max(zero, ((height / 2) - mask_width));
    }

    for (y = 0; y < mask_width; y++) {
        c->y = y + y0;
        r->y = width - ycorr - (y + y0);
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
main(int argc, const char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
        return 1;
    }
    clock_t t0 = clock();
    Bitmap* img = loadBitmap(argv[1]);
    int width = img->width,
        height = img->height;
    clock_t t1 = clock();
    double radius = std::min(height, width) * .45,
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
    unsigned int mask_width = ceil(std::min(std::min(width, height)/2., radius));
    double *mask = (double *) calloc(
        sizeof(double),
        mask_width * mask_width << 1);
    fisheye_square_half_mask(mask, mask_width, radius, magnify_factor);
    clock_t t2 = clock();
    fisheye_inplace_from_square_half_mask(img, mask, mask_width);
    clock_t t3 = clock();
    int saved = saveBitmap(argv[2], img);
    free(mask);
    destroyBitmap(img);
    clock_t t4 = clock();

    if (!saved) {
        std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
    } else {
        std::cout << argv[1] << "\t";
        std::cout << width * height << "\t";
        std::cout << double(t4-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t1-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t2-t1)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t3-t2)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t4-t3)/CLOCKS_PER_SEC << std::endl;
    }
    return saved;
}
