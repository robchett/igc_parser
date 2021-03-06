#include "main.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "task.h"
#include "distance_map.h"
#include "igc_parser.h"

void distance_map_init(distance_map_t *obj, coordinate_set_t *set) {
    obj->coordinate_set = set;

    int64_t real_size = obj->size = set->length;

    obj->distances = NEW(uint64_t *, real_size);
    for (int16_t i = 0; i < real_size; i++) {
        obj->distances[i] = NEW(uint64_t, (real_size - i + 1));
    }
    int16_t j, i = 0;
    coordinate_t *coordinate1 = set->first;
    while (coordinate1) {
        j = 0;
        coordinate_t *coordinate2 = coordinate1->next;
        while (coordinate2) {
            obj->distances[i][j] = floor(get_distance_precise(coordinate1, coordinate2) * 1000000);
            j++;
            coordinate2 = coordinate2->next;
        }
        i++;
        coordinate1 = coordinate1->next;
    }
}

void distance_map_deinit(distance_map_t *obj) {
    for (size_t i = 0; i < obj->size; i++) {
        free(obj->distances[i]);
    }
    free(obj->distances);
}

uint64_t score_triangle(distance_map_t *obj, triangle_score *score, int16_t include_gap) {
    if (!score) {
        return 0;
    }
    uint64_t res = (MAP(obj, score->x, score->y) + MAP(obj, score->y, score->z) + MAP(obj, score->x, score->z));
    if (include_gap) {
        res -= MAP(obj, score->row, score->col);
    }
    return res;
}

