#include "main.h"
#include <libgen.h>
#include <string.h>
#include "include/json/jansson.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"
#include "task.h"
#include "igc_parser.h"
#include "formatter/formatter_kml.h"
#include "formatter/formatter_kml_earth.h"
#include "formatter/formatter_kml_split.h"
#include "formatter/formatter_js.h"
#include "distance_map.h"
#include "statistics/element.h"
#include "statistics/group.h"

char *get_os_grid_ref(coordinate_t *point) {
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
    double nu = a * F0 / sqrt(1 - e2 * sinLat * sinLat);                 // transverse radius of curvature
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

char *gridref_number_to_letter(int64_t e, int64_t n) {
    // get the 100km-grid indices
    int64_t e100k = (int64_t) floor(e / 100000);
    int64_t n100k = (int64_t) floor(n / 100000);

    if (e100k < 0 || e100k > 8 || n100k < 0 || n100k > 12) {
    }

    int64_t l1 = (19 - n100k) - (19 - n100k) % 5 + floor((e100k + 10) / 5);
    int64_t l2 = (19 - n100k) * 5 % 25 + e100k % 5;

    if (l1 > 7)
        l1++;
    if (l2 > 7)
        l2++;

    // strip 100km-grid indices from easting & northing, and reduce precision
    e = floor((e % 100000) / pow(10, 2));
    n = floor((n % 100000) / pow(10, 2));
    char *gridref = malloc(sizeof(char) * 9);
    sprintf(gridref, "%c%c%03d%03d", l1 + 65, l2 + 65, e, n);
    return gridref;
}

int os_grid_ref_to_lat_lng(char *input, double *lat, double *lng) {
    int E, N;

    int l1 = input[0] - 'A';
    int l2 = input[1] - 'A';
    // shuffle down letters after 'I' since 'I' is not used in grid:
    if (l1 > 7) l1--;
    if (l2 > 7) l2--;

    int e100km = ((l1 - 2) % 5) * 5 + (l2 % 5);
    int n100km = (19 - floor(l1 / 5) * 5) - floor(l2 / 5);

    int offset = 0;
    int multi = 10;
    if (strlen(input) == 8) {
        offset = 3;
        multi = 100;
    } else if (strlen(input) == 10) {
        offset = 4;
        multi = 10;
    }

    char northing[offset], easting[offset];
    memcpy(northing, &input[2], offset);
    memcpy(easting, &input[2+offset], offset);
    northing[offset] = '\0';
    easting[offset] = '\0';
    E = (atoi(northing) * multi) + (e100km * 100000);
    N = (atoi(easting) * multi) + (n100km * 100000);

    double a = 6377563.396, b = 6356256.909;              // Airy 1830 major & minor semi-axes
    double F0 = 0.9996012717;                             // NatGrid scale factor on central meridian
    double phi0 = 49 * M_PI / 180, lamda0 = -2 * M_PI / 180;      // NatGrid true origin
    double N0 = -100000, E0 = 400000;                     // northing & easting of true origin, metres
    double e2 = 1 - (b * b) / (a * a);                          // eccentricity squared
    double n = (a - b) / (a + b), n2 = n * n, n3 = n * n * n;         // n, n², n³

    double phi = phi0, M = 0;
    do {
        phi = (N - N0 - M) / (a * F0) + phi;

        double Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (phi - phi0);
        double Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(phi - phi0) * cos(phi + phi0);
        double Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (phi - phi0)) * cos(2 * (phi + phi0));
        double Md = (35 / 24) * n3 * sin(3 * (phi - phi0)) * cos(3 * (phi + phi0));
        M = b * F0 * (Ma - Mb + Mc - Md);              // meridional arc

    } while (N - N0 - M >= 0.00001);  // ie until < 0.01mm

    double cosphi = cos(phi), sinphi = sin(phi);
    double v = a * F0 / sqrt(1 - e2 * sinphi * sinphi);            // nu = transverse radius of curvature
    double p = a * F0 * (1 - e2) / pow(1 - e2 * sinphi * sinphi, 1.5); // rho = meridional radius of curvature
    double nu2 = v / p - 1;                                    // eta = ?

    double tanphi = tan(phi);
    double tan2phi = tanphi * tanphi, tan4phi = tan2phi * tan2phi, tan6phi = tan4phi * tan2phi;
    double secphi = 1 / cosphi;
    double v3 = v * v * v, v5 = v3 * v * v, v7 = v5 * v * v;
    double VII = tanphi / (2 * p * v);
    double VIII = tanphi / (24 * p * v3) * (5 + 3 * tan2phi + nu2 - 9 * tan2phi * nu2);
    double IX = tanphi / (720 * p * v5) * (61 + 90 * tan2phi + 45 * tan4phi);
    double X = secphi / v;
    double XI = secphi / (6 * v3) * (v / p + 2 * tan2phi);
    double XII = secphi / (120 * v5) * (5 + 28 * tan2phi + 24 * tan4phi);
    double XIIA = secphi / (5040 * v7) * (61 + 662 * tan2phi + 1320 * tan4phi + 720 * tan6phi);

    double dE = (E - E0), dE2 = dE * dE, dE3 = dE2 * dE, dE4 = dE2 * dE2, dE5 = dE3 * dE2, dE6 = dE4 * dE2, dE7 = dE5 * dE2;
    phi = phi - VII * dE2 + VIII * dE4 - IX * dE6;
    double lamda = lamda0 + X * dE - XI * dE3 + XII * dE5 - XIIA * dE7;

    *lat = phi toDEG;
    *lng = lamda toDEG;
    return 1;
}

