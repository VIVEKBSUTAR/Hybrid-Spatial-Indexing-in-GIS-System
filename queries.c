#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queries.h"
#include "octree.h"

/* ── Internal helper: search one octree node recursively (3D) ────── */

static void searchOctreeRange(OctreeNode* node, BoundingBox3D range, int* result_count) {
    visited_nodes++;
    if (!intersectsBox3D(node->boundary, range)) return;

    for (int i = 0; i < node->object_count; i++) {
        if (containsPoint3D(range,
                node->objects[i]->x, node->objects[i]->y, node->objects[i]->z)) {
            printf("%d %f %f %f\n",
                   node->objects[i]->id,
                   node->objects[i]->x,
                   node->objects[i]->y,
                   node->objects[i]->z);
            (*result_count)++;
        }
    }
    if (node->is_divided) {
        for (int i = 0; i < 8; i++) {
            searchOctreeRange(node->children[i], range, result_count);
        }
    }
}

/* ── Indexed 3D range query ──────────────────────────────────────── */

void executeRangeQuery(double minX, double minY, double minZ,
                       double maxX, double maxY, double maxZ) {
    BoundingBox3D range = {minX, minY, minZ, maxX, maxY, maxZ};
    double cw = WORLD_SIZE / GRID_SIZE;
    double ch = WORLD_SIZE / GRID_SIZE;
    double cd = WORLD_SIZE / GRID_SIZE;

    int minCx = (int)(minX / cw), maxCx = (int)(maxX / cw);
    int minCy = (int)(minY / ch), maxCy = (int)(maxY / ch);
    int minCz = (int)(minZ / cd), maxCz = (int)(maxZ / cd);
    
    if (minCx < 0)          minCx = 0;
    if (maxCx >= GRID_SIZE) maxCx = GRID_SIZE - 1;
    if (minCy < 0)          minCy = 0;
    if (maxCy >= GRID_SIZE) maxCy = GRID_SIZE - 1;
    if (minCz < 0)          minCz = 0;
    if (maxCz >= GRID_SIZE) maxCz = GRID_SIZE - 1;

    visited_nodes = 0;
    int result_count = 0;
    for (int i = minCx; i <= maxCx; i++)
        for (int j = minCy; j <= maxCy; j++)
            for (int k = minCz; k <= maxCz; k++)
                searchOctreeRange(grid[i][j][k].root, range, &result_count);

    fprintf(stderr, "visited=%ld results=%d\n", visited_nodes, result_count);
}

/* ── Naive 3D range query ────────────────────────────────────────── */

void executeRangeQueryNaive(double minX, double minY, double minZ,
                            double maxX, double maxY, double maxZ) {
    int result_count = 0;
    for (int i = 0; i < total_objects; i++) {
        if (all_objects[i].x >= minX && all_objects[i].x <= maxX &&
            all_objects[i].y >= minY && all_objects[i].y <= maxY &&
            all_objects[i].z >= minZ && all_objects[i].z <= maxZ) {
            printf("%d %f %f %f\n",
                   all_objects[i].id,
                   all_objects[i].x,
                   all_objects[i].y,
                   all_objects[i].z);
            result_count++;
        }
    }
    fprintf(stderr, "visited=%d results=%d\n", total_objects, result_count);
}

/* ── Nearest neighbour in 3D (grid ring expansion + octree pruning) ── */

