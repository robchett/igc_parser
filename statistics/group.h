#pragma once

void init_statistics_set(TSRMLS_D);

PHP_METHOD(statistics_set, __construct);
PHP_METHOD(statistics_set, height);
PHP_METHOD(statistics_set, speed);
PHP_METHOD(statistics_set, climb);

typedef struct statistics_set_object {
    zend_object std;
    double height_max, speed_max, climb_max;
    double height_min, speed_min, climb_min;
} statistics_set_object;

zend_class_entry *statistics_set_ce;
static zend_function_entry statistics_set_methods[];
zend_object_handlers statistics_set_handlers;

zend_object* create_statistics_set_object(zend_class_entry *class_type TSRMLS_DC);
void free_statistics_set_object(statistics_set_object *intern TSRMLS_DC);
static zend_object statistics_set_object_clone(zval *object TSRMLS_DC);
statistics_set_object* fetch_statistics_set_object(zval* obj);