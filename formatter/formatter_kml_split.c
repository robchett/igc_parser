#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "kml.h"
#include "formatter_kml_split.h"

void formatter_kml_split_init(formatter_split_t *obj, coordinate_set_t *set) {
    obj->set = set;
}

mxml_node_t *get_linestring_subset(coordinate_subset_t *obj) {
    coordinate_t *coordinate = obj->first;
    int16_t i = 0;

    char *buffer = create_buffer("");
    while (coordinate && coordinate != obj->last) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        coordinate = coordinate->next;
        free(kml_coordinate);
    }

    mxml_node_t *root, *folder1;

    root = mxmlNewElement(MXML_NO_PARENT, "LineString");
        new_text_node(root, "extrude", "0");
        new_text_node(root, "altitudeMode", "absolute");
        new_text_node(root, "coordinates", buffer);

    free(buffer);

    return root;
}

char *kml_colour(int i) {
    switch (i % 9) {
        case 0:
            return "FF0000FF"; // Red
        case 1:
            return "FFFF0000"; // Blue
        case 2:
            return "FF008000"; // Dark Green
        case 3:
            return "FF00FF00"; // Green
        case 4:
            return "FF008CFF"; // Orange
        case 5:
            return "FF13458B"; // Brown
        case 6:
            return "FFB48246"; // Light Blue
        case 7:
            return "FF9314FF"; // Pink
        case 8:
            return "FF800080"; // Purple
    }
}

char *heat_colour(int i) {
    switch (i % 17) {
        case 0:
            return "FF0000";
        case 1:
            return "EF000F";
        case 2:
            return "DF001F";
        case 3:
            return "CF002F";
        case 4:
            return "BF003F";
        case 5:
            return "AF004F";
        case 6:
            return "9F005F";
        case 7:
            return "8F006F";
        case 8:
            return "7F007F";
        case 9:
            return "6F008F";
        case 10:
            return "5F009F";
        case 11:
            return "4F00AF";
        case 12:
            return "3F00BF";
        case 13:
            return "2F00CF";
        case 14:
            return "1F00DF";
        case 15:
            return "0F00EF";
        case 16:
            return "0000FF";
    }
}

char *formatter_kml_split_output(formatter_split_t *obj, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp) {

        mxml_node_t *xml, *folder, *folder2, *folder3, *folder4, *folder5, *folder6;

        xml = mxmlNewXML("1.0");
        folder = mxmlNewElement(xml, "Document");

        coordinate_subset_t *subset = obj->set->first_subset;
        int16_t i = 0;
        while (subset) {
            folder2 = mxmlNewElement(folder, "Placemark");
                folder3 = mxmlNewElement(folder2, "Style");
                    folder4 = mxmlNewElement(folder3, "LineStyle");
                        new_text_node(folder6, "color", kml_colour(i++));
                        new_text_node(folder6, "width", "2");
                mxmlAdd(folder2, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, get_linestring_subset(subset));

            subset = subset->next;
        }

        mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
        fclose(fp);
    } else {
        printf("Failed to open file: %s", filename);
    }
}