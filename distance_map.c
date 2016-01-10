#include <php.h>
#include <math.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "task.h"
#include "distance_map.h"
#include "geometry.h"


zend_object_handlers distance_map_object_handler;

void init_distance_map(TSRMLS_D) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "distance_map", distance_map_methods);
    distance_map_ce = zend_register_internal_class(&ce TSRMLS_CC);
    distance_map_ce->create_object = create_distance_map_object;

    memcpy(&distance_map_object_handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    distance_map_object_handler.free_obj = free_distance_map_object;
    distance_map_object_handler.offset = XtOffsetOf(distance_map_object, std);
}

zend_object* create_distance_map_object(zend_class_entry *class_type TSRMLS_DC) {
    distance_map_object* retval;

    retval = ecalloc(1, sizeof(distance_map_object) + zend_object_properties_size(class_type));

    zend_object_std_init(&retval->std, class_type TSRMLS_CC);
    object_properties_init(&retval->std, class_type);

    retval->std.handlers = &distance_map_object_handler;

    return &retval->std;
}

inline distance_map_object* fetch_distance_map_object(zval* obj) {
    return (distance_map_object*) ((char*) Z_OBJ_P(obj) - XtOffsetOf(distance_map_object, std));
}


void free_distance_map_object(distance_map_object *intern TSRMLS_DC) {
    zend_object_std_dtor(&intern->std TSRMLS_CC);
    if (intern->distances) {
        for (int64_t i; i < intern->size; i++) {
            free(intern->distances[i]);
        }
        free(intern->distances);
    }
    free(intern);
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

    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    coordinate_set_object *coordinate_set_object = fetch_coordinate_set_object(coordinate_set_zval TSRMLS_CC);
    create_distance_map(intern, coordinate_set_object);
}

uint64_t score_triangle(distance_map_object *intern, triangle_score *score, int16_t include_gap) {
    if (!score) {
        return 0;
    }
    uint64_t res = (MAP(intern, score->x, score->y) + MAP(intern, score->y, score->z) + MAP(intern, score->x, score->z));
    if (include_gap) {
        res -= MAP(intern, score->row, score->col);
    }
    return res;
}

int create_distance_map(distance_map_object *map, coordinate_set_object *set) {
     map->coordinate_set = set;
 
     int64_t real_size = map->size = set->length;

     map->distances = malloc(sizeof(uint64_t *) * (real_size));
     for (int16_t i = 0; i < real_size; i++) {
        note("%d", i); 
        map->distances[i] = malloc((real_size - i  + 1) * sizeof(uint64_t));

     }
     int16_t j, i= 0;
     coordinate_object *coordinate1 = set->first;
     while (coordinate1) {
         j = 0;
         coordinate_object *coordinate2 = coordinate1->next;
         while (coordinate2) {
             map->distances[i][j] = floor(get_distance_precise(coordinate1, coordinate2) * 1000000);
             j++;
             coordinate2 = coordinate2->next;
         }
         i++;
         coordinate1 = coordinate1->next;
     }
     return 1;

}

inline triangle_score *check_y(int64_t x, int64_t y, int64_t z, int64_t row, int64_t col, distance_map_object *intern, uint64_t *_minleg, triangle_score *score) {
    int64_t distance = (MAP(intern, x, y) + MAP(intern, y, z) + MAP(intern, x, z));
    int64_t min = fmin(MAP(intern, x, y), fmin(MAP(intern, y, z), MAP(intern, x, z))); 
    if (distance > score_triangle(intern, score, 0) && min > distance * 0.28) {
        triangle_score *new_score = malloc(sizeof(triangle_score));
        new_score->x = x;
        new_score->y = y;
        new_score->z = z;
        new_score->row = row;
        new_score->col = col;
        new_score->prev = score;
        new_score->next = NULL;
        if (score) {
            score->next = new_score;
            // warn("New set: x: %d, y: %d, z: %d", new_score->x, new_score->y, new_score->z);
            // warn("Prev set: x: %d, y: %d, z: %d", new_score->prev->x, new_score->prev->y, new_score->prev->z);
        } else {
            // warn("First set: x: %d, y: %d, z: %d", new_score->x, new_score->y, new_score->z);
        }  
        return new_score;
    } 
    return score;
}

inline triangle_score *scan_between(int64_t x, int64_t z, int64_t row, int64_t col, distance_map_object *intern, uint64_t *_minleg, triangle_score *score) {
    int64_t y = 0;
    uint64_t skip;
    for (y = floor((x + z) / 2); y <= (z - 1); ++y) {        
        if ( 
            skip_up(intern, &y, MAP(intern, x, y), *_minleg, 2) ||
            skip_up(intern, &y, MAP(intern, y, z), *_minleg, 2)
        ) continue;
        score = check_y(x, y, z, row, col, intern, _minleg, score);
    }
    y = floor((x + z) / 2);
    for (; y >= (x + 1); --y) {
        if (
            skip_down(intern, &y, MAP(intern, x, y), *_minleg, 2) ||
            skip_down(intern, &y, MAP(intern, y, z), *_minleg, 2)
        ) continue;
        score = check_y(x, y, z, row, col, intern, _minleg, score);
    }
    return score;
}

