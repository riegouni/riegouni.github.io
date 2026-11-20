# Redes WiFi del ESP32 desde el panel

Con `wifi_redes.h`, el ESP32 guarda hasta 5 redes WiFi en su memoria. Desde el panel (sección **Operación → Red WiFi del ESP32**), los operadores pueden:

- Tocar **Buscar redes**: el ESP32 busca las redes que tiene cerca y el panel las muestra con su señal.
- Elegir una red de la lista y escribir solo la contraseña.
- Quitar redes guardadas.

## Órdenes que manda el panel (tema `riego_uni/cmd`)

| Orden | Qué hace |
|---|---|
| `wifi_scan` | Busca las redes cercanas y publica la lista en `riego_uni/redes`. No corta la conexión actual. |
| `wifi_add=<SSID>` + salto de línea + `<clave>` | Guarda la red sin cambiar la conexión actual. |
| `wifi_usar=<SSID>` + salto de línea + `<clave>` | La guarda y se conecta ya. Si no entra en 20 s, vuelve a las redes que funcionaban y deja la nueva de última. |
| `wifi_usar=<SSID>` (sin salto de línea) | Se conecta a una red ya guardada con la clave que tiene. |
| `wifi_del=<SSID>` | Borra la red. Nunca borra la última que queda. |

La lista de redes cercanas que publica el ESP32 se ve así (`r` es la señal en dBm, `k` es 1 si la red pide contraseña):

```json
{"vistas":[{"s":"UNI-Feria","r":-52,"k":1},{"s":"Cafe UNI","r":-80,"k":0}]}
```

## Cómo agregarlo a tu programa

1. Copia `wifi_redes.h` en la misma carpeta que tu `.ino` y agrégalo arriba:

   ```cpp
   #include "wifi_redes.h"
   ```

2. En `setup()`, cambia la conexión fija por esta. Tu red actual queda como la primera guardada:

   ```cpp
   // antes: WiFi.begin(WIFI_SSID, WIFI_PASS); while (WiFi.status() != WL_CONNECTED) delay(500);
   RedesWifi::iniciar(WIFI_SSID, WIFI_PASS);
   RedesWifi::conectar();
   ```

3. En la función que recibe los mensajes MQTT, revisa primero las órdenes de WiFi:

   ```cpp
   String msg;
   for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
   int igual = msg.indexOf('=');
   String clave = igual < 0 ? msg : msg.substring(0, igual);
   String valor = igual < 0 ? "" : msg.substring(igual + 1);
   if (RedesWifi::comando(clave, valor)) return;
   // ... aquí siguen tus órdenes de siempre (modo, regar, tlim, slim, h1, h2...)
   ```

4. En `loop()`:

   ```cpp
   if (RedesWifi::procesar()) mqtt.disconnect();          // cambió la red: reconecta MQTT
   if (WiFi.status() != WL_CONNECTED) RedesWifi::conectar();
   String vistas = RedesWifi::escaneo();                  // redes cercanas pedidas por el panel
   if (vistas.length()) mqtt.publish("riego_uni/redes", vistas.c_str());
   ```

5. En el JSON que publicas en `riego_uni/estado`, agrega esto justo antes de la `}` final:

   ```cpp
   estado += RedesWifi::json();   // agrega ,"ssid":"...","redes":[...]
   ```

   El estado crece unos 150 bytes y la lista de redes cercanas ocupa hasta 900 bytes. Si usas PubSubClient, pon `mqtt.setBufferSize(1024);` en `setup()`.

## Permisos en EMQX

El usuario `visor` (visitantes) solo puede suscribirse a `riego_uni/estado`, `riego_uni/online` y `riego_uni/historial`. Todo lo demás le está prohibido, también `riego_uni/cmd` (por donde viajan las claves WiFi) y `riego_uni/redes`. Los usuarios `control` y `esp32` no tienen reglas, así que pueden usar `riego_uni/redes` sin cambiar nada.

## Notas

- Buscar redes tarda unos segundos y no detiene el riego.
- Mientras prueba una red nueva, el ESP32 espera hasta 20 s y se pausa el resto del programa. Hazlo cuando no esté regando.
- El panel nunca muestra las claves. El ESP32 solo informa los nombres de las redes.
- Las redes ocultas no salen en la búsqueda. Para esas, usa **Otra red** en el panel y escribe el nombre a mano.
