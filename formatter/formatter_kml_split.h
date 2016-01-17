#pragma once

#include "formatter.h"

typedef struct formatter_split_object { coordinate_set_t *set; } formatter_split_t;

void formatter_kml_split_init(formatter_split_t *this, coordinate_set_t *set);
char *formatter_kml_split_output(formatter_split_t *this, char *filename);