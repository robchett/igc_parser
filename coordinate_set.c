#include "main.h"
#include "string.h"
#include <stdio.h>
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"
#include "./statistics/group.h"
#include "igc_parser.h"

void parse_igc_coordinate(char *line, coordinate_t *this);
coordinate_t *match_b_record(char *line);
int is_valid_subset(coordinate_subset_t *start);
int parse_h_record(coordinate_set_t *coordinate_set, char *line);
int has_height_data(coordinate_set_t *coordinate_set);
int is_h_record(char *line);
int is_b_record(char *line);

void coordinate_set_init(coordinate_set_t *this) {
    this->length = 0;
    this->real_length = 0;
    this->first_subset = this->last_subset = NULL;
    this->first = this->last = NULL;
    this->real_first = this->real_last = NULL;
    this->subset_count = 0;

    this->max_alt = this->min_alt = this->max_alt_t = this->min_alt_t = 0;
    this->max_ele = this->min_ele = this->max_ele_t = this->min_ele_t = 0;
    this->max_climb_rate = this->min_climb_rate = 0;
    this->max_speed = 0;
}

void coordinate_set_deinit(coordinate_set_t *this) {
    for (size_t i = 0; i < this->subset_count; i++) {
        coordinate_subset_deinit(this->first_subset, this);
    }
}

char *coordinate_set_date(coordinate_set_t *this) {
    char *buffer = malloc(sizeof(char) * 11);
    sprintf(buffer, "%04d-%02d-%02d", this->year, this->month, this->day);
    return buffer;
}

void coordinate_set_append(coordinate_set_t *this, coordinate_t *coordinate) {
    coordinate->id = this->length++;
    this->real_length++;
    coordinate->prev = this->last;
    if (!this->first) {
        coordinate_subset_t *subset = malloc(sizeof(coordinate_subset_t));
        coordinate_subset_init(subset, this, coordinate);
        this->first = this->real_first = coordinate;
        this->subset_count++;
    } else {
        this->last->next = coordinate;
        if (!this->last || coordinate->timestamp - this->last->timestamp > 60 || get_distance(coordinate, this->last) > 5) {
            coordinate_subset_t *subset = malloc(sizeof(coordinate_subset_t));
            coordinate_subset_init(subset, this, coordinate);
            this->subset_count++;
        } else {
            this->last_subset->length++;
        }
    }
    this->last = this->real_last = coordinate;
    this->last_subset->last = coordinate;
    coordinate->coordinate_set = this;
    coordinate->coordinate_subset = this->last_subset;
}

coordinate_t *coordinate_set_get_coordinate(coordinate_set_t *this, uint16_t index) {
    if (index < this->real_length) {
        coordinate_t *coordinate = this->real_first;
        int16_t i = 0;
        while (coordinate && ++i < index) {
            coordinate = coordinate->next;
        }
        return coordinate;
    }
    return NULL;
}

coordinate_t *coordinate_set_get_coordinate_by_id(coordinate_set_t *this, uint64_t id) {
    if (id < this->length) {
        coordinate_t *coordinate = this->first;
        int16_t i = 0;
        while (coordinate && coordinate->id != id) {
            coordinate = coordinate->next;
        }
        return coordinate;
    }
    return NULL;
}

