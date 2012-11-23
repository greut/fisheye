#include <iostream>
#include <math.h>
#include <time.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3

static void
fisheye(Bitmap* dst, const Bitmap* src) {
    geometry_t geometry = {
        src->width, src->height,
        {round(src->width/2.), round(src->height/2.)}};
    point_t *c = point_new(0., 0.);
    int x, y, nx, ny,
        height = src->height,
        width = src->width * COLORS;
    double radius = (src->height < src->width ? src->height : src->width) * .4,
           m = 5.0,
           dx, dy, r0, g0, b0, r1, g1, b1, r, g, b;
    const unsigned char *data, *data0, *data1;
    unsigned char *to;
    for (y=0; y < height; y++) {
        c->y = (double) y;
        to = &(dst->data[y * width]);
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
main(int argc, const char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " source dest"<< std::endl;
        return 1;
    }
    clock_t t0 = clock();
    Bitmap* src = loadBitmap(argv[1]);
    clock_t t1 = clock();
    Bitmap* dst = createBitmap(src->width, src->height, 24);
    fisheye(dst, src);
    clock_t t2 = clock();
    clock_t saved = saveBitmap(argv[2], dst);
    free(src);
    free(dst);
    clock_t t3 = clock();

    if (!saved) {
        std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
    } else {
        std::cout << argv[1] << "\t";
        std::cout << double(t3-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t1-t0)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t2-t1)/CLOCKS_PER_SEC << "\t";
        std::cout << double(t3-t2)/CLOCKS_PER_SEC << std::endl;
    }
    return saved;
}