char *load_file(char *filename) {
    char *source = NULL;
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);

        source = malloc(fsize + 1);
        size_t newLen = fread(source, 1, fsize, fp);
        if (newLen == 0) {
            return NULL;
        } else {
            source[newLen++] = '\0';
        }
        fclose(fp);
    }
    return source;
}

uint8_t _main(json_t *data);

void format_task(task_t *task, char *title, int type);

int8_t main(int argc, char **argv) {
    char *source_file;
    char *root;

    if (argc >= 2) {
        json_error_t error;
        json_t *root = json_loads(argv[1], 1, &error);

        if (!root) {
            printf("{\"error\": \"JSON decode - line %d: %s\"}", error.line, error.text);
            return 1;
        }

        if (json_is_object(root)) {
            _main(root);
        } else {
            for (size_t i = 0; i < json_array_size(root); i++) {
                json_t *data = json_array_get(root, i);
                if (!json_is_object(data)) {
                    printf("{\"error\": \"entry %d is not an object\"", i + 1);
                } else {
                    _main(data);
                };
            }
        }

        json_decref(root);
        return 1;
    } else if (argc == 0) {
        printf("{\"error\": \"Please provide a file\"}");
        exit(2);
    }
}

uint8_t _main(json_t *data) {
    json_t *_source, *_destination, *_set_start, *_set_end, *_pilot, *_task;
    char *source, *destination, *pilot;
    size_t set_start, set_end;

    _source = json_object_get(data, "source");
    _destination = json_object_get(data, "destination");
    _set_start = json_object_get(data, "set_start");
    _set_end = json_object_get(data, "set_end");
    _pilot = json_object_get(data, "pilot");
    _task = json_object_get(data, "task");

    if (!json_is_string(_source)) {
        fprintf(stderr, "{\"error\": \"No source file provided\"}");
        return 0;
    }

    source = json_string_value(_source);

    if (!json_is_string(_destination)) {
        destination = calloc(strlen(source) + 1, sizeof(char));
        strcpy(destination, source);
        dirname(destination);
    } else {
        destination = json_string_value(_destination);
    }

    if (!json_is_string(_pilot)) {
        pilot = json_string_value(_pilot);
    }

    char out_file_1[strlen(destination) + 20];
    char out_file_2[strlen(destination) + 20];
    char out_file_3[strlen(destination) + 20];
    char out_file_4[strlen(destination) + 20];
    sprintf(out_file_1, "%s%s", destination, "/track.js");
    sprintf(out_file_2, "%s%s", destination, "/track.kml");
    sprintf(out_file_3, "%s%s", destination, "/track_earth.kml");
    sprintf(out_file_4, "%s%s", destination, "/track_split.kml");

    char *igc_file = load_file(source);
    if (igc_file != NULL) {
        coordinate_set_t *set = malloc(sizeof(coordinate_set_t));
        coordinate_set_init(set);
        coordinate_set_parse_igc(set, igc_file);

        size_t initial_length = set->length;
        coordinate_set_trim(set);
        coordinate_set_repair(set);
        coordinate_set_simplify(set, 1500);
        coordinate_set_extrema(set);

        if (set->subset_count == 1) {

            task_t *task = NULL;

            if (_task) {
                if (json_is_object(_task)) {
                    task = malloc(sizeof(task_t));
                    json_t *_task_type = json_object_get(_task, "type");
                    const char *task_type = json_is_string(_task_type) ? json_string_value(_task_type) : "os_gridref";
                    json_t *_coordinate = json_object_get(_task, "coordinate");
                    size_t count = json_array_size(_coordinate);
                    task->coordinate = malloc(sizeof(coordinate_t) * count);
                    for (size_t i = 0; i < count; i++) {
                        const char *gridref = json_string_value(json_array_get(_coordinate, i));
                        double lat = 0, lng = 0;
                        os_grid_ref_to_lat_lng(gridref, &lat, &lng);
                        coordinate_t *coordinate = malloc(sizeof(coordinate_t));
                        coordinate_init(coordinate, lat, lng, 0, 0);
                        task->coordinate[i] = coordinate;
                    }
                    task->size = count;
                    task->gap = NULL;
                    task->type = 1;
                } else {
                    // Not object
                }
            }

            distance_map_t *map = malloc(sizeof(distance_map_t));
            distance_map_init(map, set);

            char *ids;

            printf("{");
            printf("\"total_points\": %d,", initial_length);
            printf("\"sets\": %d,", set->subset_count);
            printf("\"date\": \"%04d-%02d-%02d\",", set->year, set->month, set->day);
            printf("\"start_time\": %d,", set->first->timestamp);
            printf("\"duration\": %d,", set->last->timestamp - set->first->timestamp);
            printf("\"completes_task\": %s,", "false");
            printf("\"points\": %d,", set->length);
            printf("\"stats\": {", set->length);
            printf("\"height\" : {\"min\": %d, \"max\": %d},", set->first->ele, set->last->ele);
            printf("\"speed\" : {\"min\": %d, \"max\": %d},", set->first->speed, set->last->speed);
            printf("\"climb_rate\" : {\"min\": %d, \"max\": %d}", set->first->climb_rate, set->last->climb_rate);
            printf("},");
            printf("\"task\": {", set->length);

            task_t *od, *or, *tr;

            od = distance_map_score_open_distance_3tp(map);
            format_task(od, "open_distance", 1);
            printf(",");

            or = distance_map_score_out_and_return(map);
            format_task(or, "out_and_return", 2);
            printf(",");

            tr = distance_map_score_triangle(map);
            format_task(tr, "triangle", 3);

            if (task) {
                printf(",");
                format_task(task, "declared", 4);
                printf(", \"complete\": %d", task_completes_task(task, set));
            }

            printf("}, \"output\": {\"js\": \"%s\",\"kml\": \"%s\",\"earth\": \"%s\"}", out_file_1, out_file_2, out_file_3);

            formatter_t *formatter;

            formatter = malloc(sizeof(formatter_t));
            formatter_js_init(formatter, set, 1, od, or, tr);
            formatter_js_output(formatter, out_file_1);
            free(formatter);

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_init(formatter, set, pilot ?: "Bob", od, or, tr, task);
            formatter_kml_output(formatter, out_file_2);
            free(formatter);

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_earth_init(formatter, set, pilot ?: "Bob", od, or, tr, task);
            formatter_kml_earth_output(formatter, out_file_3);
            free(formatter);

            printf("}");

            if (od) {
                task_deinit(od);
            }
            if (or) {
                task_deinit(or);
            }
            if (tr) {
                task_deinit(tr);
            }
            distance_map_deinit(map);

        } else {
            size_t i = 0;
            coordinate_subset_t *current = set->first_subset;
            printf("{\"output\": \"%s\",\"sets\":[", out_file_4);
            do {
                if (current->next) {
                    printf("{\"duration\": %d,\"skipped_distance\": %05.5f,\"points\": %d,\"skipped_duration\": %d}, ", coordinate_subset_duration(current),
                           get_distance_precise(current->last, current->next->first), current->length, current->next->first->timestamp - current->last->timestamp);
                } else {
                    printf("{\"duration\": %d,\"points\": %d}", coordinate_subset_duration(current), current->length);
                }
            } while (current = current->next);
            printf("]}");
            formatter_split_t *formatter = malloc(sizeof(formatter_split_t));
            formatter_kml_split_init(formatter, set);
            formatter_kml_split_output(formatter, out_file_4);
            free(formatter);

            formatter_t *js_formatter = malloc(sizeof(formatter_t));
            formatter_js_init(js_formatter, set, 1, NULL, NULL, NULL);
            formatter_js_output(js_formatter, out_file_1);
            free(js_formatter);
        }
        coordinate_set_deinit(set);
        printf("\n");
    } else {
        fprintf(stderr, "Failed: %s", source);
        exit(1);
    }

    free(igc_file);
    free(destination);
    exit(0);
}

