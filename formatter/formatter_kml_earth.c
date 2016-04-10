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

void formatter_kml_earth_init(formatter_t *obj, coordinate_set_t *set, char *name, task_t *task_od, task_t *task_or, task_t *task_tr, task_t *task_ft, task_t *task) {
    obj->set = set;
    obj->open_distance = task_od;
    obj->out_and_return = task_or;
    obj->triangle = task_tr;
    obj->flat_triangle = task_ft;
    obj->task = task;
    obj->name = name;
}

void get_kml_styles_earth(mxml_node_t *root);
void get_colour_by_height(coordinate_set_t *set, mxml_node_t *root);
void get_colour_by_climb_rate(coordinate_set_t *set, mxml_node_t *root);
void get_colour_by_speed(coordinate_set_t *set, mxml_node_t *root);
void get_colour_by_time(coordinate_set_t *set, mxml_node_t *root);

mxml_node_t *get_meta_data_earth(formatter_t *obj) {
    coordinate_t *coordinate = obj->set->first;
    int16_t i = 0;
    char *buffer = create_buffer("");
    while (coordinate) {
        char *timestamp = itos(coordinate->timestamp);
        buffer = vstrcat(buffer, timestamp, " ", NULL);
        free(timestamp);
        coordinate = coordinate->next;
    }

    mxml_node_t *root, *folder1;

    root = mxmlNewElement(MXML_NO_PARENT, "Metadata");
        new_text_node(root, "SecondsFromTimeOfFirstPoint", buffer);

    return root;
}

mxml_node_t *get_linestring_earth(formatter_t *obj, char *style, char *altitude_mode, char *extrude) {
    char *coordinates = calloc((200 + (30 * obj->set->length)), sizeof(char));
    coordinate_t *coordinate = obj->set->first;
    int16_t i = 0;
    while (coordinate) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        strcat(coordinates, kml_coordinate);
        free(kml_coordinate);
        coordinate = coordinate->next;
    }

    mxml_node_t *root, *folder1;

    root = mxmlNewElement(MXML_NO_PARENT, "LineString");
        if (style) new_text_node(root, "styleUrl", style);
        new_text_node(root, "extrude", extrude);
        new_text_node(root, "altitudeMode", altitude_mode);
        new_text_node(root, "coordinates", coordinates);

    return root;
}

mxml_node_t *get_partial_linestring_earth(coordinate_t *coordinate, coordinate_t *last, size_t style) {
    char *buffer = create_buffer("");
    int16_t i = 0;
    while (coordinate != last) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        free(kml_coordinate);
        coordinate = coordinate->next;
    }

    mxml_node_t *root, *folder1;

    char level[5];
    sprintf(level, "#S%d", style);

    root = mxmlNewElement(MXML_NO_PARENT, "Placemark");
        if (style) new_text_node(root, "styleUrl", level);
        folder1 = mxmlNewElement(root, "LineString");
            new_text_node(folder1, "extrude", "0");
            new_text_node(folder1, "altitudeMode", "absolute");
            new_text_node(folder1, "coordinates", buffer);

    free(buffer);

    return root;
}

char *format_task_point_earth(coordinate_t *coordinate, int16_t index, coordinate_t *prev, double *total_distance) {
    double distance = 0;
    if (prev && coordinate) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = convert_latlng_to_gridref(coordinate->lat, coordinate->lng);
    char *buffer = NEW(char, 60);
    sprintf(buffer, "%-2d   %-8.5f   %-9.5f   %-s   %-5.2f      %-5.2f", index, coordinate->lat, coordinate->lng, gridref, distance, *total_distance);
    free(gridref);
    return buffer;
}

