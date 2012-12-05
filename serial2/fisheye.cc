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
    int x, y, ycol, xcol, wx, wy, wxcol, wycol;
    double dx, dy;
    // Pre calculate the new position
    for (y=0; y <= radius; y++) {
        c->y = y;
        wy = width - y;
        ycol = y * width * 2;
        wycol = wy * width * 2;
        wy *= 2;
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
                wx = width - x;
                xcol = x * width * 2;
                wxcol = wx * width * 2;
                wx *= 2;
                // 0
                mask[ycol + x * 2] = dx;
                mask[ycol + x * 2 + 1] = dy;
                // 7
                mask[ycol + wx] = -dx;
                mask[ycol + wx + 1] = dy;
                // 3
                mask[wycol + x * 2] = dx;
                mask[wycol + x * 2 + 1] = -dy;
                // 4
                mask[wycol + wx] = -dx;
                mask[wycol + wx + 1] = -dy;
                // 1
                mask[xcol + y * 2] = dy;
                mask[xcol + y * 2 + 1] = dx;
                // 6
                mask[xcol + wy] = -dy;
                mask[xcol + wy + 1] = dx;
                // 3
                mask[wxcol + y * 2] = dy;
                mask[wxcol + y * 2 + 1] = -dx;
                // 4
                mask[wxcol + wy] = -dy;
                mask[wxcol + wy + 1] = -dx;
            }
            free(p);
        }
    }
    free(c);
}

void
fisheye_from_square_mask(Bitmap* dst, const Bitmap* src, const double* mask, double mask_width) {
    point_t *c = point_new(0., 0.), *nc;
    int x, y, x0, y0, nx, ny,
        width = src->width * COLORS,
        height = src->height;
    double dx, dy, r0, g0, b0, r1, g1, b1, r, g, b;
    const unsigned char *data, *data0, *data1;
    const double* dv;
    unsigned char *to;
    x0 = max(0, (src->width - mask_width)/2);
    y0 = max(0, (src->height - mask_width)/2);
    for (y=0; y < height; y++) {
        c->y = (double) y;
        to = &(dst->data[y * width]);
        data = &(src->data[y * width]);
        for (x=0; x < width; x+=COLORS) {
            c->x = x / (double) COLORS;
            if (c->x >= x0 && c->y >= y0 &&
                c->x < x0 + mask_width && c->y < y0 + mask_width) {
                dv = &(mask[int(((c->y - y0) * mask_width + (c->x - x0)) * 2)]);
                if (dv[0] != 0 || dv[1] != 0) {
                    nc = point_new(c->x + dv[0], c->y + dv[1]);
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
                    continue;
                }
            }

            to[x] = data[x];
            to[x+1] = data[x+1];
            to[x+2] = data[x+2];
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
    Bitmap* src = loadBitmap(argv[1]);
    int width = src->width,
        height = src->height;
    clock_t t1 = clock();
    double radius = (height < width ? height : width) * .45,
           magnify_factor = 5.0;
    unsigned int mask_width = ceil(2 * radius);
    double *mask = (double *) calloc(
        sizeof(double),
        mask_width * mask_width * 2);
    Bitmap* dst = createBitmap(src->width, src->height, 24);
    fisheye_square_mask(mask, mask_width, radius, magnify_factor);
    clock_t t2 = clock();
    fisheye_from_square_mask(dst, src, mask, 2*radius);
    clock_t t3 = clock();
    clock_t saved = saveBitmap(argv[2], dst);
    free(mask);
    free(dst);
    free(src);
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
