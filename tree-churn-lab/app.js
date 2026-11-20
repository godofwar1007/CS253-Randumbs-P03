"use strict";

const { Lab, strategies, makeDemo } = window.TreeLab;
const { datasets, topics, proofs, revision, uniformFinal } = window.ChurnData;
const $ = selector => document.querySelector(selector);
const format = new Intl.NumberFormat("en-US");
const rootUrl = "https://github.com/godofwar1007/DSA-project/blob/" + revision + "/";
const canvas = $("#tree");
const plot = $("#plot");
let lab, timer, inspected, positions, plotState, codeTopic;
let demo, demoTimer;

function surface(element) {
  const width = element.clientWidth;
  const height = element.clientHeight;
  const scale = window.devicePixelRatio || 1;
  element.width = Math.round(width * scale);
  element.height = Math.round(height * scale);
  const ctx = element.getContext("2d");
  ctx.setTransform(scale, 0, 0, scale, 0, 0);
  return { ctx, width, height };
}

function circle(ctx, x, y, r, fill, stroke, weight) {
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.fillStyle = fill;
  ctx.fill();
  if (stroke) {
    ctx.lineWidth = weight;
    ctx.strokeStyle = stroke;
    ctx.stroke();
  }
}

function line(ctx, x, y, toX, toY, color, weight) {
  ctx.beginPath();
  ctx.moveTo(x, y);
  ctx.lineTo(toX, toY);
  ctx.strokeStyle = color;
  ctx.lineWidth = weight;
  ctx.stroke();
}

function drawCover() {
  const { ctx, width, height } = surface($("#cover-tree"));
  const x = amount => width * amount;
  const y = amount => height * amount;
  const nodes = [[.51,.19,48], [.29,.45,26], [.73,.45,72], [.14,.73,14], [.40,.73,35], [.61,.73,61], [.86,.73,86], [.95,.92,94]];
  const highlight = new Set([0, 2, 6, 7]);
  const edges = [[0,1],[0,2],[1,3],[1,4],[2,5],[2,6],[6,7]];
  for (const [a,b] of edges) {
    const lit = highlight.has(a) && highlight.has(b);
    line(ctx, x(nodes[a][0]), y(nodes[a][1]), x(nodes[b][0]), y(nodes[b][1]), lit ? "#e0fb64" : "#a592c5", lit ? 3 : 2);
  }
  const r = Math.min(23, height * .095);
  nodes.forEach(([nx,ny,key],i) => {
    const active = highlight.has(i);
    const radius = i === 7 ? r * .78 : r;
    circle(ctx, x(nx) + 3, y(ny) + 3, radius, "#151020");
    circle(ctx, x(nx), y(ny), radius, active ? "#e0fb64" : "#ded0ff", "#171323", 2);
    ctx.fillStyle = "#171323";
    ctx.font = "600 " + Math.round(radius * .72) + "px IBM Plex Mono, monospace";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(key, x(nx), y(ny));
  });
}

function drawTree() {
  const { ctx, width, height } = surface(canvas);
  const tree = lab.tree;
  positions = [];
  function visit(item, depth, parent) {
    if (!item) return;
    const point = { item, depth, parent };
    visit(item.left, depth + 1, point);
    positions.push(point);
    visit(item.right, depth + 1, point);
  }
  visit(tree.root, 0, null);
  if (!positions.length) {
    ctx.fillStyle = "#5a5165";
    ctx.textAlign = "center";
    ctx.font = "600 15px Bricolage Grotesque, sans-serif";
    ctx.fillText("Empty tree · insert a key to start", width / 2, height / 2);
    return;
  }
  const minimumGap = Math.min(width / Math.max(positions.length, 1), (height - 36) / Math.max(tree.stats.height, 1));
  const radius = Math.min(19, Math.max(3.7, minimumGap * .41));
  const padding = Math.max(radius + 9, 14);
  const horizontal = width - 2 * padding;
  const vertical = Math.min(66, (height - 2 * padding - 14) / Math.max(tree.stats.height, 1));
  positions.forEach((point, index) => {
    point.x = positions.length === 1 ? width / 2 : padding + horizontal * index / (positions.length - 1);
    point.y = padding + point.depth * vertical;
    point.radius = radius;
  });
  const active = new Set(lab.path);
  const highlightRoot = lab.rebuilt?.newRoot;
  for (const point of positions) if (point.parent) {
    const lit = active.has(point.item.key) && active.has(point.parent.item.key);
    line(ctx, point.x, point.y, point.parent.x, point.parent.y, lit ? "#6743c4" : "#4d3a62", lit ? 2.4 : 1.5);
  }
  for (const p of positions) {
    const key = p.item.key;
    const fill = key === highlightRoot ? "#f5b7e3" : key === inspected ? "#d4bcff" : active.has(key) ? "#e0fb64" : "#fffdf7";
    circle(ctx, p.x + 1.6, p.y + 1.6, radius, "#171323");
    circle(ctx, p.x, p.y, radius, fill, "#171323", radius < 8 ? 1 : 1.75);
    if (radius >= 8) {
      const digits = String(key).length;
      ctx.fillStyle = "#171323";
      ctx.font = "600 " + Math.min(12, Math.max(6, radius * .82), radius * 2.1 / Math.max(1, digits * .65)) + "px IBM Plex Mono, monospace";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(key, p.x, p.y);
    }
  }
}

