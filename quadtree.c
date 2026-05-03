#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "quadtree.h"

/* ── Geometry helpers ─────────────────────────────────────────────── */

bool containsPoint(BoundingBox box, double x, double y) {
    return (x >= box.minX && x <= box.maxX &&
            y >= box.minY && y <= box.maxY);
}

bool intersectsBox(BoundingBox a, BoundingBox b) {
    return !(b.minX > a.maxX || b.maxX < a.minX ||
             b.minY > a.maxY || b.maxY < a.minY);
}

double pointDistance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2, dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

/* ── Node creation ────────────────────────────────────────────────── */

QuadTreeNode* createNode(BoundingBox boundary) {
    QuadTreeNode* node = (QuadTreeNode*)malloc(sizeof(QuadTreeNode));
    node->boundary     = boundary;
    node->object_count = 0;
    node->is_divided   = false;
    node->nw = node->ne = node->sw = node->se = NULL;
    return node;
}

/* ── Subdivision ──────────────────────────────────────────────────── */

void subdivideNode(QuadTreeNode* node) {
    double midX = (node->boundary.minX + node->boundary.maxX) / 2.0;
    double midY = (node->boundary.minY + node->boundary.maxY) / 2.0;

    BoundingBox nwB = {node->boundary.minX, midY,               midX,               node->boundary.maxY};
    BoundingBox neB = {midX,               midY,               node->boundary.maxX, node->boundary.maxY};
    BoundingBox swB = {node->boundary.minX, node->boundary.minY, midX,               midY};
    BoundingBox seB = {midX,               node->boundary.minY, node->boundary.maxX, midY};

    node->nw = createNode(nwB);
    node->ne = createNode(neB);
    node->sw = createNode(swB);
    node->se = createNode(seB);
    node->is_divided = true;
}

/* ── Insertion ────────────────────────────────────────────────────── */

bool insertQuadTree(QuadTreeNode* node, SpatialObject* obj, int depth) {
    if (!containsPoint(node->boundary, obj->x, obj->y)) return false;

    if (node->object_count < MAX_OBJECTS || depth >= MAX_DEPTH) {
        node->objects[node->object_count++] = obj;
        return true;
    }

    if (!node->is_divided) {
        subdivideNode(node);
        /* Push existing objects down into children */
        for (int i = 0; i < node->object_count; i++) {
            SpatialObject* old = node->objects[i];
            if (insertQuadTree(node->nw, old, depth + 1)) continue;
            if (insertQuadTree(node->ne, old, depth + 1)) continue;
            if (insertQuadTree(node->sw, old, depth + 1)) continue;
            if (insertQuadTree(node->se, old, depth + 1)) continue;
        }
        node->object_count = 0;
    }

    if (insertQuadTree(node->nw, obj, depth + 1)) return true;
    if (insertQuadTree(node->ne, obj, depth + 1)) return true;
    if (insertQuadTree(node->sw, obj, depth + 1)) return true;
    if (insertQuadTree(node->se, obj, depth + 1)) return true;
    return false;
}

/* ── Statistics ───────────────────────────────────────────────────── */

/*
 * Recursively walk the quadtree and accumulate:
 *   total_nodes  - every QuadTreeNode allocated
 *   max_depth    - deepest level reached
 *   obj_count    - total SpatialObject pointers stored
 */
void getQuadTreeStats(QuadTreeNode* node, int depth,
                      int* total_nodes, int* max_depth, int* obj_count) {
    if (!node) return;
    (*total_nodes)++;
    if (depth > *max_depth) *max_depth = depth;
    *obj_count += node->object_count;

    if (node->is_divided) {
        getQuadTreeStats(node->nw, depth + 1, total_nodes, max_depth, obj_count);
        getQuadTreeStats(node->ne, depth + 1, total_nodes, max_depth, obj_count);
        getQuadTreeStats(node->sw, depth + 1, total_nodes, max_depth, obj_count);
        getQuadTreeStats(node->se, depth + 1, total_nodes, max_depth, obj_count);
    }
}

/*
 * Aggregate stats across all 10x10 grid cells and print to stderr
 * so the Python server can forward it alongside query results.
 */
void printGridStats(void) {
    int total_nodes = 0, max_depth = 0, obj_count = 0;
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            getQuadTreeStats(grid[i][j].root, 0,
                             &total_nodes, &max_depth, &obj_count);

    fprintf(stderr, "STATS total_nodes=%d max_depth=%d total_objects=%d\n",
            total_nodes, max_depth, obj_count);
}
