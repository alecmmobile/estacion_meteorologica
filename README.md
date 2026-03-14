# 🌦️ Estación Meteorológica Casera

Proyecto DIY de estación meteorológica basada en ESP32 LilyGo T-Display. Integra sensores BME680, MQ-2 y MQ-8 para monitoreo ambiental y detección de gases. Visualización en pantalla TFT integrada y transmisión de datos vía WiFi.

---

## 📋 Descripción

Este proyecto consiste en una estación meteorológica casera capaz de medir variables ambientales en tiempo real, detectar gases peligrosos y mostrar toda la información en la pantalla TFT integrada del ESP32 LilyGo T-Display. Los datos también pueden transmitirse de forma inalámbrica vía WiFi.

---

## 🧰 Hardware utilizado

| Componente | Función |
|---|---|
| ESP32 LilyGo T-Display | Microcontrolador principal + pantalla TFT 1.14" |
| BME680 | Temperatura, humedad, presión atmosférica y COV |
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

## 📦 Librerías necesarias

Instalar desde el gestor de librerías del IDE de Arduino:

- [`Adafruit BME680`](https://github.com/adafruit/Adafruit_BME680) — Sensor ambiental
- [`MQUnifiedsensor`](https://github.com/miguel5612/MQSensorsLib) — Sensores de gas MQ-2 y MQ-8
- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI) — Pantalla TFT del LilyGo T-Display
- `Adafruit Unified Sensor` — Dependencia del BME680

---

## ⚙️ Configuración del entorno

1. Instalar el soporte para placas ESP32 en el IDE de Arduino
2. Seleccionar la placa: **TTGO T-Display**
3. Configurar la librería `TFT_eSPI` con el archivo `User_Setup.h` correspondiente al LilyGo T-Display
4. Instalar todas las librerías listadas arriba
5. Conectar el hardware según el diagrama de conexiones
6. Compilar y subir el código

---

## 📊 Variables medidas

- 🌡️ Temperatura (°C)
- 💧 Humedad relativa (%)
- 🔵 Presión atmosférica (hPa)
- 🌫️ Compuestos orgánicos volátiles / calidad del aire (COV)
- 💨 Concentración de gas GLP / humo (MQ-2)
- ⚗️ Concentración de hidrógeno / alcohol (MQ-8)

---

## 📹 Video explicativo

Puedes ver la explicación completa del proyecto en YouTube:

👉 [Ver video en YouTube](#) *(próximamente)*

---

## 📁 Estructura del repositorio

```
📦 estacion-meteorologica
 ┣ 📂 src
 ┃ ┗ 📄 estacion_meteorologica.ino
 ┣ 📂 docs
 ┃ ┗ 📄 diagrama_conexiones.png
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
