#include "../main.h"
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "formatter_kml_split.h"

void formatter_kml_split_init(formatter_t *this, coordinate_set_t *set) {
    this->set = set;
}

char *get_linestring_subset(coordinate_subset_t *this) {
    char *buffer = create_buffer("<LineString>\n\
    <extrude>0</extrude>\n\
    <altitudeMode>absolute</altitudeMode>\n\
    <coordinates>");
    coordinate_t *coordinate = this->first;
    int16_t i = 0;
    while (coordinate && coordinate != this->last) {
        char *kml_coordinate = coordinate_to_kml(coordinate);
        buffer = vstrcat(buffer, kml_coordinate, NULL);
        if (i++ == 5) {
            i = 0;
            buffer = vstrcat(buffer, "\n\t\t\t\t", NULL);
        }
        coordinate = coordinate->next;
    }
    return vstrcat(buffer, "\n</coordinates></LineString>", "", NULL);
}

char *kml_colour(int i) {
    switch (i % 9) {
        case 0:
            return "0000FF"; // Red
        case 1:
            return "FF0000"; // Blue
        case 2:
            return "008000"; // Dark Green
        case 3:
            return "00FF00"; // Green
        case 4:
            return "008CFF"; // Orange
        case 5:
            return "13458B"; // Brown
        case 6:
            return "B48246"; // Light Blue
        case 7:
            return "9314FF"; // Pink
        case 8:
            return "800080"; // Purple
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

char *formatter_kml_split_output(formatter_split_object *this) {
    char *output = create_buffer("");
    output = vstrcat(output, "<?xml version='1.0' encoding='UTF-8'?>\n\
<Document>",
                     NULL);
    coordinate_subset_t *subset = this->set->first_subset;
    int16_t i = 0;
    while (subset) {
        char *linestring = get_linestring_subset(subset);
        output = vstrcat(output, "\
    <Placemark>\n\
        <Style>\n\
            <LineStyle>\n\
                <color>ff",
                         kml_colour(i++), "</color>\n\
                <width>2</width>\n\
            </LineStyle>\n\
        </Style>\n\
        ",
                         linestring, "\n\
    </Placemark>\n",
                         NULL);
        free(linestring);
        subset = subset->next;
    }

    output = vstrcat(output, "</Document>", NULL);
}