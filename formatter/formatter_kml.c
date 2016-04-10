#include "../main.h"
#include <string.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "../igc_parser.h"
#include "../task.h"
#include "kml.h"
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

mxml_node_t *get_meta_data(formatter_t *obj) {
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

mxml_node_t *get_linestring(formatter_t *obj) {
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
        new_text_node(root, "extrude", "0");
        new_text_node(root, "altitudeMode", "absolute");
        new_text_node(root, "coordinates", coordinates);

    return root;
}

char *format_task_point(coordinate_t *coordinate, int16_t index, coordinate_t *prev, double *total_distance) {
    double distance = 0;
    if (prev) {
        distance = get_distance_precise(coordinate, prev);
        *total_distance += distance;
    }
    char *gridref = convert_latlng_to_gridref(coordinate->lat, coordinate->lng);
    char *buffer = NEW(char, 60);
    sprintf(buffer, "%-2d   %-8.5f   %-9.5f   %-s     %-5.2f      %-5.2f", index, coordinate->lat, coordinate->lng, gridref, distance, *total_distance);
    free(gridref);
    return buffer;
}

mxml_node_t *get_task_generic(task_t *task, char *title, char *colour) {
    double distance = 0;
    char *info = create_buffer("<pre>TP   Latitude   Longitude   OS Gridref   Distance   Total\n");
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

    info = vstrcat(info, "Duration:  ", task_get_duration(task) ,"</pre>", NULL);

    mxml_node_t *root, *folder1, *folder2, *folder3;
    root = mxmlNewElement(MXML_NO_PARENT, "Folder");
        new_text_node(root, "name", title);
        new_text_node(root, "visibility", "1");
        new_text_node(root, "styleUrl", "#hideChildren");
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
        buffer = vstrcat(buffer, lng_string, ",", lat_string, ",1000 ", NULL);
        free(lng_string);
        free(lat_string);
    }
    return buffer;
}

mxml_node_t *get_defined_task(task_t *task) {
    double distance = 0;

    char *kml_coordinates = create_buffer("");

    mxml_node_t *root, *folder1, *folder2, *folder3, *folder4;

    root = mxmlNewElement(MXML_NO_PARENT, "Folder");
    new_text_node(root, "name", "Task");

    int16_t i;
    for (i = 0; i < task->size; i++) {
        char *coordinates = get_circle_coordinates(task->coordinate[i], 400);
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

mxml_node_t *get_task_od(formatter_t *obj) {
    return get_task_generic(obj->open_distance, "Open Distance", "FF00D7FF");
}

mxml_node_t *get_task_or(formatter_t *obj) {
    return get_task_generic(obj->out_and_return, "Out and Return", "FF00FF00");
}

mxml_node_t *get_task_tr(formatter_t *obj) {
    return get_task_generic(obj->triangle, "FAI Triangle", "FF0000FF");
}

mxml_node_t *get_task_ft(formatter_t *obj) {
    return get_task_generic(obj->flat_triangle, "Flat Triangle", "FFFF0066");
}

char *formatter_kml_output(formatter_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        char od_results[100];
        char or_results[100];
        char tr_results[100];
        char ft_results[100];
        od_results[0] = or_results[0] = tr_results[0] = ft_results[0] = '\0';
        mxml_node_t *open_distance = NULL;
        mxml_node_t *out_and_return = NULL;
        mxml_node_t *triangle = NULL;
        mxml_node_t *flat_triangle = NULL;
        mxml_node_t *task = NULL;
        if (obj->open_distance) {
            open_distance = get_task_od(obj);
            sprintf(od_results, "OD Score / Time\t\t %3.2f / %s", get_task_distance(obj->open_distance), task_get_duration(obj->open_distance));
        }
        if (obj->open_distance) {
            out_and_return = get_task_or(obj);
            sprintf(or_results, "OR Score / Time\t\t %3.2f / %s", get_task_distance(obj->out_and_return), task_get_duration(obj->out_and_return));
        }
        if (obj->triangle) {
            triangle = get_task_tr(obj);
            sprintf(tr_results, "TR Score / Time\t\t %3.2f / %s", get_task_distance(obj->triangle), task_get_duration(obj->triangle));
        }
        if (obj->flat_triangle) {
            flat_triangle = get_task_ft(obj);
            sprintf(ft_results, "FT Score / Time %3.2f / %d", get_task_distance(obj->flat_triangle), get_task_time(obj->flat_triangle));
        }
        if (obj->task) {
            task = get_defined_task(obj->task);
        }

        char description[2048];
        sprintf(description, "<pre>\n\
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
            %s                           \n\
		</pre>", obj->name, obj->set->day, obj->set->month, obj->set->year, obj->set->max_ele, obj->set->min_ele, od_results, or_results, tr_results, ft_results);

        mxml_node_t *xml, *folder, *folder2, *folder3, *folder4, *folder5, *folder6;

        xml = mxmlNewXML("1.0");
        folder = mxmlNewElement(xml, "Document");
            new_text_node(folder, "open", "1");
            folder2 = mxmlNewElement(folder, "Style");
            mxmlElementSetAttr(folder2, "id", "Shadow");
                folder3 = mxmlNewElement(folder2, "LineStyle");
                    new_text_node(folder3, "color", "AA000000");
                    new_text_node(folder3, "width", "1");
                folder3 = mxmlNewElement(folder2, "PolyStyle");
                    new_text_node(folder3, "color", "55AAAAAA");
            folder2 = mxmlNewElement(folder, "Style");
            mxmlElementSetAttr(folder2, "id", "S1");
                folder3 = mxmlNewElement(folder2, "LineStyle");
                    new_text_node(folder3, "color", "FF0000");
                    new_text_node(folder3, "width", "2");
            folder2 = mxmlNewElement(folder, "Folder");
                new_text_node(folder2, "name", obj->name);
                new_text_node(folder2, "visibility", "1");
                new_text_node(folder2, "open", "1");
                new_cdata_node(folder2, "description", description);
                folder4 = mxmlNewElement(folder2, "Placemark");
                    folder5 = mxmlNewElement(folder4, "Style");
                        folder6 = mxmlNewElement(folder5, "LineStyle");
                            new_text_node(folder6, "color", "ffff0000");
                            new_text_node(folder6, "width", "2");
                    mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_meta_data(obj));
                    mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_linestring(obj));
                folder4 = mxmlNewElement(folder2, "Folder");
                    new_text_node(folder4, "name", "Task");
                    new_text_node(folder4, "visibility", "1");
                    if (open_distance) mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, open_distance);
                    if (out_and_return) mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, out_and_return);
                    if (triangle) mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, triangle);
                    if (flat_triangle) mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, flat_triangle);
                    if (task) mxmlAdd(folder4, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, task);

        mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
        fclose(fp);
    } else {
        printf("Failed to open file: %s", filename);
    }
}
