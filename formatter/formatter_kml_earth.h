#pragma once

#include "formatter.h"
#include <math.h>

char *formatter_kml_earth_output(formatter_t *this);
char *get_colour_by_height(coordinate_set_t *set);
char *get_colour_by_speed(coordinate_set_t *set);
char *get_colour_by_climb_rate(coordinate_set_t *set);
char *get_colour_by_time(coordinate_set_t *set);
char *get_kml_styles_earth();
#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI