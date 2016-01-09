#pragma once

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
    unsigned long size;
} distance_map_object;

typedef struct triangle_score {
    unsigned long x,y,z,row,col;
    struct triangle_score *prev, *next;
} triangle_score;

zend_class_entry *distance_map_ce;
zend_object_handlers distance_map_handlers;
static zend_function_entry distance_map_methods[];

zend_object* create_distance_map_object(zend_class_entry *class_type TSRMLS_DC);
void free_distance_map_object(distance_map_object *intern TSRMLS_DC);

void close_gap(distance_map_object *intern, triangle_score *score);

unsigned long score_triangle(distance_map_object *intern, triangle_score *trianlge, int include_gap);

unsigned long maximum_bound_index_back(distance_map_object *map, unsigned long point, unsigned long *index);
unsigned long maximum_bound_index_fwrd(distance_map_object *map, unsigned long point, unsigned long *index);

unsigned long skip_up(distance_map_object *map, unsigned long *index, unsigned long required, unsigned long current, int effected_legs);
unsigned long skip_down(distance_map_object *map, unsigned long *index, unsigned long required, unsigned long current, int effected_legs);

coordinate_object *get_coordinate(distance_map_object *map, unsigned long index);
int create_distance_map(distance_map_object *map, coordinate_set_object *set);

#ifdef DEBUG_LEVEL
     #define MAP(map, from, to) (from >= to ? errn("Map points in wrong order") : map->distances[from][to - from - 1])
  #else 
     #define MAP(map, from, to) map->distances[from][to - from - 1]
#endif
