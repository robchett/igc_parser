#include <php.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../geometry.h"
#include "../task.h"
#include "formatter_js.h"
#include "../include/json/jansson.h"

zend_object_handlers formatter_js_object_handler;

void init_formatter_js(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "formatter_js", formatter_js_methods);
    formatter_js_ce = zend_register_internal_class(&ce TSRMLS_CC);
    formatter_js_ce->create_object = create_formatter_js_object;

    memcpy(&formatter_js_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    formatter_js_object_handler.free_obj = free_formatter_js_object;
    formatter_js_object_handler.offset = XtOffsetOf(formatter_object, std);
}

zend_object* create_formatter_js_object(zend_class_entry *class_type TSRMLS_DC) {
    formatter_object* retval;

    retval = ecalloc(1, sizeof(formatter_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&retval->std, class_type TSRMLS_CC);
    object_properties_init(&retval->std, class_type);

    retval->std.handlers = &formatter_js_object_handler;

    return &retval->std;
}

void free_formatter_js_object(formatter_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    free(intern);
}

static zend_function_entry formatter_js_methods[] = {
    PHP_ME(formatter_js, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(formatter_js, output, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD(formatter_js, __construct) {
    zval *coordinate_set_zval;
    zval *od_zval = NULL;
    zval *or_zval = NULL;
    zval *tr_zval = NULL;
    int64_t id;
    formatter_object *intern = fetch_formatter_object(getThis() TSRMLS_CC);
    if (zend_parse_parameters(
                ZEND_NUM_ARGS() TSRMLS_CC,
                "Ol|zzz",
                &coordinate_set_zval, coordinate_set_ce,
                &id,
                &od_zval,
                &or_zval,
                &tr_zval
            ) == FAILURE) {
        return;
    }

    intern->set = (coordinate_set_object *) fetch_coordinate_set_object(coordinate_set_zval TSRMLS_CC);
    intern->open_distance = is_object_of_type(od_zval, task_ce) ? (task_object *) fetch_task_object(od_zval TSRMLS_CC) : NULL;
    intern->out_and_return = is_object_of_type(or_zval, task_ce) ? (task_object *) fetch_task_object(or_zval TSRMLS_CC) : NULL;
    intern->triangle = is_object_of_type(tr_zval, task_ce) ? (task_object *) fetch_task_object(tr_zval TSRMLS_CC) : NULL;
    intern->id = id;
}

json_t *get_bounds(coordinate_set_object *set) {
    coordinate_object *coordinate = set->first;
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
    json_t *center = json_pack("{s:f, s:f}", 
        "lat", (south + north) / 2,
        "lng", (west + east) / 2
    );

    coordinate_object *se = malloc(sizeof(coordinate_object));
    se->lat = south;
    se->lng = east;
    coordinate_object *nw = malloc(sizeof(coordinate_object));
    nw->lat = north;
    nw->lng = west;

    json_t *point = json_pack("{s:f, s:f, s:f, s:f, s:o, s:f}", 
        "north", north,  
        "east", east, 
        "south", south,
        "west", west,
        "center", center,
        "range", get_distance_precise(se, nw) * 5
    );
    return point;
}

char *get_html_output(formatter_object *intern) {
    char *string = create_buffer("");
    char *id = (intern->id ? itos(intern->id) :  "");
    string = vstrcat(string,  "<div class=\"kmltree\" data-post='{\"id\":", id, "}'>", "", "</div>", NULL);
    free(id);
    return string;
}

char *get_html_output_earth(formatter_object *intern) {
    char *string = create_buffer("");
    char *id = (intern->id ? itos(intern->id) :  "");
    string = vstrcat(string,  "<div class=\"kmltree\" data-post='{\"id\":", id, "}'>", "", "</div>", NULL);
    free(id);
    return string;
}

PHP_METHOD(formatter_js, output) {
    formatter_object *intern = fetch_formatter_object(getThis() TSRMLS_CC);
    json_t *json = json_object();

    set_graph_values(intern->set);

    json_object_set(json, "id", json_integer(intern->id));
    json_object_set(json, "xMin", json_integer(intern->set->first->timestamp));

    json_object_set(json, "xMax", json_integer(intern->set->last->timestamp));
    if (intern->open_distance) {
        json_object_set(json, "od_score", json_real(get_task_distance(intern->open_distance)));
        json_object_set(json, "od_time", json_integer(get_task_time(intern->open_distance)));
    }
    if (intern->out_and_return) {
        json_object_set(json, "or_score", json_real(get_task_distance(intern->out_and_return)));
        json_object_set(json, "or_time", json_integer(get_task_time(intern->out_and_return)));
    }
    if (intern->triangle) {
        json_object_set(json, "tr_score", json_real(get_task_distance(intern->triangle)));
        json_object_set(json, "tr_time", json_integer(get_task_time(intern->triangle)));
    }
    // if (intern->failed_triangle) {
    //     json_object_set(json, "ft_score", json_real(get_task_distance(intern->failed_triangle)));
    //     json_object_set(json, "ft_time", json_integer(get_task_time(intern->failed_triangle)));
    // }

    json_t *inner = json_object();
    json_object_set(inner, "draw_graph", json_integer(1));
    json_object_set(inner, "pilot", json_string("N/A"));
    json_object_set(inner, "colour", json_string("FF0000"));
    json_object_set(inner, "min_ele", json_integer(intern->set->min_ele));
    json_object_set(inner, "max_ele", json_integer(intern->set->max_ele));
    json_object_set(inner, "min_cr", json_integer(intern->set->min_climb_rate));
    json_object_set(inner, "max_cr", json_integer(intern->set->max_climb_rate));
    json_object_set(inner, "min_speed", json_integer(0));
    json_object_set(inner, "max_speed", json_integer(intern->set->max_speed));
    json_object_set(inner, "total_dist", json_integer(0));
    json_object_set(inner, "av_speed", json_integer(0));
    json_object_set(inner, "bounds", get_bounds(intern->set));
    // Can I not do this?
    // char *html = get_html_output(intern);
    // json_object_set(inner, "html", json_string(html));
    // free(html);
    // char *html_earth = get_html_output_earth(intern);
    // json_object_set(inner, "html_earth", json_string(html_earth));
    // free(html_earth);

    json_t *data = json_array();
    json_t *coordinates = json_array();
    coordinate_object *coordinate = intern->set->first;
    while (coordinate) {
        json_t *point = json_pack("{s:i, s:f, s:f}", "ele", coordinate->ele, "lat", coordinate->lat, "lng", coordinate->lng);
        json_t *point_data = json_pack("[iifff]", coordinate->timestamp - intern->set->first->timestamp, coordinate->ele, coordinate->climb_rate, coordinate->speed, coordinate->bearing);
        json_array_append(coordinates, point);
        json_array_append(data, point_data);
        coordinate = coordinate->next;
    }
    json_object_set(inner, "coords", coordinates);
    json_object_set(inner, "data", data);

    json_t *tracks = json_array();

    json_array_append(tracks, inner);
    json_object_set(json, "track", tracks);
    RETURN_STRING(json_dumps(json, JSON_PRESERVE_ORDER));
}