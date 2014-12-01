#include <php.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../geometry.h"
#include "../task.h"
#include "formatter_kml_earth.h"

void init_formatter_kml_earth(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "formatter_kml_earth", formatter_kml_earth_methods);
    formatter_kml_earth_ce = zend_register_internal_class(&ce TSRMLS_CC);
    formatter_kml_earth_ce->create_object = create_formatter_kml_earth_object;
}

zend_object_value create_formatter_kml_earth_object(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;

    formatter_object *intern = emalloc(sizeof(formatter_object));
    memset(intern, 0, sizeof(formatter_object));

    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object,  (zend_objects_free_object_storage_t)free_formatter_kml_earth_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

void free_formatter_kml_earth_object(formatter_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    efree(intern);
}

static zend_function_entry formatter_kml_earth_methods[] = {
    PHP_ME(formatter_kml_earth, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(formatter_kml_earth, output, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};


PHP_METHOD(formatter_kml_earth, __construct) {
    zval *coordinate_set_zval;
    zval *od_zval = NULL;
    zval *or_zval = NULL;
    zval *tr_zval = NULL;
    zval *task_zval = NULL;
    char *name;
    long length;
    formatter_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    intern->set = NULL;
    intern->open_distance = NULL;
    intern->triangle = NULL;
    intern->task = NULL;
    intern->name = NULL;
    if (zend_parse_parameters(
                ZEND_NUM_ARGS() TSRMLS_CC,
                "Os|zzzz",
                &coordinate_set_zval, coordinate_set_ce,
                &name, &length,
                &od_zval,
                &or_zval,
                &tr_zval,
                &task_zval
            ) == FAILURE) {
        return;
    }

    intern->set = (coordinate_set_object *)zend_object_store_get_object(coordinate_set_zval TSRMLS_CC);
    intern->open_distance = is_object_of_type(od_zval, task_ce) ? (task_object *)zend_object_store_get_object(od_zval TSRMLS_CC) : NULL;
    intern->out_and_return = is_object_of_type(or_zval, task_ce) ? (task_object *)zend_object_store_get_object(or_zval TSRMLS_CC) : NULL;
    intern->triangle = is_object_of_type(tr_zval, task_ce) ? (task_object *)zend_object_store_get_object(tr_zval TSRMLS_CC) : NULL;
    intern->task = is_object_of_type(task_zval, task_ce) ? (task_object *)zend_object_store_get_object(task_zval TSRMLS_CC) : NULL;
    intern->name = name;
}

char *get_meta_data_earth(formatter_object *intern) {
    char *buffer = create_buffer("<Metadata><SecondsFromTimeOfFirstPoint>");
    coordinate_object *coordinate = intern->set->first;
    int i = 0;
    while (coordinate) {
        char *timestamp = itos(coordinate->timestamp);
        buffer = vstrcat(buffer, timestamp, " ", NULL);
        efree(timestamp);
        if (i++ == 15) {
            i = 0;
            buffer = vstrcat(buffer, "\n", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n</SecondsFromTimeOfFirstPoint></Metadata>", NULL);
}

char *get_linestring_earth(formatter_object *intern, char *style, char *altitude_mode, char *extrude) {
    char *buffer = create_buffer("");
    buffer = vstrcat(buffer, "\n\
    <styleUrl>#", style, "</styleUrl>    \
    <LineString>\n\
	<extrude>", extrude, "</extrude>\n\
	<altitudeMode>", altitude_mode, "</altitudeMode>\n\
	<coordinates>", NULL);
    coordinate_object *coordinate = intern->set->first;
    int i = 0;
    while (coordinate) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        efree(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            buffer = vstrcat(buffer, "\n\t\t\t\t", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n</coordinates></LineString>", NULL);
}

char *get_partial_linestring_earth(coordinate_object *coordinate, coordinate_object *last, char *style) {
    char *buffer = create_buffer("");
    buffer = vstrcat(buffer, "\
<Placemark>\n\
    <styleUrl>#S", style , "</styleUrl>\n\
    <LineString>\n\
        <extrude>0</extrude>\n\
        <altitudeMode>absolute</altitudeMode>\n\
        <coordinates>\n", NULL);
    int i = 0;
    while (coordinate != last) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        efree(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            buffer = vstrcat(buffer, "\n", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n\
        </coordinates>\n\
    </LineString>\n\
</Placemark>\n", NULL);
}

char *format_task_point_earth(coordinate_object *coordinate, int index, coordinate_object *prev, double *total_distance) {
    double distance = 0;
    if (prev && coordinate) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = get_os_grid_ref(coordinate);
    char *buffer = emalloc(sizeof(char) * 60);
    sprintf(buffer, "%-2d   %-8.5f   %-9.5f   %-s   %-5.2f      %-5.2f", index,  coordinate->lat,  coordinate->lng,  gridref,  distance,  *total_distance);
    efree(gridref);
    return buffer;
}

char *get_task_generic_earth(task_object *task, char *title, char *colour) {
    double distance = 0;
    char *info = create_buffer("");
    char *coordinates = create_buffer("");
    coordinate_object *prev = NULL;
    int i;
    for (i = 0; i < task->size; i++) {
        if (task->coordinate[i]) {
            char *line = format_task_point_earth(task->coordinate[i], i + 1, prev, &distance);
            info = vstrcat(info, line, "\n", NULL);
            prev = task->coordinate[i];
            char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
            coordinates = vstrcat(coordinates, kml_coordinate, NULL);
            efree(kml_coordinate);
            efree(line);
        } else {
            printf("%s -> %d missing\n", title, i);
        }
    }
    char *buffer = create_buffer("");
    buffer = vstrcat(buffer, "\n\
        <Folder>\n\
            <name>", title, "</name>\n\
            <visibility>1</visibility>\n\
            <styleUrl>#hideChildren</styleUrl>\n\
            <Placemark>\n\
            <visibility>1</visibility>\n\
                <name>", title, "</name>\n\
                <description>\n\
                <![CDATA[<pre>\n\
TP   Latitude   Longitude   OS Gridref   Distance   Total\
", info, "\n\
                                          Duration: 01:56:00\n\
                    </pre>]]>\n\
                </description>\n\
                <Style>\n\
                    <LineStyle>\n\
                        <color>FF", colour, "</color>\n\
                        <width>2</width>\n\
                    </LineStyle>\n\
                </Style>\n\
                <LineString>\n\
                    <coordinates>\
                        ", coordinates, "\n\
                    </coordinates>\n\
                </LineString>\n\
            </Placemark>\n\
        </Folder>", NULL);
    efree(info);
    efree(coordinates);
    return buffer;
}

char *get_circle_coordinates_earth(coordinate_object *coordinate, int radius) {
    double angularDistance = radius / 6378137.0;
    double sin_lat = 0;
    double cos_lat = 0;
    double cos_lng = 0;
    double sin_lng = 0;
    sincos(coordinate->lat toRAD, &sin_lat, &cos_lat);
    sincos(coordinate->lng toRAD, &sin_lng, &cos_lng);
    char *buffer = create_buffer("");
    int i = 0;
    for (i = 0; i <= 360; i++) {
        double bearing = i toRAD;
        double lat = asin(sin_lat * cos(angularDistance) + cos_lat * sin(angularDistance) * cos(bearing));
        double dlon = atan2(sin(bearing) * sin(angularDistance) * cos_lat, cos(angularDistance) - sin_lat * sin(lat));
        double lng = fmod((coordinate->lng toRAD + dlon + M_PI), 2 * M_PI) - M_PI;

        char *lng_string = dtos(lng toDEG);
        char *lat_string = dtos(lat toDEG);
        buffer = vstrcat(buffer, lng_string, ",", lat_string, ",0 ", NULL);
        efree(lng_string);
        efree(lat_string);
    }
    return buffer;
}

char *get_defined_task_earth(task_object *task) {
    double distance = 0;
    char *info = create_buffer("\
<Folder>\
    <name>Task</name>");
    char *kml_coordinates = create_buffer("");
    int i;
    for (i = 0; i < task->size; i++) {
        char *coordinates = get_circle_coordinates_earth(task->coordinate[i], 400);
        char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
        kml_coordinates = vstrcat(kml_coordinates, kml_coordinate, NULL);
        efree(kml_coordinate);
        info = vstrcat(info, "\n\
    <Placemark>\n\
        <Style>\n\
            <PolyStyle>\n\
              <color>99ffffaa</color>\n\
              <fill>1</fill>\n\
              <outline>1</outline>\n\
            </PolyStyle>\n\
        </Style>\n\
        <Polygon>\n\
            <tessellate>1</tessellate>\n\
            <outerBoundaryIs>\n\
                <LinearRing>\n\
                    <coordinates>\n\n",
                       coordinates, "\n\
                    </coordinates>\n\
                </LinearRing>\n\
            </outerBoundaryIs>\n\
        </Polygon>\n\
    </Placemark>", "\n\n", NULL);
        efree(coordinates);
    }
    info = vstrcat(info, "\n\
    <Placemark>\n\
        <LineStyle>\n\
            <color>FFFFFF00</color>\n\
            <width>2</width>\n\
        </LineStyle>\n\
        <LineString>\n\
            <altitudeMode>clampToGround</altitudeMode>\n\
            <coordinates>", kml_coordinates, "</coordinates>\n\
        </LineString>\n\
     </Placemark>\n\
</Folder>", NULL);

    efree(kml_coordinates);
    return info;
}

char *get_task_od_earth(formatter_object *intern) {
    char *buffer = get_task_generic_earth( intern->open_distance, "Open Distance", "00D7FF");
    return buffer;
}

char *get_task_or_earth(formatter_object *intern) {
    char *buffer = get_task_generic_earth( intern->out_and_return , "Out and Return", "00FF00");
    return buffer;
}

char *get_task_tr_earth(formatter_object *intern) {
    char *buffer = get_task_generic_earth(intern->triangle, "FAI Triangle", "0000FF");
    return buffer;
}

char *get_task_ft_earth(formatter_object *intern) {
    char *buffer = get_task_generic_earth(intern->triangle, "Flat Triangle", "FF0066");
    return buffer;
}

PHP_METHOD(formatter_kml_earth, output) {
    formatter_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_STRING(formatter_kml_earth_output(intern), 1);
}

char *formatter_kml_earth_output(formatter_object *intern) {
    char *year = itos(intern->set->year);
    char *month = fitos(intern->set->month, "%02d");
    char *day = fitos(intern->set->day, "%02d");
    char *min_ele = itos(intern->set->min_ele);
    char *max_ele = itos(intern->set->max_ele);

    // TASKS
    char *tasks = create_buffer("");
    char *tasks_info = create_buffer("");
    if (intern->open_distance) {
        char *od_d = fdtos(get_task_distance(intern->open_distance), "%.2f");
        char *od_t = itos(get_task_time(intern->open_distance));
        char *open_distance = get_task_od_earth(intern);
        tasks = vstrcat(tasks, open_distance, NULL);
        tasks_info = vstrcat(tasks_info, "OD Score / Time      ", od_d, " / ", od_t, "s\n", NULL);
        efree(od_d);
        efree(od_t);
        efree(open_distance);
    }
    if (intern->open_distance) {
        char *or_d = fdtos(get_task_distance(intern->out_and_return), "%.2f");
        char *or_t = itos(get_task_time(intern->out_and_return ));
        char *out_and_return = get_task_or_earth(intern);
        tasks = vstrcat(tasks, out_and_return, NULL);
        tasks_info = vstrcat(tasks_info, "OR Score / Time      ", or_d, " / ", or_t, "s\n", NULL);
        efree(or_d);
        efree(or_t);
        efree(out_and_return);
    }
    if (intern->triangle) {
        char *tr_d = fdtos(get_task_distance(intern->triangle), "%.2f");
        char *tr_t = itos(get_task_time(intern->triangle));
        char *triangle = get_task_tr_earth(intern);
        tasks = vstrcat(tasks, triangle, NULL);
        tasks_info = vstrcat(tasks_info, "TR Score / Time      ", tr_d, " / ", tr_t, "s\n", NULL);
        efree(tr_d);
        efree(tr_t);
        efree(triangle);
    }
    if (intern->task) {
        char *task = get_defined_task_earth(intern->task);
        tasks = vstrcat(tasks, task, NULL);
        efree(task);
    }

    char *styles = get_kml_styles_earth();
    char *metadata = get_meta_data_earth(intern);
    char *linestring = get_linestring_earth(intern, "", "absolute", "0");
    char *shadow = get_linestring_earth(intern, "shadow", "clampToGround", "0");
    char *shadow_extrude = get_linestring_earth(intern, "shadow", "absolute", "1");
    char *height = get_colour_by_height(intern->set);
    char *speed = get_colour_by_speed(intern->set);
    char *climb_rate = get_colour_by_climb_rate(intern->set);
    char *timestamp = get_colour_by_time(intern->set);

    char *output = create_buffer("");
    output = vstrcat(output, "<?xml version='1.0' encoding='UTF-8'?>\n\
<Document>\n\
	<open>1</open>\n",
                     styles, "\n\
    <Folder>\n\
        <name>", intern->name, "</name>\n\
		<visibility>1</visibility>\n\
        <open>1</open>\n\
        <Folder>\n\
            <name>Main Track</name>\n\
            <visibility>1</visibility>\n\
            <open>1</open>\n\
            <styleUrl>radioFolder</styleUrl>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour by height</name>\n\
                <visibility>1</visibility>\n",
                     height,  "\
                <open>0</open>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour by ground speed</name>\n\
                <visibility>0</visibility>\n",
                     speed,  "\
                <open>0</open>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour By Climb</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n",
                     climb_rate,  "\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour by time</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n",
                     timestamp,  "\
            </Folder>\n\
        </Folder>\n\
        <Folder>\n\
            <name>Shadow</name>\n\
            <visibility>1</visibility>\n\
            <open>1</open>\n\
            <styleUrl>radioFolder</styleUrl>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>None</name>\n\
                <visibility>1</visibility>\n\
                <open>0</open>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Shadow</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n\
                <Placemark>\n",
                     shadow,  "\
                 </Placemark>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Extrude</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n\
                 <Placemark>\n",
                     shadow_extrude,  "\
                 </Placemark>\n\
            </Folder>\n\
        </Folder>\n\
		<Folder>\n\
		  <name>Task</name>\n\
		  <visibility>1</visibility>\n\
		  ", tasks, "\n\
		</Folder>\n\
	</Folder>\n\
</Document>", NULL);

    efree(year);
    efree(month);
    efree(day);
    efree(max_ele);
    efree(min_ele);
    efree(metadata);
    efree(linestring);
    efree(tasks);
    efree(tasks_info);
    efree(styles);
    efree(height);
    efree(climb_rate);
    efree(timestamp);
    efree(shadow);
    efree(shadow_extrude);
    efree(speed);
    return output;
}

char *colour_grad_16[] = {
    "ff0000",
    "ff3f00",
    "ff7f00",
    "ffbf00",
    "ffff00",
    "bfff00",
    "7fff00",
    "3fff00",
    "00ff00",
    "00ff3f",
    "00ff7f",
    "00ffbf",
    "00ffff",
    "00bfff",
    "007fff",
    "003fff",
};

char *get_kml_styles_earth() {
    char *buffer = create_buffer("\
    <Style id='hideChildren'>\n\
        <ListStyle>\n\
            <listItemType>checkHideChildren</listItemType>\n\
        </ListStyle>\n\
    </Style>\n\
    <Style id='radioFolder'>\n\
        <ListStyle>\n\
            <listItemType>radioFolder</listItemType>\n\
        </ListStyle>\n\
    </Style>\n\
    <Style id=\"shadow\">\n\
        <LineStyle>\n\
            <color>AA000000</color>\n\
            <width>1</width>\n\
        </LineStyle>\n\
        <PolyStyle>\n\
            <color>55AAAAAA</color>\n\
        </PolyStyle>\n\
    </Style>\n");

    int i;
    for (i = 0; i < 16; i++) {
        char *level = itos(i);
        buffer = vstrcat(buffer, "\n\
    <Style id=\"S", level, "\">\n\
        <LineStyle>\n\
            <width>2</width>\n\
            <color>FF", colour_grad_16[i], "</color>\n\
        </LineStyle>\n\
    </Style>", NULL);
        efree(level);
    }
    //$kml->set_animation_styles(1);
    //public function set_animation_styles() {
    //    for ($i = 0; $i < 10; $i++) {
    //        for ($j = 0; $j < 360; $j += 5) {
    //            $this->styles .= '<Style id="A' . $i . $j . '"><IconStyle><heading>' . $j . '</heading><Icon><href>http://' . host . '/img/Markers/' . _get::kml_colour($i) . '.gif' . '</href></Icon></IconStyle></Style>';
    //        }
    //    }
    //}
    return buffer;
}

char *get_colour_by_height(coordinate_set_object *set) {
    char *buffer = create_buffer("");
    long min = set->min_ele;
    double delta = (set->max_ele - set->min_ele ? : 1) / 16;
    coordinate_object *last, *first, *current;
    first = current = set->first;
    int last_level, current_level;
    last_level = floor((current->ele - min) / delta);
    while (current) {
        char *level = itos(last_level);
        current_level = floor((current->ele - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            efree(linestring);
            efree(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        efree(linestring);
        efree(level);
    }
    return buffer;
}

char *get_colour_by_climb_rate(coordinate_set_object *set) {
    char *buffer = create_buffer("");
    long min = set->min_climb_rate;
    double delta = (set->max_climb_rate - set->min_climb_rate ? : 1) / 16;
    coordinate_object *last, *first, *current;
    first = current = set->first;
    int last_level, current_level;
    last_level = floor((current->climb_rate - min) / delta);
    while (current) {
        char *level = itos(last_level);
        current_level = floor((current->climb_rate - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            efree(linestring);
            efree(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        efree(linestring);
        efree(level);
    }
    return buffer;
}


char *get_colour_by_speed(coordinate_set_object *set) {
    char *buffer = create_buffer("");
    long min = 0;
    double delta = (set->max_speed ? : 1) / 16;
    coordinate_object *last, *first, *current;
    first = current = set->first;
    int last_level, current_level;
    last_level = floor((current->speed - min) / delta);
    while (current) {
        current_level = floor((current->speed - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *level = itos(last_level);
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            efree(linestring);
            efree(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        efree(linestring);
        efree(level);
    }
    return buffer;
}

char *format_timestamp(int year, int month, int day, int ts) {
    char *buffer = emalloc(sizeof(char) * 20);
    struct tm point_time = {
        .tm_year = year - 1990,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = floor(ts / 3600),
        .tm_min = floor((ts % 3600) / 60),
        .tm_sec = (ts % 60)
    };
    strftime(buffer, 20, "%Y-%m-%dT%T", &point_time);
    return buffer;
}

char *get_colour_by_time(coordinate_set_object *set) {
    char *buffer = create_buffer("");
    long min = set->first->timestamp;
    double delta = (set->last->timestamp - min ? : 1) / 16;
    coordinate_object *current = set->first;
    int current_level;
    char *point, *next_point;
    char *point_date, *next_point_date;
    if (current) {
        point = coordinate_to_kml(current);
        point_date = format_timestamp(set->year, set->month, set->day, current->timestamp);
        while (current && current->next) {
            next_point = coordinate_to_kml(current->next);
            next_point_date = format_timestamp(set->year, set->month, set->day, current->next->timestamp);
            char *level = itos(floor((current->timestamp - min) / delta));
            buffer = vstrcat(buffer, "\n\
<Placemark>\n\
    <styleUrl>#S", level , "</styleUrl>\n\
    <TimeSpan>\n\
        <begin>", point_date, "Z</begin>\n\
        <end>", next_point_date, "Z</end>\n\
    </TimeSpan>\n\
    <LineString>\n\
        <altitudeMode>absolute</altitudeMode>\n\
        <coordinates>\n", point, " ", next_point, "\n\
        </coordinates>\n\
    </LineString>\n\
</Placemark>\n", NULL);
            efree(point);
            efree(level);
            efree(point_date);
            current = current->next;
            point = next_point;
            point_date = next_point_date;
        }
    };
    return buffer;
}