mxml_node_t *get_task_generic_earth(task_t *task, char *title, char *colour) {
    double distance = 0;
    char *info = create_buffer("TP   Latitude   Longitude   OS Gridref   Distance   Total\n");
    char *coordinates = create_buffer("");
    coordinate_t *prev = NULL;
    int16_t i;
    for (i = 0; i < task->size; i++) {
        char *line = format_task_point_earth(task->coordinate[i], i + 1, prev, &distance);
        info = vstrcat(info, line, "\n", NULL);
        prev = task->coordinate[i];
        char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
        coordinates = vstrcat(coordinates, kml_coordinate, NULL);
        free(kml_coordinate);
        free(line);
    }
    info = vstrcat(info, "Duration:  ", task_get_duration(task) ,"</pre>", NULL);


    mxml_node_t *root, *folder1, *folder2, *folder3;

    root = mxmlNewElement(MXML_NO_PARENT, "Folder");
        new_text_node(root, "name", title);
        new_text_node(root, "visibility", "1");
        new_text_node(root, "styleUrl", colour);
        folder1 = mxmlNewElement(root, "Placemark");
            new_text_node(folder1, "visibility", "1");
            new_text_node(folder1, "name", title);
            new_cdata_node(folder1, "description", info);
            folder2 = mxmlNewElement(folder1, "Style");
                folder3 = mxmlNewElement(folder2, "LineStyle");
                    new_text_node(folder3, "color", colour);
                    new_text_node(folder3, "width", "2");
            folder2 = mxmlNewElement(folder1, "LineString");
                new_text_node(folder2, "coordinates", coordinates);

    free(info);
    free(coordinates);

    return root;
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

mxml_node_t *get_defined_task_earth(task_t *task) {
    double distance = 0;

    char *kml_coordinates = create_buffer("");

    mxml_node_t *root, *folder1, *folder2, *folder3, *folder4;

    root = mxmlNewElement(MXML_NO_PARENT, "Folder");
    new_text_node(root, "name", "Task");

    int16_t i;
    for (i = 0; i < task->size; i++) {
        char *coordinates = get_circle_coordinates_earth(task->coordinate[i], 400);
        char *kml_coordinate = coordinate_to_kml(task->coordinate[i]);
        kml_coordinates = vstrcat(kml_coordinates, kml_coordinate, NULL);
        free(kml_coordinate);
        folder1 = mxmlNewElement(root, "Placemark");
            folder2 = mxmlNewElement(folder1, "Style");
                folder3 = mxmlNewElement(folder2, "LineStyle");
                    new_text_node(folder3, "color", "FF9900cc");
                    new_text_node(folder3, "width", "2");
                folder3 = mxmlNewElement(folder2, "PolyStyle");
                    new_text_node(folder3, "color", "999900cc");
                    new_text_node(folder3, "fill", "1");
                    new_text_node(folder3, "outline", "0");
            folder2 = mxmlNewElement(folder1, "Polygon");
                new_text_node(folder2, "tessellate", "1");
                folder3 = mxmlNewElement(folder2, "outerBoundaryIs");
                    folder4 = mxmlNewElement(folder3, "LinearRing");
                        new_text_node(folder4, "coordinates", coordinates);
        free(coordinates);
    }


    folder1 = mxmlNewElement(root, "Placemark");
        folder2 = mxmlNewElement(folder1, "Style");
            folder3 = mxmlNewElement(folder2, "LineStyle");
                new_text_node(folder3, "color", "FF9900CC");
                new_text_node(folder3, "width", "2");
        folder2 = mxmlNewElement(folder1, "LineString");
            new_text_node(folder2, "altitudeMode", "clampToGround");
            new_text_node(folder2, "coordinates", kml_coordinates);

    free(kml_coordinates);
    return root;
}

mxml_node_t *get_task_od_earth(formatter_t *obj) {
    return get_task_generic_earth(obj->open_distance, "Open Distance", "FF00D7FF");
}

mxml_node_t *get_task_or_earth(formatter_t *obj) {
    return get_task_generic_earth(obj->out_and_return, "Out and Return", "FF00FF00");
}

mxml_node_t *get_task_tr_earth(formatter_t *obj) {
    return get_task_generic_earth(obj->triangle, "FAI Triangle", "FF0000FF");
}

mxml_node_t *get_task_ft_earth(formatter_t *obj) {
    return get_task_generic_earth(obj->flat_triangle, "Flat Triangle", "FFFF0066");
}

char *formatter_kml_earth_output(formatter_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {

        mxml_node_t *xml, *folder1, *folder2, *folder3, *folder4, *folder5, *folder6, *folder7;

        xml = mxmlNewXML("1.0");
            folder1 = mxmlNewElement(xml, "Document");
                new_text_node(folder1, "open", "1");
                get_kml_styles_earth(folder1);
                folder2 = mxmlNewElement(folder1, "Folder");
                    new_text_node(folder2, "name", obj->name);
                    new_text_node(folder2, "visibility", "1");
                    new_text_node(folder2, "open", "1");
                    folder3 = mxmlNewElement(folder2, "Folder");
                        new_text_node(folder3, "name", "Main Track");
                        new_text_node(folder3, "visibility", "1");
                        new_text_node(folder3, "open", "1");
                        new_text_node(folder3, "styleUrl", "#radioFolder");
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "default");
                            new_text_node(folder4, "visibility", "1");
                            new_text_node(folder4, "open", "0");
                            folder5 = mxmlNewElement(folder4, "Placemark");
                                folder6 = mxmlNewElement(folder5, "Style");
                                    folder7 = mxmlNewElement(folder6, "LineStyle");
                                        new_text_node(folder7, "color", "ffff0000");
                                        new_text_node(folder7, "width", "2");
                                mxmlAdd(folder5, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_meta_data_earth(obj));
                                mxmlAdd(folder5, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_linestring_earth(obj, "", "absolute", "0"));
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Colour by height");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            get_colour_by_height(obj->set, folder4);
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Colour by ground speed");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            get_colour_by_speed(obj->set, folder4);
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Colour by climb");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            get_colour_by_climb_rate(obj->set, folder4);
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Colour by time");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            get_colour_by_time(obj->set, folder4);
                    folder3 = mxmlNewElement(folder2, "Folder");
                        new_text_node(folder3, "name", "Shadow");
                        new_text_node(folder3, "visibility", "1");
                        new_text_node(folder3, "open", "1");
                        new_text_node(folder3, "styleUrl", "#radioFolder");
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "None");
                            new_text_node(folder4, "visibility", "1");
                            new_text_node(folder4, "open", "0");
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Shadow");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            folder5 = mxmlNewElement(folder4, "Placemark");
                                mxmlAdd(folder5, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_linestring_earth(obj, "#shadow", "clampToGround", "0"));
                        folder4 = mxmlNewElement(folder3, "Folder");
                            new_text_node(folder4, "styleUrl", "#hideChildren");
                            new_text_node(folder4, "name", "Extrude");
                            new_text_node(folder4, "visibility", "0");
                            new_text_node(folder4, "open", "0");
                            folder5 = mxmlNewElement(folder4, "Placemark");
                                mxmlAdd(folder5, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_linestring_earth(obj, "#shadow", "absolute", "1"));
                    folder3 = mxmlNewElement(folder2, "Folder");
                        new_text_node(folder3, "name", "Task");
                        new_text_node(folder3, "visibility", "1");
                        if (obj->open_distance) mxmlAdd(folder3, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_task_od_earth(obj));
                        if (obj->out_and_return) mxmlAdd(folder3, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_task_or_earth(obj));
                        if (obj->triangle) mxmlAdd(folder3, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_task_tr_earth(obj));
                        if (obj->flat_triangle) mxmlAdd(folder3, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_task_ft_earth(obj));
                        if (obj->task) mxmlAdd(folder3, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_defined_task_earth(obj->task));

        mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
        fclose(fp);
    } else {
        printf("Failed to open file: %s", filename);
    }
}

