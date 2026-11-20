#pragma once
// ============================================================================
//  wifi_redes.h  -  Redes WiFi guardadas para el ESP32 "UNI riego"
//
//  Guarda hasta 5 redes en la memoria del ESP32 (Preferences) y atiende estas
//  ordenes que manda el panel web por el tema riego_uni/cmd:
//
//    wifi_add=<SSID>\n<clave>    guarda la red sin cambiar la conexion actual
//    wifi_usar=<SSID>\n<clave>   la guarda y se conecta ya; si no entra en
//                                20 s, vuelve a las redes que ya funcionaban
//    wifi_del=<SSID>             borra una red guardada (nunca borra la ultima)
//
//  Para que el panel muestre la red, agrega json() al estado que se publica:
//    ,"ssid":"<red actual>","redes":["Red 1","Red 2"]
//
//  Pasos para agregarlo al programa: ver LEEME.md
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

namespace RedesWifi {

const uint8_t  MAX_REDES     = 5;
const uint32_t ESPERA_USAR   = 20000;  // ms para entrar a una red pedida desde el panel
const uint32_t ESPERA_NORMAL = 12000;  // ms por red al arrancar o al reconectar

struct Red { String ssid; String clave; };

Red     redes[MAX_REDES];
uint8_t total = 0;
Red     pendiente;
bool    cambioPendiente = false;

// ---------------------------------------------------------------- memoria
inline void guardar() {
  Preferences p;
  p.begin("redeswifi", false);
  p.clear();
  p.putUChar("n", total);
  char k[6];
  for (uint8_t i = 0; i < total; i++) {
    snprintf(k, sizeof k, "s%u", (unsigned)i); p.putString(k, redes[i].ssid);
    snprintf(k, sizeof k, "p%u", (unsigned)i); p.putString(k, redes[i].clave);
  }
  p.end();
}

// Llamar una vez en setup(). Si no hay redes guardadas, usa la del programa.
inline void iniciar(const char* ssidInicial, const char* claveInicial) {
  Preferences p;
  p.begin("redeswifi", true);
  uint8_t n = p.getUChar("n", 0);
  total = n > MAX_REDES ? MAX_REDES : n;
  char k[6];
  for (uint8_t i = 0; i < total; i++) {
    snprintf(k, sizeof k, "s%u", (unsigned)i); redes[i].ssid  = p.getString(k, "");
    snprintf(k, sizeof k, "p%u", (unsigned)i); redes[i].clave = p.getString(k, "");
  }
  p.end();
  if (total == 0 && ssidInicial && ssidInicial[0]) {
    redes[0].ssid  = ssidInicial;
    redes[0].clave = claveInicial ? claveInicial : "";
    total = 1;
    guardar();
  }
}

// ---------------------------------------------------------------- lista
inline int buscar(const String& ssid) {
  for (uint8_t i = 0; i < total; i++) if (redes[i].ssid == ssid) return i;
  return -1;
}

// Agrega al final (o actualiza la clave si ya existe). Con la lista llena,
// reemplaza la ultima red que no sea la que esta en uso.
inline void agregar(const String& ssid, const String& clave) {
  int i = buscar(ssid);
  if (i >= 0) { redes[i].clave = clave; return; }
  if (total < MAX_REDES) { redes[total].ssid = ssid; redes[total].clave = clave; total++; return; }
  String enUso = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String("");
  for (int j = MAX_REDES - 1; j >= 0; j--) {
    if (redes[j].ssid != enUso) { redes[j].ssid = ssid; redes[j].clave = clave; return; }
  }
}

// Deja la red en la primera posicion (la que se intenta primero).
inline void ponerPrimero(const String& ssid, const String& clave) {
  int i = buscar(ssid);
  if (i < 0) {
    if (total < MAX_REDES) total++;
    i = total - 1;  // con la lista llena se pierde la ultima
  }
  for (int j = i; j > 0; j--) redes[j] = redes[j - 1];
  redes[0].ssid = ssid;
  redes[0].clave = clave;
}

// Manda la red al final (la que se intenta de ultima).
inline void ponerUltima(const String& ssid) {
  int i = buscar(ssid);
  if (i < 0) return;
  Red r = redes[i];
  for (uint8_t j = i; j + 1 < total; j++) redes[j] = redes[j + 1];
  redes[total - 1] = r;
}

inline bool quitar(const String& ssid) {
  int i = buscar(ssid);
  if (i < 0 || total <= 1) return false;  // nunca deja al ESP32 sin redes
  for (uint8_t j = i; j + 1 < total; j++) redes[j] = redes[j + 1];
  total--;
  redes[total] = Red();
  return true;
}

// ---------------------------------------------------------------- conexion
inline bool intentar(const Red& r, uint32_t ms) {
  WiFi.disconnect();
  WiFi.begin(r.ssid.c_str(), r.clave.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < ms) delay(200);
  return WiFi.status() == WL_CONNECTED;
}

// Prueba las redes guardadas en orden. Usar en setup() y cuando se cae el WiFi.
inline bool conectar(uint32_t msPorRed = ESPERA_NORMAL) {
  WiFi.mode(WIFI_STA);
  for (uint8_t i = 0; i < total; i++) if (intentar(redes[i], msPorRed)) return true;
  return false;
}

// ---------------------------------------------------------------- ordenes
// Llamar desde la funcion que recibe los mensajes MQTT, con la orden ya
// separada en clave y valor (lo que esta antes y despues del primer '=').
// Devuelve true si era una orden de WiFi.
inline bool comando(const String& clave, const String& valor) {
  if (clave != "wifi_add" && clave != "wifi_usar" && clave != "wifi_del") return false;
  if (clave == "wifi_del") {
    if (quitar(valor)) guardar();
    return true;
  }
  int corte = valor.indexOf('\n');
  String ssid  = corte < 0 ? valor : valor.substring(0, corte);
  String pass  = corte < 0 ? String("") : valor.substring(corte + 1);
  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 63) return true;  // datos invalidos
  if (clave == "wifi_add") {
    agregar(ssid, pass);
    guardar();
  } else {  // wifi_usar: se atiende en procesar(), fuera de la funcion de MQTT
    pendiente.ssid = ssid;
    pendiente.clave = pass;
    cambioPendiente = true;
  }
  return true;
}

// Llamar en cada vuelta de loop(). Devuelve true si cambio la conexion
// (en ese caso hay que volver a conectar MQTT).
inline bool procesar() {
  if (!cambioPendiente) return false;
  cambioPendiente = false;
  if (intentar(pendiente, ESPERA_USAR)) {
    ponerPrimero(pendiente.ssid, pendiente.clave);
  } else {
    agregar(pendiente.ssid, pendiente.clave);  // la deja guardada,
    ponerUltima(pendiente.ssid);               // pero de ultima,
    conectar();                                // y vuelve a las que funcionan
  }
  guardar();
  return true;
}

// ---------------------------------------------------------------- estado
inline String escapar(const String& t) {
  String r;
  for (unsigned i = 0; i < t.length(); i++) {
    char c = t[i];
    if (c == '"' || c == '\\') { r += '\\'; r += c; }
    else if ((uint8_t)c < 0x20) r += ' ';
    else r += c;
  }
  return r;
}

// Texto para agregar al final del JSON de estado, justo antes de la '}' final.
inline String json() {
  String s = ",\"ssid\":\"";
  s += escapar(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(""));
  s += "\",\"redes\":[";
  for (uint8_t i = 0; i < total; i++) {
    if (i) s += ',';
    s += '"';
    s += escapar(redes[i].ssid);
    s += '"';
  }
  s += ']';
  return s;
}

}  // namespace RedesWifi
