#pragma once

void init_coordinate_set(TSRMLS_D);

PHP_METHOD(coordinate_set, __construct);
PHP_METHOD(coordinate_set, set);
PHP_METHOD(coordinate_set, get);
PHP_METHOD(coordinate_set, first);
PHP_METHOD(coordinate_set, last);
PHP_METHOD(coordinate_set, date);
PHP_METHOD(coordinate_set, count);
PHP_METHOD(coordinate_set, get_id);
PHP_METHOD(coordinate_set, part_count);
PHP_METHOD(coordinate_set, parse_igc);
PHP_METHOD(coordinate_set, get_date);
PHP_METHOD(coordinate_set, trim);
PHP_METHOD(coordinate_set, repair);
PHP_METHOD(coordinate_set, stats);
PHP_METHOD(coordinate_set, has_height_data);
PHP_METHOD(coordinate_set, set_graph_values);
PHP_METHOD(coordinate_set, set_section);
PHP_METHOD(coordinate_set, set_ranges);
PHP_METHOD(coordinate_set, simplify);
PHP_METHOD(coordinate_set, part_duration);
PHP_METHOD(coordinate_set, part_length);

typedef struct coordinate_subset {
    coordinate_object *first, *last;
    long length;
    struct coordinate_subset *next, *prev;
} coordinate_subset;

typedef struct coordinate_set_object {
    zend_object std;

    coordinate_object *first, *last;
    coordinate_object *real_first, *real_last;
    unsigned long length, real_length;
    long offset;

    // Date
    long day;
    long month;
    long year;

    // Graph limits
    long max_ele, max_ele_t, min_ele, min_ele_t;
    long max_alt, max_alt_t, min_alt, min_alt_t;
    double max_climb_rate, min_climb_rate, max_speed;

    // Subset
    coordinate_subset *first_subset, *last_subset;
    long subset_count;
} coordinate_set_object;

zend_class_entry *coordinate_set_ce;
static zend_function_entry coordinate_set_methods[];
zend_object_handlers coordinate_set_handlers;

zend_object* create_coordinate_set_object(zend_class_entry *class_type TSRMLS_DC);
void free_coordinate_set_object(coordinate_set_object *intern TSRMLS_DC);

coordinate_object *match_b_record(char *line);
int set_graph_values(coordinate_set_object *intern);
int parse_igc(coordinate_set_object *intern, char *string);
void parse_igc_coordinate(char *string, coordinate_object *intern);
int is_b_record(char *line);
int is_h_record(char *line);
int is_valid_subset(coordinate_subset *start);
void push_coordinate(coordinate_set_object *set, coordinate_object *coordinate);
int parse_h_record(coordinate_set_object *coordinate_set, char *line);
int has_height_data(coordinate_set_object *coordinate_set);
void clone_coordinate_object(coordinate_object *source, coordinate_object *dest);
void coordinate_object_trim(coordinate_set_object *source);
void free_subset(coordinate_set_object *parser, coordinate_subset *subset);
coordinate_set_object* fetch_coordinate_set_object(zval* obj);
void coordinate_object_set_section(coordinate_set_object *intern, long start, long end);