#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include <time.h>
#include <string.h>
#include "kml.h"
#include "formatter_kml_earth.h"
#include "formatter_kml.h"

void formatter_kml_earth_init(formatter_t *obj, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task_ft, task_t *task) {
    obj->set = set;
    obj->open_distance = task_od;
    obj->out_and_return = task_or;
    obj->triangle = task_tr;
    obj->flat_triangle = task_ft;
    obj->task = task;
    obj->name = name;
}

void get_partial_linestring_earth(HDF *hdf, char *type, int i, coordinate_t *coordinate, coordinate_t *last, size_t style) {
    NEOERR *err;
    int16_t j = 0;
    while (coordinate != last) {
        _cs_set_valuef(hdf, "gradients.%s.sets.%d.points.%d.lng=%f", type, i, j, coordinate->lng);
        _cs_set_valuef(hdf, "gradients.%s.sets.%d.points.%d.lat=%f", type, i, j, coordinate->lat);
        _cs_set_valuef(hdf, "gradients.%s.sets.%d.points.%d.ele=%f", type, i, j, coordinate->ele);
        _cs_set_valuef(hdf, "gradients.%s.sets.%d.points.%d.time=%f", type, i, j, coordinate->timestamp);
        coordinate = coordinate->next;
        j++;
    }

    _cs_set_valuef(hdf, "gradients.%s.sets.%d.style=S%d", type, i, style);
}

char *colour_grad_16[] = {
    "ff0000", "ff3f00", "ff7f00", "ffbf00", "ffff00", "bfff00", "7fff00", "3fff00", "00ff00", "00ff3f", "00ff7f", "00ffbf", "00ffff", "00bfff", "007fff", "003fff",
};

void get_kml_styles_earth(HDF *hdf) {
    NEOERR *err;
    for (int16_t i = 0; i < 16; i++) {
        _cs_set_valuef(hdf, "styles.%d.id=S%d", i, i);
        _cs_set_valuef(hdf, "styles.%d.colour=ff%s", i, colour_grad_16[i]);
    }
}

void get_colour_by_x(coordinate_set_t *set, HDF *hdf, char *title, char *type, double delta, int min, size_t (*func)(coordinate_t *, double, int)) {
    NEOERR *err;
    coordinate_t *last, *first, *current;
    first = current = set->first;
    int16_t last_level, current_level;
    last_level = (*func)(current, delta, min);
    int group = 0;
    while (current) {
        current_level = (*func)(current, delta, min);
        if (current_level != last_level && current_level != 16) {
            get_partial_linestring_earth(hdf, type, group++, first, current->next, last_level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        get_partial_linestring_earth(hdf, type, group++, first, current, last_level);
    }
    _cs_set_valuef(hdf, "gradients.%s.title=%s", type, title);
}

void get_colour_by_height(coordinate_set_t *set, HDF *hdf) {
    double delta = (set->max_ele - set->min_ele ?: 1) / 16;
    get_colour_by_x(set, hdf, "Colour by height", "height", delta, set->min_ele, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->ele - min) / delta); } a; } ));
}

void get_colour_by_climb_rate(coordinate_set_t *set, HDF *hdf) {
    double delta = (set->max_climb_rate - set->min_climb_rate ?: 0) / 16;
    get_colour_by_x(set, hdf, "Colour by climb", "climb", delta, set->min_climb_rate ?: 0, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->climb_rate - min) / delta); } a; } ));
}

void get_colour_by_speed(coordinate_set_t *set, HDF *hdf) {
    double delta = (set->max_climb_rate) / 16;
    get_colour_by_x(set, hdf, "Colour by ground speed", "speed", delta, 0, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->speed - min) / delta); } a; } ));
}

void get_colour_by_time(coordinate_set_t *set, HDF *hdf) {
    double delta = (set->last->timestamp - set->first->timestamp) / 16;
    get_colour_by_x(set, hdf, "Colour by time", "time", delta, set->first->timestamp, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->timestamp - min) / delta); } a; } ));
}

