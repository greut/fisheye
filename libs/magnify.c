#include "magnify.h"
#include <math.h>
#include <stdlib.h>

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


