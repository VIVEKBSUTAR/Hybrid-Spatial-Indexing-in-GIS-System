# Hybrid Spatial Indexing in GIS System

## Overview

This project is a Mini Geographic Information System (GIS) that implements hybrid spatial indexing using Grid-based indexing and Quadtree data structures. The goal is to efficiently store, organize, and query spatial data points.

The system demonstrates how advanced data structures improve the performance of spatial queries compared to naive approaches.

---

## Key Features

* Efficient storage of spatial data points
* Grid-based spatial indexing for uniform partitioning
* Quadtree-based hierarchical spatial indexing
* Fast query processing for spatial operations
* Modular architecture using multiple programming languages
* Basic web interface for interaction

---

## System Architecture

The project follows a multi-layer architecture:

### 1. Core Processing Layer (C)

* Implements spatial data structures
* Handles indexing and query processing
* Includes grid and quadtree implementations

### 2. Server Layer (Python)

* Acts as a bridge between frontend and backend
* Executes the compiled C program
* Handles request-response communication

### 3. Presentation Layer (HTML)

* Provides a simple user interface
* Allows users to input queries and view results

---

## Project Structure

```
MINI_GIS/
│
├── main.c              # Entry point of the application
├── types.h             # Common data structures
├── grid.c / grid.h     # Grid-based indexing implementation
├── quadtree.c / quadtree.h  # Quadtree implementation
├── queries.c / queries.h    # Query handling logic
├── points.txt          # Input dataset
├── server.py           # Python backend server
├── index.html          # Frontend interface
├── main                # Compiled executable (ignored in git)
└── README.md
```

---

## Concepts Used

* Spatial data structures
* Quadtree decomposition
* Grid indexing
* File handling in C
* Client-server architecture
* Modular programming

---

## Prerequisites

Make sure the following are installed:

* GCC compiler (for C programs)
* Python 3.x
* Git

---

## Setup and Installation

### 1. Clone the repository

```
git clone https://github.com/VIVEKBSUTAR/Hybrid-Spatial-Indexing-in-GIS-System.git
cd Hybrid-Spatial-Indexing-in-GIS-System
```

### 2. Compile the C program

```
gcc main.c grid.c quadtree.c queries.c -o main
```

### 3. Run the Python server

```
python server.py
```

### 4. Open the frontend

Open `index.html` in a browser.

---

## How It Works

1. Spatial data is loaded from `points.txt`
2. Data is indexed using:

   * Grid-based indexing
   * Quadtree structure
3. User sends queries through the frontend
4. Python server processes the request
5. C backend executes spatial queries
6. Results are returned and displayed

---

## Example Use Cases

* Finding points within a region
* Spatial range queries
* Demonstrating indexing performance

---

## Limitations

* Uses a static dataset
* Basic user interface
* No real-time map visualization

---

## Future Improvements

* Integration with map visualization libraries (e.g., Leaflet)
* Support for dynamic data insertion
* Nearest neighbor search optimization
* API-based interaction
* Use of real-world geographic coordinates

---


## License

This project is for academic and educational purposes.
