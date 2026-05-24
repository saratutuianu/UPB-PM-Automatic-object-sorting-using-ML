# UPB-PM-Automatic-object-sorting-using-ML
## Overview
 
This project implements a real-time fruit detection system using an ESP32-CAM module with a custom-trained TensorFlow classification model. When a fruit is detected on a ramp, a servo motor automatically opens a gate to allow it to pass through. The system displays results on an OLED screen and streams live video to a web browser.
 
---
 
## How It Works
 
The ESP32-CAM continuously streams video via MJPEG to a web browser. When the user clicks the **"Classify"** button, a single frame is captured and sent over WiFi to a Python Flask server running on a laptop. The server converts the raw RGB565 image data, runs inference using a custom MobileNetV2 model trained on apple and lemon images, and sends the result back to the ESP32-CAM. The result is displayed on the OLED screen, and if an apple is detected, the servo motor opens the gate for 3 seconds before closing it again.
 
---
 
## Hardware Components
 
| Component | Role |
|---|---|
| **ESP32-CAM (AI Thinker)** | Main microcontroller with integrated camera |
| **RHYX M21-45 Camera** | Captures RGB565 raw frames (no JPEG hardware compression) |
| **OLED Display SSD1306 0.96"** | Displays detected fruit and confidence score via I2C |
| **Servo Motor SG90** | Controls the physical gate mechanism |
| **FTDI FT232RL** | USB to UART converter for programming and powering the ESP32-CAM |
| **4xAA Battery Holder (6V)** | Powers the servo motor separately |
| **Breadboard + Jumper Wires** | For prototyping connections |
 
---
 
## Software Architecture
 
```
Browser (laptop)
    ↓ Click "Classify"
Flask Server (Python)
    ↓ GET /jpeg
ESP32-CAM (port 82)
    ↓ RGB565 raw frame
Flask Server
    ↓ Convert RGB565 → BGR
    ↓ Run MobileNetV2 model
    ↓ GET /rezultat?label=apple&conf=95
ESP32-CAM
    ↓ Display on OLED
    ↓ GET /servo?grade=90
ESP32-CAM
    ↓ Open gate 3 seconds
    ↓ Close gate
```
 
---
 
## Pin Configuration
 
| Pin | Component | Signal |
|---|---|---|
| GPIO0 | Camera / Boot | XCLK / Programming mode |
| GPIO14 | OLED | SCL (I2C) |
| GPIO15 | OLED | SDA (I2C) |
| GPIO16 | Servo SG90 | PWM signal |
| GPIO26 | Camera | SCCB SDA |
| GPIO27 | Camera | SCCB SCL |
| GPIO25 | Camera | VSYNC |
| GPIO1 | FTDI | UART TX |
| GPIO3 | FTDI | UART RX |
 
---
 
## ML Model
 
The classification model uses **MobileNetV2** with transfer learning, trained on a custom dataset of approximately 240 images per class. The model classifies three categories: `apple`, `lemon`, and `empty` (no object). Training was done with TensorFlow 2.18 and achieves **100% validation accuracy** on the test set.
 
---
 
## Servers
 
The system runs two HTTP servers on the ESP32-CAM simultaneously:
 
| Port | Type | Description |
|---|---|---|
| **81** | MJPEG stream | Live video stream via eloquent_esp32cam library |
| **82** | REST API | Three endpoints for capture, classification and servo control |
 
### API Endpoints (port 82)
 
| Endpoint | Method | Description |
|---|---|---|
| `/jpeg` | GET | Captures and returns a single raw RGB565 frame |
| `/rezultat` | GET | Receives classification result and displays on OLED |
| `/servo` | GET | Controls servo motor angle (`?grade=90`) |
 
---
 
## Key Technical Challenges
 
### RGB565 Camera
The RHYX camera does not support hardware JPEG compression, so raw RGB565 data is sent to Python for conversion before inference.
 
### Byte Swapping
The camera transmits RGB565 bytes in reversed order, requiring a byte-swap operation in Python before the image can be decoded correctly.
 
```python
raw_swapped = bytes([raw[i+1] if i%2==0 else raw[i-1] 
                    for i in range(len(raw))])
```
 
### Dual Server
Running both the MJPEG stream and the REST API simultaneously required port separation and avoiding shared camera access conflicts.
 
---
 
## Setup
 
### Hardware
1. Connect OLED to GPIO14 (SCL) and GPIO15 (SDA)
2. Connect servo signal wire to GPIO16
3. Power servo from 4xAA batteries (6V) with shared GND
4. Connect FTDI for programming (GPIO0 → GND for flash mode)
### Software
 
Install Python dependencies:
```bash
pip install flask tensorflow opencv-python numpy requests
```
 
Train the model:
```bash
python train_model.py
```
 
Start the Flask server:
```bash
python server.py
```
 
Upload the Arduino sketch to ESP32-CAM, then open:
```
http://localhost:5000
```
 
---
 
## Project Structure
 
```
project/
├── server.py               ← Flask server + ML inference
├── train_model.py          ← Model training script
├── model_fructe.keras      ← Trained TensorFlow model
├── class_indices.json      ← Class label mapping
├── dataset/
│   ├── apple/              ← Training images (~240)
│   ├── lemon/              ← Training images (~240)
│   └── empty/              ← Training images (~150)
└── arduino/
    └── clasificare/
        └── clasificare.ino ← ESP32-CAM Arduino sketch
```
 
---
 
## Dependencies
 
### Python
- `flask` — web server
- `tensorflow` — ML model inference
- `opencv-python` — image processing and RGB565 conversion
- `numpy` — array operations
- `requests` — HTTP communication with ESP32-CAM
### Arduino Libraries
- `eloquent_esp32cam` — camera streaming
- `Adafruit_SSD1306` — OLED display
- `Adafruit_GFX` — graphics library
- `ESP-IDF LEDC driver` — servo PWM control
---
 
## License
 
MIT License
