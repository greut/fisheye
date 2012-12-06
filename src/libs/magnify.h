#ifndef INCLUDED_MAGNIFY_H
#define INCLUDED_MAGNIFY_H

// Cartesian to Polar conversion:
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
// A point is in the (x,y) referencial (top-left corner).
// A geometry is the known space.
// A polar is in the (radius, angle) referencial, the center is in the
//  middle of the geometry given.
//
// Y

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct geometry_t geometry_t;
typedef struct polar_t polar_t;
typedef struct point_t point_t;

struct point_t {
    double x;
    double y;
};

struct geometry_t {
    int width;
    int height;
    point_t center;
};

struct polar_t {
    double r; // radius
    double a; // angle
};

point_t *
point_new (double x, double y);

polar_t *
polar_new (double radius, double angle);

polar_t *
geometry_polar_from_point (const geometry_t *g, const point_t *c);

point_t *
geometry_point_from_polar (const geometry_t *g, const polar_t *p);

polar_t *
unmagnify (const polar_t *p, double radius, double m);

polar_t *
magnify (const polar_t *p, double radius, double m);

#ifdef __cplusplus
}
#endif
#endif
