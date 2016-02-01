#pragma once

struct coordinate_set_t;
struct coordinate_t;

typedef struct coordinate_t {
    double lat, lng;
    double sin_lat, cos_lat, cos_lng;

    int64_t ele;
    int64_t alt;
    int64_t timestamp;
    int64_t id;
    double climb_rate;
    double speed;
    double bearing;
    struct coordinate_t *prev;
    struct coordinate_t *next;
    struct coordinate_set_t *coordinate_set;
    struct coordinate_subset_t *coordinate_subset;
} coordinate_t;

#define toRAD *M_PI / 180
#define toDEG *180 / M_PI

void coordinate_init(coordinate_t *this, double lat, double lng, int64_t ele, int64_t timestamp);
void coordinate_deinit(coordinate_t *this);

double get_distance(coordinate_t *point1, coordinate_t *point2);
double get_distance_precise(coordinate_t *point1, coordinate_t *point2);
double get_bearing(coordinate_t *point1, coordinate_t *point2);
char *coordinate_to_kml(coordinate_t *coordinate);