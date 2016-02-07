#pragma once

#define toRAD *M_PI / 180
#define toDEG *180 / M_PI

char *convert_latlng_to_gridref(double lat, double lng);
int convert_gridref_to_latlng(const char *input, double *lat, double *lng);