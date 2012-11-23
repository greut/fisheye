#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3
// http://stackoverflow.com/questions/3437404/min-and-max-in-c 
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

// The Fish-Eye table contains relative position of each points, because of the
// symmetry, it can be computed once and translated to the eight parts
//
// X: the destination
// O: the source
// (dx, dy) the vector from dest to source
//
// \ (dx, dy)  | (-dx, dy) /
//  \          |          /
//   \   X     |     X   /
//    \        |        /
//     \   O   |   O   /
//      \      |      /
//  X    \     |     /     X
//     O  \    |    /   O
//         \   |   /
// (dy, dx) \  |  / (-dy , dx)
//           \ | /
// ------------+--------------> X
//           / | \                  .
// (dy, -dx)/  |  \ (-dy, -dx)      .
//         /   |   \                .
//     O  /    |    \   O           .
//  X    /     |     \     X        .
//      /  O   |   O  \             .
//     /       |       \            .
//    /  X     |    X   \           .
//   /         |         \          .
//  /          |          \         .
// / (dx, -dy) | (-dx, -dy)\        .
//             v
//
//             Y

void fisheye_square_mask(double * mask, int width, double r, double m) {
    int radius = ceil(r);
    geometry_t g = {
        width,
        width,
        {r, r}
    };
    point_t *c = point_new(0., 0.), *nc;
    polar_t *p, *np;
    int x, y, height = width;
    double dx, dy;
    // Pre calculate the new position
    for (y=0; y <= radius; y++) {
        c->y = y;
        for (x = y; x <= radius; x++) {
            c->x = x;
            p = geometry_polar_from_point(&g, c);
            if (p->r < r) {
                np = unmagnify(p, r, m);
                nc = geometry_point_from_polar(&g, np);
                dx = nc->x - x; // nc->x = x + dx
                dy = nc->y - y; // nc->y = y + dy
                free(nc);
                free(np);
                // 0
                mask[y * width * 2 + x * 2] = dx;
                mask[y * width * 2 + x * 2 + 1] = dy;
                // 7
                mask[y * width * 2 + (width - x) * 2] = -dx;
                mask[y * width * 2 + (width - x) * 2 + 1] = dy;
                // 3
                mask[(height - y) * width * 2 + x * 2] = dx;
                mask[(height - y) * width * 2 + x * 2 + 1] = -dy;
                // 4
                mask[(height - y) * width * 2 + (width - x) * 2] = -dx;
                mask[(height - y) * width * 2 + (width - x) * 2 + 1] = -dy;
                // 1
                mask[x * width * 2 + y * 2] = dy;
                mask[x * width * 2 + y * 2 + 1] = dx;
                // 6
                mask[x * width * 2 + (width - y) * 2] = -dy;
                mask[x * width * 2 + (width - y) * 2 + 1] = dx;
                // 3
                mask[(height - x) * width * 2 + y * 2] = dy;
                mask[(height - x) * width * 2 + y * 2 + 1] = -dx;
                // 4
                mask[(height - x) * width * 2 + (width - y) * 2] = -dy;
                mask[(height - x) * width * 2 + (width - y) * 2 + 1] = -dx;
            }
            free(p);
        }
    }
    free(c);
}

void
fisheye_inplace_sub(Bitmap* img, const point_t *c, const double* dv) {
    double dx, dy, idx, idy, r0, g0, b0, r1, g1, b1;
    int x = c->x * COLORS, nx, ny, width = img->width * COLORS;
    unsigned char *data0, *data1, *to, r, g, b;
    point_t *nc = point_new(c->x + dv[0], c->y + dv[1]);

    nx = floor(nc->x);
    ny = floor(nc->y);
    dx = nc->x - nx;
    dy = nc->y - ny;
    idx = 1 - dx;
    idy = 1 - dy;
    nx *= COLORS;
    // rows
    data0 = &(img->data[ny * width]);
    data1 = &(img->data[(ny + 1) * width]);
    to = &(img->data[(int)c->y * width]);
    // intermediary points
    r0 = idx * data0[nx] + dx * data0[nx + 3];
    g0 = idx * data0[nx + 1] + dx * data0[nx + 4];
    b0 = idx * data0[nx + 2] + dx * data0[nx + 5];
    r1 = idx * data1[nx] + dx * data1[nx + 3];
    g1 = idx * data1[nx + 1] + dx * data1[nx + 4];
    b1 = idx * data1[nx + 2] + dx * data1[nx + 5];
    // final points
    r = idy * r0 + dy * r1;
    g = idy * g0 + dy * g1;
    b = idy * b0 + dy * b1;
    to[x] = r;
    to[x + 1] = g;
    to[x + 2] = b;

    free(nc);
}

