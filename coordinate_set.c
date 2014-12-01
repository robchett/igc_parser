#include <php.h>
#include <tgmath.h>
#include "coordinate.h"
#include "coordinate_set.h"

void init_coordinate_set(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "coordinate_set", coordinate_set_methods);
    coordinate_set_ce = zend_register_internal_class(&ce TSRMLS_CC);
    coordinate_set_ce->create_object = create_coordinate_set_object;

    memcpy(&coordinate_set_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    coordinate_set_handlers.clone_obj = coordinate_set_object_clone;
}


zend_object_value create_coordinate_set_object(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;

    // allocate the struct we're going to use
    coordinate_set_object *intern = emalloc(sizeof(coordinate_set_object));
    memset(intern, 0, sizeof(coordinate_set_object));

    // create a table for class properties
    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    // create a destructor for this struct
    retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) free_coordinate_set_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

void free_coordinate_set_object(coordinate_set_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    coordinate_subset *tmp, *subset = intern->first_subset;
    while (subset) {
        tmp = subset->next;
        free_subset(intern, subset);
        subset = tmp;
    }
    efree(intern);
}

static zend_object_value coordinate_set_object_clone(zval *object TSRMLS_DC) {
    coordinate_set_object *old_object = zend_object_store_get_object(object TSRMLS_CC);
    zend_object_value new_object_val = create_coordinate_set_object(Z_OBJCE_P(object) TSRMLS_CC);
    coordinate_set_object *new_object = zend_object_store_get_object_by_handle(new_object_val.handle TSRMLS_CC);

    zend_objects_clone_members(&new_object->std, new_object_val, &old_object->std, Z_OBJ_HANDLE_P(object) TSRMLS_CC);

    return new_object_val;
}

static zend_function_entry coordinate_set_methods[] = {
    PHP_ME(coordinate_set, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, first, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, last, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, count, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, parse_igc, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, date, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, trim, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, simplify, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, repair, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, set_graph_values, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, set_ranges, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, set_section, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, has_height_data, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, part_length, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, part_duration, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(coordinate_set, part_count, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD (coordinate_set, __construct) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    intern->length = 0;
    intern->first_subset = intern->last_subset = NULL;
    intern->first = intern->last = NULL;
    intern->subset_count = 0;
}


PHP_METHOD (coordinate_set, date) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    char *buffer = emalloc(sizeof(char) * 11);
    sprintf(buffer, "%04d-%02d-%02d", intern->year, intern->month, intern->day);
    RETURN_STRING(buffer, 0);
}

PHP_METHOD (coordinate_set, part_count) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(intern->subset_count);
}

PHP_METHOD (coordinate_set, set) {
    zval *coordinate;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &coordinate, coordinate_ce) != SUCCESS) {
        return;
    }

    Z_ADDREF_P(coordinate);

    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_object *coordinate_intern = zend_object_store_get_object(coordinate TSRMLS_CC);
    coordinate_intern->id = intern->length++;
    if (intern->last) {
        intern->last->next = coordinate_intern;
    }
    if (!intern->first) {
        intern->first = coordinate_intern;
    }
    coordinate_intern->prev = intern->last;
    intern->last = coordinate_intern;

    RETURN_BOOL(1);
}

void _clone_coordinate_object(coordinate_object *coordinate, coordinate_object *return_intern) {
    return_intern->lat = coordinate->lat;
    return_intern->lng = coordinate->lng;
    return_intern->ele = coordinate->ele;
    return_intern->timestamp = coordinate->timestamp;
}

PHP_METHOD (coordinate_set, first) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *ret;
    MAKE_STD_ZVAL(ret);
    object_init_ex(ret, coordinate_ce);
    coordinate_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
    if (intern->first) {
        _clone_coordinate_object(intern->first, return_intern);
        RETURN_ZVAL(ret, 1, 0);
    }
    RETURN_NULL();
}

