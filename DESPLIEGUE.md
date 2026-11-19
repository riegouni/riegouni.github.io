# Despliegue del panel de Riego UNI

El servicio web entrega el panel y responde a `GET /api/lecturas`. La vista de visitante consulta ese endpoint. El servicio usa la cuenta MQTT `visor` y se suscribe solamente a los temas de estado, conexión e historial. Los operadores conservan el inicio de sesión con su usuario y contraseña MQTT.

## Configuración

1. Instala Node.js 22 o posterior y ejecuta `npm install` en la raíz del repositorio.
2. Configura estas variables con el administrador de secretos del alojamiento o en el entorno de ejecución local.

   | Variable | Valor |
   | --- | --- |
   | `MQTT_URL` | La URL WebSocket segura del broker que ya usa el panel. Aparece como `BROKER_URL` en `index.html`. |
   | `MQTT_VIEWER_PASSWORD` | La contraseña vigente de la cuenta de solo lectura `visor`. |
   | `HOST` | `127.0.0.1` para uso local. Para un alojamiento administrado, usa el valor que indique el proveedor. |
   | `PORT` | Puerto HTTP que indique el alojamiento. El valor local predeterminado es `8080`. |

3. Ejecuta `npm start`. Para una instalación local, abre `http://127.0.0.1:8080`.
4. Para uso desde otros dispositivos, aloja este servicio Node.js detrás de HTTPS y abre la URL del servicio. GitHub Pages solo sirve el sitio estático y necesita que esta versión se aloje en un servicio que ejecute Node.js.
5. Antes de cambiar el enlace del sitio en uso, verifica que la vista de visitante actualiza las lecturas y que un operador puede entrar con su cuenta. Revisa en EMQX que la cuenta `visor` solo tenga permisos de suscripción a los tres temas de lecturas.

Usa el gestor de secretos del alojamiento para aprovisionar la contraseña. Si la contraseña apareció en un commit anterior, permanece en el historial de Git hasta que ese historial se gestione por separado.
