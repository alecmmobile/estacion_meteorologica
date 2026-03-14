# 🌦️ Estación Meteorológica Casera

Proyecto DIY de estación meteorológica basada en ESP32 LilyGo T-Display. Integra sensores BME680, MQ-2, MQ-8 y GY-30 para monitoreo ambiental, detección de gases y luminosidad. Visualización en pantalla TFT integrada y servidor HTTP con endpoints JSON vía WiFi.

---

## 📋 Descripción

Este proyecto consiste en una estación meteorológica casera capaz de medir variables ambientales en tiempo real, detectar gases peligrosos, medir la luminosidad del ambiente y mostrar toda la información en la pantalla TFT integrada del ESP32 LilyGo T-Display. Los datos también se exponen a través de un servidor HTTP embebido con endpoints JSON accesibles desde cualquier dispositivo en la misma red.

---

## 📸 Galería

### Protoboard — vista frontal
![Protoboard frente](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/ProtoBoard%2001%20-%20Frente%20-%20Proyecto%20Meteorologia.jpg)

### Protoboard — vista trasera
![Protoboard atrás](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/ProtoBoard%2001%20-%20Atras%20-%20Proyecto%20Meteorologia.jpg)

### Pantalla — BME680 (Presión, Temperatura, Humedad, Gas)
![BME680](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/BME680%20-%20Proyecto%20Meteorologia.jpg)

### Pantalla — MQ-2 (Gas combustible y humo)
![MQ-2](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/MQ2%20-%20Proyecto%20Meteorologia.jpg)

### Pantalla — GY-30 (Luminosidad en lux)
![GY-30 pantalla](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/GY-30%20-%20Proyecto%20Meteorologia%2002.jpg)

### Protoboard con GY-30 conectado
![GY-30 protoboard](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/GY-30%20-%20Proyecto%20Meteorologia.jpg)