function renderLab() {
  const { stats } = lab.tree;
  const definition = strategies[lab.strategy];
  $("#tree-name").textContent = definition.name;
  $("#tree-description").textContent = definition.description;
  $("#keys").textContent = stats.size;
  $("#depth").textContent = (stats.size ? stats.sum / stats.size : 0).toFixed(2);
  $("#height").textContent = stats.size ? stats.height : "–";
  $("#rebuilds").textContent = lab.tree.rebuilds - lab.initialRebuilds;
  const agrees = lab.agrees();
  $("#oracle-status").textContent = agrees ? "✓ ORACLE MATCH" : "ORACLE MISMATCH";
  $("#oracle-status").classList.toggle("bad", !agrees);
  $("#pair-count").textContent = demo ? "Prepared example" : format.format(lab.pairs) + (lab.pairs === 1 ? " pair" : " pairs") + " · browser simulation";
  $("#sorted-keys").textContent = lab.keys.length ? lab.keys.join("  ·  ") : "Empty";
  const items = lab.history.map(item => {
    const li = document.createElement("li");
    const label = document.createElement("span");
    label.textContent = item.kind;
    li.append(label, document.createTextNode(item.description));
    return li;
  });
  if (demo && !items.length) {
    const item = Object.assign(document.createElement("li"), { textContent: "Ready to run the prepared example." });
    items.push(item);
  }
  $("#log").replaceChildren(...items);
  drawTree();
  const point = positions.find(candidate => candidate.item.key === inspected);
  $("#inspection").textContent = point ?
    "KEY " + point.item.key + "  ·  DEPTH " + point.depth + "  ·  SUBTREE " + point.item.size + "  ·  LEFT " + (point.item.left?.key ?? "–") + "  ·  RIGHT " + (point.item.right?.key ?? "–") :
    lab.rebuilt?.count ? "REBUILT " + lab.rebuilt.count + " NODES  ·  NEW ROOT " + lab.rebuilt.newRoot :
    demo ? "Lime = highlighted path or leaves · pink = node in focus" : "Lime = last search path · pink = rebuilt subtree root";
}

function setPlayback(active) {
  clearInterval(timer);
  timer = active ? setInterval(step, 1000 / Number($("#speed").value)) : null;
  $("#play").textContent = active ? "Ⅱ Pause" : "▶ Play";
  $("#live-status").innerHTML = active ? "<i></i> RUNNING" : lab?.pairs ? "<i></i> PAUSED" : "<i></i> READY";
}

function step() {
  inspected = null;
  if (!lab.step()) setPlayback(false);
  else if (!timer) setPlayback(false);
  renderLab();
}

function reset() {
  setPlayback(false);
  const input = $("#seed");
  if (!input.checkValidity() || input.value === "") input.value = "42";
  lab = new Lab($("#strategy").value, $("#workload").value, Number($("#size").value), Number(input.value));
  inspected = null;
  setPlayback(false);
  renderLab();
}

function stopDemo() {
  clearInterval(demoTimer);
  demoTimer = null;
  $("#demo-play").textContent = "Autoplay";
}

