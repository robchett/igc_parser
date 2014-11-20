#include <php.h>
#include <math.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "task.h"
#include "distance_map.h"

void init_distance_map(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "distance_map", distance_map_methods);
    distance_map_ce = zend_register_internal_class(&ce TSRMLS_CC);
    distance_map_ce->create_object = create_distance_map_object;

    memcpy(&distance_map_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    distance_map_handlers.clone_obj = clone_distance_map_object;
}

zend_object_value create_distance_map_object(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;

    distance_map_object *intern = emalloc(sizeof(distance_map_object));
    memset(intern, 0, sizeof(distance_map_object));

    // create a table for class properties
    zend_object_std_init(&intern->std, class_type TSRMLS_CC);
    object_properties_init(&intern->std, class_type);

    // create a destructor for this struct
    retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t)free_distance_map_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

static zend_object_value clone_distance_map_object(zval *object TSRMLS_DC) {
    distance_map_object *old_object = zend_object_store_get_object(object TSRMLS_CC);
    zend_object_value new_object_val = create_distance_map_object(Z_OBJCE_P(object) TSRMLS_CC);
    distance_map_object *new_object = zend_object_store_get_object_by_handle(new_object_val.handle TSRMLS_CC);

    zend_objects_clone_members(
        &new_object->std, new_object_val,
        &old_object->std, Z_OBJ_HANDLE_P(object) TSRMLS_CC
    );

    // new_object->coordinate_set = old_object->coordinate_set;
    // if (new_object->coordinate_set) {
    //  Z_ADDREF_P(new_object->coordinate_set);
    // }
    return new_object_val;
}


void free_distance_map_object(distance_map_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    if (intern->distances) {
        int i = 0;
        for (i = 0; i < intern->size; i++) {
            efree(intern->distances[i]);
        }
        efree(intern->distances);
    }
    efree(intern);
}

