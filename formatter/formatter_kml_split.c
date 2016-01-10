#include <php.h>
#include "../string_manip.h"
#include "../coordinate.h"
#include "../coordinate_set.h"
#include "formatter_kml_split.h"

zend_object_handlers formatter_kml_split_object_handler;

void init_formatter_kml_split(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "formatter_kml_split", formatter_kml_split_methods);
    formatter_kml_split_ce = zend_register_internal_class(&ce TSRMLS_CC);
    formatter_kml_split_ce->create_object = create_formatter_kml_split_object;

    memcpy(&formatter_kml_split_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    formatter_kml_split_object_handler.free_obj = free_formatter_kml_split_object;
    formatter_kml_split_object_handler.offset = XtOffsetOf(formatter_split_object, std);
}

zend_object* create_formatter_kml_split_object(zend_class_entry *class_type TSRMLS_DC) {
    formatter_object* retval;

    retval = ecalloc(1, sizeof(formatter_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&retval->std, class_type TSRMLS_CC);
    object_properties_init(&retval->std, class_type);

    retval->std.handlers = &formatter_kml_split_object_handler;

    return &retval->std;
}

void free_formatter_kml_split_object(formatter_split_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    free(intern);
}

inline formatter_split_object* fetch_formatter_split_object(zval* obj) {
    return (formatter_split_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(formatter_split_object, std));
}

static zend_function_entry formatter_kml_split_methods[] = {
    PHP_ME(formatter_kml_split, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(formatter_kml_split, output, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};


PHP_METHOD(formatter_kml_split, __construct) {
    zval *coordinate_set_zval;
    formatter_split_object *intern = fetch_formatter_split_object(getThis() TSRMLS_CC);
    if (zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "O", &coordinate_set_zval, coordinate_set_ce ) == FAILURE) {
        return;
    }

    intern->set = (coordinate_set_object *)fetch_coordinate_set_object(coordinate_set_zval TSRMLS_CC);
}


char *get_linestring_subset(coordinate_subset *intern) {
    char *buffer = create_buffer("<LineString>\n\
    <extrude>0</extrude>\n\
    <altitudeMode>absolute</altitudeMode>\n\
    <coordinates>");
    coordinate_object *coordinate = intern->first;
    int16_t i = 0;
    while (coordinate && coordinate != intern->last) {
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

PHP_METHOD(formatter_kml_split, output) {
    formatter_split_object *intern = fetch_formatter_split_object(getThis() TSRMLS_CC);
    RETURN_STRING(formatter_kml_split_output(intern));
}

char *kml_colour(int i) {
    switch (i % 9) {
	    case 0: return "0000FF"; // Red
	    case 1: return "FF0000"; // Blue
	    case 2: return "008000"; // Dark Green
	    case 3: return "00FF00"; // Green
	    case 4: return "008CFF"; // Orange
	    case 5: return "13458B"; // Brown
	    case 6: return "B48246"; // Light Blue
	    case 7: return "9314FF"; // Pink
	    case 8: return "800080"; // Purple
    }
}

char *heat_colour(int i) {
    switch (i % 17) {
    case 0: return "FF0000";
    case 1: return "EF000F";
    case 2: return "DF001F";
    case 3: return "CF002F";
    case 4: return "BF003F";
    case 5: return "AF004F";
    case 6: return "9F005F";
    case 7: return "8F006F";
    case 8: return "7F007F";
    case 9: return "6F008F";
    case 10: return "5F009F";
    case 11: return "4F00AF";
    case 12: return "3F00BF";
    case 13: return "2F00CF";
    case 14: return "1F00DF";
    case 15: return "0F00EF";
    case 16: return "0000FF";
    }
}

char *formatter_kml_split_output(formatter_split_object *intern) {
    char *output = create_buffer("");
    output = vstrcat(output, "<?xml version='1.0' encoding='UTF-8'?>\n\
<Document>", NULL);
    coordinate_subset *subset = intern->set->first_subset;
    int16_t i = 0;
    while (subset) {
        char *linestring = get_linestring_subset(subset);
        output = vstrcat(output, "\
    <Placemark>\n\
        <Style>\n\
            <LineStyle>\n\
                <color>ff", kml_colour(i++) , "</color>\n\
                <width>2</width>\n\
            </LineStyle>\n\
        </Style>\n\
        ", linestring, "\n\
    </Placemark>\n", NULL);
        free(linestring);
        subset = subset->next;
    }

    output = vstrcat(output, "</Document>", NULL);
}