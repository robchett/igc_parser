#ifndef PHP_GEOMETRY_H
#define PHP_GEOMETRY_H 1
#define PHP_GEOMETRY_VERSION "0.0.1"
#define PHP_GEOMETRY_EXTNAME "geometry"

#define ZEND_DEBUG 0

PHP_MINIT_FUNCTION(geometry);

char *get_os_grid_ref(coordinate_object *point);
char *gridref_number_to_letter(long e, long n);
#endif