#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "octree.h"
#include "grid.h"
#include "queries.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  %s generate N\n"
            "  %s stats\n"
            "  %s range      minX minY minZ maxX maxY maxZ\n"
            "  %s range_naive minX minY minZ maxX maxY maxZ\n"
            "  %s nn  x y z\n"
            "  %s knn x y z k\n",
            argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "generate") == 0 && argc == 3) {
        generateData(atoi(argv[2]));
        return 0;
    }

    initGrid();
    loadData();

    clock_t start = clock();

    if (strcmp(argv[1], "stats") == 0) {
        printGridStats();
        fprintf(stderr, "total_loaded=%d\n", total_objects);

    } else if (strcmp(argv[1], "range") == 0 && argc == 8) {
        executeRangeQuery(atof(argv[2]), atof(argv[3]), atof(argv[4]),
                          atof(argv[5]), atof(argv[6]), atof(argv[7]));

    } else if (strcmp(argv[1], "range_naive") == 0 && argc == 8) {
        executeRangeQueryNaive(atof(argv[2]), atof(argv[3]), atof(argv[4]),
                               atof(argv[5]), atof(argv[6]), atof(argv[7]));

    } else if (strcmp(argv[1], "nn") == 0 && argc == 5) {
        executeNN(atof(argv[2]), atof(argv[3]), atof(argv[4]));

    } else if (strcmp(argv[1], "knn") == 0 && argc == 6) {
        executeKNN(atof(argv[2]), atof(argv[3]), atof(argv[4]), atoi(argv[5]));

    } else {
        fprintf(stderr, "Unknown command.\n");
        return 1;
    }

    clock_t end = clock();
    double ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    fprintf(stderr, "time_ms=%f\n", ms);
    return 0;
}
