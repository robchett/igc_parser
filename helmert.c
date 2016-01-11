#include "main.h"
#include "coordinate.h"
#include "helmert.h"
#include <math.h>

static helmert_ellipsoid wsg84 = {
    .a = 6378137,
    .b = 6356752.3142
};

static helmert_ellipsoid osgb36 = {
    .a = 6378137,
    .b = 6356752.314140
};

static helmert_transform wsg84_osgb36_to_transform = {
    .tx = -446.448,
    .ty =  125.157,
    .tz = -542.060,
    .rx = -0.1502,
    .ry = -0.2470,
    .rz =  -0.8421,
    .s =  20.4894
};

static helmert_transform osgb36_to_wsg84_transform = {
    .tx = 446.448,
    .ty = -125.157,
    .tz = 542.060,
    .rx = 0.1502,
    .ry = 0.2470,
    .rz =  0.8421,
    .s =  -20.4894
};

void osgb36_to_wgs84(coordinate_t *point) {
    helmert_trans(point, osgb36, wsg84, osgb36_to_wsg84_transform);
}

void wgs84_to_osgb36(coordinate_t *point) {
    helmert_trans(point, wsg84, osgb36, wsg84_osgb36_to_transform);
}

void helmert_trans(coordinate_t *point, const helmert_ellipsoid source_ellipse, const helmert_ellipsoid target_ellipse, const helmert_transform transform) {
    printf("%f, %f\n", point->lat, point->lng);
    double lat = point->lat;
    double lon = point->lng;

    double a = source_ellipse.a;
    double b = source_ellipse.b;

    double sinPhi = sin(lat);
    double cosPhi = cos(lat);
    double sinLambda = sin(lon);
    double cosLambda = cos(lon);
    double H = 0; // for the moment

    double eSq = (a * a - b * b) / (a * a);
    double nu = a / sqrt(1 - eSq * sinPhi * sinPhi);

    double x1 = (nu + H) * cosPhi * cosLambda;
    double y1 = (nu + H) * cosPhi * sinLambda;
    double z1 = ((1 - eSq) * nu + H) * sinPhi;


    double tx = transform.tx;
    double ty = transform.ty;
    double tz = transform.tz;
    double rx = (transform.rx / 3600) toRAD;
    double ry = (transform.ry / 3600) toRAD;
    double rz = (transform.rz / 3600) toRAD;
    double s1 = (transform.s / 1000000) + 1; // normalise ppm to (s+1)

    double x2 = tx + (x1 * s1) - (y1 * rz) + (z1 * ry);
    double y2 = ty + (x1 * rz) + (y1 * s1) - (z1 * rx);
    double z2 = tz - (x1 * ry) + (y1 * rx) + (z1 * s1);


    a = target_ellipse.a;
    b = target_ellipse.b;
    double precision = 1 / 3600000;

    eSq = (a * a - b * b) / (a * a);
    double p = sqrt(x2 * x2 + y2 * y2);
    double phi = atan2(z2, p * (1 - eSq));
    double phiP = 2 * M_PI;
    double iterations = 0;
    while (abs(phi - phiP) > precision && iterations < 1000) {
        double nu = a / sqrt(1 - eSq * sin(phi) * sin(phi));
        double phiP = phi;
        double phi = atan2(z2 + eSq * nu * sin(phi), p);
        iterations++;
    }
    double lambda = atan2(y2, x2);
    H = p / cos(phi) - nu;
    point->lat = phi toDEG;
    point->lng = lambda toDEG;
    printf("%f, %f\n", point->lat, point->lng);
}