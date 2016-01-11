#pragma once

#include "coordinate.h"

typedef struct coordinate_subset_t {
    coordinate_t *first, *last;
    int64_t length;
    struct coordinate_subset_t *next, *prev;
} coordinate_subset_t;

struct coordinate_set_t;
typedef struct coordinate_set_t {
    coordinate_t *first, *last;
    coordinate_t *real_first, *real_last;
    uint64_t length, real_length;
    int64_t offset;

    // Date
    int64_t day;
    int64_t month;
    int64_t year;

    // Graph limits
    int64_t max_ele, max_ele_t, min_ele, min_ele_t;
    int64_t max_alt, max_alt_t, min_alt, min_alt_t;
    double max_climb_rate, min_climb_rate, max_speed;

    // Subset
    coordinate_subset_t *first_subset, *last_subset;
    int64_t subset_count;
} coordinate_set_t;



void coordinate_set_init(coordinate_set_t *this);

coordinate_t *match_b_record(char *line);
int coordinate_set_set_graph_values(coordinate_set_t *this);
int parse_igc(coordinate_set_t *this, char *string);
void parse_igc_coordinate(char *string, coordinate_t *this);
int is_valid_subset(coordinate_subset_t *start);
void push_coordinate(coordinate_set_t *set, coordinate_t *coordinate);
int parse_h_record(coordinate_set_t *coordinate_set, char *line);
int has_height_data(coordinate_set_t *coordinate_set);
void clone_coordinate_t(coordinate_t *source, coordinate_t *dest);
void coordinate_trim(coordinate_set_t *source);
void coordinate_subset_free(coordinate_set_t *parser, coordinate_subset_t *subset);
void coordinate_set_section(coordinate_set_t *this, int64_t start, int64_t end);

int is_h_record(char *line);
int is_b_record(char *line);


// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 