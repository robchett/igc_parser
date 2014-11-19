#ifndef PHP_GEOMETRY_H
#define PHP_GEOMETRY_H 1
#define PHP_GEOMETRY_VERSION "0.0.1"
#define PHP_GEOMETRY_EXTNAME "geometry"

#define ZEND_DEBUG 0

PHP_MINIT_FUNCTION(geometry);

char *get_os_grid_ref(coordinate_object *point);
char *gridref_number_to_letter(long e, long n);

#define is_object_of_type(zval_ptr, type) (zval_ptr && Z_TYPE_P(zval_ptr) == IS_OBJECT && Z_OBJCE(*zval_ptr) == type)
#endif