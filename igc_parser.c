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

char *load_file(const char *filename) {
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
    const char *source;
    char *destination, *pilot;
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
        destination = (char *) json_string_value(_destination);
    }

    if (!json_is_string(_pilot)) {
        pilot = (char *) json_string_value(_pilot);
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
                        convert_gridref_to_latlng(gridref, &lat, &lng);
                        coordinate_t *coordinate = malloc(sizeof(coordinate_t));
                        coordinate_init(coordinate, lat, lng, 0, 0);
                        task->coordinate[i] = coordinate;
                    }
                    task->size = count;
                    task->gap = NULL;
                    if (
                            task->size == 4 &&
                            task->coordinate[0]->lat == task->coordinate[3]->lat &&
                            task->coordinate[0]->lng == task->coordinate[3]->lng
                            ) {
                        task->type = TRIANGLE;
                    } else if (
                            task->size == 3 &&
                            task->coordinate[0]->lat == task->coordinate[2]->lat &&
                            task->coordinate[0]->lng == task->coordinate[2]->lng
                            ) {
                        task->type = OUT_AND_RETURN;
                    } else {
                        task->type = OPEN_DISTANCE;
                    }
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
                format_task(task, "declared", task->type);
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
            char *os = convert_latlng_to_gridref(coordinate->lat, coordinate->lng);
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
                char *os = convert_latlng_to_gridref(coordinate->lat, coordinate->lng);
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