PHP_METHOD (coordinate_set, last) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *ret;
    MAKE_STD_ZVAL(ret);
    object_init_ex(ret, coordinate_ce);
    coordinate_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
    if (intern->last) {
        _clone_coordinate_object(intern->last, return_intern);
        RETURN_ZVAL(ret, 1, 0);
    }
    RETURN_NULL();
}

PHP_METHOD (coordinate_set, get) {
    long offset;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &offset) != SUCCESS) {
        return;
    }

    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    if (offset < intern->length) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        object_init_ex(ret, coordinate_ce);
        coordinate_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
        coordinate_object *coordinate = intern->first;
        int i = 0;
        while (coordinate && ++i < offset) {
            coordinate = coordinate->next;
        }
        _clone_coordinate_object(coordinate, return_intern);
        RETURN_ZVAL(ret, 1, 0);
    }
    RETURN_NULL();
}

PHP_METHOD (coordinate_set, count) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(intern->length);
}

PHP_METHOD (coordinate_set, parse_igc) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    long string_length;
    char *string;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &string, &string_length) == FAILURE) {
        return;
    }

    RETURN_LONG(parse_igc(intern, string));
}

void free_subset(coordinate_set_object *parser, coordinate_subset *set) {
    coordinate_object *tmp, *current = set->first;
    while (set->length) {
        tmp = current->next;
        _free_coordinate_object(current);
        current = tmp;
    }
    if (set->prev) {
        set->prev->next = set->next;
    }
    if (set->next) {
        set->next->prev = set->prev;
    }
    if (set == parser->first_subset) {
        parser->first_subset = set->next;
    }
    if (set == parser->last_subset) {
        parser->last_subset = set->prev;
    }
    efree(set);
}


coordinate_subset *create_subset(coordinate_set_object *set, coordinate_object *coordinate) {
    coordinate_subset *subset = emalloc(sizeof(coordinate_subset));
    subset->length = 1;
    subset->first = coordinate;
    subset->last = coordinate;
    subset->next = NULL;
    subset->prev = set->last_subset;
    if (subset->prev) {
        subset->prev->next = subset;
    }
    set->last_subset = subset;
    if (!set->first_subset) {
        set->first_subset = subset;
    }
}


int parse_igc(coordinate_set_object *intern, char *string) {
    int b_records = 0;
    char *curLine = string;
    while (curLine) {
        char *nextLine = strchr(curLine, '\n');
        if (nextLine) *nextLine = '\0';
        if (is_h_record(curLine)) {
            parse_h_record(intern, curLine);
        } else if (is_b_record(curLine)) {
            coordinate_object *coordinate = parse_igc_coordiante(curLine);
            coordinate->id = intern->length++;
            coordinate->prev = intern->last;
            if (!intern->first) {
                create_subset(intern, coordinate);
                intern->first = coordinate;
                intern->subset_count ++;
            } else {
                intern->last->next = coordinate;
                if (!intern->last || coordinate->timestamp - intern->last->timestamp > 60) {
                    create_subset(intern, coordinate);
                    intern->subset_count ++;
                } else {
                    intern->last_subset->length++;
                }
            }
            intern->last = coordinate;
            intern->last_subset->last = coordinate;
            coordinate->coordinate_set = intern;
            coordinate->coordinate_subset = intern->last_subset;
        }
        if (nextLine) *nextLine = '\n';
        curLine = nextLine ? (nextLine + 1) : NULL;
    }
    return b_records;
}

int has_height_data(coordinate_set_object *coordinate_set) {
    coordinate_object *coordinate = coordinate_set->first;
    while (coordinate) {
        if (coordinate->ele) {
            return 1;
        }
        coordinate = coordinate->next;
    }
    return 0;
}

PHP_METHOD (coordinate_set, has_height_data) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_BOOL(has_height_data(intern));
}

