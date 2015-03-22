#ifndef GEOMETRY_formatter_js_H
#define GEOMETRY_formatter_js_H

#include "formatter.h"

void init_formatter_js(TSRMLS_D);
extern zend_class_entry formatter_js;

PHP_METHOD(formatter_js, __construct);
PHP_METHOD(formatter_js, output);

zend_class_entry *formatter_js_ce;
static zend_function_entry formatter_js_methods[];

zend_object* create_formatter_js_object(zend_class_entry *class_type TSRMLS_DC);
void free_formatter_js_object(formatter_object *intern TSRMLS_DC);

#endif