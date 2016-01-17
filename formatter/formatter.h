#pragma once

#include "../task.h"

typedef struct formatter_t {
    char *name;
    int16_t id;

    coordinate_set_t *set;
    task_t *open_distance;
    task_t *out_and_return;
    task_t *triangle;
    task_t *task;
} formatter_t;