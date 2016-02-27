#include "../main.h"
#include "element.h"
#include "group.h"

void statistics_set_init(statistics_set_t *obj, coordinate_set_t *set) {
    obj->height_max = set->max_ele;
    // obj->height_max_t = set->max_ele_t;
    obj->height_min = set->min_ele;
    // obj->height_min_t = set->min_ele_t;

    obj->climb_max = set->max_climb_rate;
    // obj->climb_max_t = set->max_c_t;
    obj->climb_min = set->min_climb_rate;
    // obj->climb_min_t = set->min_ele_t;

    obj->speed_max = set->max_speed;
    // obj->speed_max_t = set->max_ele_t;
    obj->speed_min = 0;
}

void statistics_set_height(statistics_set_t *obj) {
    // return_obj->min = obj->height_min;
    // return_obj->max = obj->height_max;
}

void statistics_set_speed(statistics_set_t *obj) {
    // return_obj->min = obj->speed_min;
    // return_obj->max = obj->speed_max;
}

void statistics_set_climb(statistics_set_t *obj) {
    // return_obj->min = obj->climb_min;
    // return_obj->max = obj->climb_max;
}
