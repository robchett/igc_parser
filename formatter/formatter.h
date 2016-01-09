#pragma once

#include "../task.h"

typedef struct formatter_object {
    zend_object std;

    char *name;
    int16_t id;

    coordinate_set_object *set;
    task_object *open_distance;
    task_object *out_and_return;
    task_object *triangle;
    task_object *task;
} formatter_object;

inline formatter_object* fetch_formatter_object(zval* obj) {
    return (formatter_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(formatter_object, std));
}