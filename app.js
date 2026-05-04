const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

// ── Canvas size fills remaining space ────────────────────────────────
function resizeCanvas() {
    canvas.width  = window.innerWidth - 300;
    canvas.height = window.innerHeight;
}
resizeCanvas();
window.addEventListener("resize", () => { resizeCanvas(); draw(); });

const SERVER = "";   // same origin — served by app_server.py

const WORLD = 1000;  // world coordinate space is 0-1000

let objects   = [];
let terrain   = [];
let results   = [];
let userPoint = null;

const GRID = 50;  // terrain grid cells

// ── Coordinate scaling: world [0,1000] <-> canvas pixels ─────────────
function toCanvasX(wx) { return (wx / WORLD) * canvas.width;  }
function toCanvasY(wy) { return (wy / WORLD) * canvas.height; }
function toWorldX(cx)  { return (cx / canvas.width)  * WORLD; }
function toWorldY(cy)  { return (cy / canvas.height) * WORLD; }

// ── Terrain generation ───────────────────────────────────────────────
function generateTerrain() {
    terrain = [];
    for (let i = 0; i < GRID; i++) {
        terrain[i] = [];
        for (let j = 0; j < GRID; j++) {
            terrain[i][j] = Math.random() > 0.75 ? 1 : 0;
        }
    }
}

// ── Generate world ───────────────────────────────────────────────────
async function generateWorld() {
    objects   = [];
    results   = [];
    userPoint = null;

    generateTerrain();

    await fetch(`${SERVER}/clear`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: "{}"
    });

    const types = ["hospital", "cafe", "school"];

    for (let i = 0; i < 300; i++) {
        const x  = Math.random() * WORLD;
        const y  = Math.random() * WORLD;
        const gx = Math.floor(x / (WORLD / GRID));
        const gy = Math.floor(y / (WORLD / GRID));
        const z  = terrain[gx]?.[gy] ? 300 : 50;

        objects.push({
            id:   i + 1,
            x, y, z,
            type: types[Math.floor(Math.random() * types.length)]
        });
    }

    // Write to server — x y z only (no id), plain \n line endings
    await fetch(`${SERVER}/quadtree/insert`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({
            points: objects.map(o => ({ id: o.id, x: o.x, y: o.y, z: o.z }))
        })
    });

    await fetch(`${SERVER}/build`, { method: "POST" });

    document.getElementById("log").innerText = `Generated ${objects.length} objects`;
    draw();
}

// ── Draw scene ────────────────────────────────────────────────────────
function draw() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const cellW = canvas.width  / GRID;
    const cellH = canvas.height / GRID;

    // Terrain
    for (let i = 0; i < GRID; i++) {
        for (let j = 0; j < GRID; j++) {
            if (terrain[i] && terrain[i][j]) {
                ctx.fillStyle = "#7f1d1d";
                ctx.fillRect(i * cellW, j * cellH, cellW, cellH);
            }
        }
    }

    // All objects
    objects.forEach(o => {
        ctx.fillStyle =
            o.type === "hospital" ? "red"   :
            o.type === "cafe"     ? "green" : "blue";
        ctx.beginPath();
        ctx.arc(toCanvasX(o.x), toCanvasY(o.y), 4, 0, Math.PI * 2);
        ctx.fill();
    });

    // Result highlights (yellow ring)
    results.forEach(r => {
        ctx.strokeStyle = "yellow";
        ctx.lineWidth   = 2;
        ctx.beginPath();
        ctx.arc(toCanvasX(r.x), toCanvasY(r.y), 12, 0, Math.PI * 2);
        ctx.stroke();

        // Label
        ctx.fillStyle  = "yellow";
        ctx.font       = "11px monospace";
        ctx.fillText(`#${r.id}`, toCanvasX(r.x) + 14, toCanvasY(r.y) + 4);
    });

    // User click point
    if (userPoint) {
        ctx.fillStyle   = "white";
        ctx.strokeStyle = "#aaa";
        ctx.lineWidth   = 1.5;
        ctx.beginPath();
        ctx.arc(toCanvasX(userPoint.wx), toCanvasY(userPoint.wy), 7, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();
    }
}

// ── Canvas click → store world coords ────────────────────────────────
canvas.addEventListener("click", e => {
    const rect = canvas.getBoundingClientRect();
    const cx   = e.clientX - rect.left;
    const cy   = e.clientY - rect.top;

    userPoint = {
        cx, cy,                 // canvas pixels
        wx: toWorldX(cx),       // world coords 0-1000
        wy: toWorldY(cy)
    };

    draw();
});