function showDemoStep(index) {
  demo.index = Math.max(0, Math.min(index, demo.frames.length - 1));
  const item = demo.frames[demo.index];
  lab = item.lab;
  inspected = null;
  $("#demo-count").textContent = "STEP " + (demo.index + 1) + " / " + demo.frames.length;
  $("#demo-stage").textContent = item.label;
  $("#demo-detail").textContent = item.detail;
  const array = $("#demo-array");
  array.hidden = !item.array;
  const makeKey = (key, chosen = false) => {
    const box = Object.assign(document.createElement("span"), { textContent: key });
    if (chosen) box.className = "chosen";
    return box;
  };
  if (Array.isArray(item.array)) {
    array.replaceChildren(...item.array.map(key => makeKey(key)));
  } else if (item.array) {
    const { left, pivot, right } = item.array;
    const group = (label, keys, chosen) => {
      const part = document.createElement("div");
      const title = Object.assign(document.createElement("b"), { textContent: label });
      part.append(title, ...(keys.length ? keys.map(key => makeKey(key, chosen)) : [makeKey("–")]));
      return part;
    };
    array.replaceChildren(group("Left", left), group("Root", [pivot], true), group("Right", right));
  } else {
    array.replaceChildren();
  }
  $("#demo-back").disabled = demo.index === 0;
  $("#demo-next").disabled = demo.index === demo.frames.length - 1;
  $("#live-status").innerHTML = "<i></i> DEMO";
  renderLab();
}

function loadDemo() {
  setPlayback(false);
  stopDemo();
  demo = { frames: makeDemo($("#demo-select").value), index: 0 };
  $("#demo-controls").hidden = false;
  $("#free-controls").hidden = true;
  showDemoStep(0);
}

$("#load-demo").addEventListener("click", loadDemo);
$("#demo-select").addEventListener("change", loadDemo);
$("#free-lab").addEventListener("click", () => {
  stopDemo();
  demo = null;
  $("#demo-controls").hidden = true;
  $("#free-controls").hidden = false;
  reset();
});
$("#demo-back").addEventListener("click", () => { stopDemo(); showDemoStep(demo.index - 1); });
$("#demo-next").addEventListener("click", () => { stopDemo(); showDemoStep(demo.index + 1); });
$("#demo-restart").addEventListener("click", () => { stopDemo(); showDemoStep(0); });
$("#demo-play").addEventListener("click", () => {
  if (demoTimer) { stopDemo(); return; }
  if (demo.index === demo.frames.length - 1) showDemoStep(0);
  $("#demo-play").textContent = "Pause demo";
  demoTimer = setInterval(() => {
    showDemoStep(demo.index + 1);
    if (demo.index === demo.frames.length - 1) stopDemo();
  }, 2400);
});
$("#play").addEventListener("click", () => setPlayback(!timer));
$("#step").addEventListener("click", step);
$("#reset").addEventListener("click", reset);
for (const id of ["strategy", "workload", "size", "seed"]) $("#" + id).addEventListener("change", reset);
$("#speed").addEventListener("input", () => {
  $("#speed-value").value = $("#speed").value + "/s";
  if (timer) setPlayback(true);
});
$("#manual").addEventListener("submit", event => {
  event.preventDefault();
  setPlayback(false);
  const key = Number($("#key-input").value);
  const action = event.submitter?.value || "find";
  lab.manual(action, key);
  inspected = action === "find" ? key : null;
  renderLab();
});
$("#toggle-keys").addEventListener("click", () => {
  const keys = $("#sorted-keys");
  keys.hidden = !keys.hidden;
  $("#toggle-keys").textContent = keys.hidden ? "View sorted keys" : "Hide sorted keys";
  $("#toggle-keys").setAttribute("aria-expanded", String(!keys.hidden));
});
canvas.addEventListener("click", event => {
  const bounds = canvas.getBoundingClientRect();
  const hit = positions.find(p => Math.hypot(p.x - event.clientX + bounds.left, p.y - event.clientY + bounds.top) <= p.radius + 3);
  inspected = hit?.item.key;
  renderLab();
});
canvas.addEventListener("pointermove", event => {
  const bounds = canvas.getBoundingClientRect();
  canvas.style.cursor = positions.some(p => Math.hypot(p.x - event.clientX + bounds.left, p.y - event.clientY + bounds.top) <= p.radius + 3) ? "pointer" : "default";
});

