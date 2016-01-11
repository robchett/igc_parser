#include "main.h"
#include <math.h>
#include "string_manip.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "igc_parser.h"
#include "helmert.h"
#include "task.h"

void task_init(task_t *this, task_type type, int8_t size, ...) {
    this->type = type;
    this->size = size;

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

void task_get_coordinate_ids(task_t *this) {
    // int16_t i = 0;
    // char coordinates[this->size * 6];
    // memset(coordinates, 0, this->size * 6);

    // for (i; i < this->size; i++) {
    //     coordinate_t *point1 = this->coordinate[i];
    //     char coordinate[6];
    //     sprintf(coordinate, "%d,", point1->id + 1);
    //     coordinate[5] = '\0';
    //     strcat(coordinates, coordinate);
    // }
    // coordinates[(this->size * 6) - 1] = '\0';

    // RETURN_STRING(coordinates);
}

void task_get_gap_ids(task_t *this) {
    // if (this->gap) {
    //     char coordinate[12];
    //     sprintf(coordinate, "%5d,%5d", this->gap[0]->id + 1, this->gap[1]->id + 1);
    //     RETURN_STRING(coordinate);
    // }
    // RETURN_NULL();
}

void task_get_gridref(task_t *this) {
    // int16_t i = 0;
    // int16_t end = this->size;

    // char *coordinates = create_buffer("");

    // for (i; i < end; i++) {
    //     coordinate_t *point1 = this->coordinate[i];
    //     char *gridref = get_os_grid_ref(point1);
    //     coordinates = vstrcat(coordinates, gridref, ";", NULL);
    //     free(gridref);
    // }

    // RETURN_STRING(coordinates);
}

int get_task_time(task_t *task) {
    return task->coordinate[task->size - 1]->timestamp - task->coordinate[0]->timestamp;
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