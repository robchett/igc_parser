#pragma once

#include "formatter.h"

void formatter_kml_comp_init(formatter_comp_t *obj, size_t size, coordinate_set_t **set, task_t *task);
void formatter_kml_comp_output(formatter_comp_t *obj, char *filename, char *js_filename);