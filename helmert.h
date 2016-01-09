#pragma once

typedef struct helmert_ellipsoid {
    double a;
    double b;
} helmert_ellipsoid;


typedef struct helmert_transform {
    double tx;
    double ty;
    double tz;
    double rx;
    double ry;
    double rz;
    double s;
} helmert_transform;

static helmert_ellipsoid wsg84;
static helmert_ellipsoid osgb36;
static helmert_transform osgb36_to_wsg84_transform;
static helmert_transform wsg84_osgb36_to_transform;

void osgb36_to_wgs84(coordinate_object *point);
void wgs84_to_osgb36(coordinate_object *point);
void helmert_trans(coordinate_object *point, const helmert_ellipsoid source_ellipse, const helmert_ellipsoid target_ellipse, const helmert_transform transform);