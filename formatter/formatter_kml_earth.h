#pragma once

#include "formatter.h"

char *formatter_kml_earth_output(formatter_t *obj, char *filename);
void formatter_kml_earth_init(formatter_t *obj, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task_ft, task_t *task);

#define toRAD *M_PI / 180
#define toDEG *180 / M_PI