PHP_METHOD (coordinate_set, repair) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    if (intern->first) {
        coordinate_object *current = intern->first->next;
        while (current) {
            if (current->ele == 0) {
                current->ele = current->prev->ele;
            }
            if (current->ele > current->prev->ele + 500) {
                current->ele = current->prev->ele;
            }
            current = current->next;
        }
        RETURN_BOOL(1);
    }
    RETURN_BOOL(0);
}

PHP_METHOD (coordinate_set, set_graph_values) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_BOOL(set_graph_values(intern));
}

PHP_METHOD (coordinate_set, part_length) {
    long offset;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &offset) != SUCCESS) {
        return;
    }
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_subset *set = intern->first_subset;
    int i = 0;
    while (++i < offset && set) {
        set = set->next;
    }
    if (set) {
        RETURN_LONG(set->length);
    }
    RETURN_NULL();
}

PHP_METHOD (coordinate_set, part_duration) {
    long offset;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &offset) != SUCCESS) {
        return;
    }
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_subset *set = intern->first_subset;
    int i = 0;
    while (++i < offset && set) {
        set = set->next;
    }
    if (set) {
        RETURN_LONG(set->last->timestamp - set->first->timestamp);
    }
    RETURN_NULL();
}

PHP_METHOD (coordinate_set, set_section) {
    long offset;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &offset) != SUCCESS) {
        return;
    }
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_object_set_section(intern, offset);
    RETURN_NULL();
}

void coordinate_object_set_section(coordinate_set_object *intern, long index) {
    coordinate_subset *current = intern->first_subset;
    coordinate_subset *tmp;
    int i = 0;
    while (++i < index && current) {
        tmp = current->next;
        free_subset(intern, current);
        current = tmp;
    }
    if (current != NULL) {
        intern->first = current->first;
        intern->last = current->last;
        intern->first_subset = current;
        intern->last_subset = current;
        current = current->next;
        while (current) {
            tmp = current->next;
            free_subset(intern, current);
            current = tmp;
        }
    }
}

int set_graph_values(coordinate_set_object *intern) {
    if (intern->first) {
        coordinate_object *current = intern->first->next;
        while (current && current->next) {
            double distance = get_distance(current->next, current->prev);
            if (current->next->timestamp != current->prev->timestamp) {
                current->climb_rate = current->next->ele - current->prev->ele / (current->next->timestamp - current->prev->timestamp);
                current->speed = distance * 1000 / (current->next->timestamp - current->prev->timestamp);
            } else {
                current->climb_rate = 0;
                current->speed = 0;
            }
            current->bearing = get_bearing(current, current->next);
            current = current->next;
        }
        return 1;
    }
    return 0;
}

long coordinate_set_simplify(coordinate_set_object *set) {
    coordinate_object *tmp, *current;
    if (set->first) {
        current = set->first->next;
        long i = 0;
        while (current) {
            double distance = get_distance_precise(current, current->prev);
            if (distance < 0.001) {
                tmp = current->next;
                _free_coordinate_object(current);
                current = tmp;
                i++;
            } else {
                current = current->next;
            }
        }
        return i;
    }
    return 0;
}

PHP_METHOD (coordinate_set, simplify) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(coordinate_set_simplify(intern));
}

int is_valid_subset(coordinate_subset *set) {
    return set->length > 20;
}

PHP_METHOD (coordinate_set, trim) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_subset *tmp, *set;
    int i = 0;
    set = intern->first_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->next;
            i += set->length;
            free_subset(intern, set);
            set = tmp;
            intern->subset_count --;
        } else {
            break;
        }
    }
    set = intern->last_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->prev;
            i += set->length;
            free_subset(intern, set);
            set = tmp;
            intern->subset_count --;
        } else {
            break;
        }
    }
    RETURN_LONG(i);
}

