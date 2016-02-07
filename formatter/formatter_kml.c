#include "../main.h"
#include <string.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include "formatter_kml.h"

void formatter_kml_init(formatter_t *this, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task) {
    this->set = set;
    this->open_distance = task_od;
    this->out_and_return = task_or;
    this->triangle = task_tr;
    this->task = task;
    this->name = name;
}

char *get_meta_data(formatter_t *this) {
    char *buffer = create_buffer("<Metadata>\n\t\t\t\t\t<SecondsFromTimeOfFirstPoint>\n\t\t\t\t\t\t");
    coordinate_t *coordinate = this->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *timestamp = itos(coordinate->timestamp);
        buffer = vstrcat(buffer, timestamp, " ", NULL);
        free(timestamp);
        if (i++ == 15) {
            i = 0;
            buffer = vstrcat(buffer, "\n\t\t\t\t\t\t", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n\t\t\t\t\t</SecondsFromTimeOfFirstPoint>\n\t\t\t\t</Metadata>", NULL);
}

char *get_linestring(formatter_t *this) {
    char *coordinates = calloc((200 + (30 * this->set->length)), sizeof(char));
    coordinate_t *coordinate = this->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        strcat(coordinates, kml_coordinate);
        free(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            strcat(coordinates, "\n\t\t\t\t\t\t");
        }
        coordinate = coordinate->next;
    }
    char *buffer = malloc(sizeof(char) * (200 + (30 * this->set->length)));
    sprintf(buffer, "\
            <LineString>\n\
                <extrude>0</extrude>\n\
	            <altitudeMode>absolute</altitudeMode>\n\
	            <coordinates>\n\
                    %s\n\
                </coordinates>\n\
            </LineString>",
            coordinates);
    free(coordinates);
    return buffer;
}

char *format_task_point(coordinate_t *coordinate, int16_t index, coordinate_t *prev, double *total_distance) {
    double distance = 0;
    if (prev) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = convert_latlng_to_gridref(coordinate);
    char *buffer = malloc(sizeof(char) * 60);
    sprintf(buffer, "%-2d   %-8.5f   %-9.5f   %-s     %-5.2f      %-5.2f", index, coordinate->lat, coordinate->lng, gridref, distance, *total_distance);
    free(gridref);
    return buffer;
}

char *get_task_generic(task_t *task, char *title, char *colour) {
    double distance = 0;
    char *info = create_buffer("");
    char *coordinates = create_buffer("");
    coordinate_t *prev = NULL;
    int16_t i;
    for (i = 0; i < task->size; i++) {
        if (task->coordinate[i]) {
            char *line = format_task_point(task->coordinate[i], i + 1, prev, &distance);
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
                     info, "\
                                         Duration:  01:56:00\n\
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
                    <coordinates>\n\
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

char *get_circle_coordinates(coordinate_t *coordinate, int16_t radius) {
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

char *get_defined_task(task_t *task) {
    double distance = 0;
    char *info = create_buffer("\
<Folder>\n\
    <name>Task</name>");
    char *kml_coordinates = create_buffer("");
    int16_t i;
    for (i = 0; i < task->size; i++) {
        char *coordinates = get_circle_coordinates(task->coordinate[i], 400);
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

char *get_task_od(formatter_t *this) {
    char *buffer = get_task_generic(this->open_distance, "Open Distance", "00D7FF");
    return buffer;
}

char *get_task_or(formatter_t *this) {
    char *buffer = get_task_generic(this->out_and_return, "Out and Return", "00FF00");
    return buffer;
}

char *get_task_tr(formatter_t *this) {
    char *buffer = get_task_generic(this->triangle, "FAI Triangle", "0000FF");
    return buffer;
}

char *get_task_ft(formatter_t *this) {
    char *buffer = get_task_generic(this->triangle, "Flat Triangle", "FF0066");
    return buffer;
}

char *formatter_kml_output(formatter_t *this, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        // TASKS
        char od_results[100];
        char or_results[100];
        char tr_results[100];
        char *open_distance = NULL;
        char *out_and_return = NULL;
        char *triangle = NULL;
        char *task = NULL;
        if (this->open_distance) {
            open_distance = get_task_od(this);
            sprintf(od_results, "OD Score / Time %3.2f / %d", get_task_distance(this->open_distance), get_task_time(this->open_distance));
        }
        if (this->open_distance) {
            out_and_return = get_task_or(this);
            sprintf(or_results, "OR Score / Time %3.2f / %d", get_task_distance(this->out_and_return), get_task_time(this->out_and_return));
        }
        if (this->triangle) {
            triangle = get_task_tr(this);
            sprintf(tr_results, "TR Score / Time %3.2f / %d", get_task_distance(this->triangle), get_task_time(this->triangle));
        }
        if (this->task) {
            task = get_defined_task(this->task);
        }

        char *metadata = get_meta_data(this);
        char *linestring = get_linestring(this);

        fprintf(fp, "\
<?xml version='1.0' encoding='UTF-8'?>\n\
<Document>\n\
	<open>1</open>\n\
	<Style id=\"shadow\">\n\
		<LineStyle>\n\
			<color>AA000000</color>\n\
			<width>1</width>\n\
		</LineStyle>\n\
		<PolyStyle>\n\
			<color>55AAAAAA</color>\n\
		</PolyStyle>\n\
	</Style>\n\
	<Style id=\"S1\">\n\
		<LineStyle>\n\
			<color>FF0000</color>\n\
			<width>2</width>\n\
		</LineStyle>\n\
	</Style>\n\
	<Folder>\n\
		<name>%s</name>\n\
		<visibility>1</visibility>\n\
        <open>1</open>\n\
		<description><![CDATA[<pre>\n\
			Flight statistics\n\
			Flight: #%s                  \n\
			Pilot:                       \n\
			Club:                        \n\
			Glider:                      \n\
			Date: %02d-%02d-%04d         \n\
			Start/finish:                \n\
			Duration:                    \n\
			Max./min. height:    %d / %d \n\
            %s                           \n\
            %s                           \n\
            %s                           \n\
		</pre>]]></description>\n\
		<Placemark>\n\
			<Style>\n\
				<LineStyle>\n\
					<color>ffff0000</color>\n\
					<width>2</width>\n\
				</LineStyle>\n\
			</Style> \n\
               %s \n\
               %s \n\
		</Placemark>\n\
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
                this->name, this->name, this->set->day, this->set->month, this->set->year, this->set->max_ele, this->set->min_ele, od_results ?: "", or_results ?: "", tr_results ?: "", metadata, linestring, open_distance ?: "",
                out_and_return ?: "", triangle ?: "", task ?: "");

        free(metadata);
        free(linestring);

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
        fclose(fp);
    } else {
        printf("Failed to open file: %s", filename);
    }
}