char *format_timestamp(int year, int16_t month, int16_t day, int16_t ts) {
    char *buffer = calloc(20, sizeof(char));
    struct tm point_time = {.tm_year = year - 1990, .tm_mon = month - 1, .tm_mday = day, .tm_hour = floor(ts / 3600), .tm_min = floor((ts % 3600) / 60), .tm_sec = (ts % 60)};
    strftime(buffer, 20, "%Y-%m-%dT%T", &point_time);
    return buffer;
}

void get_shadow(HDF *hdf, char *title, char *type, BOOL extrude, char *mode) {
    NEOERR *err;
    _cs_set_valuef(hdf, "shadow.%s.title=%s", type, title);
    _cs_set_valuef(hdf, "shadow.%s.mode=%s", type, mode);
    _cs_set_valuef(hdf, "shadow.%s.extrude=%d", type, extrude);
}

void get_shadow_none(HDF *hdf) {
    get_shadow(hdf, "None", "none", 0, "");
}

void get_shadow_basic(HDF *hdf) {
    get_shadow(hdf, "Shadow", "basic", 0, "clampToGround");
}

void get_shadow_extrude(HDF *hdf) {
    get_shadow(hdf, "Extrude", "extrude", 1, "absolute");
}

char *formatter_kml_earth_output(formatter_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");

    CSPARSE *csparse;
    HDF *hdf;
    NEOERR *err;
    int stage = 0;
    if ((err = hdf_init(&hdf)) != STATUS_OK) {
        goto error;
    }
    set_formatter_values(hdf, obj);
    get_colour_by_height(obj->set, hdf);
    get_colour_by_speed(obj->set, hdf);
    get_colour_by_climb_rate(obj->set, hdf);
    get_colour_by_time(obj->set, hdf);

    get_shadow_none(hdf);
    get_shadow_basic(hdf);
    get_shadow_extrude(hdf);
    if ((err = cs_init(&csparse, hdf)) != STATUS_OK ||
        (err = cs_parse_file(csparse, "formatter/templates/flight.earth.kml.cs.xml")) != STATUS_OK ||
        (err = cs_render(csparse, fp, cs_fwrite)) != STATUS_OK) {
        goto error;
    }

    goto end;

    error:
    nerr_log_error(err);
    goto end;

    end:
    hdf_destroy(&hdf);
    cs_destroy(&csparse);
    fclose(fp);
}
//
//char *get_colour_by_time(coordinate_set_t *set) {
//    char *buffer = create_buffer("");
//    int64_t min = set->first->timestamp;
//    double delta = (set->last->timestamp - min ?: 1) / 16;
//    coordinate_t *current = set->first;
//    int16_t current_level;
//    char *point, *next_point;
//    char *point_date, *next_point_date;
//    if (current) {
//        point = coordinate_to_kml(current);
//        point_date = format_timestamp(set->year, set->month, set->day, current->timestamp);
//        while (current && current->next) {
//            next_point = coordinate_to_kml(current->next);
//            next_point_date = format_timestamp(set->year, set->month, set->day, current->next->timestamp);
//            char level[5];
//            sprintf(level, "#S%d", floor((current->timestamp - min) / delta));
//            buffer = vstrcat(buffer, "\n\
//<Placemark>\n\
//    <styleUrl>",
//                             level, "</styleUrl>\n\
//    <TimeSpan>\n\
//        <begin>",
//                             point_date, "Z</begin>\n\
//        <end>",
//                             next_point_date, "Z</end>\n\
//    </TimeSpan>\n\
//    <LineString>\n\
//        <altitudeMode>absolute</altitudeMode>\n\
//        <coordinates>\n",
//                             point, " ", next_point, "\n\
//        </coordinates>\n\
//    </LineString>\n\
//</Placemark>\n",
//                             NULL);
//            free(point);
//            free(point_date);
//            current = current->next;
//            point = next_point;
//            point_date = next_point_date;
//        }
//    };
//    return buffer;
//}
