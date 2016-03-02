#pragma once

typedef struct distance_map_t {
    coordinate_set_t *coordinate_set;
    uint64_t **distances;
    uint64_t size;
} distance_map_t;

typedef struct triangle_score {
    uint64_t x, y, z, row, col;
    struct triangle_score *prev, *next;
} triangle_score;

void close_gap(distance_map_t *obj, triangle_score *score);

uint64_t score_triangle(distance_map_t *obj, triangle_score *trianlge, int16_t include_gap);

uint64_t maximum_bound_index_back(distance_map_t *map, uint64_t point, uint64_t *index);
uint64_t maximum_bound_index_fwrd(distance_map_t *map, uint64_t point, uint64_t *index);

uint64_t skip_up(distance_map_t *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs);
uint64_t skip_down(distance_map_t *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs);
triangle_score *check_y(int64_t x, int64_t y, int64_t z, int64_t row, int64_t col, distance_map_t *obj, uint64_t *_minleg, triangle_score *score, float height_ratio);

coordinate_t *get_coordinate(distance_map_t *map, uint64_t index);

void distance_map_init(distance_map_t *map, coordinate_set_t *set);
void distance_map_deinit(distance_map_t *obj);

task_t *distance_map_score_triangle(distance_map_t *obj, float height_ratio);
task_t *distance_map_score_out_and_return(distance_map_t *obj);
task_t *distance_map_score_open_distance_3tp(distance_map_t *obj);

#ifdef DEBUG_LEVEL
#define MAP(map, from, to) (from >= to ? errn("Map points in wrong order") : map->distances[from][to - from - 1])
#else
#define MAP(map, from, to) map->distances[from][to - from - 1]
#endif
