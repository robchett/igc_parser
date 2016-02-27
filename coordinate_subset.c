#include "main.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "coordinate_subset.h"

void coordinate_subset_deinit(coordinate_subset_t *obj, coordinate_set_t *parser) {
    if (obj->prev) {
        obj->prev->next = obj->next;
    }
    if (obj->next) {
        obj->next->prev = obj->prev;
    }
    if (obj == parser->first_subset) {
        parser->first_subset = obj->next;
    }
    if (obj == parser->last_subset) {
        parser->last_subset = obj->prev;
    }

    coordinate_t *tmp;
    coordinate_t *coordinate = obj->first;
    while (obj->length) {
        tmp = coordinate->next;
        coordinate_deinit(coordinate);
        coordinate = tmp;
    }

    parser->subset_count--;
    free(obj);
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

uint64_t coordinate_subset_duration(coordinate_subset_t *obj) {
    return (obj->last->timestamp - obj->first->timestamp);
}