### Pantalla — Servidor HTTP (IP, red, endpoints)
![Servidor Web](https://raw.githubusercontent.com/alecmmobile/estacion_meteorologica/main/imagenes/Servidor%20Web%20-%20Proyecto%20Meteorologia.jpg)

---

## 🧰 Hardware utilizado

| Componente | Función |
|---|---|
| ESP32 LilyGo T-Display | Microcontrolador principal + pantalla TFT 1.14" |
| BME680 | Temperatura, humedad, presión atmosférica y COV |
| GY-30 (BH1750) | Luminosidad ambiental en lux |
| MQ-2 | Detección de gas GLP, humo y monóxido de carbono |
| MQ-8 | Detección de hidrógeno y vapores de alcohol |

---

## 🔌 Diagrama de conexiones

### BME680 (I2C)

| BME680 | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

> Dirección I2C por defecto: `0x76` o `0x77`

### GY-30 / BH1750 (I2C)

| GY-30 | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

> Comparte el bus I2C con el BME680. Dirección: `0x23` (ADDR a GND) o `0x5C` (ADDR a VCC)

### MQ-2 (Analógico)

| MQ-2 | ESP32 |
|---|---|
| VCC | 5V (VIN) |
| GND | GND |
| AOUT | GPIO34 |

### MQ-8 (Analógico)

| MQ-8 | ESP32 |
|---|---|
| VCC | 5V (VIN) |
| GND | GND |
| AOUT | GPIO35 |

> **Nota:** Los sensores MQ-2 y MQ-8 requieren 5V para su calefactor interno. La salida analógica AOUT es compatible con los 3.3V del ESP32.

---

## 💻 Código fuente

El código fuente del proyecto fue desarrollado en **Arduino IDE** y se encuentra en este mismo repositorio en la siguiente ubicación:

📄 [`_Proyecto_Meteorologia/_Proyecto_Meteorologia.ino`](https://github.com/alecmmobile/estacion_meteorologica/blob/main/_Proyecto_Meteorologia/_Proyecto_Meteorologia.ino)

### ⚠️ Configuración obligatoria antes de compilar

Antes de subir el código al ESP32, debes editar las siguientes líneas con el nombre y contraseña de tu red WiFi:

```cpp
const char* WIFI_SSID     = "TU_RED_WIFI";        // ← tu red WiFi
const char* WIFI_PASSWORD = "TU_CONTRASENA";      // ← tu contraseña
```

Estas líneas se encuentran al inicio del archivo `.ino`, en la sección `① CONFIGURACIÓN DE RED`.

---

## 📦 Librerías utilizadas

### Librerías incluidas en el código fuente

| Librería | Descripción |
|---|---|
| `FS.h` | Sistema de archivos base del ESP32. Se incluye primero para evitar conflictos de namespace con `fs::FS`. Permite acceder al almacenamiento interno (SPIFFS/LittleFS). |
| `WiFi.h` | Gestiona la conexión WiFi del ESP32. Permite conectarse a redes, obtener IP y manejar el estado de la conexión. |
| `WebServer.h` | Crea un servidor HTTP ligero dentro del ESP32. Permite definir rutas y servir páginas web o datos JSON desde el dispositivo. |
| `ArduinoJson.h` | Biblioteca para serializar y deserializar datos en formato JSON. Útil para enviar las lecturas de los sensores como respuesta HTTP o hacia servicios externos. |
| `time.h` | Librería estándar de C para manejo de tiempo. En el ESP32 se usa junto con NTP para obtener la hora real desde internet y registrar las mediciones con timestamp. |
| `Wire.h` | Implementa el protocolo de comunicación I2C. Es la que permite comunicarse con el BME680 y el GY-30, que usan este bus para transmitir sus datos al ESP32. |
| `SPI.h` | Implementa el protocolo de comunicación SPI. Lo utiliza internamente la pantalla TFT del LilyGo T-Display para recibir los datos gráficos. |
| `TFT_eSPI.h` | Librería de alto rendimiento para controlar pantallas TFT. Provee funciones para dibujar texto, figuras y colores en la pantalla integrada del LilyGo T-Display. |

### Librerías externas a instalar

Instalar desde el gestor de librerías del IDE de Arduino:

- [`Adafruit BME680`](https://github.com/adafruit/Adafruit_BME680) — Sensor de temperatura, humedad, presión y COV
- [`Adafruit Unified Sensor`](https://github.com/adafruit/Adafruit_Sensor) — Dependencia requerida por el BME680
- [`BH1750`](https://github.com/claws/BH1750) — Sensor de luminosidad GY-30
- [`MQUnifiedsensor`](https://github.com/miguel5612/MQSensorsLib) — Sensores de gas MQ-2 y MQ-8
- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI) — Pantalla TFT del LilyGo T-Display
- [`ArduinoJson`](https://arduinojson.org/) — Serialización JSON para el servidor HTTP

---

## 🌐 Servidor HTTP embebido

La estación expone un servidor web accesible desde la red local. Los endpoints disponibles son:

| Endpoint | Descripción |
|---|---|
| `/` | Página HTML con auto-refresh cada 5 segundos |
| `/data` | JSON con las lecturas de todos los sensores |
| `/health` | Ping de estado — responde `OK` |

La IP local y el nombre de la red se muestran en la pantalla al iniciar.

---

## ⚙️ Configuración del entorno

1. Instalar el soporte para placas ESP32 en el IDE de Arduino
2. Seleccionar la placa: **TTGO T-Display**
3. Configurar la librería `TFT_eSPI` con el archivo `User_Setup.h` correspondiente al LilyGo T-Display
4. Instalar todas las librerías listadas arriba
5. Editar `WIFI_SSID` y `WIFI_PASSWORD` con tus credenciales
6. Conectar el hardware según el diagrama de conexiones
7. Compilar y subir el código

---

## 📊 Variables medidas

- 🌡️ Temperatura (°C)
- 💧 Humedad relativa (%)
- 🔵 Presión atmosférica (hPa)
- 🌫️ Compuestos orgánicos volátiles / calidad del aire (COV)
- ☀️ Luminosidad ambiental (lux) — GY-30
- 💨 Concentración de gas GLP / humo (MQ-2)
- ⚗️ Concentración de hidrógeno / alcohol (MQ-8)

---

## 📹 Video explicativo

Puedes ver la explicación completa del proyecto en YouTube:

👉 [Ver video en YouTube](#) *(próximamente)*

---

## 📁 Estructura del repositorio

```
📦 estacion_meteorologica
 ┣ 📂 _Proyecto_Meteorologia
 ┃ ┗ 📄 _Proyecto_Meteorologia.ino
 ┣ 📂 imagenes
 ┃ ┣ 🖼️ BME680 - Proyecto Meteorologia.jpg
 ┃ ┣ 🖼️ Datos Estudiante - Proyecto Meteorologia.jpg
 ┃ ┣ 🖼️ GY-30 - Proyecto Meteorologia 02.jpg
 ┃ ┣ 🖼️ GY-30 - Proyecto Meteorologia.jpg
 ┃ ┣ 🖼️ MQ2 - Proyecto Meteorologia.jpg
 ┃ ┣ 🖼️ ProtoBoard 01 - Atras - Proyecto Meteorologia.jpg
 ┃ ┣ 🖼️ ProtoBoard 01 - Frente - Proyecto Meteorologia.jpg
 ┃ ┗ 🖼️ Servidor Web - Proyecto Meteorologia.jpg
 ┣ 📄 README.md
 ┗ 📄 LICENSE
```

---

## 🚀 Próximas mejoras

- [ ] Envío de datos a servidor MQTT
- [ ] Dashboard web en tiempo real
- [ ] Historial de datos con almacenamiento local
- [ ] Alertas por umbral de gas detectado
- [ ] Integración con Home Assistant

---

## 📄 Licencia

Este proyecto está bajo la licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.

---

*Hecho con ❤️ como proyecto DIY de electrónica y programación embebida.*
