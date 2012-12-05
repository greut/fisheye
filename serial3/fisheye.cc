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
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

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
    geometry_t g = {
        (int) ceil(2. * r),
        (int) ceil(2. * r),
        {r, r}
    };
    point_t *c = point_new(0., 0.), *nc;
    polar_t *p, *np;
    int x, y, x2, y2, ycol, xcol, wx, wy, wx2, wy2, wxcol, wycol,
        // shortcut
        w2 = width >> 1,
        // dividing by two on an integer round things up
        is_odd = width % 2,
        corr = is_odd ? 0 : 1;
    double dx, dy,
        shift = r - w2;

    for (y=0; y <= w2; y++) {
        c->y = (double) y + shift;
        wy = width - y - 1;
        ycol = y * width << 1;
        wycol = (wy + corr) * width << 1;
        wy2 = wy << 1;
        y2 = y << 1;
        for (x = y; x <= w2; x++) {
            c->x = (double) x + shift;
            p = geometry_polar_from_point(&g, c);
            if (p->r < r) {
                np = unmagnify(p, r, m);
                nc = geometry_point_from_polar(&g, np);
                dx = nc->x - c->x; // nc->x = x + dx
                dy = nc->y - c->y; // nc->y = y + dy
                free(nc);
                free(np);
                wx = width - x - 1;
                xcol = x * width << 1;
                wxcol = (wx + corr) * width << 1;
                wx2 = wx << 1;
                x2 = x << 1;
                // 0
                mask[ycol + x2] = dx;
                mask[ycol + x2 + 1] = dy;
                // 7
                mask[ycol + wx2] = -dx;
                mask[ycol + wx2 + 1] = dy;
                if (is_odd || wy < width - 1) {
                    // 3
                    mask[wycol + x2] = dx;
                    mask[wycol + x2 + 1] = -dy;
                    // 4
                    if (wycol + wx2 > width * width << 1) {
                        std::cerr << "FAIL: " << wy << ", " << wx << std::endl;
                    }
                    mask[wycol + wx2] = -dx;
                    mask[wycol + wx2 + 1] = -dy;
                }
                if (x != y) {
                    // 1
                    mask[xcol + y2] = dy;
                    mask[xcol + y2 + 1] = dx;
                    // 6
                    mask[xcol + wy2] = -dy;
                    mask[xcol + wy2 + 1] = dx;
                    if (is_odd || wx < width - 1) {
                        // 2
                        mask[wxcol + y2] = dy;
                        mask[wxcol + y2 + 1] = -dx;
                        // 5
                        mask[wxcol + wy2] = -dy;
                        mask[wxcol + wy2 + 1] = -dx;
                    }
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
fisheye_inplace_from_square_mask(Bitmap* img, const double* mask, unsigned int mask_width) {
    point_t *c = point_new(0., 0.);
    unsigned int x, y, x0, y0, yy, zero = 0,
        width = img->width,
        height = img->height,
        w2 = width >> 1,
        h2 = height >> 1;
    const double* dv;
    x0 = max(zero, (width - mask_width) >> 1);
    y0 = max(zero, (height - mask_width) >> 1);

    for (y = y0; y < h2; y++) {
        c->y = y;
        yy = (int)((c->y - y0) * mask_width) << 1;
        for (x = x0; x < width/2; x++) {
            c->x = x;
            dv = &(mask[yy + ((int)(c->x - x0) << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                fisheye_inplace_sub(img, c, dv);
            }
        }
        for (x = width - x0; x >= w2; x--) {
            c->x = x;
            dv = &(mask[yy + ((int)(c->x - x0) << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                fisheye_inplace_sub(img, c, dv);
            }
        }
    }
    for (y = height - y0 - 1; y >= h2; y--) {
        c->y = (double) y;
        yy = (int)((c->y - y0) * mask_width) << 1;
        for (x = x0; x < w2; x++) {
            c->x = (double) x;
            dv = &(mask[yy + ((int)(c->x - x0) << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                fisheye_inplace_sub(img, c, dv);
            }
        }
        for (x = width - x0; x >= w2; x--) {
            c->x = (double) x;
            dv = &(mask[yy + ((int)(c->x - x0) << 1)]);
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
    int width = img->width,
        height = img->height;
    clock_t t1 = clock();
    double radius = min(height, width) * .45,
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
    unsigned int mask_width = min(min(width, height), ceil(2 * radius));
    double *mask = (double *) calloc(
        sizeof(double),
        mask_width * mask_width * 2);
    fisheye_square_mask(mask, mask_width, radius, magnify_factor);
    clock_t t2 = clock();
    fisheye_inplace_from_square_mask(img, mask, mask_width);
    clock_t t3 = clock();
    clock_t saved = saveBitmap(argv[2], img);
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