void executeNN(double qx, double qy, double qz) {
    double cw = WORLD_SIZE / GRID_SIZE;
    double ch = WORLD_SIZE / GRID_SIZE;
    double cd = WORLD_SIZE / GRID_SIZE;

    int originCx = (int)(qx / cw);
    int originCy = (int)(qy / ch);
    int originCz = (int)(qz / cd);
    if (originCx >= GRID_SIZE) originCx = GRID_SIZE - 1;
    if (originCy >= GRID_SIZE) originCy = GRID_SIZE - 1;
    if (originCz >= GRID_SIZE) originCz = GRID_SIZE - 1;
    if (originCx < 0) originCx = 0;
    if (originCy < 0) originCy = 0;
    if (originCz < 0) originCz = 0;

    int    bestId   = -1;
    double bestDist = 9999999.0;
    double bestX    = 0, bestY = 0, bestZ = 0;

    visited_nodes = 0;

    for (int r = 0; r < GRID_SIZE; r++) {
        double minRingDist = (r == 0) ? 0.0 : (r - 1) * cw;
        if (minRingDist > bestDist) break;

        int minCx = originCx - r, maxCx = originCx + r;
        int minCy = originCy - r, maxCy = originCy + r;
        int minCz = originCz - r, maxCz = originCz + r;
        
        if (minCx < 0)          minCx = 0;
        if (maxCx >= GRID_SIZE) maxCx = GRID_SIZE - 1;
        if (minCy < 0)          minCy = 0;
        if (maxCy >= GRID_SIZE) maxCy = GRID_SIZE - 1;
        if (minCz < 0)          minCz = 0;
        if (maxCz >= GRID_SIZE) maxCz = GRID_SIZE - 1;

        for (int i = minCx; i <= maxCx; i++) {
            for (int j = minCy; j <= maxCy; j++) {
                for (int k = minCz; k <= maxCz; k++) {
                    /* Ring perimeter check in 3D */
                    if (r > 0 && i > minCx && i < maxCx &&
                                 j > minCy && j < maxCy &&
                                 k > minCz && k < maxCz) continue;

                    BoundingBox3D searchBox = {
                        qx - bestDist, qy - bestDist, qz - bestDist,
                        qx + bestDist, qy + bestDist, qz + bestDist
                    };

                    /* Stack-based octree traversal */
                    OctreeNode* stack[2048];
                    int top = 0;
                    stack[top++] = grid[i][j][k].root;

                    while (top > 0) {
                        OctreeNode* node = stack[--top];
                        visited_nodes++;
                        if (!intersectsBox3D(node->boundary, searchBox)) continue;

                        for (int m = 0; m < node->object_count; m++) {
                            double d = pointDistance3D(qx, qy, qz,
                                node->objects[m]->x, node->objects[m]->y, node->objects[m]->z);
                            if (d < bestDist) {
                                bestDist = d;
                                bestId   = node->objects[m]->id;
                                bestX    = node->objects[m]->x;
                                bestY    = node->objects[m]->y;
                                bestZ    = node->objects[m]->z;
                                searchBox.minX = qx - bestDist;
                                searchBox.minY = qy - bestDist;
                                searchBox.minZ = qz - bestDist;
                                searchBox.maxX = qx + bestDist;
                                searchBox.maxY = qy + bestDist;
                                searchBox.maxZ = qz + bestDist;
                            }
                        }
                        if (node->is_divided && top + 8 < 2048) {
                            for (int c = 0; c < 8; c++) {
                                stack[top++] = node->children[c];
                            }
                        }
                    }
                }
            }
        }
    }

    if (bestId != -1)
        printf("%d %f %f %f\n", bestId, bestX, bestY, bestZ);
    fprintf(stderr, "visited=%ld\n", visited_nodes);
}

/* ── K nearest neighbours in 3D ──────────────────────────────────── */

typedef struct { int id; double dist, x, y, z; } Candidate;

static void heapSwap(Candidate* a, Candidate* b) {
    Candidate tmp = *a; *a = *b; *b = tmp;
}

static void siftDown(Candidate* heap, int size, int idx) {
    while (1) {
        int largest = idx;
        int l = 2 * idx + 1, r = 2 * idx + 2;
        if (l < size && heap[l].dist > heap[largest].dist) largest = l;
        if (r < size && heap[r].dist > heap[largest].dist) largest = r;
        if (largest == idx) break;
        heapSwap(&heap[idx], &heap[largest]);
        idx = largest;
    }
}

static void heapPush(Candidate* heap, int* size, Candidate c) {
    heap[(*size)++] = c;
    int i = *size - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap[parent].dist >= heap[i].dist) break;
        heapSwap(&heap[parent], &heap[i]);
        i = parent;
    }
}

static void heapReplace(Candidate* heap, int size, Candidate c) {
    heap[0] = c;
    siftDown(heap, size, 0);
}

static int cmpCandAsc(const void* a, const void* b) {
    double da = ((Candidate*)a)->dist;
    double db = ((Candidate*)b)->dist;
    return (da > db) - (da < db);
}

