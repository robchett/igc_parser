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

zend_object_value create_formatter_kml_earth_object(zend_class_entry *class_type TSRMLS_DC);
void free_formatter_kml_earth_object(formatter_object *intern TSRMLS_DC);

char *formatter_kml_earth_output(formatter_object *intern);

#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI
#endif