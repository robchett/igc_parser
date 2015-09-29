#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include "include/json/jansson.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "task.h"
#include "geometry.h"
#include "formatter/formatter_kml.h"
#include "formatter/formatter_kml_earth.h"
#include "formatter/formatter_kml_split.h"
#include "formatter/formatter_js.h"
#include "distance_map.h"
#include "statistics/element.h"
#include "statistics/group.h"

static zend_function_entry geometry_functions[] = {
    {NULL, NULL, NULL}
};

zend_module_entry geometry_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_GEOMETRY_EXTNAME,
    geometry_functions,
    PHP_MINIT(geometry),
    NULL,
    NULL,
    NULL,
    NULL,
    PHP_GEOMETRY_VERSION,
    STANDARD_MODULE_PROPERTIES
};

PHP_MINIT_FUNCTION(geometry) {
    json_set_alloc_funcs(_emalloc, _efree);
    init_coordinate(TSRMLS_C);
    init_coordinate_set(TSRMLS_C);
    init_distance_map(TSRMLS_C);
    init_task(TSRMLS_C);
    init_formatter_kml(TSRMLS_C);
    init_formatter_js(TSRMLS_C);
    init_formatter_kml_split(TSRMLS_C);
    init_formatter_kml_earth(TSRMLS_C);
    init_statistic(TSRMLS_C);
    init_statistics_set(TSRMLS_C);
}

#ifdef COMPILE_DL_GEOMETRY
ZEND_GET_MODULE(geometry)
#endif

char *get_os_grid_ref(coordinate_object *point) {
    double lat = point->lat toRAD;
    double lon = point->lng toRAD;
    double a = 6377563.396;
    double b = 6356256.910;
    double F0 = 0.9996012717;
    double lat0 = 49 toRAD;
    double lon0 = -2 toRAD;
    double N0 = -100000;
    double E0 = 400000;
    double e2 = 1 - ((b * b) / (a * a));
    double n = (a - b) / (a + b);
    double n2 = n * n;
    double n3 = n * n * n;

    double cosLat = cos(lat);
    double sinLat = sin(lat);
    double nu = a * F0 / sqrt(1 - e2 * sinLat * sinLat); // transverse radius of curvature
    double rho = a * F0 * (1 - e2) / pow(1 - e2 * sinLat * sinLat, 1.5); // meridional radius of curvature
    double eta2 = nu / rho - 1;

    double Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (lat - lat0);
    double Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(lat - lat0) * cos(lat + lat0);
    double Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (lat - lat0)) * cos(2 * (lat + lat0));
    double Md = (35 / 24) * n3 * sin(3 * (lat - lat0)) * cos(3 * (lat + lat0));
    double M = b * F0 * (Ma - Mb + Mc - Md); // meridional arc

    double cos3lat = cosLat * cosLat * cosLat;
    double cos5lat = cos3lat * cosLat * cosLat;
    double tan2lat = tan(lat) * tan(lat);
    double tan4lat = tan2lat * tan2lat;

    double I = M + N0;
    double II = (nu / 2) * sinLat * cosLat;
    double III = (nu / 24) * sinLat * cos3lat * (5 - tan2lat + 9 * eta2);
    double IIIA = (nu / 720) * sinLat * cos5lat * (61 - 58 * tan2lat + tan4lat);
    double IV = nu * cosLat;
    double V = (nu / 6) * cos3lat * (nu / rho - tan2lat);
    double VI = (nu / 120) * cos5lat * (5 - 18 * tan2lat + tan4lat + 14 * eta2 - 58 * tan2lat * eta2);

    double dLon = lon - lon0;
    double dLon2 = dLon * dLon;
    double dLon3 = dLon2 * dLon;
    double dLon4 = dLon3 * dLon;
    double dLon5 = dLon4 * dLon;
    double dLon6 = dLon5 * dLon;

    double N = I + II * dLon2 + III * dLon4 + IIIA * dLon6;
    double E = E0 + IV * dLon + V * dLon3 + VI * dLon5;
    return gridref_number_to_letter(E, N);
}

char *gridref_number_to_letter(long e, long n) {
    // get the 100km-grid indices
    long e100k = (long)floor(e / 100000);
    long n100k = (long)floor(n / 100000);

    if (e100k < 0 || e100k > 8 || n100k < 0 || n100k > 12) { }

    long l1 = (19 - n100k) - (19 - n100k) % 5 + floor((e100k + 10) / 5);
    long l2 = (19 - n100k) * 5 % 25 + e100k % 5;

    if (l1 > 7) l1++;
    if (l2 > 7) l2++;

    // strip 100km-grid indices from easting & northing, and reduce precision
    e = floor((e % 100000) / pow(10, 2));
    n = floor((n % 100000) / pow(10, 2));
    char * gridref = emalloc(sizeof(char) * 9);
    sprintf(gridref, "%c%c%03d%03d", l1 + 65, l2 + 65, e, n);
    return gridref;
}
