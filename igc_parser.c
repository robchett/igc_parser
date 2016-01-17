#include "main.h"
#include <libgen.h>
#include <string.h>
#include "include/json/jansson.h"
#include "coordinate.h"
#include "coordinate_set.h"
#include "task.h"
#include "igc_parser.h"
#include "formatter/formatter_kml.h"
#include "formatter/formatter_kml_earth.h"
#include "formatter/formatter_kml_split.h"
#include "formatter/formatter_js.h"
#include "distance_map.h"
#include "statistics/element.h"
#include "statistics/group.h"

char *get_os_grid_ref(coordinate_t *point) {
    double lat = point->lat toRAD;
    double lon = point->lng toRAD;
    double a = 6377563.396;
    double b = 6356256.910;
    double F0 = 0.9996012717;
    double lat0 = 49 toRAD;
    double lon0 = -2 toRAD;
    double N0 = -100000;
    double E0 = 400000;
    double e2 = 1 - ((b * b) / (a * a));
    double n = (a - b) / (a + b);
    double n2 = n * n;
    double n3 = n * n * n;

    double cosLat = cos(lat);
    double sinLat = sin(lat);
    double nu = a * F0 / sqrt(1 - e2 * sinLat * sinLat);                 // transverse radius of curvature
    double rho = a * F0 * (1 - e2) / pow(1 - e2 * sinLat * sinLat, 1.5); // meridional radius of curvature
    double eta2 = nu / rho - 1;

    double Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (lat - lat0);
    double Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(lat - lat0) * cos(lat + lat0);
    double Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (lat - lat0)) * cos(2 * (lat + lat0));
    double Md = (35 / 24) * n3 * sin(3 * (lat - lat0)) * cos(3 * (lat + lat0));
    double M = b * F0 * (Ma - Mb + Mc - Md); // meridional arc

    double cos3lat = cosLat * cosLat * cosLat;
    double cos5lat = cos3lat * cosLat * cosLat;
    double tan2lat = tan(lat) * tan(lat);
    double tan4lat = tan2lat * tan2lat;

    double I = M + N0;
    double II = (nu / 2) * sinLat * cosLat;
    double III = (nu / 24) * sinLat * cos3lat * (5 - tan2lat + 9 * eta2);
    double IIIA = (nu / 720) * sinLat * cos5lat * (61 - 58 * tan2lat + tan4lat);
    double IV = nu * cosLat;
    double V = (nu / 6) * cos3lat * (nu / rho - tan2lat);
    double VI = (nu / 120) * cos5lat * (5 - 18 * tan2lat + tan4lat + 14 * eta2 - 58 * tan2lat * eta2);

    double dLon = lon - lon0;
    double dLon2 = dLon * dLon;
    double dLon3 = dLon2 * dLon;
    double dLon4 = dLon3 * dLon;
    double dLon5 = dLon4 * dLon;
    double dLon6 = dLon5 * dLon;

    double N = I + II * dLon2 + III * dLon4 + IIIA * dLon6;
    double E = E0 + IV * dLon + V * dLon3 + VI * dLon5;
    return gridref_number_to_letter(E, N);
}

char *gridref_number_to_letter(int64_t e, int64_t n) {
    // get the 100km-grid indices
    int64_t e100k = (int64_t) floor(e / 100000);
    int64_t n100k = (int64_t) floor(n / 100000);

    if (e100k < 0 || e100k > 8 || n100k < 0 || n100k > 12) {
    }

    int64_t l1 = (19 - n100k) - (19 - n100k) % 5 + floor((e100k + 10) / 5);
    int64_t l2 = (19 - n100k) * 5 % 25 + e100k % 5;

    if (l1 > 7)
        l1++;
    if (l2 > 7)
        l2++;

    // strip 100km-grid indices from easting & northing, and reduce precision
    e = floor((e % 100000) / pow(10, 2));
    n = floor((n % 100000) / pow(10, 2));
    char *gridref = malloc(sizeof(char) * 9);
    sprintf(gridref, "%c%c%03d%03d", l1 + 65, l2 + 65, e, n);
    return gridref;
}

