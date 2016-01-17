#include "../main.h"
#include "element.h"
#include "group.h"

void statistics_set_init(statistics_set_t *this, coordinate_set_t *set) {
    this->height_max = set->max_ele;
    // this->height_max_t = set->max_ele_t;
    this->height_min = set->min_ele;
    // this->height_min_t = set->min_ele_t;

    this->climb_max = set->max_climb_rate;
    // this->climb_max_t = set->max_c_t;
    this->climb_min = set->min_climb_rate;
    // this->climb_min_t = set->min_ele_t;

    this->speed_max = set->max_speed;
    // this->speed_max_t = set->max_ele_t;
    this->speed_min = 0;
}

void statistics_set_height(statistics_set_t *this) {
    // return_this->min = this->height_min;
    // return_this->max = this->height_max;
}

void statistics_set_speed(statistics_set_t *this) {
    // return_this->min = this->speed_min;
    // return_this->max = this->speed_max;
}

void statistics_set_climb(statistics_set_t *this) {
    // return_this->min = this->climb_min;
    // return_this->max = this->climb_max;
}
