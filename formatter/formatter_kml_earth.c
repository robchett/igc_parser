#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include <time.h>
#include <string.h>
#include "formatter_kml_earth.h"

void formatter_kml_earth_init(formatter_t *this, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task) {
    this->set = set;
    this->open_distance = task_od;
    this->out_and_return = task_or;
    this->triangle = task_tr;
    this->task = task;
    this->name = name;
}

char *get_meta_data_earth(formatter_t *this) {
    char *buffer = create_buffer("<Metadata><SecondsFromTimeOfFirstPoint>");
    coordinate_t *coordinate = this->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *timestamp = itos(coordinate->timestamp);
        buffer = vstrcat(buffer, timestamp, " ", NULL);
        free(timestamp);
        if (i++ == 15) {
            i = 0;
            buffer = vstrcat(buffer, "\n", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n</SecondsFromTimeOfFirstPoint></Metadata>", NULL);
}

char *get_linestring_earth(formatter_t *this, char *style, char *altitude_mode, char *extrude) {
    char *coordinates = calloc((30 * this->set->length), sizeof(char));
    coordinate_t *coordinate = this->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        strcat(coordinates, kml_coordinate);
        free(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            strcat(coordinates, "\n\t\t\t\t");
        }
        coordinate = coordinate->next;
    }
    char *buffer = malloc(sizeof(char) * (200 + (30 * this->set->length)));
    sprintf(buffer, "\
    <styleUrl>#%s</styleUrl>\n\
    <LineString>\n\
	    <extrude>%s</extrude>\n\
	    <altitudeMode>%s</altitudeMode>\n\
	    <coordinates>\n\
            %s\n\
        </coordinates>\n\
    </LineString>",
            style, extrude, altitude_mode, coordinates);
    free(coordinates);
    return buffer;
}

char *get_partial_linestring_earth(coordinate_t *coordinate, coordinate_t *last, char *style) {
    char *buffer = create_buffer("");
    buffer = vstrcat(buffer, "\
<Placemark>\n\
    <styleUrl>#S",
                     style, "</styleUrl>\n\
    <LineString>\n\
        <extrude>0</extrude>\n\
        <altitudeMode>absolute</altitudeMode>\n\
        <coordinates>\n",
                     NULL);
    int16_t i = 0;
    while (coordinate != last) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        free(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            buffer = vstrcat(buffer, "\n", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n\
        </coordinates>\n\
    </LineString>\n\
</Placemark>\n",
                   NULL);
}

char *format_task_point_earth(coordinate_t *coordinate, int16_t index, coordinate_t *prev, double *total_distance) {
    double distance = 0;
    if (prev && coordinate) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = convert_latlng_to_gridref(coordinate->lat, coordinate->lng);
    char *buffer = malloc(sizeof(char) * 60);
    sprintf(buffer, "%-2d   %-8.5f   %-9.5f   %-s   %-5.2f      %-5.2f", index, coordinate->lat, coordinate->lng, gridref, distance, *total_distance);
    free(gridref);
    return buffer;
}

char *get_task_generic_earth(task_t *task, char *title, char *colour) {
    double distance = 0;
    char *info = create_buffer("");
    char *coordinates = create_buffer("");
    coordinate_t *prev = NULL;
    int16_t i;
    for (i = 0; i < task->size; i++) {
        if (task->coordinate[i]) {
            char *line = format_task_point_earth(task->coordinate[i], i + 1, prev, &distance);
            info = vstrcat(info, line, "\n", NULL);
            prev = task->coordinate[i];
            char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
            coordinates = vstrcat(coordinates, kml_coordinate, NULL);
            free(kml_coordinate);
            free(line);
        } else {
            printf("%s -> %d missing\n", title, i);
        }
    }
    char *buffer = create_buffer("");
    buffer = vstrcat(buffer, "\n\
        <Folder>\n\
            <name>",
                     title, "</name>\n\
            <visibility>1</visibility>\n\
            <styleUrl>#hideChildren</styleUrl>\n\
            <Placemark>\n\
            <visibility>1</visibility>\n\
                <name>",
                     title, "</name>\n\
                <description>\n\
                <![CDATA[<pre>\n\
TP   Latitude   Longitude   OS Gridref   Distance   Total\n\
",
                     info, "\n\
                                          Duration: 01:56:00\n\
                    </pre>]]>\n\
                </description>\n\
                <Style>\n\
                    <LineStyle>\n\
                        <color>FF",
                     colour, "</color>\n\
                        <width>2</width>\n\
                    </LineStyle>\n\
                </Style>\n\
                <LineString>\n\
                    <coordinates>\
                        ",
                     coordinates, "\n\
                    </coordinates>\n\
                </LineString>\n\
            </Placemark>\n\
        </Folder>",
                     NULL);
    free(info);
    free(coordinates);
    return buffer;
}

char *get_circle_coordinates_earth(coordinate_t *coordinate, int16_t radius) {
    double angularDistance = radius / 6378137.0;
    double sin_lat = 0;
    double cos_lat = 0;
    double cos_lng = 0;
    double sin_lng = 0;
    sincos(coordinate->lat toRAD, &sin_lat, &cos_lat);
    sincos(coordinate->lng toRAD, &sin_lng, &cos_lng);
    char *buffer = create_buffer("");
    int16_t i = 0;
    for (i = 0; i <= 360; i++) {
        double bearing = i toRAD;
        double lat = asin(sin_lat * cos(angularDistance) + cos_lat * sin(angularDistance) * cos(bearing));
        double dlon = atan2(sin(bearing) * sin(angularDistance) * cos_lat, cos(angularDistance) - sin_lat * sin(lat));
        double lng = fmod((coordinate->lng toRAD + dlon + M_PI), 2 * M_PI) - M_PI;

        char *lng_string = dtos(lng toDEG);
        char *lat_string = dtos(lat toDEG);
        buffer = vstrcat(buffer, lng_string, ",", lat_string, ",0 ", NULL);
        free(lng_string);
        free(lat_string);
    }
    return buffer;
}

char *get_defined_task_earth(task_t *task) {
    double distance = 0;
    char *info = create_buffer("\
<Folder>\
    <name>Task</name>");
    char *kml_coordinates = create_buffer("");
    int16_t i;
    for (i = 0; i < task->size; i++) {
        char *coordinates = get_circle_coordinates_earth(task->coordinate[i], 400);
        char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
        kml_coordinates = vstrcat(kml_coordinates, kml_coordinate, NULL);
        free(kml_coordinate);
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
    </Placemark>",
                       "\n\n", NULL);
        free(coordinates);
    }
    info = vstrcat(info, "\n\
    <Placemark>\n\
        <LineStyle>\n\
            <color>FFFFFF00</color>\n\
            <width>2</width>\n\
        </LineStyle>\n\
        <LineString>\n\
            <altitudeMode>clampToGround</altitudeMode>\n\
            <coordinates>",
                   kml_coordinates, "</coordinates>\n\
        </LineString>\n\
     </Placemark>\n\
</Folder>",
                   NULL);

    free(kml_coordinates);
    return info;
}

char *get_task_od_earth(formatter_t *this) {
    char *buffer = get_task_generic_earth(this->open_distance, "Open Distance", "00D7FF");
    return buffer;
}

char *get_task_or_earth(formatter_t *this) {
    char *buffer = get_task_generic_earth(this->out_and_return, "Out and Return", "00FF00");
    return buffer;
}

char *get_task_tr_earth(formatter_t *this) {
    char *buffer = get_task_generic_earth(this->triangle, "FAI Triangle", "0000FF");
    return buffer;
}

char *get_task_ft_earth(formatter_t *this) {
    char *buffer = get_task_generic_earth(this->triangle, "Flat Triangle", "FF0066");
    return buffer;
}

char *formatter_kml_earth_output(formatter_t *this, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        char *open_distance = NULL;
        char *out_and_return = NULL;
        char *triangle = NULL;
        char *task = NULL;
        if (this->open_distance) {
            open_distance = get_task_od_earth(this);
        }
        if (this->open_distance) {
            out_and_return = get_task_or_earth(this);
        }
        if (this->triangle) {
            triangle = get_task_tr_earth(this);
        }
        if (this->task) {
            task = get_defined_task_earth(this->task);
        }

        char *styles = get_kml_styles_earth();
        char *metadata = get_meta_data_earth(this);
        char *linestring = get_linestring_earth(this, "", "absolute", "0");
        char *shadow = get_linestring_earth(this, "shadow", "clampToGround", "0");
        char *shadow_extrude = get_linestring_earth(this, "shadow", "absolute", "1");

        char *height = "";
        char *speed = "";
        char *climb_rate = "";
        char *timestamp = "";
        // char *height = get_colour_by_height(this->set);
        // char *speed = get_colour_by_speed(this->set);
        // char *climb_rate = get_colour_by_climb_rate(this->set);
        // char *timestamp = get_colour_by_time(this->set);

        fprintf(fp, "\
<?xml version='1.0' encoding='UTF-8'?>\n\
<Document>\n\
	<open>1</open>\n\
    %s\n\
    <Folder>\n\
        <name>%s</name>\n\
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
                <visibility>1</visibility>\n\
                %s\n\
                <open>0</open>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour by ground speed</name>\n\
                <visibility>0</visibility>\n\
                %s\n\
                <open>0</open>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour By Climb</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n\
                %s\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Colour by time</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n\
                %s\n\
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
                <Placemark>\n\
                    %s\n\
                </Placemark>\n\
            </Folder>\n\
            <Folder>\n\
                <styleUrl>hideChildren</styleUrl>\n\
                <name>Extrude</name>\n\
                <visibility>0</visibility>\n\
                <open>0</open>\n\
                <Placemark>\n\
                    %s\n\
                </Placemark>\n\
            </Folder>\n\
        </Folder>\n\
		<Folder>\n\
		   <name>Task</name>\n\
		   <visibility>1</visibility>\n\
           %s\n\
           %s\n\
           %s\n\
           %s\n\
		</Folder>\n\
	</Folder>\n\
</Document>",
                styles, this->name, height, speed, climb_rate, timestamp, shadow, shadow_extrude, open_distance ?: "", out_and_return ?: "", triangle ?: "", task ?: "");

        free(metadata);
        free(linestring);
        free(styles);
        //    free(height);
        //    free(climb_rate);
        //    free(timestamp);
        //    free(speed);
        free(shadow);
        free(shadow_extrude);
        fclose(fp);
        if (triangle) {
            free(triangle);
        }
        if (open_distance) {
            free(open_distance);
        }
        if (out_and_return) {
            free(out_and_return);
        }
        if (task) {
            free(task);
        }
    } else {
        printf("Failed to open file: %s", filename);
    }
}

