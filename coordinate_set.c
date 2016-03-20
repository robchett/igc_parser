#include "main.h"
#include "string.h"
#include <stdio.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"
#include "igc_parser.h"
#include "task.h"

void parse_igc_coordinate(char *line, coordinate_t *obj);
coordinate_t *match_b_record(char *line);
int is_valid_subset(coordinate_subset_t *start);
int parse_h_record(coordinate_set_t *coordinate_set, char *line);
int has_height_data(coordinate_set_t *coordinate_set);
bool is_h_record(char *line);
bool is_b_record(char *line);
bool is_c_record(char *line);
void parse_c_record(char *line, coordinate_t *coordinate);

void coordinate_set_init(coordinate_set_t *obj) {
    obj->length = 0;
    obj->real_length = 0;
    obj->first_subset = obj->last_subset = NULL;
    obj->first = obj->last = NULL;
    obj->real_first = obj->real_last = NULL;
    obj->subset_count = 0;

    obj->max_alt = obj->min_alt = obj->max_alt_t = obj->min_alt_t = 0;
    obj->max_ele = obj->min_ele = obj->max_ele_t = obj->min_ele_t = 0;
    obj->max_climb_rate = obj->min_climb_rate = 0;
    obj->max_speed = 0;
}

void coordinate_set_deinit(coordinate_set_t *obj) {
    for (size_t i = 0; i < obj->subset_count; i++) {
        coordinate_subset_deinit(obj->first_subset, obj);
    }
}

char *coordinate_set_date(coordinate_set_t *obj) {
    char *buffer = NEW(char, 11);
    sprintf(buffer, "%04d-%02d-%02d", obj->year, obj->month, obj->day);
    return buffer;
}

void coordinate_set_append(coordinate_set_t *obj, coordinate_t *coordinate) {
    coordinate->id = obj->length++;
    obj->real_length++;
    coordinate->prev = obj->last;
    if (!obj->first) {
        coordinate_subset_t *subset = NEW(coordinate_subset_t, 1);
        coordinate_subset_init(subset, obj, coordinate);
        obj->first = obj->real_first = coordinate;
        obj->subset_count++;
    } else {
        obj->last->next = coordinate;
        if (!obj->last || coordinate->timestamp - obj->last->timestamp > 60 || get_distance(coordinate, obj->last) > 5) {
            coordinate_subset_t *subset = NEW(coordinate_subset_t, 1);
            coordinate_subset_init(subset, obj, coordinate);
            obj->subset_count++;
        } else {
            obj->last_subset->length++;
        }
    }
    obj->last = obj->real_last = coordinate;
    obj->last_subset->last = coordinate;
    coordinate->coordinate_set = obj;
    coordinate->coordinate_subset = obj->last_subset;
}

coordinate_t *coordinate_set_get_coordinate(coordinate_set_t *obj, uint16_t index) {
    if (index < obj->real_length) {
        coordinate_t *coordinate = obj->real_first;
        int16_t i = 0;
        while (coordinate && ++i < index) {
            coordinate = coordinate->next;
        }
        return coordinate;
    }
    return NULL;
}

coordinate_t *coordinate_set_get_coordinate_by_id(coordinate_set_t *obj, uint64_t id) {
    if (id < obj->length) {
        coordinate_t *coordinate = obj->first;
        int16_t i = 0;
        while (coordinate && coordinate->id != id) {
            coordinate = coordinate->next;
        }
        return coordinate;
    }
    return NULL;
}

int coordinate_set_parse_igc(coordinate_set_t *obj, char *string, task_t **task) {
    int16_t b_records = 0;
    char *curLine = string;
    char *c_records[8];
    size_t c_records_cnt = 0;
    while (curLine) {
        char *nextLine = strchr(curLine, '\n');
        if (nextLine)
            *nextLine = '\0';
        if (is_h_record(curLine)) {
            parse_h_record(obj, curLine);
        } else if (is_c_record(curLine)) {
            if (c_records_cnt < 7) {
                c_records[c_records_cnt++] = curLine;
            }
        } else if (is_b_record(curLine)) {
            coordinate_t *coordinate = NEW(coordinate_t, 1);
            parse_igc_coordinate(curLine, coordinate);
            coordinate_set_append(obj, coordinate);
        }
        if (nextLine) {
            *nextLine = '\n';
        }
        curLine = nextLine ? (nextLine + 1) : NULL;
    }

    if (c_records_cnt && c_records_cnt <= 7) {
        *task = NEW(task_t, 1);
        coordinate_t **coordinates = NEW(coordinate_t*, c_records_cnt - 2);
        size_t cnt = 0;
        for (size_t i = 0; i < c_records_cnt - 2; i++) {
            coordinate_t *coordinate = NEW(coordinate_t, 1);
            parse_c_record(c_records[i + 1], coordinate);
            if (coordinate && coordinate->lat && coordinate->lng) {
                coordinates[cnt++] = coordinate;
            }
        }
        task_init_ex(*task, cnt, coordinates);
    }

    return b_records;
}

