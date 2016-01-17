#include "main.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"

void coordinate_subset_deinit(coordinate_subset_t *set, coordinate_set_t *parser) {
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
    parser->subset_count--;
    free(set);
}

void coordinate_subset_init(coordinate_subset_t *subset, coordinate_set_t *set, coordinate_t *coordinate) {
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