void close_gap(distance_map_object *intern, triangle_score *score) {
    int64_t i, j;
    uint64_t best;
    i = score->row;
    j = score->col;
    best = MAP(intern, i, j);
    for (i = score->row; i <= score->x; i++) {
        for (j = score->col; j >= score->z; j--) {
            if (MAP(intern, i, j) < best) {
                best = MAP(intern, i, j);
                score->row = i;
                score->col = j;
            }
        }
    }
}

PHP_METHOD(distance_map, score_triangle) {
    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    uint64_t maximum_distance = 0;
    triangle_score *best_score = NULL;
    int64_t closest_end = 0;
    int64_t const minleg = 800000;
    int64_t _minleg = 800000;
    int64_t row, col, x, y, z = 0;
    for (row = 0; row < intern->size; ++row) {
        for (col = intern->size - 1; col > row && col > closest_end; --col) {
            if (skip_down(intern, &col, minleg, MAP(intern, row, col), 1)) continue;
            x = row + ((col - row) / 2);
            for (x; x <= col - 2; ++x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    if (skip_down(intern, &z, MAP(intern, x, z), _minleg, 3)) continue;
                    best_score = scan_between(x, z, row, col, intern, &_minleg, best_score);
                }
            }
            x = row + ((col - row) / 2);
            for (x; x >= row; --x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    if (skip_down(intern, &z, MAP(intern, x, z), _minleg, 3)) continue;
                    best_score = scan_between(x, z, row, col, intern, &_minleg, best_score);
                }
            }
            closest_end = col;
            col = 0;
        }
    }

    triangle_score *current_score = best_score;

    if (best_score && best_score->z) {
        int64_t i = 0;
        while (current_score) {
            i++;
            if (score_triangle(intern, current_score, 0) + minleg < score_triangle(intern, best_score, 0)) break; 
            // Break if the current iteration could not possible beat the best;
            uint64_t pre_score = score_triangle(intern, current_score, 0);
            close_gap(intern, current_score);
            //warn("calc change: %d -> %d", pre_score, score_triangle(intern, current_score, 1));
            if (score_triangle(intern, current_score, 1) > score_triangle(intern, best_score, 1)) {
                best_score = current_score;
            }
            current_score = current_score->prev;
        }

        warn("Optimising tringle done: %d sets checked", i);
        warn("Best set: x: %d, y: %d, z: %d", best_score->x, best_score->y, best_score->z);

        zval *ret = malloc(sizeof(zval));
        object_init_ex(ret, task_ce);
        task_object *return_intern = fetch_task_object(ret TSRMLS_CC);
        return_intern->size = 4;
        return_intern->type = TRIANGLE;
        return_intern->coordinate = malloc(sizeof(coordinate_object *) * 4);
        return_intern->coordinate[0] = get_coordinate(intern, best_score->x);
        return_intern->coordinate[1] = get_coordinate(intern, best_score->y);
        return_intern->coordinate[2] = get_coordinate(intern, best_score->z);
        return_intern->coordinate[3] = get_coordinate(intern, best_score->x);

        return_intern->gap = malloc(sizeof(coordinate_object *) * 2);
        return_intern->gap[0] = get_coordinate(intern, best_score->row);
        return_intern->gap[1] = get_coordinate(intern, best_score->col);
        RETURN_ZVAL(ret, 0, 1);
    }
    RETURN_NULL();
}

PHP_METHOD(distance_map, score_out_and_return) {
    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    note("%d", intern->size);
    int64_t distance, maximum_distance = 0;
    int64_t indexes[] = {0, 0, 0};
    int64_t const minLeg = 800000;
    for (int64_t row = 0; row < intern->size; ++row) {
        for (int64_t col = (intern->size - 1); col > (row + 2); --col) {
            if(skip_down(intern, &col, minLeg, MAP(intern, row, col), 1)) continue;
            if (MAP(intern, row, col) < minLeg) {
                for (int64_t x = row + 1; x < col; x++) {
                    distance = MAP(intern, row, x) + MAP(intern, x, col) - MAP(intern, row, col);
                    if (distance > maximum_distance) {
                        //printf("%d, %d, %ld\n", row, col, MAP(intern, row, col));
                        maximum_distance = distance;
                        indexes[0] = row;
                        indexes[1] = x;
                        indexes[2] = col;
                    } else {
                        skip_up(intern, &x, maximum_distance, distance, 2);
                    }
                }
            }
        }
    }

    if (maximum_distance) {
        zval *ret = malloc(sizeof(task_object));
        object_init_ex(ret, task_ce);
        task_object *return_intern = fetch_task_object(ret TSRMLS_CC);
        return_intern->size = 3;
        return_intern->type = OUT_AND_RETURN;
        return_intern->coordinate = malloc(sizeof(coordinate_object *) * 3);
        return_intern->coordinate[0] = get_coordinate(intern, indexes[0]);
        return_intern->coordinate[1] = get_coordinate(intern, indexes[1]);
        return_intern->coordinate[2] = get_coordinate(intern, indexes[2]);

        return_intern->gap = malloc(sizeof(coordinate_object *) * 2);
        return_intern->gap[0] = get_coordinate(intern, indexes[0]);
        return_intern->gap[1] = get_coordinate(intern, indexes[2]);
        RETURN_ZVAL(ret, 0, 1);
    }
    RETURN_NULL();
}

