import http.server
import socketserver
import json
import subprocess
import os
import random


PORT = 8000
DATA_FILE = "points.txt"
ENGINE_CMD = "./main"

if os.name == 'nt':
    ENGINE_CMD = "main.exe"


def parse_stderr(stderr_text):
    """Parse key=value pairs out of stderr lines."""
    meta = {}
    for token in stderr_text.split():
        if '=' in token:
            k, v = token.split('=', 1)
            meta[k] = v
    return meta


class GISHandler(http.server.SimpleHTTPRequestHandler):

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        req = json.loads(post_data.decode('utf-8'))

        response_data = {}

        

        # ── Insert points (3D) ───────────────────────────────────────
        if self.path == '/insert':
            with open(DATA_FILE, "a") as f:
                for pt in req.get('points', []):
                    z = pt.get('z', 0.0)  # Default z to 0.0 if not provided
                    f.write(f"{pt['x']} {pt['y']} {z}\n")
            response_data = {"status": "success"}

        # ── Clear all data ───────────────────────────────────────────
        elif self.path == '/clear':
            open(DATA_FILE, "w").close()
            response_data = {"status": "cleared"}

        # ── Generate random dataset (3D) ────────────────────────────
        elif self.path == '/generate':
            n = req.get('n', 1000)
            cmd = [ENGINE_CMD, "generate", str(n)]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True)
                response_data = {"status": "generated", "n": n,
                                 "message": proc.stderr.strip()}
            except Exception as e:
                response_data = {"error": str(e)}

        # ── Octree stats ────────────────────────────────────────────
        elif self.path == '/stats':
            cmd = [ENGINE_CMD, "stats"]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True)
                meta = parse_stderr(proc.stderr)
                response_data = {
                    "total_nodes":   int(meta.get("total_nodes", 0)),
                    "max_depth":     int(meta.get("max_depth", 0)),
                    "total_objects": int(meta.get("total_objects", 0)),
                    "total_loaded":  int(meta.get("total_loaded", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response_data = {"error": str(e)}

                # ── Dump octree nodes ────────────────────────────
        elif self.path == '/dump':
            cmd = [ENGINE_CMD, "dump"]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, check=True)

                nodes = []
                for line in proc.stdout.strip().split('\n'):
                    parts = line.split()
                    if len(parts) == 5:
                        nodes.append({
                            "depth": int(parts[0]),
                            "x": float(parts[1]),
                            "y": float(parts[2]),
                            "z": float(parts[3]),
                            "size": float(parts[4])
                        })

                response_data = {
                    "nodes": nodes,
                    "count": len(nodes)
                }

            except Exception as e:
                response_data = {"error": str(e)}

        # ── Indexed 3D range query ──────────────────────────────────
        elif self.path == '/range':
            box = req['box']
            minZ = box.get('minZ', 0.0)
            maxZ = box.get('maxZ', 1000.0)
            cmd = [ENGINE_CMD, "range",
                   str(box['minX']), str(box['minY']), str(minZ),
                   str(box['maxX']), str(box['maxY']), str(maxZ)]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
                points = _parse_points_3d(proc.stdout)
                meta = parse_stderr(proc.stderr)
                response_data = {
                    "points":        points,
                    "result_count":  len(points),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response_data = {"error": str(e)}

        # ── Naive 3D range query (baseline comparison) ───────────────
        elif self.path == '/range_naive':
            box = req['box']
            minZ = box.get('minZ', 0.0)
            maxZ = box.get('maxZ', 1000.0)
            cmd = [ENGINE_CMD, "range_naive",
                   str(box['minX']), str(box['minY']), str(minZ),
                   str(box['maxX']), str(box['maxY']), str(maxZ)]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
                points = _parse_points_3d(proc.stdout)
                meta = parse_stderr(proc.stderr)
                response_data = {
                    "points":        points,
                    "result_count":  len(points),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response_data = {"error": str(e)}

        # ── Nearest neighbour (3D) ──────────────────────────────────
        elif self.path == '/nn':
            z = req.get('z', 500.0)  # Default z to 500 if not provided
            cmd = [ENGINE_CMD, "nn", str(req['x']), str(req['y']), str(z)]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
                meta = parse_stderr(proc.stderr)
                line = proc.stdout.strip()
                point = None
                if line:
                    parts = line.split()
                    if len(parts) == 4:
                        point = {"id": int(parts[0]),
                                 "x": float(parts[1]),
                                 "y": float(parts[2]),
                                 "z": float(parts[3])}
                response_data = {
                    "point":         point,
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response_data = {"error": str(e)}

        # ── K nearest neighbours (3D) ──────────────────────────────
        elif self.path == '/knn':
            z = req.get('z', 500.0)  # Default z to 500 if not provided
            cmd = [ENGINE_CMD, "knn",
                   str(req['x']), str(req['y']), str(z), str(req['k'])]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
                points = _parse_points_3d(proc.stdout)
                meta = parse_stderr(proc.stderr)
                response_data = {
                    "points":        points,
                    "result_count":  len(points),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response_data = {"error": str(e)}

        # ── Random sample for display (avoids sending full dataset) ─────────
        elif self.path == '/sample':
            n_sample = req.get('n', 5000)
            try:
                with open(DATA_FILE, 'r') as f:
                    lines = [l.strip() for l in f if l.strip()]
                if len(lines) > n_sample:
                    lines = random.sample(lines, n_sample)
                points = []
                for idx, line in enumerate(lines):
                    parts = line.split()
                    if len(parts) >= 3:
                        x, y = float(parts[0]), float(parts[1])
                        z = float(parts[2]) if len(parts) >= 3 else 0.0
                        points.append({'id': idx + 1, 'x': x, 'y': y, 'z': z})
                response_data = {
                    'points': points,
                    'total_sampled': len(points),
                    'note': f'Showing {len(points)} of {len(lines)} total points'
                }
            except Exception as e:
                response_data = {'error': str(e)}

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(response_data).encode('utf-8'))

    def log_message(self, format, *args):
        pass  # suppress per-request noise in terminal


def _parse_points_3d(stdout_text):
    """Parse 3D points from stdout (id x y z format)."""
    points = []
    for line in stdout_text.strip().split('\n'):
        if line:
            parts = line.split()
            if len(parts) == 4:
                points.append({"id": int(parts[0]),
                                "x": float(parts[1]),
                                "y": float(parts[2]),
                                "z": float(parts[3])})
    return points


def _parse_points(stdout_text):
    """Parse 2D points from stdout for backward compatibility."""
    points = []
    for line in stdout_text.strip().split('\n'):
        if line:
            parts = line.split()
            if len(parts) == 3:
                points.append({"id": int(parts[0]),
                                "x": float(parts[1]),
                                "y": float(parts[2])})
    return points


with socketserver.TCPServer(("", PORT), GISHandler) as httpd:
    open(DATA_FILE, "a").close()
    print(f"Server running on http://localhost:{PORT}")
    print("Compile:  gcc -O2 -o main.exe main.c octree.c grid.c queries.c -lm")
    httpd.serve_forever()
