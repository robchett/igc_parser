#include <php.h>
#include <math.h>
#include "string_manip.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "geometry.h"
#include "helmert.h"
#include "task.h"

zend_object_handlers task_object_handler;

void init_task(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "task", task_methods);
    task_ce = zend_register_internal_class(&ce TSRMLS_CC);
    task_ce->create_object = create_task_object;

    memcpy(&task_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    task_object_handler.free_obj = free_task_object;
    task_object_handler.offset = XtOffsetOf(task_object, std);
}

zend_object* create_task_object(zend_class_entry *class_type TSRMLS_DC) {
    task_object* retval;

    retval = ecalloc(1, sizeof(task_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&retval->std, class_type TSRMLS_CC);
    object_properties_init(&retval->std, class_type);

    retval->std.handlers = &task_object_handler;

    return &retval->std;
}

task_object* fetch_task_object(zval* obj) {
    return (task_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(task_object, std));
}

void free_task_object(task_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    efree(intern);
}

static zend_function_entry task_methods[] = {
    PHP_ME(task, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(task, get_distance, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(task, get_gridref, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(task, get_duration, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(task, get_coordinate_ids, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(task, completes_task, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD(task, __construct) {
    zval *coordinate_zval_1 = NULL, *coordinate_zval_2 = NULL, *coordinate_zval_3 = NULL, *coordinate_zval_4 = NULL, *coordinate_zval_5 = NULL;
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);

    if (
        zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC,
            "|OOOOO",
            &coordinate_zval_1, coordinate_ce,
            &coordinate_zval_2, coordinate_ce,
            &coordinate_zval_3, coordinate_ce,
            &coordinate_zval_4, coordinate_ce,
            &coordinate_zval_5, coordinate_ce
        ) == FAILURE
    ) {
        return;
    }

    coordinate_object *coordinate_1, *coordinate_2, *coordinate_3, *coordinate_4, *coordinate_5;
    coordinate_1 = coordinate_zval_1 ? fetch_coordinate_object(coordinate_zval_1 TSRMLS_CC) : NULL;
    coordinate_2 = coordinate_zval_2 ? fetch_coordinate_object(coordinate_zval_2 TSRMLS_CC) : NULL;
    coordinate_3 = coordinate_zval_3 ? fetch_coordinate_object(coordinate_zval_3 TSRMLS_CC) : NULL;
    coordinate_4 = coordinate_zval_4 ? fetch_coordinate_object(coordinate_zval_4 TSRMLS_CC) : NULL;
    coordinate_5 = coordinate_zval_5 ? fetch_coordinate_object(coordinate_zval_5 TSRMLS_CC) : NULL;

    if (coordinate_4 && coordinate_1 && !coordinate_5 && coordinate_1->lat == coordinate_4->lat) {
        intern->type = TRIANGLE;
        intern->size = 4;
        intern->coordinate = emalloc(sizeof(coordinate_object) * 4);
        intern->coordinate[0] = coordinate_1;
        intern->coordinate[1] = coordinate_2;
        intern->coordinate[2] = coordinate_3;
        intern->coordinate[3] = coordinate_4;
    } else if (coordinate_3 && coordinate_1 && !coordinate_4 && coordinate_1->lat == coordinate_3->lat) {
        intern->type = OUT_AND_RETURN;
        intern->size = 3;
        intern->coordinate = emalloc(sizeof(coordinate_object) * 3);
        intern->coordinate[0] = coordinate_1;
        intern->coordinate[1] = coordinate_2;
        intern->coordinate[2] = coordinate_3;
    } else {
        intern->type = OPEN_DISTANCE;
        if (coordinate_5) {
            intern->size = 5;
            intern->coordinate = emalloc(sizeof(coordinate_object) * 5);
            intern->coordinate[0] = coordinate_1;
            intern->coordinate[1] = coordinate_2;
            intern->coordinate[2] = coordinate_3;
            intern->coordinate[3] = coordinate_4;
            intern->coordinate[4] = coordinate_5;
        } else if (coordinate_4) {
            intern->size = 4;
            intern->coordinate = emalloc(sizeof(coordinate_object) * 4);
            intern->coordinate[0] = coordinate_1;
            intern->coordinate[1] = coordinate_2;
            intern->coordinate[2] = coordinate_3;
            intern->coordinate[3] = coordinate_4;
        } else if (coordinate_3) {
            intern->size = 3;
            intern->coordinate = emalloc(sizeof(coordinate_object) * 3);
            intern->coordinate[0] = coordinate_1;
            intern->coordinate[1] = coordinate_2;
            intern->coordinate[2] = coordinate_3;
        } else if (coordinate_2) {
            intern->size = 2;
            intern->coordinate = emalloc(sizeof(coordinate_object) * 2);
            intern->coordinate[0] = coordinate_1;
            intern->coordinate[1] = coordinate_2;
        }
    }
}

PHP_METHOD(task, completes_task) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);
    zval *coordinate_set_zval = NULL;

    if (zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "O", &coordinate_set_zval, coordinate_set_ce) == FAILURE) {
        return;
    }

    coordinate_set_object *set = fetch_coordinate_set_object(coordinate_set_zval TSRMLS_CC);
    RETURN_BOOL(completes_task(set, intern));
}

int completes_task(coordinate_set_object *set, task_object *task) {
    int16_t i = 0, j = 0;
    coordinate_object *task_point;
    coordinate_object *track_point = set->first;
start:
    for (i; i < task->size; i++) {
        task_point = task->coordinate[i];
        while (track_point) {
            j++;
            double distance = get_distance_precise(track_point, task_point);
            if (distance < 0.4) {
                i++;
                goto start;
            }
            track_point = track_point->next;
        }
        return 0;
    }
    return 1;
}

PHP_METHOD(task, get_coordinate_ids) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);

    int16_t i = 0;
    char coordinates[intern->size * 6];
    memset(coordinates, 0, intern->size * 6);

    for (i; i < intern->size; i++) {
        coordinate_object *point1 = intern->coordinate[i];
        char coordinate[6];
        sprintf(coordinate, "%d,", point1->id + 1);
        coordinate[5] = '\0';
        strcat(coordinates, coordinate);
    }
    coordinates[(intern->size * 6) - 1] = '\0';

    RETURN_STRING(coordinates);
}

PHP_METHOD(task, get_gap_ids) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);
    if (intern->gap) {
        char coordinate[12];
        sprintf(coordinate, "%5d,%5d", intern->gap[0]->id + 1, intern->gap[1]->id + 1);
        RETURN_STRING(coordinate);
    }
    RETURN_NULL();
}

