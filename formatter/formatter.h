#pragma once

#include "../task.h"
#include <string.h>
#include "../include/clearsilver/clearsilver.h"

typedef struct formatter_t {
    char *name;
    int16_t id;

    coordinate_set_t *set;
    task_t *open_distance;
    task_t *out_and_return;
    task_t *triangle;
    task_t *flat_triangle;
    task_t *task;
} formatter_t;

# define _cs_set_value(hdf, key, value) if ((err = hdf_set_value(hdf, key, value)) != STATUS_OK) { nerr_log_error(err); return; }

# define _cs_set_value_d(hdf, key, value) if ((err = hdf_set_valuef(hdf, "#key=%d", value)) != STATUS_OK) {  nerr_log_error(err); return; }

# define _cs_set_value_f(hdf, key, value) if ((err = hdf_set_valuef(hdf, "#key=%f", value)) != STATUS_OK) {  nerr_log_error(err); return; }

# define _cs_set_valuef(hdf, format, ...) if ((err = hdf_set_valuef(hdf, format, ##__VA_ARGS__)) != STATUS_OK) {  nerr_log_error(err); return; }