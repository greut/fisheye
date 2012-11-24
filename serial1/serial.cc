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
        // FIXME: DOESN'T WORK THAT WELL WITH NON-SQUARISH PICTURES!!
        //for (x = (y * src->width / height); x <= ceil(src->width / 2.); x++) {
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
                // 7
                mask[y * width * 2 + (width - x) * 2] = -dx;
                mask[y * width * 2 + (width - x) * 2 + 1] = dy;
                // 3
                mask[(height - y) * width * 2 + x * 2] = dx;
                mask[(height - y) * width * 2 + x * 2 + 1] = -dy;
                // 4
                mask[(height - y) * width * 2 + (width - x) * 2] = -dx;
                mask[(height - y) * width * 2 + (width - x) * 2 + 1] = -dy;
                /*
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
                */
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
    clock_t t1 = clock();
    double *mask = (double *) calloc(
        sizeof(double),
        src->width * src->height * 2);
    Bitmap* dst = createBitmap(src->width, src->height, 24);
    fisheye_mask(mask, src->width, src->height);
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
        std::cout << double(t4-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t1-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t2-t1)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t3-t2)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t4-t3)/CLOCKS_PER_SEC << std::endl;
    }
    return saved;
}
