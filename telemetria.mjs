import http from "node:http";
import { readFile } from "node:fs/promises";

const topics = {
  state: "riego_uni/estado",
  online: "riego_uni/online",
  history: "riego_uni/historial",
};

const stateFields = ["suelo", "temp", "hr", "nivel", "dist", "rssi", "temporal", "slim", "tlim", "up"];
const flags = ["ldr", "agua", "r1", "r2", "prog", "manual"];
const reading = value => Number.isFinite(value) ? value : null;

function cleanState(payload) {
  if (!payload || typeof payload !== "object" || Array.isArray(payload)) return null;
  const value = Object.fromEntries(stateFields.map(key => [key, reading(payload[key])]));
  for (const key of flags) {
    value[key] = typeof payload[key] === "boolean" || payload[key] === 0 || payload[key] === 1
      ? Boolean(payload[key])
      : null;
  }
  value.modo = ["auto", "manual", "paro"].includes(payload.modo) ? payload.modo : null;
  return value;
}

function cleanHistory(payload) {
  const series = ["u", "s", "t", "n", "r"];
  if (!payload || !Number.isFinite(payload.up) || !series.every(key => Array.isArray(payload[key]))) return null;
  const length = payload.u.length;
  if (!series.every(key => payload[key].length === length)) return null;
  return { up: payload.up, ...Object.fromEntries(series.map(key => [key, payload[key].slice(-300).map(reading)])) };
}

export function createTelemetry(client) {
  let connected = false;
  let online = null;
  let state = null;
  let stateAgeOrigin = null;
  let history = null;

  client.on("connect", () => {
    client.subscribe(Object.values(topics), { qos: 1 }, (error, grants) => {
      connected = !error && grants?.length === 3 && grants.every(grant => grant.qos < 128);
    });
  });
  client.on("close", () => {
    connected = false;
    online = null;
  });
  client.on("error", () => {});
  client.on("message", (topic, data) => {
    if (data.length > 65536) return;
    const text = data.toString();
    if (topic === topics.online) {
      online = text === "1" ? true : text === "0" ? false : null;
      return;
    }
    if (topic !== topics.state && topic !== topics.history) return;
    let payload;
    try { payload = JSON.parse(text); } catch { return; }
    if (topic === topics.history) {
      const next = cleanHistory(payload);
      if (next) history = next;
      return;
    }
    const next = cleanState(payload);
    if (!next) return;
    state = next;
    stateAgeOrigin = Date.now();
  });

  return () => ({
    connected: Boolean(connected),
    online,
    state,
    receivedAt: stateAgeOrigin,
    ageMs: stateAgeOrigin === null ? null : Date.now() - stateAgeOrigin,
    history,
  });
}

export async function createWebServer(snapshot) {
  const files = new Map(await Promise.all([
    ["/", "./index.html", "text/html; charset=utf-8"],
    ["/assets/campo-riego.jpg", "./assets/campo-riego.jpg", "image/jpeg"],
  ].map(async ([route, file, mime]) => [route, { bytes: await readFile(new URL(file, import.meta.url)), mime }])));
  files.set("/index.html", files.get("/"));

  return http.createServer((request, response) => {
    const route = request.url.split("?")[0];
    response.setHeader("X-Content-Type-Options", "nosniff");
    if (request.method !== "GET" && request.method !== "HEAD") {
      response.writeHead(405, { Allow: "GET, HEAD" }).end();
      return;
    }
    if (route === "/api/lecturas") {
      response.writeHead(200, { "Content-Type": "application/json; charset=utf-8", "Cache-Control": "no-store" });
      response.end(request.method === "HEAD" ? undefined : JSON.stringify(snapshot()));
      return;
    }
    const file = files.get(route);
    if (!file) {
      response.writeHead(404).end();
      return;
    }
    response.writeHead(200, { "Content-Type": file.mime, "Cache-Control": "no-cache" });
    response.end(request.method === "HEAD" ? undefined : file.bytes);
  });
}
