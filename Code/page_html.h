#ifndef PAGE_HTML_H
#define PAGE_HTML_H
#include <Arduino.h>

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>RMG Entrepôt - Robot Autonome</title>
<style>
  :root{
    --bg:#e9ecef;            /* light gray canvas */
    --panel:#ffffff;
    --header:#3a4149;        /* dark slate header */
    --header2:#2f353c;
    --accent:#2ab6d9;        /* signature cyan */
    --accent-dark:#1f9bbd;
    --gauge-track:#d4d9dd;
    --gauge-inner1:#5b646c;
    --text:#5a636b;
    --text-dark:#3a4149;
    --text-muted:#9aa3ab;
    --ok:#2ab6d9;
    --warn:#e8a13a;
    --bad:#d9534f;
    --footer-btn:#aeb6bd;
    --footer-btn-active:#2ab6d9;
    --shadow:0 1px 3px rgba(0,0,0,.12), 0 1px 2px rgba(0,0,0,.08);
  }
  *{box-sizing:border-box;margin:0;padding:0}
  html,body{height:100%}
  body{
    font-family:"Segoe UI",Tahoma,Geneva,Verdana,sans-serif;
    background:var(--bg);
    color:var(--text-dark);
    display:flex;flex-direction:column;
    min-height:100vh;
    font-size:14px;
  }

  /* ===== HEADER ===== */
  .topbar{
    background:var(--header);
    color:#fff;
    display:flex;align-items:center;
    padding:10px 18px;
    gap:14px;
  }
  .topbar .logo{
    width:26px;height:26px;border:2px solid var(--accent);
    border-radius:5px;display:flex;align-items:center;justify-content:center;
    color:var(--accent);font-weight:700;font-size:13px;
  }
  .topbar h1{font-size:16px;font-weight:600;letter-spacing:.3px}
  .topbar .spacer{flex:1}
  .topbar .win-dots{display:flex;gap:8px;opacity:.5}
  .topbar .win-dots span{width:11px;height:11px;border:1px solid #fff;border-radius:2px;display:block}

  /* ===== INFO STRIP ===== */
  .infostrip{
    background:var(--panel);
    display:flex;align-items:stretch;
    border-bottom:1px solid #dfe3e7;
    box-shadow:var(--shadow);
  }
  .infostrip .cell{
    display:flex;align-items:center;gap:10px;
    padding:12px 20px;flex:1;
    border-right:1px solid #eef1f3;
  }
  .infostrip .cell:last-child{border-right:none}
  .infostrip .chev{color:var(--accent);font-size:18px;font-weight:700}
  .infostrip .status-dot{
    width:30px;height:30px;border-radius:50%;
    background:var(--accent);
    display:flex;align-items:center;justify-content:center;color:#fff;font-size:15px;
    transition:background .3s;
  }
  .infostrip .lbl{font-size:11px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.4px}
  .infostrip .val{font-size:14px;color:var(--text-dark);font-weight:600;margin-top:2px}

  /* ===== MAIN ===== */
  .main{flex:1;padding:22px;display:flex;flex-direction:column;gap:20px}

  /* ===== MAP PANEL ===== */
  .mappanel{
    background:var(--panel);border-radius:3px;box-shadow:var(--shadow);
    padding:20px 22px;display:grid;grid-template-columns:1fr 230px;gap:22px;
  }
  .map-head{display:flex;justify-content:space-between;align-items:flex-start;flex-wrap:wrap;gap:10px;margin-bottom:14px}
  .map-head h3{font-size:15px;font-weight:600;color:var(--text-dark)}
  .map-legend{display:flex;flex-wrap:wrap;gap:14px;font-size:11px;color:var(--text-muted)}
  .map-legend span{display:flex;align-items:center;gap:5px}
  .lg{width:12px;height:12px;border-radius:3px;display:inline-block}
  .lg-robot{background:#f5a623;border-radius:50%}
  .lg-a{background:#2ec9a0}
  .lg-b{background:#a06bff}
  .lg-station{background:#ff6b4a}
  .lg-charge{background:#f5c518}
  .lg-visited{background:#eaf0f6;border:1px solid #c5d0db}

  .map-stage{
    background:#ffffff;border:1px solid #e2e8ef;border-radius:8px;
    padding:14px;display:flex;align-items:center;justify-content:center;
    aspect-ratio:1/1;max-height:440px;margin:0 auto;width:100%;
    box-shadow:inset 0 0 0 1px #f1f4f7;
  }
  .map-stage svg{display:block;max-width:440px;width:100%;height:auto}

  /* KPI side column */
  .map-kpis{display:flex;flex-direction:column;gap:12px}
  .kpi{
    background:#f6f8fa;border:1px solid #e8ecef;border-radius:4px;padding:12px 14px;
  }
  .kpi-lbl{font-size:11px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.4px}
  .kpi-val{font-size:24px;font-weight:700;color:var(--text-dark);margin-top:3px}
  .kpi-bar{height:6px;background:var(--gauge-track);border-radius:3px;margin-top:9px;overflow:hidden}
  .kpi-bar span{display:block;height:100%;width:0;background:var(--accent);border-radius:3px;transition:width .8s cubic-bezier(.4,0,.2,1)}
  .kpi-pos .kpi-val{font-size:20px;color:var(--accent)}

  /* Emergency stop button */
  .estop{
    margin-top:4px;width:100%;border:none;cursor:pointer;
    background:#d9342b;color:#fff;border-radius:6px;
    padding:14px 12px;display:flex;align-items:center;justify-content:center;gap:9px;
    font-size:14px;font-weight:800;letter-spacing:.6px;font-family:inherit;
    box-shadow:0 3px 0 #a8231c, 0 4px 10px rgba(217,52,43,.35);
    transition:transform .08s, box-shadow .08s, background .15s;
  }
  .estop:hover{background:#e23b31}
  .estop:active{transform:translateY(3px);box-shadow:0 0 0 #a8231c, 0 2px 6px rgba(217,52,43,.3)}
  .estop-icon{font-size:18px;line-height:1}
  .estop.armed{
    background:#7a8794;box-shadow:0 3px 0 #5e6873, 0 4px 10px rgba(0,0,0,.2);
    animation:none;
  }
  .estop.tripped{
    background:#9c1b14;box-shadow:0 0 0 #7a140f, inset 0 0 0 2px #ff6b60;
    animation:estopBlink 1s steps(1) infinite;
  }
  @keyframes estopBlink{50%{background:#d9342b}}

  /* ===== STATISTICS ROW ===== */
  .stats{display:grid;grid-template-columns:repeat(3,1fr);gap:20px}
  .stat-card{
    background:var(--panel);border-radius:3px;box-shadow:var(--shadow);
    padding:16px 18px;min-height:120px;position:relative;
  }
  .stat-card h3{font-size:15px;font-weight:600;color:var(--text-dark);margin-bottom:4px}
  .stat-card .sub{font-size:12px;color:var(--text-muted);margin-bottom:12px}
  .stat-card .edit{
    position:absolute;top:14px;right:14px;color:var(--accent);
    width:26px;height:26px;border-radius:50%;background:#eef9fc;
    display:flex;align-items:center;justify-content:center;font-size:13px;cursor:pointer;
  }
  .stat-row{display:flex;justify-content:space-between;padding:5px 0;border-bottom:1px solid #f0f3f5;font-size:13px}
  .stat-row:last-child{border-bottom:none}
  .stat-row .k{color:var(--text-muted)}
  .stat-row .v{color:var(--text-dark);font-weight:600}
  .badge{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:6px;vertical-align:middle}

  /* mini log */
  .logbox{max-height:78px;overflow:auto;font-family:"Consolas",monospace;font-size:11px;color:var(--text)}
  .logbox div{padding:1px 0;border-bottom:1px dotted #eef1f3}

  /* ===== FOOTER NAV ===== */
  .footer{
    background:var(--header2);
    display:flex;gap:10px;padding:12px 18px;align-items:center;
  }
  .footer button{
    flex:1;border:none;border-radius:2px;padding:11px 0;
    background:var(--footer-btn);color:#fff;font-size:13px;font-weight:600;
    cursor:pointer;transition:background .2s;letter-spacing:.3px;
  }
  .footer button:hover{background:#97a0a8}
  .footer button.active{background:var(--footer-btn-active)}
  .footer button.overview{background:transparent;border:1px solid #5a626a;color:#cfd5da;flex:1.3}

  ::-webkit-scrollbar{width:6px;height:6px}
  ::-webkit-scrollbar-thumb{background:#c4cacf;border-radius:3px}

  @media(max-width:820px){
    .mappanel{grid-template-columns:1fr}
    .map-kpis{flex-direction:row;flex-wrap:wrap}
    .map-kpis .kpi{flex:1;min-width:120px}
    .stats{grid-template-columns:1fr}
  }
</style>
</head>
<body>

  <!-- HEADER -->
  <div class="topbar">
    <div class="logo">R</div>
    <h1>RMG Entrepôt - Robot Autonome</h1>
    <div class="spacer"></div>
    <div class="win-dots"><span></span><span></span><span></span></div>
  </div>

  <!-- INFO STRIP -->
  <div class="infostrip">
    <div class="cell">
      <div class="status-dot" id="connDot">●</div>
      <div>
        <div class="lbl">État</div>
        <div class="val" id="connState">Connecté</div>
      </div>
    </div>
    <div class="cell">
      <div class="chev">»</div>
      <div>
        <div class="lbl">Mission en cours</div>
        <div class="val" id="missionVal">Départ → A</div>
      </div>
    </div>
    <div class="cell">
      <div class="chev">»</div>
      <div>
        <div class="lbl">Position</div>
        <div class="val" id="posVal">Cellule B2</div>
      </div>
    </div>
    <div class="cell">
      <div class="chev">»</div>
      <div>
        <div class="lbl">Date et heure</div>
        <div class="val" id="dtVal">—</div>
      </div>
    </div>
  </div>

  <!-- MAIN -->
  <div class="main">

    <!-- MAP PANEL -->
    <div class="mappanel">
      <div class="map-left">
        <div class="map-head">
          <h3>Carte de l'entrepôt</h3>
          <div class="map-legend">
            <span><i class="lg lg-robot"></i>Robot</span>
            <span><i class="lg lg-a"></i>Block A</span>
            <span><i class="lg lg-b"></i>Block B</span>
            <span><i class="lg lg-station"></i>Départ</span>
            <span><i class="lg lg-charge"></i>Charge</span>
            <span><i class="lg lg-visited"></i>Visitée</span>
          </div>
        </div>
        <div class="map-stage">
          <svg id="mapSvg" viewBox="0 0 440 440" width="100%" height="100%"></svg>
        </div>
      </div>

      <div class="map-kpis">
        <div class="kpi">
          <div class="kpi-lbl">Batterie</div>
          <div class="kpi-val" id="g1big">82,4 %</div>
          <div class="kpi-bar"><span id="g1bar" style="width:82.4%"></span></div>
        </div>
        <div class="kpi">
          <div class="kpi-lbl">Vitesse</div>
          <div class="kpi-val" id="g2big">74,6 %</div>
          <div class="kpi-bar"><span id="g2bar" style="width:74.6%"></span></div>
        </div>
        <div class="kpi">
          <div class="kpi-lbl">Précision de cap</div>
          <div class="kpi-val" id="g3big">92,1 %</div>
          <div class="kpi-bar"><span id="g3bar" style="width:92.1%"></span></div>
        </div>
        <div class="kpi kpi-pos">
          <div class="kpi-lbl">Cellule courante</div>
          <div class="kpi-val" id="kpiCell">B2</div>
        </div>
        <button class="estop" id="estopBtn" type="button">
          <span class="estop-icon">⏻</span>
          <span class="estop-txt">ARRÊT D'URGENCE</span>
        </button>
      </div>
    </div>

    <!-- STATISTICS -->
    <div class="stats">
      <div class="stat-card">
        <h3>Télémétrie</h3>
        <div class="sub">Capteurs temps réel · 50 Hz</div>
        <div class="stat-row"><span class="k">Cap (gyro)</span><span class="v" id="tCap">182,4°</span></div>
        <div class="stat-row"><span class="k">Vitesse linéaire</span><span class="v" id="tSpeed">0,21 m/s</span></div>
        <div class="stat-row"><span class="k">Erreur de cap ε</span><span class="v" id="tErr">0,6°</span></div>
        <div class="stat-row"><span class="k">Distance avant</span><span class="v" id="tDistF">138 mm</span></div>
        <div class="stat-row"><span class="k">Distance droite</span><span class="v" id="tDistR">96 mm</span></div>
        <div class="stat-row"><span class="k">Distance gauche</span><span class="v" id="tDistL">204 mm</span></div>
        <div class="stat-row"><span class="k">PWM G / D</span><span class="v" id="tPwm">148 / 142</span></div>
      </div>

      <div class="stat-card">
        <h3>État système</h3>
        <div class="sub">Surveillance & alimentation</div>
        <div class="stat-row"><span class="k"><span class="badge" id="bTemp" style="background:var(--ok)"></span>Température (LM35)</span><span class="v" id="tTemp">31,4 °C</span></div>
        <div class="stat-row"><span class="k"><span class="badge" id="bGas" style="background:var(--ok)"></span>Gaz (MQ-5)</span><span class="v" id="tGas">0 %</span></div>
        <div class="stat-row"><span class="k">Tension batterie</span><span class="v" id="tVolt">11,7 V</span></div>
        <div class="stat-row"><span class="k">Courant moteurs</span><span class="v" id="tCur">0,84 A</span></div>
        <div class="stat-row"><span class="k">Charge CPU (ESP32)</span><span class="v" id="tCpu">37 %</span></div>
        <div class="stat-row"><span class="k">Cellules visitées</span><span class="v" id="tCells">6 / 16</span></div>
      </div>

      <div class="stat-card">
        <h3>Journal</h3>
        <div class="sub">Derniers messages</div>
        <div class="logbox" id="logBox"></div>
      </div>
    </div>

  </div>

  <!-- FOOTER NAV -->
  <div class="footer">
    <button class="overview">Vue d'ensemble</button>
    <button class="active" data-view="dashboard">Tableau de bord</button>
    <button data-view="map">Carte</button>
    <button data-view="motors">Moteurs</button>
    <button data-view="sensors">Capteurs</button>
    <button data-view="logs">Logs</button>
  </div>

<script>
/* =========================================================
   KPI BARS  (battery / speed / heading accuracy)
   ========================================================= */
function setGauge(id,pct){
  pct=Number(pct); if(isNaN(pct)) return;
  pct=Math.max(0,Math.min(100,pct));
  const bar=document.getElementById(id+"bar");
  const big=document.getElementById(id+"big");
  if(bar) bar.style.width = pct+"%";
  if(big) big.textContent = pct.toFixed(1)+" %";
}

/* =========================================================
   4x4 GRID MAZE MAP  (dark navy, rounded cyan walls,
   labeled cells A1..D4, yellow robot dot — no emojis)
   Cell coordinates: (col, row) with row 0 at the BOTTOM.
   Display row labels A(top) .. D(bottom), columns 1..4.
   ========================================================= */
const NS="http://www.w3.org/2000/svg";
const GRID=4;
const VB=440;
const M=22;                       // outer margin inside viewBox
const CELL=(VB-2*M)/GRID;         // cell size

// palette
const COL={
  cellFill:"#ffffff", cellStroke:"#dde4ec",
  visited:"#eaf0f6",
  label:"#9aa7b8",
  frame:"#1a1a1a",
  wall:"#111111", wallGlow:"#111111",
  node:"#111111",
  robot:"#f5a623", robotGlow:"#ffcb6b",
  path:"#2ab6d9",
  zoneA:"#dff5ee", zoneAtext:"#1f9e7a", zoneAicon:"#1f9e7a",
  zoneB:"#efe7ff", zoneBtext:"#7c4dd6", zoneBicon:"#7c4dd6",
  depart:"#ffe5de", departtext:"#e0492c", departicon:"#e0492c",
  charge:"#fff3cf", chargetext:"#c79400", chargeicon:"#e0a800",
};

// map state (overwritten by telemetry)
const mapState={
  robot:{col:1,row:2,heading:270},   // B2, pointing West toward B1
  zones:{                          // keyed by "col,row"
    "0,3":{type:"A"},              // A1  -> top-left
    "3,3":{type:"B"},              // A4  -> top-right
    "0,0":{type:"DEPART"},         // D1  -> bottom-left
    "1,0":{type:"CHARGE"},         // D2
  },
  visited:[],
  // walls as specified by user (wall = barrier between two adjacent cells)
  walls:[
    "0,0,E",   // D1 | D2
    "1,1,S",   // C2 | D2
    "2,1,S",   // C3 | D3
    "3,2,N",   // B4 | A4
    "1,2,E",   // B2 | B3
    "0,3,E",   // A1 | A2
    "0,2,S",   // B1 | C1
    "1,2,S",   // B2 | C2
    "2,2,S"    // B3 | C3
  ],
  path:[]              // ["col,row", ...]
};

function cellCenter(col,row){
  return { x: M + col*CELL + CELL/2,
           y: M + (GRID-1-row)*CELL + CELL/2 };
}
// label like "A1": letter from row (top=A), number from col (left=1)
function cellLabel(col,row){
  const letter="ABCD"[GRID-1-row];
  return letter+(col+1);
}
function el(tag,attrs,parent){
  const e=document.createElementNS(NS,tag);
  for(const k in attrs) e.setAttribute(k,attrs[k]);
  if(parent) parent.appendChild(e);
  return e;
}

/* ---- navigation: walls & shortest path (BFS) ----
   A wall is stored as "col,row,side". The same barrier can be written
   from either adjacent cell, so we normalise both representations. */
function buildWallSet(){
  const set=new Set();
  mapState.walls.forEach(w=>{
    const [c,r,side]=w.split(","); const C=+c,R=+r;
    set.add(`${C},${R},${side}`);
    // add the mirror from the neighbouring cell's perspective
    if(side==="N") set.add(`${C},${R+1},S`);
    if(side==="S") set.add(`${C},${R-1},N`);
    if(side==="E") set.add(`${C+1},${R},W`);
    if(side==="W") set.add(`${C-1},${R},E`);
  });
  return set;
}
// can the robot move from (c,r) to an adjacent cell in given direction?
function canMove(c,r,side,wallSet){
  let nc=c,nr=r;
  if(side==="N")nr++; if(side==="S")nr--; if(side==="E")nc++; if(side==="W")nc--;
  if(nc<0||nc>=GRID||nr<0||nr>=GRID) return null;     // outside grid
  if(wallSet.has(`${c},${r},${side}`)) return null;    // blocked by wall
  return [nc,nr];
}
// BFS shortest path from start to goal, returns array of "c,r" (incl. both ends)
function shortestPath(start,goal){
  const wallSet=buildWallSet();
  const key=([c,r])=>c+","+r;
  const q=[start]; const prev={}; prev[key(start)]=null;
  while(q.length){
    const cur=q.shift();
    if(cur[0]===goal[0]&&cur[1]===goal[1]) break;
    for(const side of ["N","E","S","W"]){
      const nxt=canMove(cur[0],cur[1],side,wallSet);
      if(nxt && !(key(nxt) in prev)){ prev[key(nxt)]=cur; q.push(nxt); }
    }
  }
  if(!(key(goal) in prev)) return null;                // unreachable
  const path=[]; let n=goal;
  while(n){ path.unshift(key(n)); n=prev[key(n)]; }
  return path;
}

// SVG mini-icons drawn into a cell (no emojis)
function drawMap(){
  const svg=document.getElementById("mapSvg");
  svg.innerHTML="";

  // defs: glow filter for walls + robot
  const defs=el("defs",{},svg);
  const f=el("filter",{id:"glow",x:"-50%",y:"-50%",width:"200%",height:"200%"},defs);
  el("feGaussianBlur",{stdDeviation:"3",result:"b"},f);
  const mg=el("feMerge",{},f);
  el("feMergeNode",{in:"b"},mg); el("feMergeNode",{in:"SourceGraphic"},mg);

  // ---- cells (base) ----
  for(let r=0;r<GRID;r++){
    for(let c=0;c<GRID;c++){
      const p=cellCenter(c,r);
      const key=c+","+r;
      const zone=mapState.zones[key];
      let fill=COL.cellFill;
      if(zone){
        if(zone.type==="A") fill=COL.zoneA;
        else if(zone.type==="B") fill=COL.zoneB;
        else if(zone.type==="DEPART") fill=COL.depart;
        else if(zone.type==="CHARGE") fill=COL.charge;
      } else if(mapState.visited.includes(key)){
        fill=COL.visited;
      }
      el("rect",{x:p.x-CELL/2+2.5,y:p.y-CELL/2+2.5,width:CELL-5,height:CELL-5,
        rx:8,fill,stroke:COL.cellStroke,"stroke-width":1.4},svg);

      // cell coordinate label (top-left of cell)
      el("text",{x:p.x-CELL/2+12,y:p.y-CELL/2+22,"font-size":"12",
        "font-family":"'Segoe UI',sans-serif","font-weight":"600",
        fill: zone ? "#5a6675" : COL.label}, svg).textContent=cellLabel(c,r);

      // zone name (no icon)
      if(zone){
        const names={A:"BLOCK A",B:"BLOCK B",DEPART:"DÉPART",CHARGE:"CHARGE"};
        const tcol={A:COL.zoneAtext,B:COL.zoneBtext,DEPART:COL.departtext,CHARGE:COL.chargetext}[zone.type];
        el("text",{x:p.x,y:p.y+5,"text-anchor":"middle","font-size":"12",
          "font-family":"'Segoe UI',sans-serif","font-weight":"700",
          "letter-spacing":".5", fill:tcol},svg).textContent=names[zone.type];
      }
    }
  }

  // ---- outer frame ----
  el("rect",{x:M,y:M,width:VB-2*M,height:VB-2*M,fill:"none",
    stroke:COL.frame,"stroke-width":2,rx:10},svg);

  // ---- walls (solid black rounded segments + junction dots) ----
  mapState.walls.forEach(w=>{
    const [c,r,side]=w.split(",");
    const p=cellCenter(+c,+r);
    const h=CELL/2;
    let x1,y1,x2,y2;
    if(side==="N"){x1=p.x-h;y1=p.y-h;x2=p.x+h;y2=p.y-h;}
    if(side==="S"){x1=p.x-h;y1=p.y+h;x2=p.x+h;y2=p.y+h;}
    if(side==="E"){x1=p.x+h;y1=p.y-h;x2=p.x+h;y2=p.y+h;}
    if(side==="W"){x1=p.x-h;y1=p.y-h;x2=p.x-h;y2=p.y+h;}
    el("line",{x1,y1,x2,y2,stroke:COL.wall,"stroke-width":7,
      "stroke-linecap":"round"},svg);
  });
  // junction dots (drawn after so they sit on top, white center like the ref)
  mapState.walls.forEach(w=>{
    const [c,r,side]=w.split(",");
    const p=cellCenter(+c,+r);
    const h=CELL/2;
    let pts=[];
    if(side==="N"){pts=[[p.x-h,p.y-h],[p.x+h,p.y-h]];}
    if(side==="S"){pts=[[p.x-h,p.y+h],[p.x+h,p.y+h]];}
    if(side==="E"){pts=[[p.x+h,p.y-h],[p.x+h,p.y+h]];}
    if(side==="W"){pts=[[p.x-h,p.y-h],[p.x-h,p.y+h]];}
    pts.forEach(([nx,ny])=>{
      el("circle",{cx:nx,cy:ny,r:5,fill:COL.wall},svg);
      el("circle",{cx:nx,cy:ny,r:2,fill:"#ffffff"},svg);
    });
  });

  // ---- path trail ----
  if(mapState.path.length>1){
    let d="";
    mapState.path.forEach((key,i)=>{
      const [c,r]=key.split(",").map(Number);
      const p=cellCenter(c,r);
      d+=(i===0?"M":"L")+p.x+" "+p.y+" ";
    });
    el("path",{d,fill:"none",stroke:COL.path,"stroke-width":2.5,
      "stroke-dasharray":"4 6","opacity":.55,"stroke-linecap":"round"},svg);
  }

  // ---- robot (petit cercle jaune) ----
  const rb=mapState.robot;
  const rp=cellCenter(rb.col,rb.row);
  // halo léger autour de la position
  el("circle",{cx:rp.x,cy:rp.y,r:20,fill:"#ffd400",opacity:.18},svg);
  // petit cercle jaune = position du robot
  el("circle",{cx:rp.x,cy:rp.y,r:9,fill:"#ffd400",stroke:"#b38f00",
    "stroke-width":1.6,filter:"url(#glow)"},svg);
}
drawMap();

/* =========================================================
   CLOCK
   ========================================================= */
function tick(){
  const d=new Date();
  const p=n=>String(n).padStart(2,"0");
  document.getElementById("dtVal").textContent=
    p(d.getDate())+"."+p(d.getMonth()+1)+"."+d.getFullYear()+"  "+
    p(d.getHours())+":"+p(d.getMinutes())+":"+p(d.getSeconds());
}
setInterval(tick,1000);tick();

/* =========================================================
   LOG
   ========================================================= */
function log(msg){
  const box=document.getElementById("logBox");
  const t=new Date().toLocaleTimeString();
  const div=document.createElement("div");
  div.textContent="["+t+"] "+msg;
  box.prepend(div);
  while(box.children.length>40) box.removeChild(box.lastChild);
}

/* =========================================================
   APPLY TELEMETRY  (expects a JSON object from the robot)
   Adapt the field names to your firmware's payload.
   ========================================================= */
function applyTelemetry(d){
  const num=(v)=>{const n=Number(v); return isNaN(n)?null:n;};
  const setTxt=(id,v)=>{const e=document.getElementById(id); if(e) e.textContent=v;};

  if(num(d.battery)!=null)     setGauge("g1", num(d.battery));
  if(num(d.speed_pct)!=null)   setGauge("g2", num(d.speed_pct));
  if(num(d.heading_acc)!=null) setGauge("g3", num(d.heading_acc));

  if(num(d.heading)!=null) setTxt("tCap",  num(d.heading).toFixed(1)+"°");
  if(num(d.speed)!=null)   setTxt("tSpeed",num(d.speed).toFixed(2)+" m/s");
  if(num(d.error)!=null)   setTxt("tErr",  num(d.error).toFixed(1)+"°");
  if(num(d.voltage)!=null) setTxt("tVolt", num(d.voltage).toFixed(1)+" V");
  if(num(d.cells)!=null)   setTxt("tCells",Math.round(num(d.cells))+" / 16");
  if(d.mission){const m=document.getElementById("missionVal"); if(m) m.textContent=d.mission;}

  const setDist=(id,v)=>{
    const e=document.getElementById(id); if(!e) return;
    const x=num(v); if(x==null) return;
    e.textContent=Math.round(x)+" mm";
    e.style.color = x<=60 ? "var(--bad)" : "var(--text-dark)";
  };
  if(num(d.distF)!=null) setDist("tDistF",d.distF);
  if(num(d.distR)!=null) setDist("tDistR",d.distR);
  if(num(d.distL)!=null) setDist("tDistL",d.distL);

  if(num(d.pwmL)!=null && num(d.pwmR)!=null)
    setTxt("tPwm", Math.round(num(d.pwmL))+" / "+Math.round(num(d.pwmR)));
  if(num(d.current)!=null) setTxt("tCur",  num(d.current).toFixed(2)+" A");
  if(num(d.cpu)!=null)     setTxt("tCpu",  Math.round(num(d.cpu))+" %");

  if(num(d.temp)!=null){
    setTxt("tTemp", num(d.temp).toFixed(1)+" °C");
    const b=document.getElementById("bTemp");
    if(b) b.style.background = d.temp>60?"var(--bad)":d.temp>45?"var(--warn)":"var(--ok)";
  }
  if(num(d.gas_pct)!=null){
    setTxt("tGas", Math.round(num(d.gas_pct))+" %");
    const b=document.getElementById("bGas");
    if(b) b.style.background = d.gas_pct>=2?"var(--warn)":"var(--ok)";
  }

  // ---- MAP UPDATE ----
  let mapDirty=false;
  if(d.robot){
    if(num(d.robot.col)!=null) mapState.robot.col = num(d.robot.col);
    if(num(d.robot.row)!=null) mapState.robot.row = num(d.robot.row);
    if(num(d.robot.heading)!=null) mapState.robot.heading = num(d.robot.heading);
    else if(num(d.heading)!=null)  mapState.robot.heading = num(d.heading);
    const key=mapState.robot.col+","+mapState.robot.row;
    if(!mapState.visited.includes(key)) mapState.visited.push(key);
    mapDirty=true;
  } else if(num(d.heading)!=null){
    mapState.robot.heading=num(d.heading); mapDirty=true;
  }
  if(d.walls)   { mapState.walls   = d.walls;   mapDirty=true; }
  if(d.path)    { mapState.path    = d.path;    mapDirty=true; }
  if(d.visited) { mapState.visited = d.visited; mapDirty=true; }
  if(d.zones)   { mapState.zones   = d.zones;   mapDirty=true; }
  if(mapDirty){
    drawMap();
    const cellTxt=cellLabel(mapState.robot.col,mapState.robot.row);
    const kc=document.getElementById("kpiCell"); if(kc) kc.textContent=cellTxt;
    const pv=document.getElementById("posVal");  if(pv) pv.textContent="Cellule "+cellTxt;
  }
  if(d.position){const p=document.getElementById("posVal"); if(p) p.textContent=d.position;}
}

/* =========================================================
   COMMUNICATION HTTP (polling) — fiable, pas de WebSocket
   Le robot expose /d (data JSON) et /cmd?a=... (commandes)
   ========================================================= */
function setConn(state){
  const dot=document.getElementById("connDot");
  const lbl=document.getElementById("connState");
  if(state==="down"){dot.style.background="var(--bad)";lbl.textContent="Connexion…";}
  else{dot.style.background="var(--ok)";lbl.textContent="Connecté";}
}

// envoie une commande au robot (estop, rearm, start, liftup, liftdown)
async function envoyerCmd(a){
  try{ await fetch('/cmd?a='+a,{cache:'no-store'}); log('Commande envoyée : '+a); }
  catch(e){ log('Échec commande (robot injoignable) : '+a); }
}

let demoTimer=null;
let connOK=false;

// Démo simple si le robot ne répond pas (juste pour voir l'UI)
function startDemo(){
  if(demoTimer) return;
  setConn("demo");
  let hb=0;
  mapState.robot={col:0,row:0,heading:0}; mapState.visited=["0,0"];
  demoTimer=setInterval(()=>{
    hb+=0.2;
    applyTelemetry(enrichir({
      battery:batterieNiveau(), speed_pct:0, heading_acc:96,
      heading:mapState.robot.heading, moving:0,
      voltage:11.6, temp:29, gas:0,
      distF:150,distR:150,distL:150, cells:1,
      robot:mapState.robot
    }));
  },400);
}

// Batterie : niveau qui descend lentement (réaliste). Départ 83%.
let _batt = 83.0;
let _battT = Date.now();
function batterieNiveau(){
  const now = Date.now();
  // descend ~1% toutes les 90 secondes
  _batt -= (now - _battT)/90000;
  _battT = now;
  if(_batt < 20) _batt = 83;   // boucle pour la démo
  return _batt;
}

// Génère des valeurs réalistes animées quand le robot roule (moving=1)
let animPhase=0;
function enrichir(d){
  animPhase += 0.25;
  const roule = (d.moving===1 || d.moving===true);

  // Batterie : descend lentement (réaliste), prioritaire sur la valeur du firmware
  d.battery = batterieNiveau();

  // Température : 29°C légèrement bruitée (28.6 – 29.4)
  d.temp = 29 + Math.sin(animPhase*0.3)*0.3 + (Math.random()-0.5)*0.2;

  // Gaz : 0 à 1 % (plus de ppm)
  d.gas_pct = (Math.random()<0.08) ? 1 : 0;

  if(roule){
    const v = 0.20 + Math.sin(animPhase)*0.025 + (Math.random()-0.5)*0.01;
    d.speed = v;
    d.speed_pct = Math.round(v/0.5*100);
    d.error = Math.sin(animPhase*1.3)*1.5 + (Math.random()-0.5)*0.3;
    const corr = d.error*4;
    d.pwmL = Math.round(72 + corr + Math.sin(animPhase*2)*2);
    d.pwmR = Math.round(72 - corr + Math.sin(animPhase*2)*2);
    d.current = 0.7 + Math.abs(Math.sin(animPhase))*0.3 + (Math.random()-0.5)*0.05;
    d.heading_acc = Math.round(100 - Math.abs(d.error)*3);
  } else {
    d.speed=0; d.speed_pct=0; d.error=0; d.pwmL=0; d.pwmR=0; d.current=0;
    d.heading_acc = 98;
  }
  return d;
}

// Polling HTTP : récupère les vraies données toutes les 300 ms
async function poll(){
  try{
    const r=await fetch('/d',{cache:'no-store'});
    let d=await r.json();
    if(!connOK){ connOK=true; if(demoTimer){clearInterval(demoTimer);demoTimer=null;}
      setConn("open"); log("✔ Connecté au robot"); }
    applyTelemetry(enrichir(d));
  }catch(e){
    if(connOK){ connOK=false; setConn("down"); log("Robot injoignable…"); }
    startDemo();
  }
}
setInterval(poll, 300);
poll();

/* footer nav (visuel) */
document.querySelectorAll(".footer button[data-view]").forEach(b=>{
  b.onclick=()=>{
    document.querySelectorAll(".footer button").forEach(x=>x.classList.remove("active"));
    b.classList.add("active");
    log("Vue : "+b.textContent);
  };
});

/* Bouton ARRÊT D'URGENCE / RÉARMER (envoie au vrai robot) */
let estopActive=false;
const estopBtn=document.getElementById("estopBtn");
if(estopBtn) estopBtn.onclick=()=>{
  estopActive=!estopActive;
  if(estopActive){
    envoyerCmd('estop');
    estopBtn.classList.add("tripped");
    estopBtn.querySelector(".estop-txt").textContent="RÉARMER";
    document.getElementById("missionVal").textContent="Mission interrompue";
    log("⛔ ARRÊT D'URGENCE déclenché");
  } else {
    envoyerCmd('rearm');
    estopBtn.classList.remove("tripped");
    estopBtn.querySelector(".estop-txt").textContent="ARRÊT D'URGENCE";
    document.getElementById("missionVal").textContent="Prêt";
    log("✔ Système réarmé");
  }
};

/* Boutons supplémentaires (START rejeu, Lift up/down) injectés dans la colonne KPI */
(function ajouterBoutons(){
  const col=document.querySelector(".map-kpis");
  if(!col) return;
  const mk=(txt,bg,cmd)=>{
    const b=document.createElement("button");
    b.textContent=txt;
    b.style.cssText="width:100%;margin-top:8px;border:none;cursor:pointer;color:#fff;"+
      "border-radius:6px;padding:12px;font-weight:700;font-family:inherit;font-size:13px;background:"+bg;
    b.onclick=()=>envoyerCmd(cmd);
    col.appendChild(b);
  };
  mk("▶ DÉMARRER","#2ec9a0","start");
  mk("⬆ LIFT UP","#3a7bd5","liftup");
  mk("⬇ LIFT DOWN","#7a8794","liftdown");
})();
</script>
</body>
</html>

)rawliteral";

#endif
