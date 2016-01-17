#include "../main.h"
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
    char *buffer = create_buffer("<LineString>\n\
	\t\t\t\t<extrude>0</extrude>\n\
	\t\t\t\t<altitudeMode>absolute</altitudeMode>\n\
	\t\t\t\t<coordinates>\n\t\t\t\t\t\t");
    coordinate_t *coordinate = this->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        free(kml_coordinate);
        if (i++ == 5) {
            i = 0;
            buffer = vstrcat(buffer, "\n\t\t\t\t\t\t", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n\t\t\t\t\t</coordinates>\n\t\t\t\t</LineString>", "", NULL);
}

char *format_task_point(coordinate_t *coordinate, int16_t index, coordinate_t *prev, double *total_distance) {
    double distance = 0;
    if (prev) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = get_os_grid_ref(coordinate);
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
<Folder>\
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

char *formatter_kml_output(formatter_t *this) {
    char *year = itos(this->set->year);
    char *month = fitos(this->set->month, "%02d");
    char *day = fitos(this->set->day, "%02d");
    char *min_ele = itos(this->set->min_ele);
    char *max_ele = itos(this->set->max_ele);

    // TASKS
    char *tasks = create_buffer("");
    char *tasks_info = create_buffer("");
    if (this->open_distance) {
        char *od_d = fdtos(get_task_distance(this->open_distance), "%.2f");
        char *od_t = itos(get_task_time(this->open_distance));
        char *open_distance = get_task_od(this);
        tasks = vstrcat(tasks, open_distance, NULL);
        tasks_info = vstrcat(tasks_info, "\t\t\tOD Score / Time      ", od_d, " / ", od_t, "s\n", NULL);
        free(od_d);
        free(od_t);
        free(open_distance);
    }
    if (this->open_distance) {
        char *or_d = fdtos(get_task_distance(this->out_and_return), "%.2f");
        char *or_t = itos(get_task_time(this->out_and_return));
        char *out_and_return = get_task_or(this);
        tasks = vstrcat(tasks, out_and_return, NULL);
        tasks_info = vstrcat(tasks_info, "\t\t\tOR Score / Time      ", or_d, " / ", or_t, "s\n", NULL);
        free(or_d);
        free(or_t);
        free(out_and_return);
    }
    if (this->triangle) {
        char *tr_d = fdtos(get_task_distance(this->triangle), "%.2f");
        char *tr_t = itos(get_task_time(this->triangle));
        char *triangle = get_task_tr(this);
        tasks = vstrcat(tasks, triangle, NULL);
        tasks_info = vstrcat(tasks_info, "\t\t\tTR Score / Time      ", tr_d, " / ", tr_t, "s\n", NULL);
        free(tr_d);
        free(tr_t);
        free(triangle);
    }
    if (this->task) {
        char *task = get_defined_task(this->task);
        tasks = vstrcat(tasks, task, NULL);
        free(task);
    }

    char *metadata = get_meta_data(this);
    char *linestring = get_linestring(this);

    char *output = create_buffer("");
    output = vstrcat(output, "<?xml version='1.0' encoding='UTF-8'?>\n\
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
		<name>",
                     this->name, "</name>\n\
		<visibility>1</visibility>\n\
		<description><![CDATA[<pre>\n\
			Flight statistics\n\
			Flight #             ",
                     this->name, "\n\
			Pilot                \n\
			Club                 \n\
			Glider               \n\
			Date                 ",
                     year, "-", month, "-", day, "\n\
			Start/finish         \n\
			Duration             \n\
			Max./min. height     ",
                     max_ele, " / ", min_ele, " m\n", tasks_info, "\n\
		</pre>]]>\n\
		</description>\n\
		<Folder>\n\
			<name>",
                     this->name, "</name>\n\
			<visibility>1</visibility>\n\
			<open>1</open>\n\
			<Placemark>\n\
				<Style>\n\
					<LineStyle>\n\
						<color>ffff0000</color>\n\
						<width>2</width>\n\
					</LineStyle>\n\
				</Style>\n\
				",
                     metadata, "\n\t\t\t\t", linestring, "\n\
			</Placemark>\n\
		</Folder>\n\
		<Folder>\n\
		<name>Task</name>\n\
		<visibility>1</visibility>\n\
		",
                     tasks, "\n\
		</Folder>\n\
	</Folder>\n\
</Document>",
                     NULL);

    free(year);
    free(month);
    free(day);
    free(max_ele);
    free(min_ele);
    free(metadata);
    free(linestring);
    free(tasks);
    free(tasks_info);
    return output;
}
