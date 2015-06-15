#include <php.h>
#include "element.h"

void init_statistic(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "statistic", statistic_methods);
    statistic_ce = zend_register_internal_class(&ce TSRMLS_CC);
    statistic_ce->create_object = create_statistic_object;

    memcpy(&statistic_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    statistic_handlers.clone_obj = statistic_object_clone;
}


zend_object_value create_statistic_object(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;

    // allocate the struct we're going to use
    statistic_object *intern = emalloc(sizeof(statistic_object));
    memset(intern, 0, sizeof(statistic_object));

    // create a table for class properties
    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    // create a destructor for this struct
    retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_object_std_dtor, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

static zend_object_value statistic_object_clone(zval *object TSRMLS_DC) {
    statistic_object *old_object = zend_object_store_get_object(object TSRMLS_CC);
    zend_object_value new_object_val = create_statistic_object(Z_OBJCE_P(object) TSRMLS_CC);
    statistic_object *new_object = zend_object_store_get_object_by_handle(new_object_val.handle TSRMLS_CC);

    zend_objects_clone_members(&new_object->std, new_object_val, &old_object->std, Z_OBJ_HANDLE_P(object) TSRMLS_CC);

    return new_object_val;
}

static zend_function_entry statistic_methods[] = {
    PHP_ME(statistic, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistic, min, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(statistic, max, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD (statistic, __construct) {
    statistic_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    intern->max = 0;
    intern->min = 0;
}


PHP_METHOD (statistic, min) {
    statistic_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(intern->min);
}

PHP_METHOD (statistic, max) {
    statistic_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(intern->max);
}