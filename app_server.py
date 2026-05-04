import http.server
import socketserver
import json
import subprocess
import os

PORT = 9000

# Paths resolved relative to this script so it works from any cwd
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_FILE  = os.path.join(SCRIPT_DIR, "points.txt")

if os.name == 'nt':
    OCTREE_ENGINE   = os.path.join(SCRIPT_DIR, "main_octree.exe")
    QUADTREE_ENGINE = os.path.join(SCRIPT_DIR, "main_quadtree.exe")
else:
    OCTREE_ENGINE   = os.path.join(SCRIPT_DIR, "main_octree")
    QUADTREE_ENGINE = os.path.join(SCRIPT_DIR, "main_quadtree")


def parse_stderr(text):
    meta = {}
    for token in text.split():
        if '=' in token:
            k, v = token.split('=', 1)
            meta[k] = v
    return meta


def parse_points_3d(stdout_text):
    points = []
    for line in stdout_text.strip().split('\n'):
        parts = line.split()
        if len(parts) == 4:
            try:
                points.append({
                    "id": int(parts[0]),
                    "x": float(parts[1]),
                    "y": float(parts[2]),
                    "z": float(parts[3])
                })
            except ValueError:
                pass
    return points


def parse_points_2d(stdout_text):
    points = []
    for line in stdout_text.strip().split('\n'):
        parts = line.split()
        if len(parts) == 3:
            try:
                points.append({
                    "id": int(parts[0]),
                    "x": float(parts[1]),
                    "y": float(parts[2]),
                    "z": 0.0
                })
            except ValueError:
                pass
    return points


class AppHandler(http.server.SimpleHTTPRequestHandler):

    # BUG-1 FIX: serve HTML/JS over HTTP instead of file://
    def do_GET(self):
        if self.path in ('/', '/application.html', ''):
            self._serve_file('application.html', 'text/html; charset=utf-8')
        elif self.path == '/app.js':
            self._serve_file('app.js', 'application/javascript; charset=utf-8')
        else:
            super().do_GET()

    def _serve_file(self, filename, content_type):
        filepath = os.path.join(SCRIPT_DIR, filename)
        try:
            with open(filepath, 'rb') as f:
                data = f.read()
            self.send_response(200)
            self.send_header('Content-type', content_type)
            self.send_header('Content-Length', len(data))
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(data)
        except FileNotFoundError:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b'Not found')

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        data = post_data.decode('utf-8')
        req = json.loads(data) if data.strip() else {}

        response = {}
        path = self.path

        # CLEAR
        if path == '/clear':
            open(DATA_FILE, 'wb').close()
            response = {"status": "cleared"}

        # BUG-2 + BUG-7 FIX: handle /quadtree/insert, write x y z (no id)
        elif path in ('/insert', '/quadtree/insert', '/octree/insert'):
            with open(DATA_FILE, 'w', newline='\n') as f:
                for pt in req.get('points', []):
                    f.write(f"{pt['x']} {pt['y']} {pt.get('z', 0.0)}\n")
            response = {"status": "inserted"}

        # BUG-3 FIX: /build is a no-op (C engines load per-call)
        elif path == '/build':
            response = {"status": "built"}

        # BUG-4 FIX: /octree/knn
        elif path in ('/knn', '/octree/knn'):
            cmd = [
                OCTREE_ENGINE, "knn",
                str(req["x"]), str(req["y"]), str(req.get("z", 100)),
                str(req["k"])
            ]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True,
                                      cwd=SCRIPT_DIR, timeout=10)
                meta = parse_stderr(proc.stderr)
                response = {
                    "points":        parse_points_3d(proc.stdout),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response = {"error": str(e), "points": []}

        # BUG-5 FIX: /quadtree/knn
        elif path == '/quadtree/knn':
            cmd = [
                QUADTREE_ENGINE, "knn",
                str(req["x"]), str(req["y"]),
                str(req["k"])
            ]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True,
                                      cwd=SCRIPT_DIR, timeout=10)
                meta = parse_stderr(proc.stderr)
                response = {
                    "points":        parse_points_2d(proc.stdout),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response = {"error": str(e), "points": []}

        # BUG-6 FIX: /octree/range
        elif path in ('/range', '/octree/range'):
            b = req["box"]
            cmd = [
                OCTREE_ENGINE, "range",
                str(b["minX"]), str(b["minY"]), str(b.get("minZ", 0)),
                str(b["maxX"]), str(b["maxY"]), str(b.get("maxZ", 1000))
            ]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True,
                                      cwd=SCRIPT_DIR, timeout=10)
                meta = parse_stderr(proc.stderr)
                response = {
                    "points":        parse_points_3d(proc.stdout),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response = {"error": str(e), "points": []}

        # BUG-6 FIX: /quadtree/range
        elif path == '/quadtree/range':
            b = req["box"]
            cmd = [
                QUADTREE_ENGINE, "range",
                str(b["minX"]), str(b["minY"]),
                str(b["maxX"]), str(b["maxY"])
            ]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True,
                                      cwd=SCRIPT_DIR, timeout=10)
                meta = parse_stderr(proc.stderr)
                response = {
                    "points":        parse_points_2d(proc.stdout),
                    "visited_nodes": int(meta.get("visited", 0)),
                    "time_ms":       meta.get("time_ms", "0")
                }
            except Exception as e:
                response = {"error": str(e), "points": []}

        else:
            response = {"error": f"unknown path: {path}"}

        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.end_headers()
        self.wfile.write(json.dumps(response).encode())

    def log_message(self, format, *args):
        super().log_message(format, *args)


# BUG-1 FIX: chdir so relative file paths resolve correctly
os.chdir(SCRIPT_DIR)

with socketserver.TCPServer(("", PORT), AppHandler) as httpd:
    open(DATA_FILE, "a").close()
    print(f"App Server running on http://localhost:{PORT}")
    print(f">>> Open:  http://localhost:{PORT}/application.html  <<<")
    print(f"Serving from: {SCRIPT_DIR}")
    httpd.serve_forever()