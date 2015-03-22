#ifndef GEOMETRY_formatter_kml_earth_H
#define GEOMETRY_formatter_kml_earth_H

#include "formatter.h"

void init_formatter_kml_earth(TSRMLS_D);

extern zend_class_entry formatter_kml_earth;

PHP_METHOD(formatter_kml_earth, __construct);
PHP_METHOD(formatter_kml_earth, output);
PHP_METHOD(formatter_kml_earth, output_split);

zend_class_entry *formatter_kml_earth_ce;
static zend_function_entry formatter_kml_earth_methods[];

zend_object* create_formatter_kml_earth_object(zend_class_entry *class_type);
void free_formatter_kml_earth_object(formatter_object *intern);

char *formatter_kml_earth_output(formatter_object *intern);
char *get_colour_by_height(coordinate_set_object *set);
char *get_colour_by_speed(coordinate_set_object *set);
char *get_colour_by_climb_rate(coordinate_set_object *set);
char *get_colour_by_time(coordinate_set_object *set);
char *get_kml_styles_earth();
#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI
#endif