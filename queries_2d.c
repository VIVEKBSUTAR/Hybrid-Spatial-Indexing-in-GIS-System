#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"

/* ───────── Distance ───────── */
double distance2D(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

/* ───────── NN (Nearest Neighbor) ───────── */

void nnSearch2D(QuadtreeNode* node,
                double qx, double qy,
                SpatialObject2D** best,
                double* bestDist) {

    if (!node) return;

    visited_nodes++;

    for (int i = 0; i < node->object_count; i++) {
        SpatialObject2D* obj = node->objects[i];
        double d = distance2D(qx, qy, obj->x, obj->y);

        if (d < *bestDist) {
            *bestDist = d;
            *best = obj;
        }
    }

    if (node->is_divided) {
        for (int i = 0; i < 4; i++) {
            nnSearch2D(node->children[i], qx, qy, best, bestDist);
        }
    }
}

/* Wrapper */
SpatialObject2D* nearestNeighbor2D(QuadtreeNode* root,
                                   double x, double y) {

    SpatialObject2D* best = NULL;
    double bestDist = 1e18;

    visited_nodes = 0;

    nnSearch2D(root, x, y, &best, &bestDist);

    return best;
}

/* ───────── KNN (K Nearest Neighbors) ───────── */

typedef struct {
    SpatialObject2D* obj;
    double dist;
} Neighbor;

int compareNeighbors(const void* a, const void* b) {
    return ((Neighbor*)a)->dist - ((Neighbor*)b)->dist;
}

/* Collect all points */
void collectAllPoints(QuadtreeNode* node,
                      Neighbor* list,
                      int* count,
                      double qx, double qy) {

    if (!node) return;

    visited_nodes++;

    for (int i = 0; i < node->object_count; i++) {
        SpatialObject2D* obj = node->objects[i];

        list[*count].obj = obj;
        list[*count].dist = distance2D(qx, qy, obj->x, obj->y);
        (*count)++;
    }

    if (node->is_divided) {
        for (int i = 0; i < 4; i++) {
            collectAllPoints(node->children[i], list, count, qx, qy);
        }
    }
}

/* Main KNN */
int knnSearch2D(QuadtreeNode* root,
                double x, double y,
                int k,
                SpatialObject2D* results[]) {

    Neighbor* list = malloc(sizeof(Neighbor) * MAX_TOTAL_OBJECTS);
    int count = 0;

    visited_nodes = 0;

    collectAllPoints(root, list, &count, x, y);

    qsort(list, count, sizeof(Neighbor), compareNeighbors);

    int resultCount = (k < count) ? k : count;

    for (int i = 0; i < resultCount; i++) {
        results[i] = list[i].obj;
    }

    free(list);

    return resultCount;
}