PHP_METHOD (coordinate_set, set_ranges) {
    coordinate_set_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);

    intern->max_ele = intern->max_ele = intern->max_alt = intern->min_alt = 0;
    intern->max_climb_rate = intern->min_climb_rate = intern->max_speed = 0;
    coordinate_object *current = intern->first;
    while (current) {
        // Compare heights with max/min
        if (current->ele > intern->max_ele) {
            intern->max_ele = current->ele;
            intern->max_ele_t = current->timestamp - intern->first->timestamp;
        }
        if (current->ele < intern->min_ele) {
            intern->min_ele = current->ele;
            intern->min_ele_t = current->timestamp - intern->first->timestamp;
        }
        if (current->alt > intern->max_alt) {
            intern->max_alt = current->alt;
            intern->max_alt_t = current->timestamp - intern->first->timestamp;
        }
        if (current->alt < intern->min_alt) {
            intern->min_alt = current->alt;
            intern->min_alt_t = current->timestamp - intern->first->timestamp;
        }
        if (current->climb_rate < intern->min_climb_rate) {
            intern->min_climb_rate = current->climb_rate;
        }
        if (current->climb_rate > intern->max_climb_rate) {
            intern->max_climb_rate = current->climb_rate;
        }
        if (current->speed > intern->max_speed) {
            intern->max_speed = current->speed;
        }
        current = current->next;
        //intern->bounds->add_coordinate_to_bounds(coordinate->lat(), coordinate->lng());
    }
}

int is_h_record(char *line) {
    if (strncmp(line, "H", 1) == 0) {
        return 1;
    }
    return 0;
}
int is_b_record(char *line) {
    if (strncmp(line, "B", 1) == 0) {
        return 1;
    }
    return 0;
}

double parse_degree(char *p, size_t length, char negative) {
    //  Extract lat
    char direction[2], deg[length + 1], sec[6];
    strncpy(deg, p, length);
    deg[length] = 0;
    strncpy(sec, p + length, 5);
    sec[5] = 0;
    strncpy(direction, p + length + 5, 1);
    direction[1] = 0;
    double ret = atof(deg) + (atof(sec) / 60000);
    if (*direction == negative) {
        ret *= -1;
    }
    return ret;
}

coordinate_object *parse_igc_coordiante(char *line) {
    coordinate_object *intern = emalloc(sizeof(coordinate_object));
    char *p = line + 1;
    intern->bearing = 0;
    intern->climb_rate = 0;
    intern->speed = 0;
    intern->prev = intern->next = NULL;
    intern->coordinate_set = NULL;
    intern->coordinate_subset = NULL;

    // Extract time
    char hour[3], min[3], seconds[3];
    strncpy(hour, p, 2);
    hour[2] = 0;
    strncpy(min, p + 2, 2);
    min[2] = 0;
    strncpy(seconds, p + 4, 2);
    seconds[2] = 0;
    intern->timestamp = (atoi(hour) * 3600) + (atoi(min) * 60) + atoi(seconds);
    p += 6;

    intern->lat = parse_degree(p, 2, 'S');
    p += 8;
    intern->lng = parse_degree(p, 3, 'W');
    sincos(intern->lat toRAD, &intern->sin_lat, &intern->cos_lat);;
    p += 10;

    // Extract alt
    char alt[6];
    strncpy(alt, p, 5);
    alt[5] = 0;
    intern->alt = atoi(alt);
    p += 5;

    // Extract ele
    char ele[6];
    strncpy(ele, p, 5);
    ele[5] = 0;
    intern->ele = atoi(ele);
    return intern;

}

int parse_h_record(coordinate_set_object *parser, char *line) {
    if (strncmp(line, "HFDTE", 5) == 0) {
        char day[3], month[3], year[3];
        strncpy(day, line += 5, 2);
        day[2] = 0;
        strncpy(month, line += 2, 2);
        month[2] = 0;
        strncpy(year, line += 2, 2);
        year[2] = 0;
        parser->day = atoi(day);
        parser->month = atoi(month);
        parser->year = atoi(year) + 2000;
        return 1;
    }
    return 0;
}