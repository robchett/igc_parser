#ifndef GEOMETRY_DISTANCE_MAP_H
#define GEOMETRY_DISTANCE_MAP_H

void init_distance_map(TSRMLS_D);

PHP_METHOD(distance_map, __construct);
PHP_METHOD(distance_map, get);
PHP_METHOD(distance_map, get_precise);
PHP_METHOD(distance_map, score_open_distance_3tp);
PHP_METHOD(distance_map, score_out_and_return);
PHP_METHOD(distance_map, score_triangle);

typedef struct distance_map_object {
    zend_object std;
    coordinate_set_object *coordinate_set;
    unsigned long **distances;
    long size;
    unsigned long maximum_distance;
} distance_map_object;

zend_class_entry *distance_map_ce;
zend_object_handlers distance_map_handlers;
static zend_function_entry distance_map_methods[];

zend_object_value create_distance_map_object(zend_class_entry *class_type TSRMLS_DC);
void free_distance_map_object(distance_map_object *intern TSRMLS_DC);
static zend_object_value clone_distance_map_object(zval *object TSRMLS_DC);

// zval* _distance_map_offset_get(distance_map_object *intern, size_t offset);
// void _distance_map_offset_set(distance_map_object *intern, long offset, zval *value);
unsigned long map_read(distance_map_object *map, long i, long j);
unsigned long maximum_bound_index_back(distance_map_object *map, long point, long *index);
unsigned long maximum_bound_index_fwrd(distance_map_object *map, long point, long *index);
unsigned long furthest_between(distance_map_object *map, long from, long to);
//#define SKIP(distance, target, maximum_distance, index) 
#define MAP(map, from, to)  map->distances[to - from][from]
coordinate_object *get_coordinate(distance_map_object *map, long index);
#endif