char *colour_grad_16[] = {
    "ff0000", "ff3f00", "ff7f00", "ffbf00", "ffff00", "bfff00", "7fff00", "3fff00", "00ff00", "00ff3f", "00ff7f", "00ffbf", "00ffff", "00bfff", "007fff", "003fff",
};

void get_kml_styles_earth(mxml_node_t *root) {

    mxml_node_t *folder1, *folder2, *folder3, *folder4, *folder5, *folder6;

    folder1 = mxmlNewElement(root, "Style");
        mxmlElementSetAttr(folder1, "id", "hideChildren");
        folder2 = mxmlNewElement(folder1, "ListStyle");
            new_text_node(folder2, "listItemType", "checkHideChildren");
    folder1 = mxmlNewElement(root, "Style");
        mxmlElementSetAttr(folder1, "id", "radioFolder");
        folder2 = mxmlNewElement(folder1, "ListStyle");
            new_text_node(folder2, "listItemType", "radioFolder");
    folder1 = mxmlNewElement(root, "Style");
        mxmlElementSetAttr(folder1, "id", "shadow");
        folder2 = mxmlNewElement(folder1, "LineStyle");
            new_text_node(folder2, "color", "AA000000");
            new_text_node(folder2, "width", "1");
        folder2 = mxmlNewElement(folder1, "PolyStyle");
            new_text_node(folder2, "color", "55AAAAAA");

    int16_t i;
    for (i = 0; i < 16; i++) {
        char style[4];
        sprintf(style, "S%d", i);
        char color[9];
        sprintf(color, "ff%s", colour_grad_16[i]);
        folder1 = mxmlNewElement(root, "Style");
            mxmlElementSetAttr(folder1, "id", style);
            folder2 = mxmlNewElement(folder1, "LineStyle");
                new_text_node(folder2, "color", color);
                new_text_node(folder2, "width", "2");
    }
}

