#pragma once

#include "coordinate.h"

char *convert_latlng_to_gridref(coordinate_t *point);
int convert_gridref_to_latlng(const char *input, double *lat, double *lng);