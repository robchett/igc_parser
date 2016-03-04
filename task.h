#pragma once

#include "coordinate_set.h"

typedef enum { OPEN_DISTANCE, OUT_AND_RETURN, GOAL, TRIANGLE } task_type;

typedef struct task_t {
    coordinate_t **coordinate;
    coordinate_t **gap;
    int16_t size;
    task_type type;
} task_t;

double get_task_distance(task_t *task);
int get_task_time(task_t *task);
int8_t task_completes_task(task_t *obj, coordinate_set_t *set);
char *task_get_coordinate_ids(task_t *obj);

char *task_get_gridref(task_t *obj);
char *task_get_duration(task_t *obj);

void task_init(task_t *obj, task_type type, int8_t size, ...);
void task_init_ex(task_t *obj, size_t size, coordinate_t **coordinate);
void task_deinit(task_t *obj);
void task_add_gap(task_t *obj, coordinate_t *start, coordinate_t *end);

struct json_t;
typedef struct json_t json_t;