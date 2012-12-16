#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "lspbmp.h"
#include "magnify.h"

#define COLORS 3
#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

void fisheye_square_half_mask(double * mask, int width, double r, double m) {
    geometry_t g = {
        (int) ceil(2. * r),
        (int) ceil(2. * r),
        {r, r}
    };
    point_t *c, *nc;
    polar_t *p, *np;
    int x, y, x2, y2, ycol, xcol;
    double dx, dy,
        shift = r - width;
#if defined (_OPENMP)
#pragma omp parallel private(c,p,nc,np,x,y,x2,y2,ycol,xcol,dx,dy)
{
#pragma omp for schedule(dynamic)
#endif
    for (y=0; y < width; y++) {
        c = point_new(0., 0.);
        c->y = (double) y + shift;
        ycol = y * width << 1;
        y2 = y << 1;
        for (x = y; x < width; x++) {
            c->x = (double) x + shift;
            p = geometry_polar_from_point(&g, c);
            if (p->r < r) {
                np = unmagnify(p, r, m);
                nc = geometry_point_from_polar(&g, np);
                dx = nc->x - c->x; // nc->x = x + dx
                dy = nc->y - c->y; // nc->y = y + dy

                free(nc);
                free(np);
                xcol = x * width << 1;
                x2 = x << 1;

                mask[ycol + x2] = dx;
                mask[ycol + x2 + 1] = dy;
                if (x != y) {
                    mask[xcol + y2] = dy;
                    mask[xcol + y2 + 1] = dx;
                }
            }
            free(p);
        }
        free(c);
    }
#if defined (_OPENMP)
}
#endif
}

void
fisheye_sub(const Bitmap *src, Bitmap *dst,  const point_t *c, const double* dv) {
    double dx, dy, idx, idy, r0, g0, b0, r1, g1, b1;
    unsigned int nx, ny, mx, my,
        x = c->x * COLORS,
        width = src->width * COLORS;
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
    data0 = &(src->data[ny * width]);
    data1 = &(src->data[my * width]);
    to = &(dst->data[(int)(c->y * width)]);
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
fisheye_from_square_half_mask(const Bitmap* src, Bitmap* dst, double* mask, unsigned int mask_width) {
    unsigned int x, y, x0 = 0, y0 = 0, yy, zero = 0, sx, sy, swx, swy, s,
        swidth = src->width * COLORS,
        width = src->width,
        height = src->height,
        ycorr = height % 2 ? 0 : 1,
        xcorr = width % 2 ? 0 : 1;

    double* dv;
    if (width / 2 > mask_width) {
        x0 = max(zero, ((int) ceil(width / 2.) - mask_width));
    }
    if (height / 2 > mask_width) {
        y0 = max(zero, ((int) ceil(height / 2.) - mask_width));
    }
#if defined (_OPENMP)
#pragma omp parallel default(none) \
    firstprivate(mask,mask_width,x0,y0,width,height,dst,src,xcorr,ycorr) \
    private(dv,x,y,yy,sx,sy,swx,swy,s)
{
#pragma omp for schedule(dynamic)
#endif
    for (y = 0; y < mask_width; y++) {
        point_t *c = point_new(0, 0), *r = point_new(0, 0);
        c->y = y + y0;
        r->y = height - ycorr - (y + y0);
        yy = y * mask_width << 1;
        sy = (y + y0) * width * COLORS;
        swy = (height - 1 - y - y0) * width * COLORS;
        for (x = 0; x < mask_width; x++) {
            dv = &(mask[yy + (x << 1)]);
            if (dv[0] != 0 || dv[1] != 0) {
                // North-West
                c->x = x + x0;
                fisheye_sub(src, dst, c, dv);

                // North-East
                dv[0] = -dv[0];
                c->x = width - xcorr - (x + x0);
                fisheye_sub(src, dst, c, dv);

                // South-East
                dv[1] = -dv[1];
                r->x = width - xcorr - (x + x0);
                fisheye_sub(src, dst, r, dv);
                dv[0] = -dv[0];

                // South-West
                r->x = x + x0;
                fisheye_sub(src, dst, r, dv);
                dv[1] = -dv[1];
            } else {
                // Fill the sides of the lens
                sx = (x + x0) * COLORS;
                swx = (width - 1 - x - x0) * COLORS;
                // North-West
                s = sy + sx;
                dst->data[s] = src->data[s];
                dst->data[s + 1] = src->data[s + 1];
                dst->data[s + 2] = src->data[s + 2];

                // North-East
                s = sy + swx;
                dst->data[s] = src->data[s];
                dst->data[s + 1] = src->data[s + 1];
                dst->data[s + 2] = src->data[s + 2];

                // South-East
                s = swy + swx;
                dst->data[s] = src->data[s];
                dst->data[s + 1] = src->data[s + 1];
                dst->data[s + 2] = src->data[s + 2];

                // South-West
                s = swy + sx;
                dst->data[s] = src->data[s];
                dst->data[s + 1] = src->data[s + 1];
                dst->data[s + 2] = src->data[s + 2];
            }
        }
        free(c);
        free(r);
    }
#if defined (_OPENMP)
}
#endif
    if (x0 && y0) {
#if defined (_OPENMP)
#pragma omp parallel default(none) \
    firstprivate(dst,src,width,height,swidth,x0,y0) \
    private(x,y,yy,sx,sy,swx,swy,s)
{
#pragma omp for schedule(dynamic)
#endif
        // Fill the borders
        for (y=0; y<height/2; y++) {
            sy = y * swidth;
            swy = (width - 1 - y) * swidth;
            if (y < y0) {
                for (x=0; x<width; x++) {
                    sx = x * COLORS;
                    s = sy + sx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];

                    s = swy + sx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];
                }
            } else {
                for (x=0; x<x0; x++) {
                    sx = x * COLORS;
                    swx = (width - 1 - x) * COLORS;

                    s = sy + sx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];

                    s = sy + swx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];

                    s = swy + swx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];

                    s = swy + sx;
                    dst->data[s] = src->data[s];
                    dst->data[s + 1] = src->data[s + 1];
                    dst->data[s + 2] = src->data[s + 2];
                }
            }
        }