void format_task(task_t *task, char *title, int type) {
    if (task) {
        printf("\"%s\": {", title);
        printf("\"type\":\"%d\", \"distance\": %.5f,\"duration\": %d,\"coordinates\":[", type, task->coordinate[task->size - 1]->timestamp - task->coordinate[0]->timestamp, get_task_distance(task));

        size_t i;
        for (i = 0; i < task->size; i++) {
            coordinate_t *coordinate = task->coordinate[i];
            char *os = get_os_grid_ref(coordinate);
            printf("{\"lat\": %.5f,\"lng\": %.5f,\"id\": %d, \"os_gridref\":\"%s\"}", coordinate->lat, coordinate->lng, coordinate->id, os);
            if (i < task->size - 1) {
                printf(", ");
            }
            free(os);
        }

        if (task->gap) {
            printf("],");
            printf("\"gap\":[", get_task_distance(task));

            size_t i;
            for (i = 0; i < 2; i++) {
                coordinate_t *coordinate = task->gap[i];
                char *os = get_os_grid_ref(coordinate);
                printf("{\"lat\": %.5f,\"lng\": %.5f,\"id\": %d, \"os_gridref\":\"%s\"}", coordinate->lat, coordinate->lng, coordinate->id, os);
                if (i == 0) {
                    printf(", ");
                }
                free(os);
            }
        }
        printf("]}");
    } else {
        printf("\"%s\": null", title);
    }
}