char *colour_grad_16[] = {
    "ff0000", "ff3f00", "ff7f00", "ffbf00", "ffff00", "bfff00", "7fff00", "3fff00", "00ff00", "00ff3f", "00ff7f", "00ffbf", "00ffff", "00bfff", "007fff", "003fff",
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

    int16_t i;
    for (i = 0; i < 16; i++) {
        char *level = itos(i);
        buffer = vstrcat(buffer, "\n\
    <Style id=\"S",
                         level, "\">\n\
        <LineStyle>\n\
            <width>2</width>\n\
            <color>FF",
                         colour_grad_16[i], "</color>\n\
        </LineStyle>\n\
    </Style>",
                         NULL);
        free(level);
    }
    //$kml->set_animation_styles(1);
    // public function set_animation_styles() {
    //    for ($i = 0; $i < 10; $i++) {
    //        for ($j = 0; $j < 360; $j += 5) {
    //            $this->styles .= '<Style id="A' . $i . $j .
    //            '"><IconStyle><heading>' . $j .
    //            '</heading><Icon><href>http://' . host . '/img/Markers/' .
    //            _get::kml_colour($i) . '.gif' .
    //            '</href></Icon></IconStyle></Style>';
    //        }
    //    }
    //}
    return buffer;
}

char *get_colour_by_height(coordinate_set_t *set) {
    char *buffer = create_buffer("");
    int64_t min = set->min_ele;
    double delta = (set->max_ele - set->min_ele ?: 1) / 16;
    coordinate_t *last, *first, *current;
    first = current = set->first;
    int16_t last_level, current_level;
    last_level = floor((current->ele - min) / delta);
    while (current) {
        char *level = itos(last_level);
        current_level = floor((current->ele - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            free(linestring);
            free(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        free(linestring);
        free(level);
    }
    return buffer;
}

char *get_colour_by_climb_rate(coordinate_set_t *set) {
    char *buffer = create_buffer("");
    int64_t min = set->min_climb_rate;
    double delta = (set->max_climb_rate - set->min_climb_rate ?: 1) / 16;
    coordinate_t *last, *first, *current;
    first = current = set->first;
    int16_t last_level, current_level;
    last_level = floor((current->climb_rate - min) / delta);
    while (current) {
        char *level = itos(last_level);
        current_level = floor((current->climb_rate - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            free(linestring);
            free(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        free(linestring);
        free(level);
    }
    return buffer;
}

char *get_colour_by_speed(coordinate_set_t *set) {
    char *buffer = create_buffer("");
    int64_t min = 0;
    double delta = (set->max_speed ?: 1) / 16;
    coordinate_t *last, *first, *current;
    first = current = set->first;
    int16_t last_level, current_level;
    last_level = floor((current->speed - min) / delta);
    while (current) {
        current_level = floor((current->speed - min) / delta);
        if (current_level != last_level && current_level != 16) {
            char *level = itos(last_level);
            char *linestring = get_partial_linestring_earth(first, current->next, level);
            buffer = vstrcat(buffer, linestring, NULL);
            free(linestring);
            free(level);
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        char *level = itos(last_level);
        char *linestring = get_partial_linestring_earth(first, current, level);
        buffer = vstrcat(buffer, linestring, NULL);
        free(linestring);
        free(level);
    }
    return buffer;
}

char *format_timestamp(int year, int16_t month, int16_t day, int16_t ts) {
    char *buffer = calloc(20, sizeof(char));
    struct tm point_time = {.tm_year = year - 1990, .tm_mon = month - 1, .tm_mday = day, .tm_hour = floor(ts / 3600), .tm_min = floor((ts % 3600) / 60), .tm_sec = (ts % 60)};
    strftime(buffer, 20, "%Y-%m-%dT%T", &point_time);
    return buffer;
}

char *get_colour_by_time(coordinate_set_t *set) {
    char *buffer = create_buffer("");
    int64_t min = set->first->timestamp;
    double delta = (set->last->timestamp - min ?: 1) / 16;
    coordinate_t *current = set->first;
    int16_t current_level;
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
    <styleUrl>#S",
                             level, "</styleUrl>\n\
    <TimeSpan>\n\
        <begin>",
                             point_date, "Z</begin>\n\
        <end>",
                             next_point_date, "Z</end>\n\
    </TimeSpan>\n\
    <LineString>\n\
        <altitudeMode>absolute</altitudeMode>\n\
        <coordinates>\n",
                             point, " ", next_point, "\n\
        </coordinates>\n\
    </LineString>\n\
</Placemark>\n",
                             NULL);
            free(point);
            free(level);
            free(point_date);
            current = current->next;
            point = next_point;
            point_date = next_point_date;
        }
    };
    return buffer;
}
