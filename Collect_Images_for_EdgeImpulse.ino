/**
 * Collect images for Edge Impulse image
 * classification / object detection
 *
 * BE SURE TO SET "TOOLS > CORE DEBUG LEVEL = INFO"
 * to turn on debug messages
 */

// if you define WIFI_SSID and WIFI_PASS before importing the library, 
// you can call connect() instead of connect(ssid, pass)
//
// If you set HOSTNAME and your router supports mDNS, you can access
// the camera at http://{HOSTNAME}.local

#define WIFI_SSID "DIGI-34bY"
#define WIFI_PASS "PCtQZ3CNTN"
#define HOSTNAME "esp32cam"


#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/wifi/sta.h>
#include <eloquent_esp32cam/viz/image_collection.h>

using eloq::camera;
using eloq::wifi;
using eloq::viz::collectionServer;

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define SERVO_PIN 16
Servo servo;

#define I2C_SDA 15
#define I2C_SCL 14
TwoWire I2Cbus = TwoWire(0);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cbus, OLED_RESET);


void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("___IMAGE COLLECTION SERVER___");

    I2Cbus.begin(I2C_SDA, I2C_SCL, 100000);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("OLED failed!");
        while (true);
    }

    // Mesaj initial pe display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Hello world!");
    display.display();

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
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    // connect to WiFi
    while (!wifi.connect().isOk())
      Serial.println(wifi.exception.toString());

    // init face detection http server
    while (!collectionServer.begin().isOk())
        Serial.println(collectionServer.exception.toString());

    // ESP32PWM::allocateTimer(1);
    // servo.setPeriodHertz(50);
    // servo.attach(SERVO_PIN, 500, 2400);
    // servo.write(0);

    Serial.println("Camera OK");
    Serial.println("WiFi OK");
    Serial.println("Image Collection Server OK");
    Serial.println(collectionServer.address());
    
}


void loop() {
    // server runs in a separate thread, no need to do anything here
    // servo.write(90);
    // delay(2000);
    // servo.write(0);
    // delay(2000);
}