#include "main.h"
#include <string.h>
#include "string_manip.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "igc_parser.h"
#include "helmert.h"
#include "task.h"
#include "stdarg.h"

void task_init(task_t *this, task_type type, int8_t size, ...) {
    this->type = type;
    this->size = size;
    this->gap = NULL;

    this->coordinate = malloc(sizeof(coordinate_t *) * size);

    va_list va;
    va_start(va, size);
    for (size_t i = 0; i < size; i++) {
        coordinate_t *coordinate = (coordinate_t *)va_arg(va, coordinate_t *);
        this->coordinate[i] = coordinate;
    }
    va_end(va);
}

void task_deinit(task_t *this) {
    if (this->gap) {
        free(this->gap);
    }
    free(this->coordinate);
    free(this);
}

void task_add_gap(task_t *this, coordinate_t *start, coordinate_t *end) {
    this->gap = malloc(sizeof(coordinate_t *) * 2);
    this->gap[0] = start;
    this->gap[1] = end;
}

int8_t task_completes_task(task_t *this, coordinate_set_t *set) {
    int16_t i = 0, j = 0;
    coordinate_t *task_point;
    coordinate_t *track_point = set->first;
start:
    for (i; i < this->size; i++) {
        task_point = this->coordinate[i];
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

char *task_get_coordinate_ids(task_t *this) {
    char *res = calloc(40, sizeof(char));
    coordinate_t **c = this->coordinate;
    switch (this->size) {
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

char *task_get_gap_ids(task_t *this) {
    if (this->gap) {
        char *coordinate = malloc(sizeof(char) * 12);
        sprintf(coordinate, "%d,%d", this->gap[0]->id + 1, this->gap[1]->id + 1);
        return coordinate;
    }
    return NULL;
}

char *task_get_gridref(task_t *this) {
    char *coordinates = create_buffer("");

    for (int16_t i = 0; i < this->size; i++) {
        coordinate_t *point1 = this->coordinate[i];
        char *gridref = convert_latlng_to_gridref(point1->lat, point1->lng);
        coordinates = vstrcat(coordinates, gridref, ";", NULL);
        free(gridref);
    }

    return coordinates;
}

int get_task_time(task_t *this) {
    if (this->gap) {
        return this->gap[1]->timestamp - this->gap[0]->timestamp;
    } else {
        return this->coordinate[this->size - 1]->timestamp - this->coordinate[0]->timestamp;
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