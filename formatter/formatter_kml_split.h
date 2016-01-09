#pragma once

#include "formatter.h"

typedef struct formatter_split_object {
    zend_object std;
    coordinate_set_object *set;
} formatter_split_object;

void init_formatter_kml_split(TSRMLS_D);

extern zend_class_entry formatter_kml_split;

PHP_METHOD(formatter_kml_split, __construct);
PHP_METHOD(formatter_kml_split, output);

zend_class_entry *formatter_kml_split_ce;
static zend_function_entry formatter_kml_split_methods[];

zend_object* create_formatter_kml_split_object(zend_class_entry *class_type TSRMLS_DC);
void free_formatter_kml_split_object(formatter_split_object *intern TSRMLS_DC);

char *formatter_kml_split_output(formatter_split_object *intern);