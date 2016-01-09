#include <php.h>
#include "element.h"
#include "group.h"





zend_object_handlers statistics_set_object_handler;

void init_statistics_set(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "statistics_set", statistics_set_methods);
    statistics_set_ce = zend_register_internal_class(&ce TSRMLS_CC);
    statistics_set_ce->create_object = create_statistics_set_object;

    memcpy(&statistics_set_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    statistics_set_object_handler.free_obj = NULL;
    statistics_set_object_handler.offset = XtOffsetOf(statistic_object, std);
}

zend_object* create_statistics_set_object(zend_class_entry *class_type TSRMLS_DC) {
    statistics_set_object *intern;

    intern = ecalloc(1, sizeof(statistics_set_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &statistics_set_object_handler;

    return &intern->std;  
}

statistics_set_object* fetch_statistics_set_object(zval* obj) {
    return (statistics_set_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(statistics_set_object, std));
}

static zend_function_entry statistics_set_methods[] = {
    PHP_ME(statistics_set, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, climb, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, speed, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, height, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD (statistics_set, __construct) {
    statistics_set_object *intern = fetch_statistics_set_object(getThis() TSRMLS_CC);
    intern->height_max = intern->speed_max = intern->climb_max = 0;
    intern->height_min = intern->speed_min = intern->climb_min = 0;
}


PHP_METHOD (statistics_set, height) {
    statistics_set_object *intern = fetch_statistics_set_object(getThis() TSRMLS_CC);
    zval *ret = emalloc(sizeof(zval));
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = fetch_statistics_object(ret TSRMLS_CC);
    return_intern->min = intern->height_min;
    return_intern->max = intern->height_max;
    RETURN_ZVAL(ret, 1, 0);
}

PHP_METHOD (statistics_set, speed) {
    statistics_set_object *intern = fetch_statistics_set_object(getThis() TSRMLS_CC);
    zval *ret = emalloc(sizeof(zval));
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = fetch_statistics_object(ret TSRMLS_CC);
    return_intern->min = intern->speed_min;
    return_intern->max = intern->speed_max;
    RETURN_ZVAL(ret, 1, 0);
}

PHP_METHOD (statistics_set, climb) {
    statistics_set_object *intern = fetch_statistics_set_object(getThis() TSRMLS_CC);
    zval *ret = emalloc(sizeof(zval));
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = fetch_statistics_object(ret TSRMLS_CC);
    return_intern->min = intern->climb_min;
    return_intern->max = intern->climb_max;
    RETURN_ZVAL(ret, 1, 0);
}