int coordinate_set_has_height_data(coordinate_set_t *coordinate_set) {
    coordinate_t *coordinate = coordinate_set->first;
    while (coordinate) {
        if (coordinate->ele) {
            return 1;
        }
        coordinate = coordinate->next;
    }
    return 0;
}

int coordinate_set_repair(coordinate_set_t *obj) {
    if (obj->first) {
        coordinate_t *current = obj->first->next;
        while (current) {
            if (current->ele == 0) {
                current->ele = current->prev->ele;
            }
            if (current->ele > current->prev->ele + 500) {
                current->ele = current->prev->ele;
            }
            current = current->next;
        }
        return 1;
    }
    return 0;
}

uint64_t coordinate_subset_length(coordinate_set_t *obj, uint16_t offset) {
    coordinate_subset_t *set = obj->first_subset;
    uint16_t i = 0;
    while (i++ < offset && set) {
        set = set->next;
    }
    if (set) {
        return set->length;
    }
    return 0;
}

void coordinate_set_select_section(coordinate_set_t *obj, uint16_t start, uint16_t end) {
    if (!end) {
        end = start;
    }
    coordinate_subset_t *current = obj->first_subset;
    coordinate_subset_t *tmp;
    int16_t i = 0;
    while (i++ < start && current) {
        tmp = current->next;
        coordinate_subset_deinit(current, obj);
        current = tmp;
    }
    i = 0;
    int64_t diff = end - start;
    if (current) {
        obj->first = current->first;
        obj->first_subset = current;
        while (i++ < diff && current) {
            current = current->next;
        }
        if (current) {
            obj->last = current->last;
            obj->last_subset = current;
            current = current->next;
            while (current) {
                tmp = current->next;
                coordinate_subset_deinit(current, obj);
                current = tmp;
            }
        }
    }
}

int8_t coordinate_set_extrema(coordinate_set_t *obj) {
    if (obj->first) {
        coordinate_t *current = obj->first->next;
        while (current && current->next) {
            double distance = get_distance(current->next, current->prev);
            if (current->next->timestamp != current->prev->timestamp) {
                current->climb_rate = current->next->ele - current->prev->ele / (current->next->timestamp - current->prev->timestamp);
                current->speed = distance / (current->next->timestamp - current->prev->timestamp);
            } else {
                current->climb_rate = 0;
                current->speed = 0;
            }
            current->bearing = get_bearing(current, current->next);
            current = current->next;
        }

        obj->max_ele = obj->max_ele = obj->max_alt = obj->min_alt = 0;
        obj->max_climb_rate = obj->min_climb_rate = obj->max_speed = 0;
        current = obj->first;
        while (current) {
            // Compare heights with max/min
            if (current->ele > obj->max_ele) {
                obj->max_ele = current->ele;
                obj->max_ele_t = current->timestamp - obj->first->timestamp;
            }
            if (current->ele < obj->min_ele) {
                obj->min_ele = current->ele;
                obj->min_ele_t = current->timestamp - obj->first->timestamp;
            }
            if (current->alt > obj->max_alt) {
                obj->max_alt = current->alt;
                obj->max_alt_t = current->timestamp - obj->first->timestamp;
            }
            if (current->alt < obj->min_alt) {
                obj->min_alt = current->alt;
                obj->min_alt_t = current->timestamp - obj->first->timestamp;
            }
            if (current->climb_rate < obj->min_climb_rate) {
                obj->min_climb_rate = current->climb_rate;
            }
            if (current->climb_rate > obj->max_climb_rate) {
                obj->max_climb_rate = current->climb_rate;
            }
            if (current->speed > obj->max_speed) {
                obj->max_speed = current->speed;
            }
            current = current->next;
            // obj->bounds->add_coordinate_to_bounds(coordinate->lat(),
            // coordinate->lng());
        }

        return 1;
    }
    return 0;
}

