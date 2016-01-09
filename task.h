#pragma once

#include "coordinate_set.h"

void init_task(TSRMLS_D);

extern zend_class_entry task;

PHP_METHOD(task, __construct);
PHP_METHOD(task, get_distance);
PHP_METHOD(task, get_gridref);
PHP_METHOD(task, get_coordinate_ids);
PHP_METHOD(task, get_duration);
PHP_METHOD(task, completes_task);

typedef enum {OPEN_DISTANCE, OUT_AND_RETURN, TRIANGLE} task_type;

typedef struct task_object {
    zend_object std;

    coordinate_object **coordinate;
    coordinate_object **gap;
    int16_t size;
    task_type type;
} task_object;

zend_class_entry *task_ce;
static zend_function_entry task_methods[];

zend_object* create_task_object(zend_class_entry *class_type TSRMLS_DC);
void free_task_object(task_object *intern TSRMLS_DC);
double get_task_distance(task_object *task);
int get_task_time(task_object *task);
int completes_task(coordinate_set_object *set, task_object *task);
task_object* fetch_task_object(zval* obj);