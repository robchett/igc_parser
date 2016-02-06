#pragma once

#include "formatter.h"

char *formatter_kml_earth_output(formatter_t *this, char *filename);
char *get_colour_by_height(coordinate_set_t *set);
char *get_colour_by_speed(coordinate_set_t *set);
char *get_colour_by_climb_rate(coordinate_set_t *set);
char *get_colour_by_time(coordinate_set_t *set);
char *get_kml_styles_earth();

void formatter_kml_earth_init(formatter_t *this, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task);
#define toRAD *M_PI / 180
#define toDEG *180 / M_PI