static zend_function_entry distance_map_methods[] = {
    PHP_ME(distance_map, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(distance_map, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(distance_map, get_precise, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(distance_map, score_open_distance_3tp, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(distance_map, score_out_and_return, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(distance_map, score_triangle, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

PHP_METHOD(distance_map, __construct) {
    zval *coordinate_set_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &coordinate_set_zval, coordinate_set_ce) == FAILURE) {
        return;
    }

    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    coordinate_set_object *coordinate_set_object = zend_object_store_get_object(coordinate_set_zval TSRMLS_CC);
    create_distance_map(intern, coordinate_set_object);
}

int create_distance_map(distance_map_object *map, coordinate_set_object *set) {
    map->coordinate_set = set;

    long real_size = map->size = set->length;
    map->distances = (unsigned long **) emalloc(sizeof(unsigned long *) * real_size);
    int i = 0;
    for (i; i < real_size; i++) {
        map->distances[i] = (unsigned long *) emalloc((real_size - i  + 1) * sizeof(unsigned long));
    }
    int j;
    coordinate_object *coordinate1 = set->first;
    i = 0;
    while (coordinate1) {
        j = i + 1;
        coordinate_object *coordinate2 = coordinate1->next;
        while (coordinate2) {
            map->distances[i][j - i - 1] = floor(get_distance(coordinate1, coordinate2) * 1000000);
            j++;
            coordinate2 = coordinate2->next;
        }
        i++;
        coordinate1 = coordinate1->next;
    }
    map->maximum_distance = 0;
    for (i = 0; i < real_size - 2; i++) {
        if (MAP(map, i, i + 1) > map->maximum_distance) {
            map->maximum_distance = MAP(map, i, i + 1);
        }
    }
    return 1;
}

inline unsigned long check_y(long x, long y, long z, long row, long col, distance_map_object *intern, unsigned long *_minleg, unsigned long *best_score, long (*indexes)[5]) {
    long distance = (MAP(intern, x, y) + MAP(intern, y, z) + MAP(intern, x, z));
    long min = fmin(MAP(intern, x, y), fmin(MAP(intern, y, z), MAP(intern, x, z)));
    if (distance > *best_score && min > distance * 0.28) {
        unsigned long minleg = *_minleg;
        unsigned long newleg = fmax(distance * 0.28, minleg);
        *best_score = distance;
        (*indexes)[0] = row;
        (*indexes)[1] = x;
        (*indexes)[2] = y;
        (*indexes)[3] = z;
        (*indexes)[4] = col;
        *_minleg = newleg;
        return *_minleg;
    } else {
        return (unsigned long) fmin(*best_score - distance, (distance * 0.28) - min);
    }
    return 0;
}

inline int scan_between(long x, long z, long row, long col, distance_map_object *intern, unsigned long *_minleg, unsigned long *best_score, long (*indexes)[5]) {
    long y = 0;
    unsigned long skip;
    for (y = floor((x + z) / 2); y <= (z - 1); ++y) {
        skip_up(y, *_minleg, MAP(intern, x, y), intern->maximum_distance);
        skip_up(y, *_minleg, MAP(intern, y, z), intern->maximum_distance);
        skip = check_y(x, y, z, row, col, intern, _minleg, best_score, indexes);
        if (skip) {
            skip_up(y, skip, 0, intern->maximum_distance); // Possibly risky
        }
    }
    y = floor((x + z) / 2);
    for (; y >= (x + 1); --y) {
        skip_down(y, *_minleg, MAP(intern, x, y), intern->maximum_distance);
        skip_down(y, *_minleg, MAP(intern, y, z), intern->maximum_distance);
        skip = check_y(x, y, z, row, col, intern, _minleg, best_score, indexes);
        if (skip) {
            skip_down(y, skip, 0, intern->maximum_distance); // Possibly risky
        }
    }
    return 0;
}

PHP_METHOD(distance_map, score_triangle) {
    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    unsigned long maximum_distance = 0;
    long indexes[] = {0, 0, 0, 0, 0};
    long closest_end = 0;
    long best_score = 0;
    long const minleg = 800000;
    long _minleg = 800000;
    long row, col, x, y, z = 0;
    for (row = 0; row < intern->size; ++row) {
        for (col = intern->size - 1; col > row && col > closest_end; --col) {
            skip_down(col, MAP(intern, row, col), minleg, intern->maximum_distance);
            x = row + ((col - row) / 3);
            for (x; x <= col - 2; ++x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    skip_down(z, _minleg, MAP(intern, x, z), intern->maximum_distance);
                    scan_between(x, z, row, col, intern, &_minleg, &best_score, &indexes);
                }
            }
            x = row + ((col - row) / 3);
            for (x; x >= row; --x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    skip_down(z, _minleg, MAP(intern, x, z), intern->maximum_distance);
                    scan_between(x, z, row, col, intern, &_minleg, &best_score, &indexes);
                }
            }
            closest_end = col;
            col = 0;
        }
    }

    if (best_score && indexes[4]) {
        unsigned long sgap = MAP(intern, indexes[0], indexes[4]);
        long a, b;
        for (a = indexes[4]; a >= indexes[3]; a--) {
            for (b = indexes[0]; b < indexes[1]; b++) {
                if (MAP(intern, b, a) < sgap) {
                    indexes[4] = a;
                    indexes[0] = b;
                    sgap = MAP(intern, b, a);
                }
            }
        }

        zval *ret;
        MAKE_STD_ZVAL(ret);
        object_init_ex(ret, task_ce);
        task_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
        return_intern->size = 5;
        return_intern->type = TRIANGLE;
        return_intern->coordinate = emalloc(sizeof(coordinate_object *) * 5);
        return_intern->coordinate[0] = get_coordinate(intern, indexes[0]);
        return_intern->coordinate[1] = get_coordinate(intern, indexes[1]);
        return_intern->coordinate[2] = get_coordinate(intern, indexes[2]);
        return_intern->coordinate[3] = get_coordinate(intern, indexes[3]);
        return_intern->coordinate[4] = get_coordinate(intern, indexes[4]);
        RETURN_ZVAL(ret, 0, 1);
    }
    RETURN_NULL();
}

PHP_METHOD(distance_map, score_out_and_return) {
    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    unsigned long distance, maximum_distance = 0;
    long indexes[] = {0, 0, 0};
    long row, col, x;
    long const minLeg = 8000000;
    for (row = 0; row < intern->size; ++row) {
        for (col = intern->size - 1; col > row + 2; --col) {
            // skip_down(col, MAP(intern, row, col), minLeg, intern->maximum_distance);
            if (MAP(intern, row, col) < minLeg) {
                for (x = row + 1; x < col; x++) {
                    distance = MAP(intern, row, x) + MAP(intern, x, col) - MAP(intern, row, col);
                    if (distance > maximum_distance) {
                        maximum_distance = distance;
                        indexes[0] = row;
                        indexes[1] = x;
                        indexes[2] = col;
                    } else {
                        skip_up(x, distance, maximum_distance, intern->maximum_distance);
                    }
                }
            }
        }
    }

    if (maximum_distance) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        object_init_ex(ret, task_ce);
        task_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
        return_intern->size = 3;
        return_intern->type = OUT_AND_RETURN;
        return_intern->coordinate = emalloc(sizeof(coordinate_object *) * 3);
        return_intern->coordinate[0] = get_coordinate(intern, indexes[0]);
        return_intern->coordinate[1] = get_coordinate(intern, indexes[1]);
        return_intern->coordinate[2] = get_coordinate(intern, indexes[2]);
        RETURN_ZVAL(ret, 0, 1);
    }
    RETURN_NULL();
}

PHP_METHOD(distance_map, score_open_distance_3tp) {
    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    unsigned long bestBack[intern->size], bestFwrd[intern->size];
    long bestBack_index[intern->size], bestFwrd_index[intern->size];
    long i;
    double best_score = 0;
    long indexes[] = {0, 0, 0, 0, 0};
    long row, j;
    long maxF, maxB, midB, endB, midF, endF;
    for (i = 0; i < intern->size; i++) {
        bestBack_index[i] = bestFwrd_index[i] = 0;
        bestBack[i] = maximum_bound_index_back(intern, i, &bestBack_index[i]);
        bestFwrd[i] = maximum_bound_index_fwrd(intern, i, &bestFwrd_index[i]);
    }
    for (row = 0; row < intern->size; ++row) {
        maxF = midF = endF = endB = maxB = midB = 0;
        for (j = 0; j < row; ++j) {
            if (((MAP(intern, j, row)) + bestBack[j]) > maxB) {
                maxB = (MAP(intern, j, row)) + bestBack[j];
                midB = j;
                endB = bestBack_index[j];
            }
        }
        for (j = row + 1; j < intern->size; ++j) {
            if (((MAP(intern, row, j)) + bestFwrd[j]) > maxF) {
                maxF = (MAP(intern, row, j)) + bestFwrd[j];
                midF = j;
                endF = bestFwrd_index[j];
            }
        }
        if (maxF + maxB > best_score) {
            best_score = maxF + maxB;
            indexes[0] = endB;
            indexes[1] = midB;
            indexes[2] = row;
            indexes[3] = midF;
            indexes[4] = endF;
        }
    }

    if (best_score) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        object_init_ex(ret, task_ce);
        task_object *return_intern = zend_object_store_get_object(ret TSRMLS_CC);
        return_intern->size = 5;
        return_intern->type = OPEN_DISTANCE;
        return_intern->coordinate = emalloc(sizeof(coordinate_object *) * 5);
        return_intern->coordinate[0] = get_coordinate(intern, indexes[0]);
        return_intern->coordinate[1] = get_coordinate(intern, indexes[1]);
        return_intern->coordinate[2] = get_coordinate(intern, indexes[2]);
        return_intern->coordinate[3] = get_coordinate(intern, indexes[3]);
        return_intern->coordinate[4] = get_coordinate(intern, indexes[4]);
        RETURN_ZVAL(ret, 0, 1);
    }
    RETURN_NULL();
}

PHP_METHOD(distance_map, get_precise) {
    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    unsigned long offset1;
    unsigned long offset2;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &offset1, &offset2) != SUCCESS) {
        return;
    }

    if (offset1 > offset2) {
        unsigned long temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }

    coordinate_object *point1 = get_coordinate(intern, offset1);
    coordinate_object *point2 = get_coordinate(intern, offset2);
    double distance = get_distance_precise(point1, point2);
    RETURN_DOUBLE(distance);
}
PHP_METHOD(distance_map, get) {

    long offset1;
    long offset2;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &offset1, &offset2) != SUCCESS) {
        return;
    }

    if (offset1 > offset2) {
        long temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }

    distance_map_object *intern = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE((double)MAP(intern, offset1, offset2) / 1000000);
}

coordinate_object *get_coordinate(distance_map_object *map, unsigned long index) {
    int i = 0;
    coordinate_object *current = map->coordinate_set->first;
    while (i < index) {
        i++;
        if (current->next) {
            current = current->next;
        }
    }
    return current;
}

unsigned long maximum_bound_index_back(distance_map_object *map, unsigned long point, unsigned long *index) {
    unsigned long best_index = point;
    unsigned long i = 0;
    unsigned long best_score = 0;
    for (i; i < point; ++i) {
        if (best_score < MAP(map, i, point)) {
            best_index = i;
            best_score = MAP(map, i, point);
        }
    }
    *index = best_index;
    return best_score;
}

unsigned long maximum_bound_index_fwrd(distance_map_object *map, unsigned long point, unsigned long *index) {
    unsigned long best_index = point;
    unsigned long i = point;
    unsigned long best_score = 0;
    for (i; i < map->size; ++i) {
        if (best_score < MAP(map, point, i)) {
            best_index = i;
            best_score = MAP(map, point, i);
        }
    }
    *index = best_index;
    return best_score;
}