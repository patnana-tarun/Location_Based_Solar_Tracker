#include <stdio.h>
#include <stdlib.h>
#include "spa.h"

int main(int argc, char *argv[])
{
    if (argc != 9) {
        fprintf(stderr, "Usage: %s <year> <month> <day> <hour> <minute> <second> <lat> <lon>\n", argv[0]);
        fprintf(stderr, "Example: %s 2026 1 7 12 0 0 12.9716 77.5946\n", argv[0]);
        return 1;
    }

    spa_data spa;

    /* Parse command-line arguments */
    spa.year   = atoi(argv[1]);
    spa.month  = atoi(argv[2]);
    spa.day    = atoi(argv[3]);
    spa.hour   = atoi(argv[4]);
    spa.minute = atoi(argv[5]);
    spa.second = atof(argv[6]);

    /* Location */
    spa.latitude  = atof(argv[7]);
    spa.longitude = atof(argv[8]);
    spa.timezone  = 5.5;
    spa.elevation = 900;

    /* Environment */
    spa.pressure    = 1013.25;
    spa.temperature = 27;

    /* Other required fields */
    spa.delta_ut1 = 0;
    spa.delta_t   = 69.0;
    spa.slope     = 0;
    spa.azm_rotation = 0;
    spa.atmos_refract = 0.5667;

    spa.function = SPA_ZA;  // Calculate zenith and azimuth only

    int result = spa_calculate(&spa);

    if (result == 0)
    {
        // Output in easy-to-parse format
        printf("AZIMUTH=%.6f\n", spa.azimuth);
        printf("ELEVATION=%.6f\n", 90.0 - spa.zenith);
        printf("ZENITH=%.6f\n", spa.zenith);
    }
    else
    {
        fprintf(stderr, "SPA_ERROR=%d\n", result);
        return 1;
    }

    return 0;
}