#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "formatter_kml_split.h"

void formatter_kml_split_init(formatter_split_t *obj, coordinate_set_t *set) {
    obj->set = set;
}

void formatter_kml_split_output(formatter_split_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");

    CSPARSE *csparse;
    HDF *hdf;
    NEOERR *err;
    int stage = 0;
    if ((err = hdf_init(&hdf)) != STATUS_OK) {
        goto error;
    }

    coordinate_subset_t *subset = obj->set->first_subset;
    int16_t i = 0;
    while (subset) {
        _cs_set_valuef(hdf, "tracks.%d.colour=%s", i, kml_colours[i % 9]);
        coordinate_t *coordinate = subset->first;
        int16_t j = 0;
        while (coordinate && coordinate != subset->last) {
            _cs_set_valuef(hdf, "tracks.%d.points.%d.lng=%f", i, j, coordinate->lng);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.lat=%f", i, j, coordinate->lat);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.ele=%d", i, j, coordinate->ele);
            _cs_set_valuef(hdf, "tracks.%d.points.%d.time=%f", i, j, coordinate->timestamp);
            coordinate = coordinate->next;
            j++;
        }
        subset = subset->next;
        i++;
    }

    if ((err = cs_init(&csparse, hdf)) != STATUS_OK ||
        (err = cs_parse_file(csparse, "formatter/templates/flight.split.kml.cs.xml")) != STATUS_OK ||
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