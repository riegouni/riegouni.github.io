# Redes WiFi del ESP32 desde el panel

Con `wifi_redes.h`, el ESP32 guarda hasta 5 redes WiFi en su memoria. Los operadores pueden agregar, usar o quitar redes desde el panel (sección **Operación → Red WiFi del ESP32**).

## Órdenes que manda el panel (tema `riego_uni/cmd`)

| Orden | Qué hace |
|---|---|
| `wifi_add=<SSID>` + salto de línea + `<clave>` | Guarda la red sin cambiar la conexión actual. |
| `wifi_usar=<SSID>` + salto de línea + `<clave>` | La guarda y se conecta ya. Si no entra en 20 s, vuelve a las redes que funcionaban y deja la nueva de última. |
| `wifi_del=<SSID>` | Borra la red. Nunca borra la última que queda. |

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
   ```

5. En el JSON que publicas en `riego_uni/estado`, agrega esto justo antes de la `}` final:

   ```cpp
   estado += RedesWifi::json();   // agrega ,"ssid":"...","redes":[...]
   ```

   El estado crece unos 150 bytes. Si usas PubSubClient, comprueba que `mqtt.setBufferSize(...)` alcance (por ejemplo 1024).

## Antes de usarlo

En EMQX (Access Control → Authorization), agrega una regla para que el usuario `visor` **no pueda suscribirse** a `riego_uni/cmd`. Las claves WiFi viajan cifradas por ese tema, pero hoy cualquier cuenta puede leerlo.

## Notas

- Mientras prueba una red nueva, el ESP32 espera hasta 20 s y se pausa el resto del programa. Hazlo cuando no esté regando.
- El panel nunca muestra las claves. El ESP32 solo informa los nombres de las redes.
