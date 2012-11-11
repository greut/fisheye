#include <iostream>
#include <stdlib.h>
#include <math.h>
#include "lspbmp.h"

#define BYTE 8
#define COLORS 3


// Cartesian to Polar conversion
//
// +------------------------------------> X
// |                   | a = Pi/2
// |                   |
// |                   |
// |                   |
// |                   |
// |                   |
// |                   |O (width/2, height/2)
// |-------------------+-----------------
// |                   |               angle = 0
// |                   |
// |                   |
// |                   |
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
    Bitmap* src = loadBitmap(argv[1]);
    Bitmap* dst = createBitmap(src->width, src->height, 24);
    geometry_t g = {
        src->width, src->height,
        {round(src->width/2.), round(src->height/2.)}};
    point_t *c = point_new_2i(0., 0.);
    int x, y,
        height = src->height,
        width = src->width * COLORS;
    double radius = (src->height < src->width ? src->height : src->width) * .4,
           m = 5.0;
    for (y=0; y < height; y++) {
        c->y = (double) y;
        unsigned char *to = &(dst->data[y * width]);
        unsigned char *data; 
        for (x=0; x < width; x+=COLORS) {
            c->x = x / (double) COLORS;
            polar_t *p = geometry_polar_from_point(&g, c);
            if (p->r < radius) {
                polar_t *np = unmagnify(p, radius, m);
                point_t *nc = geometry_point_from_polar(&g, np);
                //printf("(%.1f; %.1f) ", c->x, c->y);
                //printf("[%.1f, %.1f] ", p->r, p->a);
                //printf("[%.1f, %.1f] ", np->r, np->a);
                //printf("(%.1f; %.1f)\n", nc->x, nc->y);

                // TODO: billinear interpolation
                int nx = round(nc->x), ny = round(nc->y);
                data = &(src->data[ny * width]);
                nx *= COLORS;
                to[x] = data[nx] ^ 0x00;    // R
                to[x+1] = data[nx+1] ^ 0x00; // G
                to[x+2] = data[nx+2] ^ 0x00; // B
                free(nc);
                free(np);
            } else {
                data = &(src->data[y * width]);
                to[x] = data[x];
                to[x+1] = data[x+1];
                to[x+2] = data[x+2];
            }
            free(p);
        }
    }
    free(c);

    if (saveBitmap(argv[2], dst) == 0) {
        std::cerr << "The picture could not be saved to " << argv[2] << std::endl;
    }
    return 0;
}

/* TEST

int main2(void) {
    geometry_t g = {100, 100, {50, 50}};
    point_t *c, *nc;
    polar_t *p;

    c = point_new(100, 0);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(50, 0);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(0, 0);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(0, 50);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(0, 100);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(50, 100);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    c = point_new(100, 100);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);


    c = point_new(100, 50);
    p = geometry_polar_from_point(&g, c);
    nc = geometry_point_from_polar(&g, p);
    printf("((%.1f; %.1f) -> (%.1f; %.1f) -> (%.1f; %.1f)\n",
        c->x, c->y, p->r, p->a, nc->x, nc->y);
    free(nc);
    free(p);
    free(c);

    return 0;
}
*/
