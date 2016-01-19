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
    int64_t e100k = (int64_t)floor(e / 100000);
    int64_t n100k = (int64_t)floor(n / 100000);

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

void format_task(task_t *task, char *title);

int8_t main(int argc, char **argv) {
    char *source_file;
    char *root;

    fprintf(stderr, "\n\nArgs: %d\n0: %s\n", argc, argv[0]);
    if (argc >= 2) {
        json_error_t error;
        json_t *root = json_loads(argv[1], 1, &error);

        if (!root) {
            fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
            return 1;
        }

        if (json_is_object(root)) {
            _main(root);
        } else {
            for (size_t i = 0; i < json_array_size(root); i++) {
                json_t *data = json_array_get(root, i);
                if (!json_is_object(data)) {
                    fprintf(stderr, "error: entry %d is not an object\n", i + 1);
                } else {
                    _main(data);
                };
            }
        }

        json_decref(root);
        return 1;
    } else if (argc == 0) {
        fprintf(stderr, "Please provide a file\n\nUsage:\nigc_parser [file] {output_dir}");
        exit(2);
    }
}

uint8_t _main(json_t *data) {
    json_t *_source, *_destination, *_set_start, *_set_end, *_pilot;
    char *source, *destination, *pilot;
    size_t set_start, set_end;

    _source = json_object_get(data, "source");
    _destination = json_object_get(data, "destination");
    _set_start = json_object_get(data, "set_start");
    _set_end = json_object_get(data, "set_end");
    _pilot = json_object_get(data, "pilot");

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
            distance_map_t *map = malloc(sizeof(distance_map_t));
            distance_map_init(map, set);

            char *ids;

            printf("{");
            printf("\n\t\"total_points\": %d,", initial_length);
            printf("\n\t\"sets\": %d,", set->subset_count);
            printf("\n\t\"points\": %d,", set->length);
            printf("\n\t\"task\": {\n", set->length);

            task_t *od, * or, *tr;

            od = distance_map_score_open_distance_3tp(map);
            format_task(od, "open_distance");
            printf(",\n");

            or = distance_map_score_out_and_return(map);
            format_task(or, "out_and_return");
            printf(",\n");

            tr = distance_map_score_triangle(map);
            format_task(tr, "triangle");
            printf("\n\t},\n\t\"output\": {\n\t\t\"js\": \"%s\",\n\t\t\"kml\": \"%s\",\n\t\t\"earth\": \"%s\"\n\t}", out_file_1, out_file_2, out_file_3);

            formatter_t *formatter;

            formatter = malloc(sizeof(formatter_t));
            formatter_js_init(formatter, set, 1, od, or, tr);
            formatter_js_output(formatter, out_file_1);
            free(formatter);

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_init(formatter, set, pilot ?: "Bob", od, or, tr, NULL);
            formatter_kml_output(formatter, out_file_2);
            free(formatter);

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_earth_init(formatter, set, pilot ?: "Bob", od, or, tr, NULL);
            formatter_kml_earth_output(formatter, out_file_3);
            free(formatter);

            printf("\n}");

            task_deinit(od);
            task_deinit(or );
            task_deinit(tr);
            distance_map_deinit(map);

        } else {
            size_t i = 0;
            coordinate_subset_t *current = set->first_subset;
            printf("{\n\t\"output\": \"%s\",\n\t\"sets\":\n\t[\n\t\t", out_file_4);
            do {
                if (current->next) {
                    printf("{\n\t\t\t\"duration\": %d,\n\t\t\t\"skipped_distance\": %05.5f,\n\t\t\t\"points\": %d,\n\t\t\t\"skipped_duration\": %d\n\t\t}, ", coordinate_subset_duration(current),
                           get_distance_precise(current->last, current->next->first), current->length, current->next->first->timestamp - current->last->timestamp);
                } else {
                    printf("{\n\t\t\t\"duration\": %d,\n\t\t\t\"points\": %d\n\t\t}\n", coordinate_subset_duration(current), current->length);
                }
            } while (current = current->next);
            printf("\t]\n}\n");
            formatter_split_t *formatter = malloc(sizeof(formatter_split_t));
            formatter_kml_split_init(formatter, set);
            formatter_kml_split_output(formatter, out_file_4);
            free(formatter);
            fprintf(stderr, "KML split output formatted\n");
        }
        coordinate_set_deinit(set);
    } else {
        fprintf(stderr, "Failed: %s\n", source);
        exit(1);
    }

    free(igc_file);
    free(destination);
    exit(0);
}

void format_task(task_t *task, char *title) {
    if (task) {
        printf("\t\t\"%s\": {", title);
        printf("\n\t\t\t\"distance\": %.5f,\n\t\t\t\"points\":\n\t\t\t[\n\t\t\t\t", get_task_distance(task));

        size_t i;
        for (i = 0; i < task->size; i++) {
            coordinate_t *coordinate = task->coordinate[i];
            printf("{\n\t\t\t\t\t\"lat\": %.5f,\n\t\t\t\t\t\"lng\": %.5f,\n\t\t\t\t\t\"id\": %d\n\t\t\t\t}", coordinate->lat, coordinate->lng, coordinate->id);
            if (i < task->size - 1) {
                printf(", ");
            }
        }

        if (task->gap) {
            printf("\n\t\t\t],");
            printf("\n\t\t\t\"gap\":\n\t\t\t[\n\t\t\t\t", get_task_distance(task));

            size_t i;
            for (i = 0; i < 2; i++) {
                coordinate_t *coordinate = task->gap[i];
                printf("{\n\t\t\t\t\t\"lat\": %.5f,\n\t\t\t\t\t\"lng\": %.5f,\n\t\t\t\t\t\"id\": %d\n\t\t\t\t}", coordinate->lat, coordinate->lng, coordinate->id);
                if (i == 0) {
                    printf(", ");
                }
            }
        }
        printf("\n\t\t\t]\n\t\t}");
    } else {
        printf("\t\t\"%s\": null", title);
    }
}