PHP_METHOD(task, get_gridref) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);

    int16_t i = 0;
    int16_t end = intern->size;

    char *coordinates = create_buffer("");

    for (i; i < end; i++) {
        coordinate_object *point1 = intern->coordinate[i];
        char *gridref = get_os_grid_ref(point1);
        coordinates = vstrcat(coordinates, gridref, ";", NULL);
        efree(gridref);
    }

    RETURN_STRING(coordinates);
}

PHP_METHOD(task, get_duration) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(get_task_time(intern));
}

int get_task_time(task_object *task) {
    return task->coordinate[task->size - 1]->timestamp - task->coordinate[0]->timestamp;
}

double get_task_distance(task_object *task) {
    int16_t i = 0;
    int16_t end = task->size - 1;

    double distance = 0;
    for (i; i < end; i++) {
        coordinate_object *point1 = task->coordinate[i];
        coordinate_object *point2 = task->coordinate[i + 1];
        double part = get_distance_precise(point1, point2);
        distance += part;
    }

    if (task->gap) {
        coordinate_object *point1 = task->gap[0];
        coordinate_object *point2 = task->gap[1];
        distance -= get_distance_precise(point1, point2);
    }
    return distance;
}

PHP_METHOD(task, get_distance) {
    task_object *intern = fetch_task_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE(get_task_distance(intern));
}