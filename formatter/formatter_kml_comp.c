#include "../main.h"
#include <string.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include "formatter_kml_comp.h"
#include "formatter_kml.h"

void formatter_kml_comp_init(formatter_comp_t *obj, size_t size, coordinate_set_t **sets, task_t *task) {
    obj->size = size;
    obj->sets = sets;
    obj->task = task;
}

void set_formatter_comp_values(HDF *hdf, formatter_comp_t *obj) {
    NEOERR *err;

    if (obj->task) {
        get_task_generic(obj->task, "GO", "Defined Task", "FF9900CC", hdf);
        get_defined_task(obj->task, "GO", hdf);
    }

    for (size_t i = 0; i < obj->size; i++) {
        coordinate_set_t *set = obj->sets[i];
        coordinate_t *coordinate = set->first;
        int16_t j = 0;
        _cs_set_valuef(hdf, "tracks.%d.colour=%s", i, kml_colours[i % 9]);
        _cs_set_valuef(hdf, "tracks.%d.min_height=%d", i, set->min_ele);
        _cs_set_valuef(hdf, "tracks.%d.max_height=%d", i, set->max_ele);
        _cs_set_valuef(hdf, "tracks.%d.min_climb_rate=%f", i, set->min_climb_rate);
        _cs_set_valuef(hdf, "tracks.%d.max_climb_rate=%f", i, set->max_climb_rate);
        _cs_set_valuef(hdf, "tracks.%d.min_speed=%d", i, 0);
        _cs_set_valuef(hdf, "tracks.%d.max_speed=%f", i, set->max_speed);
        _cs_set_valuef(hdf, "tracks.%d.turnpoints=%d", i, 2);
        while (coordinate) {
            _cs_set_valuef(hdf, "tracks.%d.points.%d.lng=%f", i, j, coordinate->lng);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.lat=%f", i, j, coordinate->lat);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.ele=%d", i, j, coordinate->ele);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.time=%f", i, j, coordinate->timestamp);
            coordinate = coordinate->next;
            j++;
        }
    }
}

void formatter_kml_comp_output(formatter_comp_t *obj, char *kml_filename, char *js_filename) {
    FILE *fp = fopen(kml_filename, "w");
    FILE *js_fp = fopen(js_filename, "w");

    CSPARSE *csparse;
    HDF *hdf;
    NEOERR *err;
    int stage = 0;
    if ((err = hdf_init(&hdf)) != STATUS_OK) {
        goto error;
    }
    set_formatter_comp_values(hdf, obj);
    if ((err = cs_init(&csparse, hdf)) != STATUS_OK ||
        (err = cs_parse_file(csparse, "formatter/templates/comp.kml.cs.xml")) != STATUS_OK ||
        (err = cs_render(csparse, fp, cs_fwrite)) != STATUS_OK) {
        goto error;
    }
    if ((err = cs_init(&csparse, hdf)) != STATUS_OK ||
        (err = cs_parse_file(csparse, "formatter/templates/comp.js.cs.json")) != STATUS_OK ||
        (err = cs_render(csparse, js_fp, cs_fwrite)) != STATUS_OK) {
        goto error;
    }

    hdf_write_file(hdf, "test/comp/0/out.hdf");

    goto end;

    error:
        nerr_log_error(err);
        goto end;

    end:
        hdf_destroy(&hdf);
        cs_destroy(&csparse);
        fclose(fp);
}
