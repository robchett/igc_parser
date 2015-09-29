#include <math.h>
#include <php.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "geometry.h"
#include "string_manip.h"

void init_coordinate(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "coordinate", coordinate_methods);
    coordinate_ce = zend_register_internal_class(&ce TSRMLS_CC);
    coordinate_ce->create_object = create_coordinate_object;
}

zend_object* create_coordinate_object(zend_class_entry *class_type TSRMLS_DC) {
    coordinate_object* retval;
    zend_object_handlers handlers;

    coordinate_object *intern = ecalloc(1, sizeof(coordinate_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    zend_objects_store_put(&intern->std);

    memcpy(&handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    //handlers.offset = XtOffsetof(coordinate_object, std);
    //handlers.free_obj = free_coordinate_object;

    return &retval->std;
}

coordinate_object* fetch_coordinate_object(zend_object* obj) {
    return (coordinate_object*) ((char*) obj - XtOffsetOf(coordinate_object, std));
}

void _free_coordinate_object(coordinate_object *intern) {
    if (intern->prev) {
        intern->prev->next = intern->next;
    }
    if (intern->next) {
        intern->next->prev = intern->prev;
    }
    if (intern->coordinate_set) {
        if (intern == intern->coordinate_set->first) {
            intern->coordinate_set->first = intern->next;
        }
        if (intern == intern->coordinate_set->last) {
            intern->coordinate_set->last = intern->prev;
        }
        intern->coordinate_set->length--;
        // printf("First exists... %s -> %d\n", intern->coordinate_set->first ? "Yes" : "No", intern->coordinate_set->first ? intern->coordinate_set->first->id : 0);
    }
    if (intern->coordinate_subset) {
        if (intern == intern->coordinate_subset->first) {
            intern->coordinate_subset->first = intern->next;
        }
        if (intern == intern->coordinate_subset->last) {
            intern->coordinate_subset->last = intern->prev;
        }
        intern->coordinate_subset->length--;
    }
    efree(intern);
}

void free_coordinate_object(coordinate_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    _free_coordinate_object(intern);
}

static zend_function_entry coordinate_methods[] = {
    PHP_ME(coordinate, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, id, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, lat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, lng, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, ele, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, timestamp, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, set_lat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, set_lng, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, set_ele, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, gridref, NULL, ZEND_ACC_PUBLIC)

    PHP_ME(coordinate, get_distance_to, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate, get_bearing_to, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD(coordinate, __construct) {
    double lat = 0, lng = 0;
    long ele = 0;
    long timestamp = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ddll", &lat, &lng, &ele, &timestamp) == FAILURE) {
        return;
    }

    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    intern->lat = lat;
    intern->lng = lng;
    intern->sin_lat = sin(lat toRAD);
    intern->cos_lat = cos(lat toRAD);
    intern->ele = ele;
    intern->id = 0;
    intern->timestamp = timestamp;
}

PHP_METHOD(coordinate, id) {
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    RETURN_LONG(intern->id);
}

PHP_METHOD(coordinate, lat) {
    zend_bool as_rad = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &as_rad) == FAILURE) {
        return;
    }

    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    if (as_rad) {
        RETURN_DOUBLE(intern->lat toRAD);
    } else {
        RETURN_DOUBLE(intern->lat);
    }
}

PHP_METHOD(coordinate, gridref) {
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    RETURN_STRING(get_os_grid_ref(intern));
}

PHP_METHOD(coordinate, timestamp) {
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    RETURN_LONG(intern->timestamp);
}

PHP_METHOD(coordinate, lng) {
    zend_bool as_rad = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &as_rad) == FAILURE) {
        return;
    }

    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    if (as_rad) {
        RETURN_DOUBLE(intern->lng toRAD);
    } else {
        RETURN_DOUBLE(intern->lng);
    }
}

PHP_METHOD(coordinate, ele) {
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    RETURN_DOUBLE(intern->ele);
}

PHP_METHOD(coordinate, set_ele) {
    long val = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &val) == FAILURE) {
        return;
    }
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    intern->ele = val;
}

PHP_METHOD(coordinate, set_lng) {
    double val = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d", &val) == FAILURE) {
        return;
    }
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    intern->lng = val;
}

PHP_METHOD(coordinate, set_lat) {
    double val = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d", &val) == FAILURE) {
        return;
    }
    coordinate_object *intern = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    intern->lat = val;
}

PHP_METHOD(coordinate, get_bearing_to) {
    zval *_point = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &_point) == FAILURE) {
        return;
    }

    coordinate_object *point1 = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    coordinate_object *point2 = fetch_coordinate_object(Z_OBJ_P(_point) TSRMLS_CC);

    RETURN_DOUBLE(get_bearing(point1, point2));

}

PHP_METHOD(coordinate, get_distance_to) {
    zval *_point = 0;
    int precise = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|b", &_point, coordinate_ce, &precise) == FAILURE) {
        return;
    }

    coordinate_object *point1 = fetch_coordinate_object(Z_OBJ_P(getThis()) TSRMLS_CC);
    coordinate_object *point2 = fetch_coordinate_object(Z_OBJ_P(_point) TSRMLS_CC);

    if (precise) {
        RETURN_DOUBLE(get_distance_precise(point1, point2));
    } else {
        RETURN_DOUBLE(get_distance(point1, point2, 1));
    }
}

double get_bearing(coordinate_object *obj1, coordinate_object *obj2) {
    double delta_rad = (obj2->lng - obj1->lng) toRAD;

    double y = sin(delta_rad) * obj2->cos_lat;
    double x = obj1->cos_lat * obj2->sin_lat - obj1->sin_lat * obj2->cos_lat * cos(delta_rad);
    double res = atan2(y, x) toDEG;
    if (res < 0) {
        res += 360;
    }
    return res;
}

double get_distance(coordinate_object *point1, coordinate_object *point2, int debug) {
    double res = 0;
    if (point1->lat != point2->lat || point1->lng != point2->lng) {
        double delta_rad = (point1->lng - point2->lng) toRAD;
        res = (point1->sin_lat * point2->sin_lat) + point1->cos_lat * point2->cos_lat * cos(delta_rad);
        res = acos(res) * 6371;
    }
    return res;
}

double get_distance_precise(coordinate_object *obj1, coordinate_object *obj2) {
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
        if (sinTheta == 0) return 0;  // co-incident points
        cosTheta = sinU1 * sinU2 + cosU1 * cosU2 * coslng;
        Theta = atan2(sinTheta, cosTheta);
        sinAlpha = cosU1 * cosU2 * sinlng / sinTheta;
        cosSqAlpha = 1 - sinAlpha * sinAlpha;
        if (cosSqAlpha != 0) {
            cos2ThetaM = cosTheta - 2 * sinU1 * sinU2 / cosSqAlpha;
        } else {
            cos2ThetaM = 0;  // equatorial line: cosSqAlpha=0 (§6)
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

char *coordinate_to_kml(coordinate_object *coordinate) {
    char *buffer = create_buffer("");
    char *lat = fdtos(coordinate->lat, "%.5f");
    char *lng = fdtos(coordinate->lng, "%.5f");
    char *ele = fitos(coordinate->ele, "%d");
    buffer = vstrcat(buffer, lng, ",", lat , ",", ele, " " , NULL);
    efree(lat);
    efree(lng);
    efree(ele);
    return buffer;
}
