#ifndef GEOMETRY_LATLNG_H
#define GEOMETRY_LATLNG_H

void init_coordinate(TSRMLS_D);

extern zend_class_entry coordinate;
struct coordinate_set_object;
struct coordinate_subset;

PHP_METHOD(coordinate, id);
PHP_METHOD(coordinate, lat);
PHP_METHOD(coordinate, timestamp);
PHP_METHOD(coordinate, lng);
PHP_METHOD(coordinate, ele);
PHP_METHOD(coordinate, set_lat);
PHP_METHOD(coordinate, set_lng);
PHP_METHOD(coordinate, set_ele);
PHP_METHOD(coordinate, gridref);

PHP_METHOD(coordinate, get_distance_to);
PHP_METHOD(coordinate, get_bearing_to);
PHP_METHOD(coordinate, __construct);

typedef struct coordinate_object {
    // required
    zend_object std;

    // actual struct contents
    double lat, lng;
    double sin_lat, cos_lat, cos_lng;

    long ele;
    long alt;
    long timestamp;
    long id;
    double climb_rate;
    double speed;
    double bearing;
    struct coordinate_object *prev;
    struct coordinate_object *next;
    struct coordinate_set_object *coordinate_set;
    struct coordinate_subset *coordinate_subset;
} coordinate_object;

zend_class_entry *coordinate_ce;
static zend_function_entry coordinate_methods[];

zend_object_value create_coordinate_object(zend_class_entry *class_type TSRMLS_DC);
void free_coordinate_object(coordinate_object *intern TSRMLS_DC);
void _free_coordinate_object(coordinate_object *intern);

#endif

#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI

double get_distance(coordinate_object *point1, coordinate_object *point2);
double get_distance_precise(coordinate_object *point1, coordinate_object *point2);
double get_bearing(coordinate_object *point1, coordinate_object *point2);
char *coordinate_to_kml(coordinate_object *coordinate);