char *load_file(char *filename) {
    char *source = NULL;
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);

        source = malloc(fsize + 1);
        size_t newLen = fread(source, 1, fsize, fp);
        if (newLen == 0) {
            return NULL;
        } else {
            source[newLen++] = '\0';
        }
        fclose(fp);
    }
    return source;
}

int8_t main(int argc, char **argv) {
    char *source_file;
    char *root;
    printf("\n\nArgs: %d\n0: %s\n", argc, argv[0]);
    if (argc >= 3) {
        printf("1: %s\n2: %s\n", argv[1], argv[2]);
        source_file = argv[1];
        root = calloc(strlen(argv[2]) + 1, sizeof(char));
        strcpy(root, argv[2]);
    } else if (argc >= 2) {
        printf("1: %s\n", argv[1]);
        source_file = argv[1];
        root = calloc(strlen(source_file) + 1, sizeof(char));
        strcpy(root, source_file);
        dirname(root);
    } else if (argc == 0) {
        printf("Please provide a file\n\nUsage:\nigc_parser [file] {output_dir}");
    }

    char out_file_1[strlen(root) + 20];
    char out_file_2[strlen(root) + 20];
    char out_file_3[strlen(root) + 20];
    char out_file_4[strlen(root) + 20];
    sprintf(out_file_1, "%s%s", root, "/track.js");
    sprintf(out_file_2, "%s%s", root, "/track.kml");
    sprintf(out_file_3, "%s%s", root, "/track_earth.kml");
    sprintf(out_file_4, "%s%s", root, "/track_split.kml");

    printf("Parsing: %s\n", source_file);
    printf("Output: %s\n", root);

    char *igc_file = load_file(source_file);
    if (igc_file != NULL) {
        coordinate_set_t *set = malloc(sizeof(coordinate_set_t));
        coordinate_set_init(set);
        coordinate_set_parse_igc(set, igc_file);
        printf("Parsed\nPoints: %d\n", set->length);

        coordinate_set_trim(set);
        coordinate_set_repair(set);
        coordinate_set_simplify(set, 1500);
        coordinate_set_extrema(set);

        printf("Sets: %d\n", set->subset_count);

        if (set->subset_count == 1) {

            printf("Simplified: %d\n", set->length);

            distance_map_t *map = malloc(sizeof(distance_map_t));
            distance_map_init(map, set);

            char *ids;

            task_t *od = distance_map_score_open_distance_3tp(map);
            if (od) {
                ids = task_get_coordinate_ids(od);
                printf("OD IDs: %s\n", ids);
                free(ids);
            } else {
                printf("OD: Not found\n");
            }

            task_t *or = distance_map_score_out_and_return(map);
            if (or) {
                ids = task_get_coordinate_ids(or);
                printf("OR IDs: %s\n", ids);
                free(ids);
            } else {
                printf("OR: Not found\n");
            }

            task_t *tr = distance_map_score_triangle(map);
            if (tr) {
                ids = task_get_coordinate_ids(tr);
                printf("TR IDs: %s\n", ids);
                free(ids);
            } else {
                printf("TR: Not found\n");
            }

            formatter_t *formatter;

            formatter = malloc(sizeof(formatter_t));
            formatter_js_init(formatter, set, 1, od, or, tr);
            formatter_js_output(formatter, out_file_1);
            free(formatter);
            printf("JS output formatted\n");

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_init(formatter, set, "Bob", od, or, tr, NULL);
            formatter_kml_output(formatter, out_file_2);
            free(formatter);
            printf("KML output formatted\n");

            formatter = malloc(sizeof(formatter_t));
            formatter_kml_earth_init(formatter, set, "Bob", od, or, tr, NULL);
            formatter_kml_earth_output(formatter, out_file_3);
            free(formatter);
            printf("KML Earth output formatted\n");

            free(od);
            free(or);
            free(tr);
            distance_map_deinit(map);

        } else {
            formatter_split_t *formatter = malloc(sizeof(formatter_split_t));
            formatter_kml_split_init(formatter, set);
            formatter_kml_split_output(formatter, out_file_4);
            free(formatter);
            printf("KML split output formatted\n");
        }
        coordinate_set_deinit(set);
    } else {
        printf("Failed: %s\n", source_file);
        exit(1);
    }

    free(igc_file);
    exit(0);
}
