# 🌱 Riego Inteligente UNI

Panel web del **Sistema de Riego Inteligente** presentado en la Feria Nacional UNI.

**Abrir el panel:** https://riegouni.github.io/

## Qué se puede hacer
- **Visitantes** (sin contraseña): ver el estado del riego, los sensores en vivo y las curvas de las últimas 24 h.
- **Usuarios autorizados** (usuario + contraseña): cambiar entre modo Automático, Manual y Paro, encender o apagar el riego, regar por minutos, ajustar límites y programar horarios.

## Cómo funciona
```
Sensor YL-69 → Arduino UNO + nRF24L01 (TX) ──radio 2.4 GHz──► ESP32 receptor (RX)
        ──UART──► ESP32 "UNI riego" (DHT11, ultrasónico, LDR, relés) ──WiFi/MQTT TLS──► Nube EMQX
        ◄──WebSocket TLS──► este panel (navegador)
```
- El ESP32 publica el estado cada 5 s y un historial (promedio cada 5 min, 24 h).
- El acceso de visitante es de **solo lectura**: la nube le prohíbe enviar órdenes.
- Por seguridad, el riego nunca enciende con el tanque vacío ni con alarma del LDR.
