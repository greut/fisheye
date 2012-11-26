#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3

//
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

void fisheye_mask(double * mask, int width, int height) {
    geometry_t g = {
        width, height,
        {round(width/2.), round(height/2.)}};
    point_t *c = point_new(0., 0.);
    int x, y;
    double radius = (height < width ? height : width) * .45,
           m = 5.0,
           dx, dy;
    // Pre calculate the new position
    for (y=0; y <= ceil(height / 2.); y++) {
        c->y = y;
        for (x = 0; x <= ceil(width / 2.); x++) {
            c->x = x;
            polar_t *p = geometry_polar_from_point(&g, c);
            if (p->r < radius) {
                polar_t *np = unmagnify(p, radius, m);
                point_t *nc = geometry_point_from_polar(&g, np);
                dx = nc->x - x; // nc->x = x + dx
                dy = nc->y - y; // nc->y = y + dy
                free(nc);
                free(np);
                // 0
                mask[y * width * 2 + x * 2] = dx;
                mask[y * width * 2 + x * 2 + 1] = dy;
                // 3
                mask[y * width * 2 + (width - x) * 2] = -dx;
                mask[y * width * 2 + (width - x) * 2 + 1] = dy;
                // 1
                mask[(height - y) * width * 2 + x * 2] = dx;
                mask[(height - y) * width * 2 + x * 2 + 1] = -dy;
                // 2
                mask[(height - y) * width * 2 + (width - x) * 2] = -dx;
                mask[(height - y) * width * 2 + (width - x) * 2 + 1] = -dy;
            }
            free(p);
        }
    }
    free(c);
}

void fisheye_from_mask(Bitmap* dst, const Bitmap* src, const double* mask) {
    point_t *c = point_new(0., 0.);
    int x, y, nx, ny,
        width = src->width * COLORS,
        height = src->height;
    double dx, dy, r0, g0, b0, r1, g1, b1, r, g, b;
    const unsigned char *data, *data0, *data1;
    const double* dv;
    unsigned char *to;
    for (y=0; y < height; y++) {
        c->y = (double) y;
        to = &(dst->data[y * width]);
        data = &(src->data[y * width]);
        for (x=0; x < width; x+=COLORS) {
            c->x = x / (double) COLORS;
            dv = &(mask[int((c->y * src->width + c->x) * 2)]);
            if (dv[0] != 0 || dv[1] != 0) {
                point_t *nc = point_new(c->x + dv[0], c->y + dv[1]);
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
            } else {
                to[x] = data[x];
                to[x+1] = data[x+1];
                to[x+2] = data[x+2];
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
    Bitmap* src = loadBitmap(argv[1]);
    if (src == NULL) {
        std::cerr << "Cannot open " << argv[1] << std::endl;
        return 1;
    }
    unsigned int width = src->width,
                 height = src->height,
                 mask_size = width * height * 2;
    clock_t t1 = clock();
    double *mask = (double *) calloc(sizeof(double), mask_size);
    if (mask == NULL) {
        std::cerr << "Cannot allocate mask of size " << mask_size << std::endl;
        free(src);
        return 1;
    }
    Bitmap* dst = createBitmap(width, height, 24);
    if (dst == NULL) {
        std::cerr << "Cannot allocate destination picture " << argv[2] << std::endl;
        free(mask);
        free(src);
        return 1;
    }
    fisheye_mask(mask, width, height);
    clock_t t2 = clock();
    fisheye_from_mask(dst, src, mask);
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