#if defined (_OPENMP)
}
#endif
    }
}

int
main(int argc, const char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s source dest\n", argv[0]);
        return 1;
    }
    double t0 = omp_get_wtime();
    Bitmap* src = loadBitmapMode(argv[1], O_RDONLY);
    Bitmap* dst = createBitmap(src->width, src->height, src->depth);
    int width = src->width,
        height = dst->height;
    double t1 = omp_get_wtime();
    double radius = min(height, width) * .45,
           magnify_factor = 5.0;
    if (argc > 3) {
        sscanf(argv[3], "%lf", &radius);
        if (radius <= 0) {
            fprintf(stderr, "Radius cannot be null or negative\n");
            return 1;
        }
    }
    if (argc > 4) {
        sscanf(argv[4], "%lf", &magnify_factor);
        if (magnify_factor < 1) {
            fprintf(stderr, "Less than 1 magnify lens are not supported\n");
            return 1;
        }
    }
    unsigned int mask_width = ceil(min(min(width, height)/2., radius));
    double *mask = (double *) calloc(
        sizeof(double),
        mask_width * mask_width << 1);
    fisheye_square_half_mask(mask, mask_width, radius, magnify_factor);
    double t2 = omp_get_wtime();
    fisheye_from_square_half_mask(src, dst, mask, mask_width);
    double t3 = omp_get_wtime();
    double saved = saveBitmap(argv[2], dst);
    free(mask);
    destroyBitmap(src);
    destroyBitmap(dst);
    double t4 = omp_get_wtime();

    if (!saved) {
        fprintf(stderr, "The picture could not be saved to %s.\n", argv[2]);
    } else {
        int size = 1;
#if defined (_OPENMP)
#pragma omp parallel default(none) shared(size)
{
        size = omp_get_num_threads();
}
#endif
        printf("%s\t%d\t%.2lf\t%.2lf\t%.2lf\t%.2lf\t%.2lf\t%d\n", argv[1], width * height,
            (t4-t0),
            (t1-t0),
            (t2-t1),
            (t3-t2),
            (t4-t3),
            size);
    }
    return saved;
}
