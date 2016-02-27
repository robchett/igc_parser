#pragma once

#include "coordinate.h"

typedef struct coordinate_subset_t {
    coordinate_t *first, *last;
    int64_t length;
    struct coordinate_subset_t *next, *prev;
} coordinate_subset_t;

struct coordinate_set_t;
typedef struct coordinate_set_t {
    coordinate_t *first, *last;
    coordinate_t *real_first, *real_last;
    uint64_t length, real_length;
    int64_t offset;

    // Date
    int64_t day;
    int64_t month;
    int64_t year;

    // Graph limits
    int64_t max_ele, max_ele_t, min_ele, min_ele_t;
    int64_t max_alt, max_alt_t, min_alt, min_alt_t;
    double max_climb_rate, min_climb_rate, max_speed;

    // Subset
    coordinate_subset_t *first_subset, *last_subset;
    int64_t subset_count;
} coordinate_set_t;

void coordinate_set_init(coordinate_set_t *obj);
void coordinate_set_deinit(coordinate_set_t *obj);

int coordinate_set_parse_igc(coordinate_set_t *obj, char *string);

int coordinate_set_repair(coordinate_set_t *obj);
int8_t coordinate_set_simplify(coordinate_set_t *set, size_t max_size);
int8_t coordinate_set_extrema(coordinate_set_t *obj);
int8_t coordinate_set_trim(coordinate_set_t *obj);

void coordinate_set_select_section(coordinate_set_t *obj, uint16_t start, uint16_t end);
