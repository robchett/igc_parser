#pragma once

#include "formatter.h"

void formatter_js_init(formatter_t *obj, coordinate_set_t *set, int64_t id, task_t *task_od, task_t *task_or, task_t *task_tr);
char *formatter_js_output(formatter_t *obj, char *filename);