int8_t coordinate_set_simplify(coordinate_set_t *set, size_t max_size) {
    coordinate_t *tmp, *current;
    if (set->first) {
        current = set->first->next;
        int64_t i = 0;
        while (current) {
            double distance = get_distance_precise(current, current->prev);
            if (distance < 0.003) {
                tmp = current->next;
                coordinate_deinit(current);
                current = tmp;
                i++;
            } else {
                current = current->next;
            }
        }

        if (set->length > max_size) {
            double total_distance = 0;
            current = set->first;
            while (current && current->next) {
                double distance = get_distance_precise(current, current->next);
                if (distance < 1) {
                    total_distance += distance;
                }
                current = current->next;
            }
            double average = total_distance / max_size;

            total_distance = 0;
            current = set->first->next;
            while (current && current->next) {
                total_distance += get_distance_precise(current, current->next);
                if (total_distance < average) {
                    tmp = current->next;
                    coordinate_deinit(current);
                    current = tmp;
                    i++;
                } else {
                    total_distance = 0;
                    current = current->next;
                }
            }
        }

        coordinate_subset_t *current = set->first_subset;
        do {
            if (current->length < 2) {
                coordinate_subset_t *tmp = current->next;
                coordinate_subset_deinit(current, set);
                current = tmp;
            } else {
                current = current->next;
            }
        } while (current);
        return i;
    }
    return 0;
}

int is_valid_subset(coordinate_subset_t *set) {
    return set->length > 20;
}

int8_t coordinate_set_trim(coordinate_set_t *obj) {
    coordinate_subset_t *tmp, *set;
    int16_t i = 0;
    set = obj->first_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->next;
            i += set->length;
            coordinate_subset_deinit(set, obj);
            set = tmp;
        } else {
            break;
        }
    }
    set = obj->last_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->prev;
            i += set->length;
            coordinate_subset_deinit(set, obj);
            set = tmp;
        } else {
            break;
        }
    }
    return i;
}

bool is_h_record(char *line) {
    return (strncmp(line, "H", 1) == 0);
}

bool is_b_record(char *line) {
    return (strncmp(line, "B", 1) == 0);
}

bool is_c_record(char *line) {
    return (strncmp(line, "C", 1) == 0);
}

double parse_degree(char *p, uint16_t length, char negative) {
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

void parse_c_record(char *line, coordinate_t *obj) {
    char *p = line + 1;
    double lat, lng;

    lat = parse_degree(p, 2, 'S');
    p += 8;
    lng = parse_degree(p, 3, 'W');
    p += 10;
    if (lat && lng) {
        coordinate_init(obj, lat, lng, 0, 0);
    } else {
        obj = NULL;
    }
}
void parse_igc_coordinate(char *line, coordinate_t *obj) {
    char *p = line + 1;
    obj->bearing = 0;
    obj->climb_rate = 0;
    obj->speed = 0;
    obj->id = 0;
    obj->prev = obj->next = NULL;
    obj->coordinate_set = NULL;
    obj->coordinate_subset = NULL;

    // Extract time
    char hour[3], min[3], seconds[3];
    strncpy(hour, p, 2);
    hour[2] = 0;
    strncpy(min, p + 2, 2);
    min[2] = 0;
    strncpy(seconds, p + 4, 2);
    seconds[2] = 0;
    obj->timestamp = (atoi(hour) * 3600) + (atoi(min) * 60) + atoi(seconds);
    p += 6;

    obj->lat = parse_degree(p, 2, 'S');
    p += 8;
    obj->lng = parse_degree(p, 3, 'W');
    sincos(obj->lat toRAD, &obj->sin_lat, &obj->cos_lat);
    ;
    p += 10;

    // Extract alt
    char alt[6];
    strncpy(alt, p, 5);
    alt[5] = 0;
    obj->alt = atoi(alt);
    p += 5;

    // Extract ele
    char ele[6];
    strncpy(ele, p, 5);
    ele[5] = 0;
    obj->ele = atoi(ele);
}

int parse_h_record(coordinate_set_t *parser, char *line) {
    size_t offset = 9;
    if (strncmp(line, "HFDTEDATE", offset) == 0 || strncmp(line, "HFDTE", offset = 5) == 0) {
        char day[3], month[3], year[3];
        if (line[offset] == ':') {
            offset += 1;
        }
        strncpy(day, line += offset, 2);
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
