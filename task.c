#include "main.h"
#include <string.h>
#include "string_manip.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "igc_parser.h"
#include "helmert.h"
#include "include/json/jansson.h"
#include "task.h"
#include "stdarg.h"

void task_init(task_t *obj, task_type type, int8_t size, ...) {
    obj->type = type;
    obj->size = size;
    obj->gap = NULL;

    obj->coordinate = NEW(coordinate_t *, size);

    va_list va;
    va_start(va, size);
    for (size_t i = 0; i < size; i++) {
        coordinate_t *coordinate = (coordinate_t *)va_arg(va, coordinate_t *);
        obj->coordinate[i] = coordinate;
    }
    va_end(va);
}

void task_deinit(task_t *obj) {
    if (obj->gap) {
        free(obj->gap);
    }
    free(obj->coordinate);
    free(obj);
}

void task_add_gap(task_t *obj, coordinate_t *start, coordinate_t *end) {
    obj->gap = NEW(coordinate_t *, 2);
    obj->gap[0] = start;
    obj->gap[1] = end;
}

int8_t task_completes_task(task_t *obj, coordinate_set_t *set) {
    int16_t i = 0, j = 0;
    coordinate_t *task_point;
    coordinate_t *track_point = set->first;
start:
    for (i; i < obj->size; i++) {
        task_point = obj->coordinate[i];
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

char *task_get_coordinate_ids(task_t *obj) {
    char *res = NEW(char, 40);
    coordinate_t **c = obj->coordinate;
    switch (obj->size) {
        case 1:
            sprintf(res, "%d", c[0]->id);
            break;
        case 2:
            sprintf(res, "%d,%d", c[0]->id, c[1]->id);
            break;
        case 3:
            sprintf(res, "%d,%d,%d", c[0]->id, c[1]->id, c[2]->id);
            break;
        case 4:
            sprintf(res, "%d,%d,%d,%d", c[0]->id, c[1]->id, c[2]->id, c[3]->id);
            break;
        case 5:
            sprintf(res, "%d,%d,%d,%d,%d", c[0]->id, c[1]->id, c[2]->id, c[3]->id, c[4]->id);
            break;
        default:
            sprintf(res, "N/A");
            break;
    }
    return res;
}

char *task_get_gap_ids(task_t *obj) {
    if (obj->gap) {
        char *coordinate = NEW(char, 12);
        sprintf(coordinate, "%d,%d", obj->gap[0]->id + 1, obj->gap[1]->id + 1);
        return coordinate;
    }
    return NULL;
}

char *task_get_gridref(task_t *obj) {
    char *coordinates = create_buffer("");

    for (int16_t i = 0; i < obj->size; i++) {
        coordinate_t *point1 = obj->coordinate[i];
        char *gridref = convert_latlng_to_gridref(point1->lat, point1->lng);
        coordinates = vstrcat(coordinates, gridref, ";", NULL);
        free(gridref);
    }

    return coordinates;
}

int get_task_time(task_t *obj) {
    if (obj->gap) {
        return obj->gap[1]->timestamp - obj->gap[0]->timestamp;
    } else {
        return obj->coordinate[obj->size - 1]->timestamp - obj->coordinate[0]->timestamp;
    }
}

double get_task_distance(task_t *task) {
    int16_t i = 0;
    int16_t end = task->size - 1;

    double distance = 0;
    for (i; i < end; i++) {
        coordinate_t *point1 = task->coordinate[i];
        coordinate_t *point2 = task->coordinate[i + 1];
        double part = get_distance_precise(point1, point2);
        distance += part;
    }

    if (task->gap) {
        coordinate_t *point1 = task->gap[0];
        coordinate_t *point2 = task->gap[1];
        distance -= get_distance_precise(point1, point2);
    }
    return distance;
}

task_t *parse_task(json_t *_task) {
    if (_task) {
        if (json_is_object(_task)) {
            task_t *task = NEW(task_t, 1);
            json_t *_task_type = json_object_get(_task, "type");
            const char *task_type = json_is_string(_task_type) ? json_string_value(_task_type) : "os_gridref";
            json_t *_coordinate = json_object_get(_task, "coordinate");
            size_t count = json_array_size(_coordinate);
            task->coordinate = NEW(coordinate_t *, count);
            for (size_t i = 0; i < count; i++) {
                const char *gridref = json_string_value(json_array_get(_coordinate, i));
                double lat = 0, lng = 0;
                convert_gridref_to_latlng(gridref, &lat, &lng);
                coordinate_t *coordinate = NEW(coordinate_t, 1);
                coordinate_init(coordinate, lat, lng, 0, 0);
                task->coordinate[i] = coordinate;
            }
            task->size = count;
            task->gap = NULL;
            if (
                    task->size == 4 &&
                    task->coordinate[0]->lat == task->coordinate[3]->lat &&
                    task->coordinate[0]->lng == task->coordinate[3]->lng
                    ) {
                task->type = TRIANGLE;
            } else if (
                    task->size == 3 &&
                    task->coordinate[0]->lat == task->coordinate[2]->lat &&
                    task->coordinate[0]->lng == task->coordinate[2]->lng
                    ) {
                task->type = OUT_AND_RETURN;
            } else {
                task->type = OPEN_DISTANCE;
            }
        } else {
            // Not object
        }
    }
    return NULL;
}