triangle_score *check_y(int64_t x, int64_t y, int64_t z, int64_t row, int64_t col, distance_map_t *obj, uint64_t *_minleg, triangle_score *score, float height_ratio) {
    int64_t distance = (MAP(obj, x, y) + MAP(obj, y, z) + MAP(obj, x, z));
    int64_t min = fmin(MAP(obj, x, y), fmin(MAP(obj, y, z), MAP(obj, x, z)));
    if (distance > score_triangle(obj, score, 0) && min > distance * height_ratio) {
        triangle_score *new_score = NEW(triangle_score, 1);
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

triangle_score *scan_between(int64_t x, int64_t z, int64_t row, int64_t col, distance_map_t *obj, uint64_t *_minleg, triangle_score *score, float height_ratio) {
    int64_t y = 0;
    uint64_t skip;
    for (y = floor((x + z) / 2); y <= (z - 1); ++y) {
        if (skip_up(obj, &y, MAP(obj, x, y), *_minleg, 2) || skip_up(obj, &y, MAP(obj, y, z), *_minleg, 2))
            continue;
        score = check_y(x, y, z, row, col, obj, _minleg, score, height_ratio);
    }
    y = floor((x + z) / 2);
    for (; y >= (x + 1); --y) {
        if (skip_down(obj, &y, MAP(obj, x, y), *_minleg, 2) || skip_down(obj, &y, MAP(obj, y, z), *_minleg, 2))
            continue;
        score = check_y(x, y, z, row, col, obj, _minleg, score, height_ratio);
    }
    return score;
}

void close_gap(distance_map_t *obj, triangle_score *score) {
    int64_t i, j;
    uint64_t best;
    i = score->row;
    j = score->col;
    best = MAP(obj, i, j);
    for (i = score->row; i <= score->x; i++) {
        for (j = score->col; j >= score->z; j--) {
            if (MAP(obj, i, j) < best) {
                best = MAP(obj, i, j);
                score->row = i;
                score->col = j;
            }
        }
    }
}

task_t *distance_map_score_triangle(distance_map_t *obj, float height_ratio) {
    uint64_t maximum_distance = 0;
    triangle_score *best_score = NULL;
    int64_t closest_end = 0;
    int64_t const minleg = 800000;
    int64_t _minleg = 800000;
    int64_t row, col, x, y, z = 0;
    for (row = 0; row < obj->size; ++row) {
        for (col = obj->size - 1; col > row && col > closest_end; --col) {
            if (skip_down(obj, &col, minleg, MAP(obj, row, col), 1))
                continue;
            x = row + ((col - row) / 2);
            for (x; x <= col - 2; ++x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    if (skip_down(obj, &z, MAP(obj, x, z), _minleg, 3))
                        continue;
                    best_score = scan_between(x, z, row, col, obj, &_minleg, best_score, height_ratio);
                }
            }
            x = row + ((col - row) / 2);
            for (x; x >= row; --x) {
                for (z = col; z > x + 1 && z > closest_end; --z) {
                    if (skip_down(obj, &z, MAP(obj, x, z), _minleg, 3))
                        continue;
                    best_score = scan_between(x, z, row, col, obj, &_minleg, best_score, height_ratio);
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
            if (score_triangle(obj, current_score, 0) + minleg < score_triangle(obj, best_score, 0))
                break;
            // Break if the current iteration could not possible beat the best;
            uint64_t pre_score = score_triangle(obj, current_score, 0);
            close_gap(obj, current_score);
            // warn("calc change: %d -> %d", pre_score, score_triangle(obj, current_score, 1));
            if (score_triangle(obj, current_score, 1) > score_triangle(obj, best_score, 1)) {
                best_score = current_score;
            }
            current_score = current_score->prev;
        }

        note("Optimising tringle done: %d sets checked", i);
        note("Best set: x: %d, y: %d, z: %d", best_score->x, best_score->y, best_score->z);

        task_t *task = NEW(task_t, 1);
        task_init(task, TRIANGLE, 4, get_coordinate(obj, best_score->x), get_coordinate(obj, best_score->y), get_coordinate(obj, best_score->z), get_coordinate(obj, best_score->x));
        task_add_gap(task, get_coordinate(obj, best_score->row), get_coordinate(obj, best_score->col));
        return task;
    }
    return NULL;
}

task_t *distance_map_score_out_and_return(distance_map_t *obj) {
    note("%d", obj->size);
    int64_t distance, maximum_distance = 0;
    int64_t indexes[] = {0, 0, 0};
    int64_t const minLeg = 800000;
    for (int64_t row = 0; row < obj->size; ++row) {
        for (int64_t col = (obj->size - 1); col > (row + 2); --col) {
            if (skip_down(obj, &col, minLeg, MAP(obj, row, col), 1))
                continue;
            if (MAP(obj, row, col) < minLeg) {
                for (int64_t x = row + 1; x < col; x++) {
                    distance = MAP(obj, row, x) + MAP(obj, x, col) - MAP(obj, row, col);
                    if (distance > maximum_distance) {
                        // printf("%d, %d, %ld\n", row, col, MAP(obj, row, col));
                        maximum_distance = distance;
                        indexes[0] = row;
                        indexes[1] = x;
                        indexes[2] = col;
                    } else {
                        skip_up(obj, &x, maximum_distance, distance, 2);
                    }
                }
            }
        }
    }

    if (maximum_distance) {
        task_t *task = NEW(task_t, 1);
        task_init(task, OUT_AND_RETURN, 3, get_coordinate(obj, indexes[0]), get_coordinate(obj, indexes[1]), get_coordinate(obj, indexes[2]));
        task_add_gap(task, get_coordinate(obj, indexes[0]), get_coordinate(obj, indexes[2]));
        return task;
    }
    return NULL;
}

task_t *distance_map_score_open_distance_3tp(distance_map_t *obj) {
    uint64_t bestBack[obj->size], bestFwrd[obj->size];
    int64_t bestBack_index[obj->size], bestFwrd_index[obj->size];
    int64_t i;
    double best_score = 0;
    int64_t indexes[] = {0, 0, 0, 0, 0};
    int64_t row, j;
    int64_t maxF, maxB, midB, endB, midF, endF;
    for (i = 0; i < obj->size; i++) {
        bestBack_index[i] = bestFwrd_index[i] = 0;
        bestBack[i] = maximum_bound_index_back(obj, i, &bestBack_index[i]);
        bestFwrd[i] = maximum_bound_index_fwrd(obj, i, &bestFwrd_index[i]);
    }
    for (row = 0; row < obj->size; ++row) {
        maxF = midF = endF = endB = maxB = midB = 0;
        for (j = 0; j < row; ++j) {
            if (((MAP(obj, j, row)) + bestBack[j]) > maxB) {
                maxB = (MAP(obj, j, row)) + bestBack[j];
                midB = j;
                endB = bestBack_index[j];
            }
        }
        for (j = row + 1; j < obj->size; ++j) {
            if (((MAP(obj, row, j)) + bestFwrd[j]) > maxF) {
                maxF = (MAP(obj, row, j)) + bestFwrd[j];
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
        task_t *task = NEW(task_t, 1);
        task_init(task, OPEN_DISTANCE, 5, get_coordinate(obj, indexes[0]), get_coordinate(obj, indexes[1]), get_coordinate(obj, indexes[2]), get_coordinate(obj, indexes[3]), get_coordinate(obj, indexes[4]));
        return task;
    }
    return NULL;
}

double distance_map_get_precise(distance_map_t *obj, uint64_t offset1, uint64_t offset2) {
    if (offset1 > offset2) {
        uint64_t temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }

    coordinate_t *point1 = get_coordinate(obj, offset1);
    coordinate_t *point2 = get_coordinate(obj, offset2);
    return get_distance_precise(point1, point2);
}

double distance_map_get(distance_map_t *obj, uint64_t offset1, uint64_t offset2) {
    if (offset1 > offset2) {
        int64_t temp = offset1;
        offset1 = offset2;
        offset2 = temp;
    }
    return (double)(MAP(obj, offset1, offset2) / 1000000);
}

coordinate_t *get_coordinate(distance_map_t *map, uint64_t index) {
    int16_t i = 0;
    coordinate_t *current = map->coordinate_set->first;
    while (i < index) {
        i++;
        if (current->next) {
            current = current->next;
        }
    }
    return current;
}

uint64_t maximum_bound_index_back(distance_map_t *map, uint64_t point, uint64_t *index) {
    uint64_t best_index = point;
    uint64_t best_score = 0;
    for (uint64_t i = 0; i < point; ++i) {
        // note("(back) %3d -> %3d = %d", i, point, MAP(map, i, point));
        if (best_score < MAP(map, i, point)) {
            best_index = i;
            best_score = MAP(map, i, point);
        }
    }
    *index = best_index;
    return best_score;
}

uint64_t maximum_bound_index_fwrd(distance_map_t *map, uint64_t point, uint64_t *index) {
    uint64_t best_index = point;
    uint64_t best_score = 0;
    for (uint64_t i = (point + 1); i < map->size; ++i) {
        // note("(fwrd) %3d -> %3d = %d", i, point, MAP(map, point, i));
        if (best_score < MAP(map, point, i)) {
            best_index = i;
            best_score = MAP(map, point, i);
        }
    }
    *index = best_index;
    return best_score;
}

inline uint64_t skip_up(distance_map_t *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs) {
    uint64_t cnt = 0;
    uint64_t dist;
    uint64_t _index = *index;
    while (_index < map->size - 1) {
        dist = MAP(map, _index, (_index + 1)) * effected_legs;
        if (current > required + dist) {
            current -= dist;
            _index += 1;
            cnt++;
        } else {
            break;
        }
    }
    *index = _index;
    if (cnt > 0) {
        note("Skipped(u) %d points, distance: %d", cnt, required);
    }
    return cnt;
}

inline uint64_t skip_down(distance_map_t *map, uint64_t *index, uint64_t required, uint64_t current, int16_t effected_legs) {
    uint64_t cnt = 0, dist, _index = *index;
    while (_index > 1) {
        dist = MAP(map, (_index - 1), _index) * effected_legs;
        if (current > required + dist) {
            current -= dist;
            _index -= 1;
            cnt++;
        } else {
            break;
        }
    }
    *index = _index;
    if (cnt > 0) {
        note("Skipped(d) %d points, distance: %d", cnt, required);
    }
    return cnt;
}