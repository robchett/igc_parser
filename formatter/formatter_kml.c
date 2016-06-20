#include "../main.h"
#include <string.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include "formatter_kml.h"

void formatter_kml_init(formatter_t *obj, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task_ft, task_t *task) {
    obj->set = set;
    obj->open_distance = task_od;
    obj->out_and_return = task_or;
    obj->triangle = task_tr;
    obj->flat_triangle = task_ft;
    obj->task = task;
    obj->name = name;
}

void get_task_generic(task_t *task, char *type, char *title, char *colour, HDF *hdf) {
    NEOERR *err;
    int stage = 0;
    double distance = 0, total_distance = 0;
    coordinate_t *prev = NULL;
    int16_t i;
    for (i = 0; i < task->size; i++) {
        if (task->coordinate[i]) {
            char *gridref = convert_latlng_to_gridref(task->coordinate[i]->lat, task->coordinate[i]->lng);
            if (prev) {
                distance = get_distance_precise(task->coordinate[i], prev);
                total_distance += distance;
            }

            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.id=%d", type, i, i);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.lat=%f", type, i, task->coordinate[i]->lat);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.lng=%f", type, i, task->coordinate[i]->lng);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.ele=%d", type, i, task->coordinate[i]->ele);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.gridref=%s", type, i, gridref);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.distance=%f", type, i, distance);
            _cs_set_valuef(hdf, "tasks.%s.turnpoints.%d.total=%f", type, i, total_distance);

            prev = task->coordinate[i];
            free(gridref);
        } else {
            errn("Missing task coordinate: %s -> %d\n", type, i);
        }

        _cs_set_valuef(hdf, "tasks.%s.short_title=%s", type, type);
        _cs_set_valuef(hdf, "tasks.%s.title=%s", type, title);
        _cs_set_valuef(hdf, "tasks.%s.colour=%s", type, colour);
        _cs_set_valuef(hdf, "tasks.%s.score=%f", type, total_distance);
        _cs_set_valuef(hdf, "tasks.%s.duration=%s", type, task_get_duration(task));
    }

    error:
        return;
}


void get_defined_task(task_t *task, char *type, HDF *hdf) {
    NEOERR *err;
    int stage = 0;
    int16_t i;
    for (i = 0; i < task->size; i++) {
        coordinate_t *coordinate = task->coordinate[i];
        int radius = 400;
        double angularDistance = radius / 6378137.0;
        double sin_lat = 0;
        double cos_lat = 0;
        double cos_lng = 0;
        double sin_lng = 0;
        sincos(coordinate->lat toRAD, &sin_lat, &cos_lat);
        sincos(coordinate->lng toRAD, &sin_lng, &cos_lng);
        int16_t j = 0;
        for (j = 0; j <= 360; j++) {
            double bearing = j toRAD;
            double lat = asin(sin_lat * cos(angularDistance) + cos_lat * sin(angularDistance) * cos(bearing));
            double dlon = atan2(sin(bearing) * sin(angularDistance) * cos_lat, cos(angularDistance) - sin_lat * sin(lat));
            double lng = fmod((coordinate->lng toRAD + dlon + M_PI), 2 * M_PI) - M_PI;

            char *lng_string = dtos(lng toDEG);
            char *lat_string = dtos(lat toDEG);
            _cs_set_valuef(hdf, "tasks.%s.zones.%d.coordinates.%d.lat=%f", type, i, j, lng toDEG)
            _cs_set_valuef(hdf, "tasks.%s.zones.%d.coordinates.%d.lng=%f", type, i, j, lat toDEG)
            _cs_set_valuef(hdf, "tasks.%s.zones.%d.coordinates.%d.ele=%d", type, i, j, 1000);
        }
    }

    error:
        return;
}

void set_formatter_values(HDF *hdf, formatter_t *obj) {
    NEOERR *err;
    _cs_set_value(hdf, "id", obj->name);
    _cs_set_value(hdf, "pilot", "");
    _cs_set_value(hdf, "club", "");
    _cs_set_value(hdf, "glider", "");
    _cs_set_value_d(hdf, "date.day", obj->set->day);
    _cs_set_value_d(hdf, "date.month", obj->set->month);
    _cs_set_value_d(hdf, "date.year", obj->set->year);
    _cs_set_value(hdf, "start", "");
    _cs_set_value(hdf, "finish", "");
    _cs_set_value(hdf, "duration", "");
    _cs_set_value_d(hdf, "height.max", obj->set->max_ele);
    _cs_set_value_d(hdf, "height.min", obj->set->min_ele);

    if (obj->open_distance) {
        get_task_generic(obj->open_distance, "OD", "Open Distance", "FF00D7FF", hdf);
    }
    if (obj->out_and_return) {
        get_task_generic(obj->out_and_return, "OR", "Out and Return", "FF00FF00", hdf);
    }
    if (obj->triangle) {
        get_task_generic(obj->triangle, "TR", "FAI Triangle", "FF0000FF", hdf);
    }
    if (obj->flat_triangle) {
        get_task_generic(obj->flat_triangle, "FT", "Flat Triangle", "FFFF0066", hdf);
    }
    if (obj->task) {
        get_task_generic(obj->task, "GO", "Defined Task", "FF9900CC", hdf);
        get_defined_task(obj->task, "GO", hdf);
    }

    coordinate_t *coordinate = obj->set->first;
    int16_t i = 0;
    while (coordinate) {
        i++;
        _cs_set_valuef(hdf, "coordinates.%d.lng=%f", i, coordinate->lng);
        _cs_set_valuef(hdf, "coordinates.%d.lat=%f", i, coordinate->lat);
        _cs_set_valuef(hdf, "coordinates.%d.ele=%d", i, coordinate->ele);
        _cs_set_valuef(hdf, "coordinates.%d.time=%f", i, coordinate->timestamp);
        coordinate = coordinate->next;
    }
}

void formatter_kml_output(formatter_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");

    CSPARSE *csparse;
    HDF *hdf;
    NEOERR *err;
    int stage = 0;
    if ((err = hdf_init(&hdf)) != STATUS_OK) {
        goto error;
    }
    set_formatter_values(hdf, obj);
    if ((err = cs_init(&csparse, hdf)) != STATUS_OK ||
        (err = cs_parse_file(csparse, "formatter/templates/flight.kml.cs.xml")) != STATUS_OK ||
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
