#include <php.h>
#include "element.h"






zend_object_handlers statistics_object_handler;

void init_statistic(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "statistic", statistic_methods);
    statistic_ce = zend_register_internal_class(&ce TSRMLS_CC);
    statistic_ce->create_object = create_statistic_object;

    memcpy(&statistics_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    statistics_object_handler.free_obj = NULL;
    statistics_object_handler.offset = XtOffsetOf(statistic_object, std);
}

zend_object* create_statistic_object(zend_class_entry *class_type TSRMLS_DC) {
    statistic_object *intern;

    intern = ecalloc(1, sizeof(statistic_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &statistics_object_handler;

    return &intern->std;
}

inline statistic_object* fetch_statistics_object(zval* obj) {
    return (statistic_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(statistic_object, std));
}

static zend_function_entry statistic_methods[] = {
    PHP_ME(statistic, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistic, min, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistic, max, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD (statistic, __construct) {
    statistic_object *intern = fetch_statistics_object(getThis() TSRMLS_CC);
    intern->max = 0;
    intern->min = 0;
}


PHP_METHOD (statistic, min) {
    statistic_object *intern = fetch_statistics_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(intern->min);
}

PHP_METHOD (statistic, max) {
    statistic_object *intern = fetch_statistics_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(intern->max);
}
