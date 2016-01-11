#pragma once

#include "../coordinate_set.h"

typedef struct statistics_set_t {
    double height_max, speed_max, climb_max;
    double height_min, speed_min, climb_min;
} statistics_set_t;

void statistics_set_init(statistics_set_t *this, coordinate_set_t *set);


// 
// 
// 
// 