const notes = {
  uniform: "On these runs, always-successor deletion ended deeper than it started at every size. The two scapegoat traces overlap because both rebuild methods produce the same shape.",
  min_delete_high_insert: "Deleting the minimum and inserting a new maximum drives the two Hibbard trees into a chain. Neither policy gets to choose a two-child replacement at the extremes.",
  max_delete_low_insert: "Deleting the maximum and inserting a new minimum builds the mirror-image chain. Both Hibbard policies again follow the same path.",
  scapegoat_stress: "This workload deletes a random key and inserts monotonically smaller keys. It forces far more rebuilds than uniform churn. The two rebuild implementations produce the same depths and counts."
};

function compact(number) {
  if (Math.abs(number) >= 1e6) return (number / 1e6).toFixed(number % 1e6 ? 1 : 0) + "m";
  if (Math.abs(number) >= 1e3) return (number / 1e3).toFixed(number % 1e3 ? 1 : 0) + "k";
  return String(Math.round(number * 10) / 10);
}

function drawPlot() {
  const workload = $("#bench-workload").value;
  const size = $("#bench-size").value;
  const field = 1 + Number($("#bench-metric").value);
  const series = Object.entries(datasets[workload][size]);
  const all = series.flatMap(([, data]) => data);
  const maxX = Math.max(...all.map(row => row[0]));
  const extreme = Math.max(...all.map(row => row[field]));
  const maxY = extreme > 0 ? extreme * 1.1 : 1;
  const { ctx, width, height } = surface(plot);
  const left = width < 430 ? 40 : 55;
  const right = width - 15, top = 14, bottom = height - 27;
  const x = value => left + value / maxX * (right - left);
  const y = value => bottom - value / maxY * (bottom - top);
  ctx.font = "10px IBM Plex Mono, monospace";
  ctx.textBaseline = "middle";
  ctx.textAlign = "right";
  for (let tick = 0; tick <= 5; tick += 1) {
    const value = tick * maxY / 5;
    line(ctx, left, y(value), right, y(value), "#ddd4e3", 1);
    ctx.fillStyle = "#51475c";
    ctx.fillText(compact(value), left - 5, y(value));
  }
  line(ctx, left, top, left, bottom, "#171323", 1.5);
  line(ctx, left, bottom, right, bottom, "#171323", 1.5);
  ctx.textBaseline = "top";
  for (let tick = 0; tick <= 4; tick += 1) {
    const value = tick * maxX / 4;
    ctx.fillStyle = "#51475c";
    ctx.textAlign = tick === 0 ? "left" : tick === 4 ? "right" : "center";
    ctx.fillText(compact(value), x(value), bottom + 6);
  }
  for (const [id, samples] of series) {
    ctx.beginPath();
    ctx.lineWidth = 2.4;
    ctx.strokeStyle = strategies[id].color;
    ctx.setLineDash(id === "scapegoat_inplace" || id === "hibbard_succ" ? [7, 4] : []);
    samples.forEach((row, index) => index ? ctx.lineTo(x(row[0]), y(row[field])) : ctx.moveTo(x(row[0]), y(row[field])));
    ctx.stroke();
  }
  ctx.setLineDash([]);
  plotState = { series, field, maxX, left, right, bottom };
  $("#plot-hover").hidden = true;
  $("#plot-label").textContent = (field === 1 ? "AVERAGE DEPTH (EDGES)" : "CUMULATIVE REBUILDS") + " × CHURN PAIRS · FIVE-SEED MEANS";
  $("#bench-note").textContent = field === 2 && series.every(([id]) => id.startsWith("hibbard")) ?
    "These are ordinary BSTs. Neither Hibbard policy performs a rebuild." : notes[workload];
  $("#legend").replaceChildren(...series.map(([id, rows]) => {
    const item = document.createElement("div");
    item.className = "legend-item";
    const swatch = document.createElement("i");
    swatch.style.background = strategies[id].color;
    const label = document.createElement("span");
    label.textContent = strategies[id].short;
    const value = document.createElement("b");
    const final = rows.at(-1)[field];
    value.textContent = field === 1 ? final.toFixed(2) + " edges" : format.format(final) + " rebuilds";
    item.append(swatch, label, value);
    return item;
  }));
}

