#ifndef GEOMETRY_formatter_H
#define GEOMETRY_formatter_H

typedef struct formatter_object {
    zend_object std;

    char *name;
    int id;

    coordinate_set_object *set;
    task_object *open_distance;
    task_object *out_and_return;
    task_object *triangle;
    task_object *task;
} formatter_object;

inline formatter_object* fetch_formatter_object(zend_object* obj) {
    return (formatter_object*) ((char*) obj - XtOffsetOf(formatter_object, std));
}

#endif