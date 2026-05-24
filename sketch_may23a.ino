#define WIFI_SSID "DIGI-34bY"
#define WIFI_PASS "PCtQZ3CNTN"
#define HOSTNAME "esp32cam"

#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/wifi/sta.h>
#include <eloquent_esp32cam/viz/mjpeg.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/ledc.h"

using eloq::camera;
using eloq::wifi;
using eloq::viz::mjpeg;

WebServer apiServer(82);

// Servo
#define SERVO_PIN 16
static bool servoInitializat = false;

void servoWrite(int grade) {
    // Valori corecte pentru SG90
    // 0 grade  → duty 102  (0.5ms din 20ms)
    // 90 grade → duty 307  (1.5ms din 20ms)  
    // 180 grade → duty 512 (2.5ms din 20ms)
    uint32_t duty = (uint32_t)(102 + (grade * 410 / 180));
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
}

void servoInitLazy() {
    if (servoInitializat) return;

    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num       = LEDC_TIMER_2,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num   = SERVO_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = LEDC_CHANNEL_2,
        .timer_sel  = LEDC_TIMER_2,
        .duty       = 205,
        .hpoint     = 0
    };
    ledc_channel_config(&channel);
    servoInitializat = true;
    Serial.println("Servo initializat!");
}

// OLED
#define I2C_SDA 15
#define I2C_SCL 14
TwoWire I2Cbus = TwoWire(0);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cbus, OLED_RESET);

void afiseazaOLED(String label, String conf) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Detectat:");
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.print(label);
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.print("Conf: ");
    display.print(conf);
    display.print("%");
    display.display();
    Serial.printf("Detectat: %s (%s%%)\r\n",
        label.c_str(), conf.c_str());
}

void handleJpeg() {
    camera_fb_t* flush = esp_camera_fb_get();
    if (flush) {
        esp_camera_fb_return(flush);
        flush = nullptr;
    }
    delay(200);

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        apiServer.send(500, "text/plain", "Capture failed");
        return;
    }

    size_t len    = fb->len;
    uint8_t* buf  = (uint8_t*)malloc(len);

    if (!buf) {
        esp_camera_fb_return(fb);
        apiServer.send(500, "text/plain", "Memory failed");
        return;
    }

    memcpy(buf, fb->buf, len);
    esp_camera_fb_return(fb);

    apiServer.send_P(200, "application/octet-stream",
        (const char*)buf, len);

    free(buf);
    Serial.printf("Frame trimis: %d bytes\r\n", len);
}

void handleRezultat() {
    String label = apiServer.arg("label");
    String conf  = apiServer.arg("conf");

    if (label.length() > 0) {
        afiseazaOLED(label, conf);
        apiServer.send(200, "text/plain", "OK");
    } else {
        apiServer.send(400, "text/plain", "Lipseste label");
    }
}

void handleServo() {
    String grade_str = apiServer.arg("grade");

    if (grade_str.length() > 0) {
        int grade = grade_str.toInt();

        // Initializeaza servo doar la primul apel
        servoInitLazy();

        servoWrite(grade);
        Serial.printf("Servo: %d grade\r\n", grade);
        apiServer.send(200, "text/plain", "OK");
    } else {
        apiServer.send(400, "text/plain", "Lipseste grade");
    }
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("___CLASIFICARE SERVER___");

    // OLED
    I2Cbus.begin(I2C_SDA, I2C_SCL, 100000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("OLED failed!");
        while (true);
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Se porneste...");
    display.display();

    // Camera
    static camera_config_t ai_thinker_config = {
        .pin_pwdn       = 32,
        .pin_reset      = -1,
        .pin_xclk       = 0,
        .pin_sccb_sda   = 26,
        .pin_sccb_scl   = 27,
        .pin_d7         = 35,
        .pin_d6         = 34,
        .pin_d5         = 39,
        .pin_d4         = 36,
        .pin_d3         = 21,
        .pin_d2         = 19,
        .pin_d1         = 18,
        .pin_d0         = 5,
        .pin_vsync      = 25,
        .pin_href       = 23,
        .pin_pclk       = 22,
        .xclk_freq_hz   = 10000000,
        .ledc_timer     = LEDC_TIMER_0,
        .ledc_channel   = LEDC_CHANNEL_0,
        .pixel_format   = PIXFORMAT_RGB565,
        .frame_size     = FRAMESIZE_QVGA,
        .jpeg_quality   = 10,
        .fb_count       = 1,
        .grab_mode      = CAMERA_GRAB_WHEN_EMPTY
    };

    camera.brownout.disable();

    esp_err_t err = esp_camera_init(&ai_thinker_config);
    if (err != ESP_OK) {
        Serial.printf("Camera failed: 0x%x\n", err);
        return;
    }
    Serial.println("Camera OK");

    // WiFi
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Conectare WiFi...");
    display.display();

    while (!wifi.connect().isOk())
        Serial.println(wifi.exception.toString());

    // MJPEG
    while (!mjpeg.begin().isOk())
        Serial.println(mjpeg.exception.toString());

    // API server
    apiServer.on("/jpeg",     HTTP_GET, handleJpeg);
    apiServer.on("/rezultat", HTTP_GET, handleRezultat);
    apiServer.on("/servo",    HTTP_GET, handleServo);
    apiServer.begin();

    // Afiseaza IP
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("WiFi OK!");
    display.setCursor(0, 16);
    display.print(wifi.ip());
    display.display();

    Serial.println("WiFi OK");
    Serial.printf("Stream:   http://%s:81\r\n", wifi.ip().c_str());
    Serial.printf("API JPEG: http://%s:82/jpeg\r\n", wifi.ip().c_str());
    Serial.printf("Rezultat: http://%s:82/rezultat\r\n", wifi.ip().c_str());
    Serial.printf("Servo:    http://%s:82/servo?grade=90\r\n", wifi.ip().c_str());
}

void loop() {
    apiServer.handleClient();
}