for (const id of ["bench-workload", "bench-size", "bench-metric"]) $("#" + id).addEventListener("change", drawPlot);
plot.addEventListener("pointermove", event => {
  const bounds = plot.getBoundingClientRect();
  const px = event.clientX - bounds.left;
  const tip = $("#plot-hover");
  if (px < plotState.left || px > plotState.right || event.clientY - bounds.top > plotState.bottom) {
    tip.hidden = true;
    return;
  }
  const { series, maxX, left, right, field } = plotState;
  const target = (px - left) * maxX / (right - left);
  const nearest = series[0][1].reduce((a,b) => Math.abs(a[0] - target) < Math.abs(b[0] - target) ? a : b);
  const lines = [format.format(nearest[0]) + " PAIRS"];
  for (const [id, rows] of series) {
    const value = rows.reduce((a,b) => Math.abs(a[0] - nearest[0]) < Math.abs(b[0] - nearest[0]) ? a : b);
    lines.push(strategies[id].short + ": " + (field === 1 ? value[field].toFixed(2) : format.format(value[field])));
  }
  tip.replaceChildren(...lines.map(text => Object.assign(document.createElement("div"), { textContent: text })));
  tip.hidden = false;
  tip.style.left = Math.max(5, Math.min(px + 8, bounds.width - tip.offsetWidth - 5)) + "px";
  tip.style.top = "10px";
});
plot.addEventListener("pointerleave", () => { $("#plot-hover").hidden = true; });

function escapeCode(markup) {
  return markup.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function showTopic(index) {
  codeTopic = topics[index];
  $("#code-heading").textContent = codeTopic.title;
  $("#code-intro").textContent = codeTopic.intro;
  $("#code-steps").replaceChildren(...codeTopic.steps.map(text => Object.assign(document.createElement("li"), { textContent: text })));
  $("#code-file").textContent = codeTopic.file + ":" + codeTopic.start;
  $("#code-link").href = rootUrl + codeTopic.file + "#L" + codeTopic.start + "-L" + codeTopic.end;
  $("#code").innerHTML = codeTopic.code.split(/(\/\/[^\n]*|\b(?:if|else|for|while|return|const|auto|bool|void|class|struct|public|std|delete|nullptr|true|false|continue|break|noexcept)\b|\b[0-9]+\b)/g).map((token, index) =>
    index % 2 ? '<span class="syntax-' + (token.startsWith("//") ? "comment" : /^\d/.test(token) ? "number" : "keyword") + '">' + escapeCode(token) + "</span>" : escapeCode(token)
  ).join("");
  $("#code").parentElement.scrollTop = 0;
  $("#copy").textContent = "Copy excerpt";
  Array.from($("#code-tabs").children).forEach((button, i) => button.setAttribute("aria-pressed", String(i === index)));
}

$("#code-tabs").replaceChildren(...topics.map((topic, index) => {
  const button = Object.assign(document.createElement("button"), { textContent: topic.tab, type: "button" });
  button.addEventListener("click", () => showTopic(index));
  return button;
}));
$("#derivations").replaceChildren(...proofs.map(([label, file]) => {
  const li = document.createElement("li");
  const link = Object.assign(document.createElement("a"), { textContent: label, href: rootUrl + "docs/derivations/" + file, target: "_blank", rel: "noopener" });
  li.append(link);
  return li;
}));
$("#copy").addEventListener("click", async () => {
  try {
    await navigator.clipboard.writeText(codeTopic.code);
    $("#copy").textContent = "Copied";
  } catch {
    const range = document.createRange();
    range.selectNodeContents($("#code"));
    window.getSelection().removeAllRanges();
    window.getSelection().addRange(range);
    $("#copy").textContent = "Text selected";
  }
});

const depthColumns = ["scapegoat_inplace", "hibbard_succ", "hibbard_rand", "scapegoat_array"];
$("#uniform-final-body").replaceChildren(...uniformFinal.map(row => {
  const tr = document.createElement("tr");
  const size = Object.assign(document.createElement("th"), { scope: "row", textContent: format.format(row.n) });
  tr.append(size, ...depthColumns.map(id => {
    const { mean, sd } = row[id];
    return Object.assign(document.createElement("td"), {
      textContent: mean.toFixed(2) + " ± " + sd.toFixed(2)
    });
  }));
  return tr;
}));
loadDemo();
showTopic(0);
drawCover();
drawPlot();
let frame;
function redraw() {
  cancelAnimationFrame(frame);
  frame = requestAnimationFrame(() => { drawCover(); drawTree(); drawPlot(); });
}
window.addEventListener("resize", redraw);
document.fonts?.ready.then(redraw);
