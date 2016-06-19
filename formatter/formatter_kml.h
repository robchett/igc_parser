#pragma once

#include "formatter.h"

void formatter_kml_init(formatter_t *obj, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *ft_task, task_t *task);
void formatter_kml_output(formatter_t *obj, char *filename);
void set_formatter_values(HDF *, formatter_t *);

#define toRAD *M_PI / 180
#define toDEG *180 / M_PI