// ── Search dispatcher ─────────────────────────────────────────────────
function search() {
    const text = document.getElementById("search").value.toLowerCase().trim();
    const k    = parseInt(document.getElementById("k").value) || 3;

    if (!userPoint) { alert("Click a location on the map first"); return; }
    if (objects.length === 0) { alert("Generate the world first"); return; }

    let type = null;
    if (text.includes("hospital")) type = "hospital";
    if (text.includes("cafe"))     type = "cafe";
    if (text.includes("school"))   type = "school";

    if (text.includes("near")) {
        document.getElementById("log").innerText = "Range Query…";
        runRange(type);
    } else if (text.includes("top") || text.includes("best")) {
        document.getElementById("log").innerText = "KNN Query…";
        runKNN(type, k);
    } else {
        document.getElementById("log").innerText = "Nearest Neighbor…";
        runNN(type);
    }
}

// ── Derive Z from terrain at world coords ─────────────────────────────
function userZ() {
    const gx = Math.floor(userPoint.wx / (WORLD / GRID));
    const gy = Math.floor(userPoint.wy / (WORLD / GRID));
    return (terrain[gx]?.[gy]) ? 300 : 50;
}

// ── Deduplicate by x,y ───────────────────────────────────────────────
function dedup(pts) {
    const seen = new Set();
    return pts.filter(p => {
        const key = `${p.x.toFixed(2)}-${p.y.toFixed(2)}`;
        if (seen.has(key)) return false;
        seen.add(key);
        return true;
    });
}

// ── Type filter using coordinate match ───────────────────────────────
function filterByType(pts, type) {
    if (!type) return pts;
    return pts.filter(p => {
        const obj = objects.find(o =>
            Math.abs(o.x - p.x) < 0.5 &&
            Math.abs(o.y - p.y) < 0.5
        );
        return obj && obj.type === type;
    });
}

// ── NN (nearest neighbor via octree KNN k=10 then pick first) ────────
async function runNN(type) {
    const z = userZ();

    const res = await fetch(`${SERVER}/octree/knn`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({ x: userPoint.wx, y: userPoint.wy, z, k: 20 })
    });

    const data = await res.json();
    let pts = dedup(data.points || []);
    pts = filterByType(pts, type);

    results = pts.slice(0, 1);
    document.getElementById("log").innerText =
        results.length > 0
            ? `Nearest: ID ${results[0].id}  (${results[0].x.toFixed(0)}, ${results[0].y.toFixed(0)})`
            : "No result found";
    draw();
}

// ── KNN: hybrid quadtree + octree merge ──────────────────────────────
async function runKNN(type, k) {
    const z = userZ();

    // Step 1: Quadtree 2D
    const res2D = await fetch(`${SERVER}/quadtree/knn`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({ x: userPoint.wx, y: userPoint.wy, k: k * 3 })
    });
    const data2D = await res2D.json();

    // Step 2: Octree 3D
    const res3D = await fetch(`${SERVER}/octree/knn`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({ x: userPoint.wx, y: userPoint.wy, z, k: k * 3 })
    });
    const data3D = await res3D.json();

    let pts = dedup([...(data2D.points || []), ...(data3D.points || [])]);
    pts = filterByType(pts, type);

    results = pts.slice(0, k);
    document.getElementById("log").innerText =
        `KNN: ${results.length} result(s) found`;
    draw();
}

// ── Range: hybrid quadtree + octree ──────────────────────────────────
async function runRange(type) {
    const z   = userZ();
    const R   = 150;  // world-unit radius
    const wx  = userPoint.wx;
    const wy  = userPoint.wy;

    // Step 1: Quadtree 2D range
    const res2D = await fetch(`${SERVER}/quadtree/range`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({
            box: { minX: wx-R, minY: wy-R, maxX: wx+R, maxY: wy+R }
        })
    });
    const data2D = await res2D.json();

    // Step 2: Octree 3D range
    const res3D = await fetch(`${SERVER}/octree/range`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({
            box: { minX: wx-R, minY: wy-R, minZ: 0,
                   maxX: wx+R, maxY: wy+R, maxZ: 1000 }
        })
    });
    const data3D = await res3D.json();

    let pts = dedup([...(data2D.points || []), ...(data3D.points || [])]);
    pts = filterByType(pts, type);

    results = pts;
    document.getElementById("log").innerText =
        `Range: ${results.length} result(s) within ${R} units`;
    draw();
}