#include <php.h>
#include "element.h"
#include "group.h"

void init_statistics_set(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "statistics_set", statistics_set_methods);
    statistics_set_ce = zend_register_internal_class(&ce TSRMLS_CC);
    statistics_set_ce->create_object = create_statistics_set_object;

    memcpy(&statistics_set_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    statistics_set_handlers.clone_obj = statistics_set_object_clone;
}


zend_object_value create_statistics_set_object(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;

    // allocate the struct we're going to use
    statistics_set_object *intern = emalloc(sizeof(statistics_set_object));
    memset(intern, 0, sizeof(statistics_set_object));

    // create a table for class properties
    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    // create a destructor for this struct
    retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_object_std_dtor, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

static zend_object_value statistics_set_object_clone(zval *object TSRMLS_DC) {
    statistics_set_object *old_object = zend_object_store_get_object(object TSRMLS_CC);
    zend_object_value new_object_val = create_statistics_set_object(Z_OBJCE_P(object) TSRMLS_CC);
    statistics_set_object *new_object = zend_object_store_get_object_by_handle(new_object_val.handle TSRMLS_CC);

    zend_objects_clone_members(&new_object->std, new_object_val, &old_object->std, Z_OBJ_HANDLE_P(object) TSRMLS_CC);

    return new_object_val;
}

static zend_function_entry statistics_set_methods[] = {
    PHP_ME(statistics_set, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, climb, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, speed, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistics_set, height, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD (statistics_set, __construct) {
    statistics_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    intern->height_max = intern->speed_max = intern->climb_max = 0;
    intern->height_min = intern->speed_min = intern->climb_min = 0;
}


PHP_METHOD (statistics_set, height) {
    statistics_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *ret;
    MAKE_STD_ZVAL(ret);
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
    return_intern->min = intern->height_min;
    return_intern->max = intern->height_max;
    RETURN_ZVAL(ret, 1, 0);
}

PHP_METHOD (statistics_set, speed) {
    statistics_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *ret;
    MAKE_STD_ZVAL(ret);
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
    return_intern->min = intern->speed_min;
    return_intern->max = intern->speed_max;
    RETURN_ZVAL(ret, 1, 0);
}

PHP_METHOD (statistics_set, climb) {
    statistics_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *ret;
    MAKE_STD_ZVAL(ret);
    object_init_ex(ret, statistic_ce);
    statistic_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
    return_intern->min = intern->climb_min;
    return_intern->max = intern->climb_max;
    RETURN_ZVAL(ret, 1, 0);
}