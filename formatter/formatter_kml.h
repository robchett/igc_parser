#pragma once

#include "formatter.h"

void formatter_kml_init(formatter_t *this, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task);
char *formatter_kml_output(formatter_t *this);
char *formatter_kml_output_split(formatter_t *this);

#define toRAD *M_PI / 180
#define toDEG *180 / M_PI