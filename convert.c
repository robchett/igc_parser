#include "main.h"
#include <string.h>
#include "convert.h"

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

char *convert_latlng_to_gridref(coordinate_t *point) {
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

int convert_gridref_to_latlng(const char *input, double *lat, double *lng) {
    int E, N;

    int l1 = input[0] - 'A';
    int l2 = input[1] - 'A';
    // shuffle down letters after 'I' since 'I' is not used in grid:
    if (l1 > 7) l1--;
    if (l2 > 7) l2--;

    int e100km = ((l1 - 2) % 5) * 5 + (l2 % 5);
    int n100km = (19 - floor(l1 / 5) * 5) - floor(l2 / 5);

    int offset = 0;
    int multi = 10;
    if (strlen(input) == 8) {
        offset = 3;
        multi = 100;
    } else if (strlen(input) == 10) {
        offset = 4;
        multi = 10;
    }

    char northing[offset], easting[offset];
    memcpy(northing, &input[2], offset);
    memcpy(easting, &input[2+offset], offset);
    northing[offset] = '\0';
    easting[offset] = '\0';
    E = (atoi(northing) * multi) + (e100km * 100000);
    N = (atoi(easting) * multi) + (n100km * 100000);

    double a = 6377563.396, b = 6356256.909;              // Airy 1830 major & minor semi-axes
    double F0 = 0.9996012717;                             // NatGrid scale factor on central meridian
    double phi0 = 49 * M_PI / 180, lamda0 = -2 * M_PI / 180;      // NatGrid true origin
    double N0 = -100000, E0 = 400000;                     // northing & easting of true origin, metres
    double e2 = 1 - (b * b) / (a * a);                          // eccentricity squared
    double n = (a - b) / (a + b), n2 = n * n, n3 = n * n * n;         // n, n², n³

    double phi = phi0, M = 0;
    do {
        phi = (N - N0 - M) / (a * F0) + phi;

        double Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (phi - phi0);
        double Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(phi - phi0) * cos(phi + phi0);
        double Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (phi - phi0)) * cos(2 * (phi + phi0));
        double Md = (35 / 24) * n3 * sin(3 * (phi - phi0)) * cos(3 * (phi + phi0));
        M = b * F0 * (Ma - Mb + Mc - Md);              // meridional arc

    } while (N - N0 - M >= 0.00001);  // ie until < 0.01mm

    double cosphi = cos(phi), sinphi = sin(phi);
    double v = a * F0 / sqrt(1 - e2 * sinphi * sinphi);            // nu = transverse radius of curvature
    double p = a * F0 * (1 - e2) / pow(1 - e2 * sinphi * sinphi, 1.5); // rho = meridional radius of curvature
    double nu2 = v / p - 1;                                    // eta = ?

    double tanphi = tan(phi);
    double tan2phi = tanphi * tanphi, tan4phi = tan2phi * tan2phi, tan6phi = tan4phi * tan2phi;
    double secphi = 1 / cosphi;
    double v3 = v * v * v, v5 = v3 * v * v, v7 = v5 * v * v;
    double VII = tanphi / (2 * p * v);
    double VIII = tanphi / (24 * p * v3) * (5 + 3 * tan2phi + nu2 - 9 * tan2phi * nu2);
    double IX = tanphi / (720 * p * v5) * (61 + 90 * tan2phi + 45 * tan4phi);
    double X = secphi / v;
    double XI = secphi / (6 * v3) * (v / p + 2 * tan2phi);
    double XII = secphi / (120 * v5) * (5 + 28 * tan2phi + 24 * tan4phi);
    double XIIA = secphi / (5040 * v7) * (61 + 662 * tan2phi + 1320 * tan4phi + 720 * tan6phi);

    double dE = (E - E0), dE2 = dE * dE, dE3 = dE2 * dE, dE4 = dE2 * dE2, dE5 = dE3 * dE2, dE6 = dE4 * dE2, dE7 = dE5 * dE2;
    phi = phi - VII * dE2 + VIII * dE4 - IX * dE6;
    double lamda = lamda0 + X * dE - XI * dE3 + XII * dE5 - XIIA * dE7;

    *lat = phi toDEG;
    *lng = lamda toDEG;
    return 1;
}
