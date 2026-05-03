#ifndef QUERIES_H
#define QUERIES_H

#include "types.h"

/* Indexed 3-D range query */
void executeRangeQuery(double minX, double minY, double minZ,
                       double maxX, double maxY, double maxZ);

/* Naive 3-D range query (brute-force baseline) */
void executeRangeQueryNaive(double minX, double minY, double minZ,
                            double maxX, double maxY, double maxZ);

/* Nearest neighbour in 3-D */
void executeNN(double qx, double qy, double qz);

/* K nearest neighbours in 3-D */
void executeKNN(double qx, double qy, double qz, int k);

#endif
