#pragma once

#include "formatter.h"

typedef struct formatter_split_object {
    coordinate_set_t *set;
} formatter_split_object;

char *formatter_kml_split_output(formatter_split_object *this);