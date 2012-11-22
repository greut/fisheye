#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "lspbmp.h"

#define BYTE 8
#define COLORS 3

// Cartesian to Polar conversion
//
// +------------------------------------> X
// |                   | a = Pi/2
// |                   |
// |                   |
// |                   |O (width/2, height/2)
// |-------------------+-----------------
// |                   |               angle = 0
// |                   |
// |                   |
// v
//
// Y

typedef struct {
    double r; // radius
    double a; // angle
} polar_t;

typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    int width;
    int height;
    point_t center;
} geometry_t;

point_t *
point_new (double x, double y) {
    point_t *t = (point_t *) malloc(sizeof(point_t));
    if (t == NULL) {
        exit(2);
    }
    t->x = x;
    t->y = y;
    return t;
}

point_t *
point_new_2i (int x, int y) {
    return point_new((double) x, (double) y);
}

polar_t *
polar_new (double radius, double angle) {
    polar_t *p = (polar_t *) malloc(sizeof(polar_t));
    if (p == NULL) {
        exit(2);
    }
    p->r = radius;
    p->a = angle;
    return p;
}

polar_t *
geometry_polar_from_point (const geometry_t *g, const point_t *c) {
    double x = c->x - g->center.x, y = -(c->y - g->center.y);
    double angle;
    if (x != 0) {
        angle = atan(y/x);
        angle = x >= 0 ? angle : angle + M_PI;
    } else {
        angle = y >= 0 ? M_PI/2. : -M_PI/2.;
    }
    return polar_new(sqrt(x*x + y*y), angle);
}

point_t *
geometry_point_from_polar (const geometry_t *g, const polar_t *p) {
    double x = p->r * cos(p->a); 
    double y = p->r * sin(p->a);
    return point_new(x + g->center.x, -y + g->center.y);
}

polar_t *
unmagnify (const polar_t *p, double radius, double m) {
    double r = p->r, d;
    if (r < radius) {
        d = (m * (radius - p->r) / p->r) + 1;
        if (d != 0) {
            r = radius / d;
        } else {
            r = 0;
        }
    }
    return polar_new(r, p->a);
}

polar_t *
magnify (const polar_t *p, double radius, double m) {
    return unmagnify(p, radius, 1/m);    
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
    clock_t t2 = clock();
    geometry_t g = {
        src->width, src->height,
        {round(src->width/2.), round(src->height/2.)}};
    point_t *c = point_new_2i(0., 0.);
    int x, y,
        height = src->height,
        width = src->width * COLORS;
    double radius = (src->height < src->width ? src->height : src->width) * .4,
           m = 5.0;
    unsigned char *data, *data0, *data1;
    for (y=0; y < height; y++) {
        c->y = (double) y;
        unsigned char *to = &(dst->data[y * width]);
        data = &(src->data[y * width]);
        for (x=0; x < width; x+=COLORS) {
            c->x = x / (double) COLORS;
            polar_t *p = geometry_polar_from_point(&g, c);
            if (p->r < radius) {
                polar_t *np = unmagnify(p, radius, m);
                point_t *nc = geometry_point_from_polar(&g, np);
                int nx = floor(nc->x),
                    ny = floor(nc->y);
                double dx = nc->x - nx, dy = nc->y - ny,
                       r0, g0, b0,
                       r1, g1, b1,
                       r, g, b;
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
    clock_t t3 = clock();
    clock_t saved = saveBitmap(argv[2], dst);
    free(c);
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