void executeKNN(double qx, double qy, double qz, int k) {
    if (k <= 0 || k > total_objects) k = total_objects;

    Candidate* heap = (Candidate*)malloc(k * sizeof(Candidate));
    int heap_size = 0;
    double searchRadius = 9999999.0;

    double cw = WORLD_SIZE / GRID_SIZE;
    double ch = WORLD_SIZE / GRID_SIZE;
    double cd = WORLD_SIZE / GRID_SIZE;

    int originCx = (int)(qx / cw);
    int originCy = (int)(qy / ch);
    int originCz = (int)(qz / cd);
    if (originCx >= GRID_SIZE) originCx = GRID_SIZE - 1;
    if (originCy >= GRID_SIZE) originCy = GRID_SIZE - 1;
    if (originCz >= GRID_SIZE) originCz = GRID_SIZE - 1;
    if (originCx < 0) originCx = 0;
    if (originCy < 0) originCy = 0;
    if (originCz < 0) originCz = 0;

    visited_nodes = 0;

    for (int r = 0; r < GRID_SIZE; r++) {
        double minRingDist = (r == 0) ? 0.0 : (r - 1) * cw;
        if (heap_size == k && minRingDist > searchRadius) break;

        int minCx = originCx - r, maxCx = originCx + r;
        int minCy = originCy - r, maxCy = originCy + r;
        int minCz = originCz - r, maxCz = originCz + r;
        
        if (minCx < 0)          minCx = 0;
        if (maxCx >= GRID_SIZE) maxCx = GRID_SIZE - 1;
        if (minCy < 0)          minCy = 0;
        if (maxCy >= GRID_SIZE) maxCy = GRID_SIZE - 1;
        if (minCz < 0)          minCz = 0;
        if (maxCz >= GRID_SIZE) maxCz = GRID_SIZE - 1;

        for (int i = minCx; i <= maxCx; i++) {
            for (int j = minCy; j <= maxCy; j++) {
                for (int kk = minCz; kk <= maxCz; kk++) {
                    /* Ring perimeter check in 3D */
                    if (r > 0 && i > minCx && i < maxCx &&
                                 j > minCy && j < maxCy &&
                                 kk > minCz && kk < maxCz) continue;

                    BoundingBox3D searchBox = {
                        qx - searchRadius, qy - searchRadius, qz - searchRadius,
                        qx + searchRadius, qy + searchRadius, qz + searchRadius
                    };

                    OctreeNode* stack[2048];
                    int top = 0;
                    stack[top++] = grid[i][j][kk].root;

                    while (top > 0) {
                        OctreeNode* node = stack[--top];
                        visited_nodes++;
                        if (!intersectsBox3D(node->boundary, searchBox)) continue;

                        for (int m = 0; m < node->object_count; m++) {
                            double d = pointDistance3D(qx, qy, qz,
                                node->objects[m]->x, node->objects[m]->y, node->objects[m]->z);

                            if (heap_size < k) {
                                Candidate c = {node->objects[m]->id, d,
                                               node->objects[m]->x,
                                               node->objects[m]->y,
                                               node->objects[m]->z};
                                heapPush(heap, &heap_size, c);
                                if (heap_size == k) {
                                    searchRadius = heap[0].dist;
                                    searchBox.minX = qx - searchRadius;
                                    searchBox.minY = qy - searchRadius;
                                    searchBox.minZ = qz - searchRadius;
                                    searchBox.maxX = qx + searchRadius;
                                    searchBox.maxY = qy + searchRadius;
                                    searchBox.maxZ = qz + searchRadius;
                                }
                            } else if (d < heap[0].dist) {
                                Candidate c = {node->objects[m]->id, d,
                                               node->objects[m]->x,
                                               node->objects[m]->y,
                                               node->objects[m]->z};
                                heapReplace(heap, heap_size, c);
                                searchRadius = heap[0].dist;
                                searchBox.minX = qx - searchRadius;
                                searchBox.minY = qy - searchRadius;
                                searchBox.minZ = qz - searchRadius;
                                searchBox.maxX = qx + searchRadius;
                                searchBox.maxY = qy + searchRadius;
                                searchBox.maxZ = qz + searchRadius;
                            }
                        }
                        if (node->is_divided && top + 8 < 2048) {
                            for (int c = 0; c < 8; c++) {
                                stack[top++] = node->children[c];
                            }
                        }
                    }
                }
            }
        }
    }

    /* Sort ascending by distance before printing */
    qsort(heap, heap_size, sizeof(Candidate), cmpCandAsc);
    for (int i = 0; i < heap_size; i++)
        printf("%d %f %f %f\n", heap[i].id, heap[i].x, heap[i].y, heap[i].z);

    fprintf(stderr, "visited=%ld results=%d\n", visited_nodes, heap_size);
    free(heap);
}
