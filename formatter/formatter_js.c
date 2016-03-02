#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include "formatter_js.h"
#include "../include/json/jansson.h"

void formatter_js_init(formatter_t *obj, coordinate_set_t *set, int64_t id, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task_ft) {
    obj->set = set;
    obj->open_distance = task_od;
    obj->out_and_return = task_or;
    obj->triangle = task_tr;
    obj->flat_triangle = task_ft;
    obj->id = id;
}

json_t *get_bounds(coordinate_set_t *set) {
    coordinate_t *coordinate = set->first;
    double north = coordinate->lat;
    double south = coordinate->lat;
    double east = coordinate->lng;
    double west = coordinate->lng;
    while (coordinate) {
        if (coordinate->lat > north) {
            north = coordinate->lat;
        }
        if (coordinate->lat < south) {
            south = coordinate->lat;
        }
        if (coordinate->lng > east) {
            east = coordinate->lng;
        }
        if (coordinate->lng > west) {
            west = coordinate->lng;
        }
        coordinate = coordinate->next;
    }
    json_t *center = json_pack("{s:f, s:f}", "lat", (south + north) / 2, "lng", (west + east) / 2);

    coordinate_t *se = NEW(coordinate_t, 1);
    se->lat = south;
    se->lng = east;
    coordinate_t *nw = NEW(coordinate_t, 1);
    nw->lat = north;
    nw->lng = west;

    json_t *point = json_pack("{s:f, s:f, s:f, s:f, s:o, s:f}", "north", north, "east", east, "south", south, "west", west, "center", center, "range", get_distance_precise(se, nw) * 5);
    return point;
}

char *get_html_output(formatter_t *obj) {
    char *string = create_buffer("");
    char *id = (obj->id ? itos(obj->id) : "");
    string = vstrcat(string, "<div class=\"kmltree\" data-post='{\"id\":", id, "}'>", "", "</div>", NULL);
    free(id);
    return string;
}

char *get_html_output_earth(formatter_t *obj) {
    char *string = create_buffer("");
    char *id = (obj->id ? itos(obj->id) : "");
    string = vstrcat(string, "<div class=\"kmltree\" data-post='{\"id\":", id, "}'>", "", "</div>", NULL);
    free(id);
    return string;
}

char *formatter_js_output(formatter_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        json_t *json = json_object();

        json_object_set(json, "id", json_integer(obj->id));
        json_object_set(json, "xMin", json_integer(obj->set->first->timestamp));

        json_object_set(json, "xMax", json_integer(obj->set->last->timestamp));
        if (obj->open_distance) {
            json_object_set(json, "od_score", json_real(get_task_distance(obj->open_distance)));
            json_object_set(json, "od_time", json_integer(get_task_time(obj->open_distance)));
        }
        if (obj->out_and_return) {
            json_object_set(json, "or_score", json_real(get_task_distance(obj->out_and_return)));
            json_object_set(json, "or_time", json_integer(get_task_time(obj->out_and_return)));
        }
        if (obj->triangle) {
            json_object_set(json, "tr_score", json_real(get_task_distance(obj->triangle)));
            json_object_set(json, "tr_time", json_integer(get_task_time(obj->triangle)));
        }
        if (obj->flat_triangle) {
            json_object_set(json, "ft_score", json_real(get_task_distance(obj->flat_triangle)));
            json_object_set(json, "ft_time",json_integer(get_task_time(obj->flat_triangle)));
        }
        
        json_t *inner = json_object();
        json_object_set(inner, "draw_graph", json_integer(1));
        json_object_set(inner, "pilot", json_string("N/A"));
        json_object_set(inner, "colour", json_string("FF0000"));
        json_object_set(inner, "min_ele", json_integer(obj->set->min_ele));
        json_object_set(inner, "max_ele", json_integer(obj->set->max_ele));
        json_object_set(inner, "min_cr", json_integer(obj->set->min_climb_rate));
        json_object_set(inner, "max_cr", json_integer(obj->set->max_climb_rate));
        json_object_set(inner, "min_speed", json_integer(0));
        json_object_set(inner, "max_speed", json_integer(obj->set->max_speed));
        json_object_set(inner, "total_dist", json_integer(0));
        json_object_set(inner, "av_speed", json_integer(0));
        json_object_set(inner, "bounds", get_bounds(obj->set));
        // Can I not do obj?
        // char *html = get_html_output(obj);
        // json_object_set(inner, "html", json_string(html));
        // free(html);
        // char *html_earth = get_html_output_earth(obj);
        // json_object_set(inner, "html_earth", json_string(html_earth));
        // free(html_earth);

        json_t *data = json_array();
        json_t *coordinates = json_array();
        coordinate_t *coordinate = obj->set->first;
        while (coordinate) {
            json_t *point = json_pack("{s:i, s:f, s:f}", "ele", coordinate->ele, "lat", coordinate->lat, "lng", coordinate->lng);
            json_t *point_data = json_pack("[iifff]", coordinate->timestamp - obj->set->first->timestamp, coordinate->ele, coordinate->climb_rate, coordinate->speed, coordinate->bearing);
            json_array_append(coordinates, point);
            json_array_append(data, point_data);
            coordinate = coordinate->next;
        }
        json_object_set(inner, "coords", coordinates);
        json_object_set(inner, "data", data);

        json_t *tracks = json_array();

        json_array_append(tracks, inner);
        json_object_set(json, "track", tracks);
        char *s = (json_dumps(json, JSON_PRESERVE_ORDER));
        fputs(s, fp);
        fclose(fp);
        free(s);
    } else {
        printf("Failed to open file: %s", filename);
    }
}