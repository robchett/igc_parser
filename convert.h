#pragma once

char *convert_latlng_to_gridref(double lat, double lng);
int convert_gridref_to_latlng(const char *input, double *lat, double *lng);