void
fisheye_inplace_from_square_mask(Bitmap* img, const double* mask, double mask_width) {
    point_t *c = point_new(0., 0.);
    int x, y, x0, y0,
        width = img->width,
        height = img->height;
    const double* dv;
    x0 = max(0, (width - mask_width)/2);
    y0 = max(0, (height - mask_width)/2);
    for (y=0; y < height/2; y++) {
        c->y = (double) y;
        for (x=0; x <= width/2; x++) {
            c->x = x;
            if (c->x >= x0 && c->y >= y0 &&
                c->x < x0 + mask_width && c->y < y0 + mask_width) {
                dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
                if (dv[0] != 0 || dv[1] != 0) {
                    fisheye_inplace_sub(img, c, dv);
                }
            }
        }
        for (x=width; x > width/2; x--) {
            c->x = x;
            if (c->x >= x0 && c->y >= y0 &&
                c->x < x0 + mask_width && c->y < y0 + mask_width) {
                dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
                if (dv[0] != 0 || dv[1] != 0) {
                    fisheye_inplace_sub(img, c, dv);
                }
            }
        }
    }
    for (y=height; y > height/2; y--) {
        c->y = (double) y;
        for (x=0; x <= width; x++) {
            c->x = x;
            if (c->x >= x0 && c->y >= y0 &&
                c->x < x0 + mask_width && c->y < y0 + mask_width) {
                dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
                if (dv[0] != 0 || dv[1] != 0) {
                    fisheye_inplace_sub(img, c, dv);
                }
            }
        }
        for (x=width; x > width/2; x--) {
            c->x = x;
            if (c->x >= x0 && c->y >= y0 &&
                c->x < x0 + mask_width && c->y < y0 + mask_width) {
                dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
                if (dv[0] != 0 || dv[1] != 0) {
                    fisheye_inplace_sub(img, c, dv);
                }
            }
        }
    }
    c->y = height/2;
    for (x = 0; x <= width/2; x++) {
        c->x = x;
        if (c->x >= x0 && c->y >= y0 &&
            c->x < x0 + mask_width && c->y < y0 + mask_width) {
            dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
            if (dv[0] != 0 || dv[1] != 0) {
                fisheye_inplace_sub(img, c, dv);
            }
        }
    }
    for (x = width; x > width/2; x--) {
        c->x = x;
        if (c->x >= x0 && c->y >= y0 &&
            c->x < x0 + mask_width && c->y < y0 + mask_width) {
            dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
            if (dv[0] != 0 || dv[1] != 0) {
                fisheye_inplace_sub(img, c, dv);
            }
        }
    }

    free(c);
}

int
main(int argc, const char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
        return 1;
    }
    clock_t t0 = clock();
    Bitmap* img = loadBitmap(argv[1]);
    clock_t t1 = clock();
    double radius = (img->height < img->width ? img->height : img->width) * .4,
           magnify_factor = 5.0;
    double *mask = (double *) calloc(
        sizeof(double),
        radius * radius * 8); // the square is 2 radius large
    fisheye_square_mask(mask, 2 * radius, radius, magnify_factor);
    clock_t t2 = clock();
    fisheye_inplace_from_square_mask(img, mask, 2*radius);
    clock_t t3 = clock();
    clock_t saved = saveBitmap(argv[2], img);
    free(mask);
    free(img);
    clock_t t4 = clock();

    if (!saved) {
        std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
    } else {
        std::cout << argv[1] << "\t";
        std::cout << double(t4-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t1-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t2-t1)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t3-t2)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t4-t3)/CLOCKS_PER_SEC << std::endl;
    }
    return saved;
}
