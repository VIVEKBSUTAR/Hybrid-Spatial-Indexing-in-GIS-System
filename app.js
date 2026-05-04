const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

canvas.width = window.innerWidth - 300;
canvas.height = window.innerHeight;


const SERVER = "http://localhost:9000";

let objects = [];
let terrain = [];

let results = [];
let userPoint = null;

const GRID = 50;

function generateTerrain() {
    terrain = [];

    for (let i = 0; i < GRID; i++) {
        terrain[i] = [];
        for (let j = 0; j < GRID; j++) {
            terrain[i][j] = Math.random() > 0.75 ? 1 : 0;
        }
    }
}

async function generateWorld() {

    objects = [];
    results = [];
    userPoint = null;

    generateTerrain();

    await fetch(`${SERVER}/clear`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: "{}"
    });

    const types = ["hospital", "cafe", "school"];

    for (let i = 0; i < 300; i++) {

        const x = Math.random() * 1000;
        const y = Math.random() * 1000;

        const gx = Math.floor(x / 20);
        const gy = Math.floor(y / 20);

        const isTerrain = terrain[gx]?.[gy];

        const z = isTerrain ? 300 : 50;

        const obj = {
            id: i + 1,
            x, y, z,
            type: types[Math.floor(Math.random() * types.length)]
        };

        objects.push(obj);
    }

    await fetch(`${SERVER}/insert`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
         body: JSON.stringify({
            points: objects.map(o => ({
                id: o.id,   // 🔥 IMPORTANT
                x: o.x,
                y: o.y,
                z: o.z
            }))
        })
    });

    await fetch(`${SERVER}/build`, {
        method: "POST"
    });

    draw();
}

function draw() {
    ctx.clearRect(0,0,canvas.width,canvas.height);

    const cell = 1000/ GRID;

    // terrain
    for (let i = 0; i < GRID; i++) {
        for (let j = 0; j < GRID; j++) {
            if (terrain[i][j]) {
                ctx.fillStyle = "#3f1d1d";
                ctx.fillStyle = terrain[i][j] ? "#7f1d1d" : "#020617";
                ctx.fillRect(i*cell, j*cell, cell, cell);
            }
        }
    }

    // objects
    objects.forEach(o => {
        ctx.fillStyle =
            o.type === "hospital" ? "red" :
            o.type === "cafe" ? "green" :
            "blue";

        ctx.beginPath();
        ctx.arc(o.x, o.y, 4, 0, Math.PI*2);
        ctx.fill();
    });

    // results
    results.forEach(r => {
        ctx.strokeStyle = "yellow";
        ctx.beginPath();
        ctx.arc(r.x, r.y, 10, 0, Math.PI*2);
        ctx.stroke();
    });

    // user
    if (userPoint) {
        ctx.fillStyle = "white";
        ctx.beginPath();
        ctx.arc(userPoint.x, userPoint.y, 6, 0, Math.PI*2);
        ctx.fill();
    }
}

canvas.addEventListener("click", e => {
    const rect = canvas.getBoundingClientRect();

    userPoint = {
        x: e.clientX - rect.left,
        y: e.clientY - rect.top
    };

    draw();
});

function search() {
    const text = document.getElementById("search").value.toLowerCase();
    const k = parseInt(document.getElementById("k").value) || 3;

    if (!userPoint) {
        alert("Click location first");
        return;
    }

    let type = null;
    if (text.includes("hospital")) type = "hospital";
    if (text.includes("cafe")) type = "cafe";
    if (text.includes("school")) type = "school";

    if (text.includes("near")) {
        runRange(type);
    } 
    else if (text.includes("top") || text.includes("best")) {
        runKNN(type, k);
    } 
    else {
        runNN(type);   // fallback
    }

    document.getElementById("log").innerText =
    text.includes("near") ? "Range Query" :
    text.includes("top") ? "KNN Query" :
    "Nearest Neighbor";
}

