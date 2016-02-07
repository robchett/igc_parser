#include "gtest/gtest.h"
#include "../convert.c"


int main(int ac, char *av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}

TEST (Conversion, latlng_to_gridref) {
    ASSERT_STREQ ("TL757503", convert_latlng_to_gridref (52.123, 0.567));
    ASSERT_STREQ ("SP693477", convert_latlng_to_gridref (52.123, -0.987));
    ASSERT_STREQ ("TW134291", convert_latlng_to_gridref (50.123, 0.987));
}
