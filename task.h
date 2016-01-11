#pragma once

#include "coordinate_set.h"

typedef enum {OPEN_DISTANCE, OUT_AND_RETURN, TRIANGLE} task_type;

typedef struct task_t {
    coordinate_t **coordinate;
    coordinate_t **gap;
    int16_t size;
    task_type type;
} task_t;

double get_task_distance(task_t *task);
int get_task_time(task_t *task);
int completes_task(coordinate_set_t *set, task_t *task);

void task_init(task_t *this, task_type type, int8_t size, ...);