void get_colour_by_x(coordinate_set_t *set, mxml_node_t *root, double delta, int min, size_t (*func)(coordinate_t *, double, int)) {
    coordinate_t *last, *first, *current;
    first = current = set->first;
    int16_t last_level, current_level;
    last_level = (*func)(current, delta, min);
    while (current) {
        current_level = (*func)(current, delta, min);
        if (current_level != last_level && current_level != 16) {
            mxmlAdd(root, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_partial_linestring_earth(first, current->next, last_level));
            last_level = current_level;
            first = current;
        }
        current = current->next;
    };
    if (first) {
        mxmlAdd(root, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_partial_linestring_earth(first, current, last_level));
    }
}

void get_colour_by_height(coordinate_set_t *set, mxml_node_t *root) {
    double delta = (set->max_ele - set->min_ele ?: 1) / 16;
    get_colour_by_x(set, root, delta, set->min_ele, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->ele - min) / delta); } a; } ));
}

void get_colour_by_climb_rate(coordinate_set_t *set, mxml_node_t *root) {
    double delta = (set->max_climb_rate - set->min_climb_rate ?: 0) / 16;
    get_colour_by_x(set, root, delta, set->min_climb_rate ?: 0, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->climb_rate - min) / delta); } a; } ));
}

void get_colour_by_speed(coordinate_set_t *set, mxml_node_t *root) {
    double delta = (set->max_climb_rate) / 16;
    get_colour_by_x(set, root, delta, 0, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->speed - min) / delta); } a; } ));
}

void get_colour_by_time(coordinate_set_t *set, mxml_node_t *root) {
    double delta = (set->last->timestamp - set->first->timestamp) / 16;
    get_colour_by_x(set, root, delta, set->first->timestamp, ( { size_t a (coordinate_t *current, double delta, int min) { return  floor((current->timestamp - min) / delta); } a; } ));
}

char *format_timestamp(int year, int16_t month, int16_t day, int16_t ts) {
    char *buffer = calloc(20, sizeof(char));
    struct tm point_time = {.tm_year = year - 1990, .tm_mon = month - 1, .tm_mday = day, .tm_hour = floor(ts / 3600), .tm_min = floor((ts % 3600) / 60), .tm_sec = (ts % 60)};
    strftime(buffer, 20, "%Y-%m-%dT%T", &point_time);
    return buffer;
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
