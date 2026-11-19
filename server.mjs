import mqtt from "mqtt";
import { createTelemetry, createWebServer } from "./telemetria.mjs";

const { MQTT_URL, MQTT_VIEWER_PASSWORD, HOST = "127.0.0.1", PORT = "8080" } = process.env;
const url = MQTT_URL && new URL(MQTT_URL);
if (!url || url.protocol !== "wss:" || url.username || url.password || !MQTT_VIEWER_PASSWORD) {
  throw new Error("Configura MQTT_URL (wss) y MQTT_VIEWER_PASSWORD en el entorno del servidor.");
}
const port = Number(PORT);
if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error("PORT debe estar entre 1 y 65535.");

const client = mqtt.connect(url.href, {
  username: "visor",
  password: MQTT_VIEWER_PASSWORD,
  clean: true,
  connectTimeout: 10000,
  reconnectPeriod: 5000,
});
const server = await createWebServer(createTelemetry(client));
server.listen(port, HOST, () => console.log("Panel disponible en el puerto " + port));
function stop() {
  client.end(true);
  server.close();
}
process.once("SIGINT", stop);
process.once("SIGTERM", stop);
