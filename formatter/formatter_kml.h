#ifndef GEOMETRY_formatter_kml_H
#define GEOMETRY_formatter_kml_H

#include "formatter.h"

void init_formatter_kml(TSRMLS_D);

extern zend_class_entry formatter_kml;

PHP_METHOD(formatter_kml, __construct);
PHP_METHOD(formatter_kml, output);
PHP_METHOD(formatter_kml, output_split);

zend_class_entry *formatter_kml_ce;
static zend_function_entry formatter_kml_methods[];

zend_object* create_formatter_kml_object(zend_class_entry *class_type TSRMLS_DC);
void free_formatter_kml_object(formatter_object *intern TSRMLS_DC);

char *formatter_kml_output(formatter_object *intern);
char *formatter_kml_output_split(formatter_object *intern);
formatter_object* fetch_formatter_object(zend_object* obj);

#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI
#endif