#ifndef GEOMETRY_STATISTICS_ELEMENT_H
#define GEOMETRY_STATISTICS_ELEMENT_H

void init_statistic(TSRMLS_D);

PHP_METHOD(statistic, __construct);
PHP_METHOD(statistic, min);
PHP_METHOD(statistic, max);

typedef struct statistic_object {
    zend_object std;
    double min, max;
} statistic_object;

zend_class_entry *statistic_ce;
static zend_function_entry statistic_methods[];
zend_object_handlers statistic_handlers;

zend_object* create_statistic_object(zend_class_entry *class_type TSRMLS_DC);
void free_statistic_object(statistic_object *intern TSRMLS_DC);
static zend_object statistic_object_clone(zval *object TSRMLS_DC);
statistic_object* fetch_statistics_object(zend_object* obj);

#endif
