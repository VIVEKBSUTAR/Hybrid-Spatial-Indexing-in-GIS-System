#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"

#define MAX_OBJECTS 8
#define MAX_DEPTH   10

/* ─────────── Create Node ─────────── */
QuadtreeNode* createNode(BoundingBox2D boundary) {
    QuadtreeNode* node = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));

    node->boundary = boundary;
    node->object_count = 0;
    node->is_divided = false;

    for (int i = 0; i < 4; i++) {
        node->children[i] = NULL;
    }

    return node;
}

/* ─────────── Contains ─────────── */
bool contains2D(BoundingBox2D b, SpatialObject2D* obj) {
    return (obj->x >= b.minX && obj->x <= b.maxX &&
            obj->y >= b.minY && obj->y <= b.maxY);
}

/* ─────────── Intersects ─────────── */
bool intersects2D(BoundingBox2D a, BoundingBox2D b) {
    return !(b.minX > a.maxX || b.maxX < a.minX ||
             b.minY > a.maxY || b.maxY < a.minY);
}

/* ─────────── Subdivide ─────────── */
void subdivide(QuadtreeNode* node) {
    double midX = (node->boundary.minX + node->boundary.maxX) / 2;
    double midY = (node->boundary.minY + node->boundary.maxY) / 2;

    // NW
    node->children[0] = createNode((BoundingBox2D){
        node->boundary.minX, midY,
        midX, node->boundary.maxY
    });

    // NE
    node->children[1] = createNode((BoundingBox2D){
        midX, midY,
        node->boundary.maxX, node->boundary.maxY
    });

    // SW
    node->children[2] = createNode((BoundingBox2D){
        node->boundary.minX, node->boundary.minY,
        midX, midY
    });

    // SE
    node->children[3] = createNode((BoundingBox2D){
        midX, node->boundary.minY,
        node->boundary.maxX, midY
    });

    node->is_divided = true;
}

/* ─────────── Insert ─────────── */
bool insertQuadtree(QuadtreeNode* node, SpatialObject2D* obj, int depth) {

    if (!contains2D(node->boundary, obj))
        return false;

    // If space available
    if (node->object_count < MAX_OBJECTS || depth >= MAX_DEPTH) {
        node->objects[node->object_count++] = obj;
        return true;
    }

    // Subdivide if not already
    if (!node->is_divided) {
        subdivide(node);
    }

    // Try inserting into children
    for (int i = 0; i < 4; i++) {
        if (insertQuadtree(node->children[i], obj, depth + 1)) {
            return true;
        }
    }

    return false;
}

void getQuadTreeStats(QuadtreeNode* node, int depth,
                      int* total_nodes, int* max_depth, int* obj_count)
{
    if (!node) return;

    (*total_nodes)++;

    if (depth > *max_depth)
        *max_depth = depth;

    *obj_count += node->object_count;

    if (node->is_divided) {
        for (int i = 0; i < 4; i++) {
            getQuadTreeStats(node->children[i],
                             depth + 1,
                             total_nodes,
                             max_depth,
                             obj_count);
        }
    }
}

/* ─────────── Range Query ─────────── */
void rangeQuery2D(QuadtreeNode* node, BoundingBox2D range,
                  SpatialObject2D* results[], int* count) {

    if (!intersects2D(node->boundary, range))
        return;

    // Check objects in this node
    for (int i = 0; i < node->object_count; i++) {
        if (contains2D(range, node->objects[i])) {
            results[(*count)++] = node->objects[i];
        }
    }

    // Recurse
    if (node->is_divided) {
        for (int i = 0; i < 4; i++) {
            rangeQuery2D(node->children[i], range, results, count);
        }
    }
}

/* ─────────── Free Memory ─────────── */
void freeQuadtree(QuadtreeNode* node) {
    if (!node) return;

    if (node->is_divided) {
        for (int i = 0; i < 4; i++) {
            freeQuadtree(node->children[i]);
        }
    }

    free(node);
}