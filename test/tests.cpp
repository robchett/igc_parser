#include "gtest/gtest.h"
#include "../convert.c"
//#include "../task.c"
#include "../coordinate.c"
#include "../coordinate_subset.c"
#include "../coordinate_set.c"
#include <math.h>
#include <string>

double rounddp(double num, int dp) {
    if (dp == 2) {
        return roundf(num * 100) / 100;
    } else if (dp == 3) {
        return roundf(num * 1000) / 1000;
    }
}

int main(int ac, char *av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}

TEST (Conversion, latlng_to_gridref) {
    EXPECT_STREQ ("TL757503", convert_latlng_to_gridref (52.123, 0.567));
    EXPECT_STREQ ("SP693477", convert_latlng_to_gridref (52.123, -0.987));
    EXPECT_STREQ ("TW134291", convert_latlng_to_gridref (50.123, 0.987));
}

TEST (Conversion, gridref_to_latlng) {
    double lat,lng;
    convert_gridref_to_latlng("TL757503", &lat, &lng);
    EXPECT_DOUBLE_EQ(52.123f, rounddp(lat, 3));
    EXPECT_DOUBLE_EQ(0.567f, rounddp(lng, 3));
    convert_gridref_to_latlng("SP693477", &lat, &lng);
    EXPECT_DOUBLE_EQ(52.123f, rounddp(lat, 3));
    EXPECT_DOUBLE_EQ(-0.988f, rounddp(lng, 3));
    convert_gridref_to_latlng("TW134291", &lat, &lng);
    EXPECT_DOUBLE_EQ(50.123f, rounddp(lat, 3));
    EXPECT_DOUBLE_EQ(0.986f, rounddp(lng, 3));
}

//TEST (Tasks, parsing) {
//    task_t *task;
//
//    json_error_t error;
//    json_t *data = json_loads("", 1, error);
//
//    task = parse_task(data);
//}

TEST (Parsing, parse_h_record) {
    char * strings [] = {
            "HFDTE240216",
            "HFDTE:240216",
            "HFDTEDATE:240216",
            "HFDTEDATE:240216,1"
    };
    for (int i = 0; i < 4; i++) {
        coordinate_set_t *set = new coordinate_set_t;
        EXPECT_EQ (true, parse_h_record (set, strings[i]));
        EXPECT_EQ (2016, set->year);
        EXPECT_EQ (2, set->month);
        EXPECT_EQ (24, set->day);
    }
}

