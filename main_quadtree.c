#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

/* From quadtree.c */
QuadtreeNode* createNode(BoundingBox2D boundary);
bool insertQuadtree(QuadtreeNode* node, SpatialObject2D* obj, int depth);
void rangeQuery2D(QuadtreeNode* node, BoundingBox2D range,
                  SpatialObject2D* results[], int* count);

/* 🔥 ADD THIS (Step 8 fix) */
void getQuadTreeStats(QuadtreeNode* node, int depth,
                      int* total_nodes, int* max_depth, int* obj_count);

/* From queries_2d.c */
SpatialObject2D* nearestNeighbor2D(QuadtreeNode* root, double x, double y);
int knnSearch2D(QuadtreeNode* root, double x, double y,
                int k, SpatialObject2D* results[]);

/* ───────── Globals ───────── */
SpatialObject2D all_objects_2d[MAX_TOTAL_OBJECTS];
int total_objects = 0;
long visited_nodes = 0;

QuadtreeNode* root = NULL;

/* ───────── Load data ───────── */
void loadPoints() {
    FILE* f = fopen("points.txt", "r");
    if (!f) return;

    double x, y, z;
    total_objects = 0;

    while (fscanf(f, "%lf %lf %lf", &x, &y, &z) == 3) {
        all_objects_2d[total_objects].id = total_objects + 1;
        all_objects_2d[total_objects].x = x;
        all_objects_2d[total_objects].y = y;
        total_objects++;
    }

    fclose(f);
}

/* ───────── Build tree ───────── */
void buildTree() {
    BoundingBox2D world = {0, 0, WORLD_SIZE, WORLD_SIZE};
    root = createNode(world);

    for (int i = 0; i < total_objects; i++) {
        insertQuadtree(root, &all_objects_2d[i], 0);
    }
}

/* ───────── MAIN ───────── */
int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("No command\n");
        return 0;
    }

    loadPoints();
    buildTree();

    /* ───── RANGE ───── */
    if (strcmp(argv[1], "range") == 0 && argc >= 6) {

        BoundingBox2D box = {
            atof(argv[2]), atof(argv[3]),
            atof(argv[4]), atof(argv[5])
        };

        SpatialObject2D* results[MAX_TOTAL_OBJECTS];
        int count = 0;

        visited_nodes = 0;

        rangeQuery2D(root, box, results, &count);

        for (int i = 0; i < count; i++) {
            printf("%d %lf %lf\n",
                results[i]->id,
                results[i]->x,
                results[i]->y);
        }

        fprintf(stderr, "visited=%ld time_ms=0\n", visited_nodes);
    }

    /* ───── RANGE NAIVE ───── */
    else if (strcmp(argv[1], "range_naive") == 0 && argc >= 6) {

        BoundingBox2D box = {
            atof(argv[2]), atof(argv[3]),
            atof(argv[4]), atof(argv[5])
        };

        visited_nodes = 0;

        for (int i = 0; i < total_objects; i++) {
            visited_nodes++;

            if (all_objects_2d[i].x >= box.minX && all_objects_2d[i].x <= box.maxX &&
                all_objects_2d[i].y >= box.minY && all_objects_2d[i].y <= box.maxY) {

                printf("%d %lf %lf\n",
                    all_objects_2d[i].id,
                    all_objects_2d[i].x,
                    all_objects_2d[i].y);
            }
        }

        fprintf(stderr, "visited=%ld time_ms=0\n", visited_nodes);
    }

    /* ───── NN ───── */
    else if (strcmp(argv[1], "nn") == 0 && argc >= 4) {

        double x = atof(argv[2]);
        double y = atof(argv[3]);

        visited_nodes = 0;

        SpatialObject2D* result = nearestNeighbor2D(root, x, y);

        if (result) {
            printf("%d %lf %lf\n",
                result->id,
                result->x,
                result->y);
        }

        fprintf(stderr, "visited=%ld time_ms=0\n", visited_nodes);
    }

    /* ───── KNN ───── */
    else if (strcmp(argv[1], "knn") == 0 && argc >= 5) {

        double x = atof(argv[2]);
        double y = atof(argv[3]);
        int k = atoi(argv[4]);

        visited_nodes = 0;

        SpatialObject2D* results[MAX_TOTAL_OBJECTS];

        int count = knnSearch2D(root, x, y, k, results);

        for (int i = 0; i < count; i++) {
            printf("%d %lf %lf\n",
                results[i]->id,
                results[i]->x,
                results[i]->y);
        }

        fprintf(stderr, "visited=%ld time_ms=0\n", visited_nodes);
    }

    /* ───── STATS (FIXED) ───── */
    else if (strcmp(argv[1], "stats") == 0) {

        int total_nodes = 0;
        int max_depth = 0;
        int obj_count = 0;

        getQuadTreeStats(root, 0, &total_nodes, &max_depth, &obj_count);

        fprintf(stderr,
            "total_nodes=%d max_depth=%d total_objects=%d\n",
            total_nodes, max_depth, obj_count);
    }

    return 0;
}