PHP_METHOD(distance_map, score_open_distance_3tp) {
    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    uint64_t bestBack[intern->size], bestFwrd[intern->size];
    int64_t bestBack_index[intern->size], bestFwrd_index[intern->size];
    int64_t i;
    double best_score = 0;
    int64_t indexes[] = {0, 0, 0, 0, 0};
    int64_t row, j;
    int64_t maxF, maxB, midB, endB, midF, endF;
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
        zval *ret = malloc(sizeof(zval));
        object_init_ex(ret, task_ce);
        task_object *return_intern = fetch_task_object(ret TSRMLS_CC);
        return_intern->size = 5;
        return_intern->gap = NULL;
        return_intern->type = OPEN_DISTANCE;
        return_intern->coordinate = malloc(sizeof(coordinate_object *) * 5);
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
    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    uint64_t offset1;
    uint64_t offset2;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &offset1, &offset2) != SUCCESS) {
        return;
    }

    if (offset1 > offset2) {
        uint64_t temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }

    coordinate_object *point1 = get_coordinate(intern, offset1);
    coordinate_object *point2 = get_coordinate(intern, offset2);
    double distance = get_distance_precise(point1, point2);
    RETURN_DOUBLE(distance);
}

PHP_METHOD(distance_map, get) {

    int64_t offset1;
    int64_t offset2;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &offset1, &offset2) != SUCCESS) {
        return;
    }

    if (offset1 > offset2) {
        int64_t temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }

    distance_map_object *intern = fetch_distance_map_object(getThis() TSRMLS_CC);
    RETURN_DOUBLE((double)MAP(intern, offset1, offset2) / 1000000);
}

coordinate_object *get_coordinate(distance_map_object *map, uint64_t index) {
    int16_t i = 0;
    coordinate_object *current = map->coordinate_set->first;
    while (i < index) {
        i++;
        if (current->next) {
            current = current->next;
        }
    }
    return current;
}

uint64_t maximum_bound_index_back(distance_map_object *map, uint64_t point, uint64_t *index) {
    uint64_t best_index = point;
    uint64_t best_score = 0;
    for (uint64_t i = 0; i < point; ++i) {
        //note("(back) %3d -> %3d = %d", i, point, MAP(map, i, point));
        if (best_score < MAP(map, i, point)) {
            best_index = i;
            best_score = MAP(map, i, point);
        }
    }
    *index = best_index;
    return best_score;
}

uint64_t maximum_bound_index_fwrd(distance_map_object *map, uint64_t point, uint64_t *index) {
    uint64_t best_index = point;
    uint64_t best_score = 0;
    for (uint64_t i = (point + 1); i < map->size; ++i) {
        //note("(fwrd) %3d -> %3d = %d", i, point, MAP(map, point, i));
        if (best_score < MAP(map, point, i)) {
            best_index = i;
            best_score = MAP(map, point, i);
        }
    }
    *index = best_index;
    return best_score;
}

inline uint64_t skip_up(distance_map_object *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs) {
    uint64_t cnt = 0, dist;
    while (*index < map->size - 1) {
        dist = MAP(map, *index, (*index + 1)) * effected_legs;
        if (current > required + dist) {
            current -= dist;
            *index += 1;
            cnt++;
        } else {
            break;
        }  
    }
    //warn("Skipped(u) %d points, distance: %d", cnt, required);
    return cnt;
}

inline uint64_t skip_down(distance_map_object *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs) {
    uint64_t cnt = 0, dist;
    while (*index > 1) {
        dist = MAP(map, (*index - 1), *index) * effected_legs;
        if (current > required + dist) {
            current -= dist;
            *index -= 1;
            cnt++;
        } else {
            break;
        }
    }
    //warn("Skipped(d) %d points, distance: %d", cnt, required);
    return cnt;
}