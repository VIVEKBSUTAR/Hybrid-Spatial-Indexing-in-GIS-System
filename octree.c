#include "octree.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Geometry helper: check if point is inside bounding box */
bool containsPoint3D(BoundingBox3D box, double x, double y, double z) {
    return x >= box.minX && x <= box.maxX &&
           y >= box.minY && y <= box.maxY &&
           z >= box.minZ && z <= box.maxZ;
}

/* Geometry helper: AABB 3-D intersection */
bool intersectsBox3D(BoundingBox3D a, BoundingBox3D b) {
    /* Return false if any axis pair is disjoint */
    return !(a.maxX < b.minX || a.minX > b.maxX ||
             a.maxY < b.minY || a.minY > b.maxY ||
             a.maxZ < b.minZ || a.minZ > b.maxZ);
}

/* Euclidean distance in 3-D */
double pointDistance3D(double x1, double y1, double z1,
                       double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Create a new octree node */
OctreeNode* createOctreeNode(BoundingBox3D boundary) {
    OctreeNode* node = (OctreeNode*)malloc(sizeof(OctreeNode));
    node->boundary = boundary;
    node->object_count = 0;
    node->is_divided = false;
    for (int i = 0; i < 8; i++) {
        node->children[i] = NULL;
    }
    return node;
}

/* Subdivide octree node into 8 children */
void subdivideOctreeNode(OctreeNode* node) {
    if (node->is_divided) return;

    double midX = (node->boundary.minX + node->boundary.maxX) / 2.0;
    double midY = (node->boundary.minY + node->boundary.maxY) / 2.0;
    double midZ = (node->boundary.minZ + node->boundary.maxZ) / 2.0;

    /* Create 8 child boundaries */
    BoundingBox3D b0 = {node->boundary.minX, midY, midZ, midX, node->boundary.maxY, node->boundary.maxZ};      /* 0: Top-NW */
    BoundingBox3D b1 = {midX, midY, midZ, node->boundary.maxX, node->boundary.maxY, node->boundary.maxZ};      /* 1: Top-NE */
    BoundingBox3D b2 = {node->boundary.minX, node->boundary.minY, midZ, midX, midY, node->boundary.maxZ};      /* 2: Top-SW */
    BoundingBox3D b3 = {midX, node->boundary.minY, midZ, node->boundary.maxX, midY, node->boundary.maxZ};      /* 3: Top-SE */
    BoundingBox3D b4 = {node->boundary.minX, midY, node->boundary.minZ, midX, node->boundary.maxY, midZ};      /* 4: Bot-NW */
    BoundingBox3D b5 = {midX, midY, node->boundary.minZ, node->boundary.maxX, node->boundary.maxY, midZ};      /* 5: Bot-NE */
    BoundingBox3D b6 = {node->boundary.minX, node->boundary.minY, node->boundary.minZ, midX, midY, midZ};      /* 6: Bot-SW */
    BoundingBox3D b7 = {midX, node->boundary.minY, node->boundary.minZ, node->boundary.maxX, midY, midZ};      /* 7: Bot-SE */

    BoundingBox3D boxes[8] = {b0, b1, b2, b3, b4, b5, b6, b7};

    for (int i = 0; i < 8; i++) {
        node->children[i] = createOctreeNode(boxes[i]);
    }

    /* Push down existing objects */
    for (int i = 0; i < node->object_count; i++) {
        SpatialObject* obj = node->objects[i];
        for (int j = 0; j < 8; j++) {
            if (containsPoint3D(node->children[j]->boundary, obj->x, obj->y, obj->z)) {
                insertOctree(node->children[j], obj, 0);
                break;
            }
        }
    }

    node->object_count = 0;
    node->is_divided = true;
}

/* Insert object into octree */
bool insertOctree(OctreeNode* node, SpatialObject* obj, int depth) {
    if (!containsPoint3D(node->boundary, obj->x, obj->y, obj->z)) {
        return false;
    }

    if (!node->is_divided) {
        if (node->object_count < MAX_OBJECTS) {
            node->objects[node->object_count] = obj;
            node->object_count++;
            return true;
        }

        if (depth < MAX_DEPTH) {
            subdivideOctreeNode(node);
        } else {
            /* Max depth reached, store here if space available */
            if (node->object_count < MAX_OBJECTS) {
                node->objects[node->object_count] = obj;
                node->object_count++;
            }
            return true;
        }
    }

    /* Node is divided, insert into appropriate child */
    for (int i = 0; i < 8; i++) {
        if (insertOctree(node->children[i], obj, depth + 1)) {
            return true;
        }
    }

    return false;
}

/* Recursively gather octree statistics */
static void gatherStats(OctreeNode* node, int depth, int* total_nodes, 
                        int* max_depth, int* obj_count) {
    if (!node) return;

    (*total_nodes)++;
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    *obj_count += node->object_count;

    if (node->is_divided) {
        for (int i = 0; i < 8; i++) {
            gatherStats(node->children[i], depth + 1, total_nodes, max_depth, obj_count);
        }
    }
}

/* Get statistics from octree */
void getOctreeStats(OctreeNode* node, int depth,
                    int* total_nodes, int* max_depth, int* obj_count) {
    *total_nodes = 0;
    *max_depth = 0;
    *obj_count = 0;
    gatherStats(node, depth, total_nodes, max_depth, obj_count);
}

void dump_octree(OctreeNode* node, int depth) {
    if (!node) return;

    BoundingBox3D b = node->boundary;

    // compute center
    double cx = (b.minX + b.maxX) / 2.0;
    double cy = (b.minY + b.maxY) / 2.0;
    double cz = (b.minZ + b.maxZ) / 2.0;

    // compute half-size (cube assumption)
    double size = (b.maxX - b.minX) / 2.0;

    printf("%d %f %f %f %f\n",
        depth, cx, cy, cz, size
    );

    if (node->is_divided) {
        for (int i = 0; i < 8; i++) {
            dump_octree(node->children[i], depth + 1);
        }
    }
}

/* Print grid statistics across all 3-D grid cells */
void printGridStats(void) {
    int total_tree_nodes = 0;
    int max_tree_depth = 0;
    int total_obj_count = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int k = 0; k < GRID_SIZE; k++) {
                int nodes = 0, depth = 0, objs = 0;
                getOctreeStats(grid[i][j][k].root, 0, &nodes, &depth, &objs);
                total_tree_nodes += nodes;
                if (depth > max_tree_depth) max_tree_depth = depth;
                total_obj_count += objs;
            }
        }
    }

    fprintf(stderr, "STATS total_nodes=%d max_depth=%d total_objects=%d\n",
            total_tree_nodes, max_tree_depth, total_obj_count);
}
