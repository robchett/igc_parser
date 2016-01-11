#pragma once

#include "formatter.h"

char *formatter_kml_output(formatter_t *this);
char *formatter_kml_output_split(formatter_t *this);

#define toRAD * M_PI / 180
#define toDEG * 180 / M_PI