async function runNN(type) {

    // 🔥 derive user height from terrain
    const gx = Math.floor(userPoint.x / 20);
    const gy = Math.floor(userPoint.y / 20);
    const userZ = terrain[gx]?.[gy] ? 300 : 50;

    const res = await fetch(`${SERVER}/knn`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({
            x: userPoint.x,
            y: userPoint.y,
            z: userZ,
            k: 10
        })
    });

    let data = await res.json();
    let pts = data.points || [];

    // 🔥 FIX: match by coordinates (NOT id)
    if (type) {
        pts = pts.filter(p => {
            const obj = objects.find(o =>
                Math.abs(o.x - p.x) < 1 &&
                Math.abs(o.y - p.y) < 1
            );
            return obj && obj.type === type;
        });
    }

    if (pts.length === 0) {
        results = [];
        draw();
        return;
    }

    results = [pts[0]];
    draw();
}

async function runKNN(type, k) {

    const gx = Math.floor(userPoint.x / 20);
    const gy = Math.floor(userPoint.y / 20);
    const userZ = terrain[gx]?.[gy] ? 300 : 50;

    // STEP 1 → QUADTREE
    const res2D = await fetch(`${SERVER}/knn`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({
            x: userPoint.x,
            y: userPoint.y,
            k: k * 3
        })
    });

    const data2D = await res2D.json();

    // STEP 2 → OCTREE
    const res3D = await fetch(`${SERVER}/knn`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({
            x: userPoint.x,
            y: userPoint.y,
            z: userZ,
            k: k * 3
        })
    });

    const data3D = await res3D.json();

    // 🔥 TRUE HYBRID MERGE
    let pts = [
        ...(data2D.points || []),
        ...(data3D.points || [])
    ];

    // 🔥 remove duplicates by coordinate
    const seen = new Set();
    pts = pts.filter(p => {
        const key = `${p.x}-${p.y}`;
        if (seen.has(key)) return false;
        seen.add(key);
        return true;
    });

    // 🔥 type filter using coordinate match
    if (type) {
        pts = pts.filter(p => {
            const obj = objects.find(o =>
                Math.abs(o.x - p.x) < 1 &&
                Math.abs(o.y - p.y) < 1
            );
            return obj && obj.type === type;
        });
    }

    results = pts.slice(0, k);
    draw();
}

async function runRange(type) {

    const gx = Math.floor(userPoint.x / 20);
    const gy = Math.floor(userPoint.y / 20);
    const userZ = terrain[gx]?.[gy] ? 300 : 50;

    // STEP 1 → QUADTREE
    const res2D = await fetch(`${SERVER}/range`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({
            box: {
                minX: userPoint.x - 100,
                minY: userPoint.y - 100,
                maxX: userPoint.x + 100,
                maxY: userPoint.y + 100
            }
        })
    });

    const data2D = await res2D.json();

    // STEP 2 → OCTREE
    const res3D = await fetch(`${SERVER}/range`, {
        method: "POST",
        headers: {"Content-Type":"application/json"},
        body: JSON.stringify({
            box: {
                minX: userPoint.x - 100,
                minY: userPoint.y - 100,
                minZ: 0,
                maxX: userPoint.x + 100,
                maxY: userPoint.y + 100,
                maxZ: 300
            }
        })
    });

    const data3D = await res3D.json();

    // 🔥 HYBRID MERGE
    let pts = [
        ...(data2D.points || []),
        ...(data3D.points || [])
    ];

    // remove duplicates
    const seen = new Set();
    pts = pts.filter(p => {
        const key = `${p.x}-${p.y}`;
        if (seen.has(key)) return false;
        seen.add(key);
        return true;
    });

    // type filter
    if (type) {
        pts = pts.filter(p => {
            const obj = objects.find(o =>
                Math.abs(o.x - p.x) < 1 &&
                Math.abs(o.y - p.y) < 1
            );
            return obj && obj.type === type;
        });
    }

    results = pts;
    draw();
}