int coordinate_set_parse_igc(coordinate_set_t *this, char *string) {
    int16_t b_records = 0;
    char *curLine = string;
    while (curLine) {
        char *nextLine = strchr(curLine, '\n');
        if (nextLine)
            *nextLine = '\0';
        if (is_h_record(curLine)) {
            parse_h_record(this, curLine);
        } else if (is_b_record(curLine)) {
            coordinate_t *coordinate = malloc(sizeof(coordinate_t));
            parse_igc_coordinate(curLine, coordinate);
            coordinate_set_append(this, coordinate);
        }
        if (nextLine)
            *nextLine = '\n';
        curLine = nextLine ? (nextLine + 1) : NULL;
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

int coordinate_set_repair(coordinate_set_t *this) {
    if (this->first) {
        coordinate_t *current = this->first->next;
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

statistics_set_t *coordinate_set_stats(coordinate_set_t *this) {
    statistics_set_t *ret = malloc(sizeof(statistics_set_t));
    statistics_set_init(ret, this);
    return ret;
}

uint64_t coordinate_subset_length(coordinate_set_t *this, uint16_t offset) {
    coordinate_subset_t *set = this->first_subset;
    uint16_t i = 0;
    while (i++ < offset && set) {
        set = set->next;
    }
    if (set) {
        return set->length;
    }
    return 0;
}

void coordinate_set_section(coordinate_set_t *this, uint16_t start, uint16_t end) {
    if (!end)
        end = start;
    warn("Setting subset: %d, %d", start, end);
    coordinate_subset_t *current = this->first_subset;
    coordinate_subset_t *tmp;
    int16_t i = 0;
    while (i++ < start && current) {
        tmp = current->next;
        coordinate_subset_deinit(current, this);
        current = tmp;
    }
    i = 0;
    int64_t diff = end - start;
    if (current != NULL) {
        this->first = current->first;
        this->first_subset = current;
        while (i++ < diff && current) {
            current = current->next;
        }
        this->last = current->last;
        this->last_subset = current;
        current = current->next;
        while (current) {
            tmp = current->next;
            coordinate_subset_deinit(current, this);
            current = tmp;
        }
    }
}

int8_t coordinate_set_extrema(coordinate_set_t *this) {
    if (this->first) {
        coordinate_t *current = this->first->next;
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

        this->max_ele = this->max_ele = this->max_alt = this->min_alt = 0;
        this->max_climb_rate = this->min_climb_rate = this->max_speed = 0;
        current = this->first;
        while (current) {
            // Compare heights with max/min
            if (current->ele > this->max_ele) {
                this->max_ele = current->ele;
                this->max_ele_t = current->timestamp - this->first->timestamp;
            }
            if (current->ele < this->min_ele) {
                this->min_ele = current->ele;
                this->min_ele_t = current->timestamp - this->first->timestamp;
            }
            if (current->alt > this->max_alt) {
                this->max_alt = current->alt;
                this->max_alt_t = current->timestamp - this->first->timestamp;
            }
            if (current->alt < this->min_alt) {
                this->min_alt = current->alt;
                this->min_alt_t = current->timestamp - this->first->timestamp;
            }
            if (current->climb_rate < this->min_climb_rate) {
                this->min_climb_rate = current->climb_rate;
            }
            if (current->climb_rate > this->max_climb_rate) {
                this->max_climb_rate = current->climb_rate;
            }
            if (current->speed > this->max_speed) {
                this->max_speed = current->speed;
            }
            current = current->next;
            // this->bounds->add_coordinate_to_bounds(coordinate->lat(),
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

int8_t coordinate_set_trim(coordinate_set_t *this) {
    coordinate_subset_t *tmp, *set;
    int16_t i = 0;
    set = this->first_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->next;
            i += set->length;
            coordinate_subset_deinit(set, this);
            set = tmp;
        } else {
            break;
        }
    }
    set = this->last_subset;
    while (set) {
        if (!is_valid_subset(set)) {
            tmp = set->prev;
            i += set->length;
            coordinate_subset_deinit(set, this);
            set = tmp;
        } else {
            break;
        }
    }
    return i;
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

void parse_igc_coordinate(char *line, coordinate_t *this) {
    char *p = line + 1;
    this->bearing = 0;
    this->climb_rate = 0;
    this->speed = 0;
    this->id = 0;
    this->prev = this->next = NULL;
    this->coordinate_set = NULL;
    this->coordinate_subset = NULL;

    // Extract time
    char hour[3], min[3], seconds[3];
    strncpy(hour, p, 2);
    hour[2] = 0;
    strncpy(min, p + 2, 2);
    min[2] = 0;
    strncpy(seconds, p + 4, 2);
    seconds[2] = 0;
    this->timestamp = (atoi(hour) * 3600) + (atoi(min) * 60) + atoi(seconds);
    p += 6;

    this->lat = parse_degree(p, 2, 'S');
    p += 8;
    this->lng = parse_degree(p, 3, 'W');
    sincos(this->lat toRAD, &this->sin_lat, &this->cos_lat);
    ;
    p += 10;

    // Extract alt
    char alt[6];
    strncpy(alt, p, 5);
    alt[5] = 0;
    this->alt = atoi(alt);
    p += 5;

    // Extract ele
    char ele[6];
    strncpy(ele, p, 5);
    ele[5] = 0;
    this->ele = atoi(ele);
}

int parse_h_record(coordinate_set_t *parser, char *line) {
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
