#include "main.h"
#include <math.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"
#include "igc_parser.h"
#include "string_manip.h"

void coordinate_init(coordinate_t *this, int64_t lat, int64_t lng, int64_t ele, int64_t timestamp) {
    this->lat = lat;
    this->lng = lng;
    this->sin_lat = sin(lat toRAD);
    this->cos_lat = cos(lat toRAD);
    this->ele = ele;
    this->id = 0;
    this->timestamp = timestamp;
}

void coordinate_deinit(coordinate_t *this) {
    if (this->prev) {
        this->prev->next = this->next;
    }
    if (this->next) {
        this->next->prev = this->prev;
    }
    if (this->coordinate_set) {
        if (this == this->coordinate_set->first) {
            this->coordinate_set->first = this->next;
        }
        if (this == this->coordinate_set->last) {
            this->coordinate_set->last = this->prev;
        }
        this->coordinate_set->length--;
    }

    if (this->coordinate_subset) {
        if (this == this->coordinate_subset->first) {
            this->coordinate_subset->first = this->next;
        }
        if (this == this->coordinate_subset->last) {
            this->coordinate_subset->last = this->prev;
        }
        this->coordinate_subset->length--;
    }
    free(this);
}

double get_bearing(coordinate_t *obj1, coordinate_t *obj2) {
    double delta_rad = (obj2->lng - obj1->lng)toRAD;

    double y = sin(delta_rad) * obj2->cos_lat;
    double x = obj1->cos_lat * obj2->sin_lat - obj1->sin_lat * obj2->cos_lat * cos(delta_rad);
    double res = atan2(y, x) toDEG;
    if (res < 0) {
        res += 360;
    }
    return res;
}

double get_distance(coordinate_t *point1, coordinate_t *point2) {
    double res = 0;
    if (point1->lat != point2->lat || point1->lng != point2->lng) {
        double delta_rad = (point1->lng - point2->lng)toRAD;
        res = (point1->sin_lat * point2->sin_lat) + point1->cos_lat * point2->cos_lat * cos(delta_rad);
        res = acos(res) * 6371;
    }
    return res;
}

double get_distance_precise(coordinate_t *obj1, coordinate_t *obj2) {
    double lat1 = obj1->lat toRAD, lng1 = obj1->lng toRAD;
    double lat2 = obj2->lat toRAD, lng2 = obj2->lng toRAD;

    double a = 6378137, b = 6356752.31425, f = 1 / 298.257223563;

    double L = lng2 - lng1;
    double tanU1 = (1 - f) * tan(lat1), cosU1 = 1 / sqrt((1 + tanU1 * tanU1)), sinU1 = tanU1 * cosU1;
    double tanU2 = (1 - f) * tan(lat2), cosU2 = 1 / sqrt((1 + tanU2 * tanU2)), sinU2 = tanU2 * cosU2;

    double sinlng, coslng, sinSqTheta, sinTheta, cosTheta, Theta, sinAlpha, cosSqAlpha, cos2ThetaM, C;

    double lng = L, lng_deriv, iterations = 0;
    do {
        sinlng = sin(lng);
        coslng = cos(lng);
        sinSqTheta = (cosU2 * sinlng) * (cosU2 * sinlng) + (cosU1 * sinU2 - sinU1 * cosU2 * coslng) * (cosU1 * sinU2 - sinU1 * cosU2 * coslng);
        sinTheta = sqrt(sinSqTheta);
        if (sinTheta == 0)
            return 0; // co-incident points
        cosTheta = sinU1 * sinU2 + cosU1 * cosU2 * coslng;
        Theta = atan2(sinTheta, cosTheta);
        sinAlpha = cosU1 * cosU2 * sinlng / sinTheta;
        cosSqAlpha = 1 - sinAlpha * sinAlpha;
        if (cosSqAlpha != 0) {
            cos2ThetaM = cosTheta - 2 * sinU1 * sinU2 / cosSqAlpha;
        } else {
            cos2ThetaM = 0; // equatorial line: cosSqAlpha=0 (§6)
        }
        C = f / 16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
        lng_deriv = lng;
        lng = L + (1 - C) * f * sinAlpha * (Theta + C * sinTheta * (cos2ThetaM + C * cosTheta * (-1 + 2 * cos2ThetaM * cos2ThetaM)));
    } while (fabs(lng - lng_deriv) > 1e-12 && ++iterations < 200);
    double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
    double A = 1 + uSq / 16384 * (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
    double B = uSq / 1024 * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));
    double deltaTheta = B * sinTheta * (cos2ThetaM + B / 4 * (cosTheta * (-1 + 2 * cos2ThetaM * cos2ThetaM) - B / 6 * cos2ThetaM * (-3 + 4 * sinTheta * sinTheta) * (-3 + 4 * cos2ThetaM * cos2ThetaM)));

    double s = b * A * (Theta - deltaTheta);
    return s / 1000;
}

char *coordinate_to_kml(coordinate_t *coordinate) {
    char *buffer = malloc(sizeof(char) * (8 + 1 + 9 + 1 + 4 + 1));
    sprintf(buffer, "%02.5f,%02.5f,%04d ", coordinate->lng, coordinate->lat, coordinate->ele);
    return buffer;
}
