// SPDX-FileCopyrightText: 2023 Limor Fried for Adafruit Industries
// SPDX-License-Identifier: MIT

#include "Adafruit_PyCamera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

Adafruit_PyCamera pycamera;

// ===== WIFI SETTINGS =====
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// ===== CAMERA SETTINGS =====
framesize_t validSizes[] = {
  FRAMESIZE_QQVGA, FRAMESIZE_QVGA, FRAMESIZE_HVGA,
  FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA,
  FRAMESIZE_HD, FRAMESIZE_SXGA, FRAMESIZE_UXGA,
  FRAMESIZE_QXGA, FRAMESIZE_QSXGA
};

uint32_t ringlightcolors_RGBW[] = {
  0x00000000, 0x00FF0000, 0x00FFFF00,
  0x0000FF00, 0x0000FFFF, 0x000000FF,
  0x00FF00FF, 0xFF000000
};

uint8_t ringlight_i = 0;
uint8_t ringlightBrightness = 100;

#define IRQ 3

// ===== WEB HANDLER =====
void handleRoot() {
  server.send(200, "text/html",
              "<h1>Hello World from Memento ESP32-S3!</h1>");
}

void setup() {

  Serial.begin(115200);
  delay(100);

  // ===== WIFI CONNECT =====
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  // ===== CAMERA INIT =====
  if (!pycamera.begin()) {
    Serial.println("Failed to initialize pyCamera interface");
    while (1) yield();
  }

  Serial.println("pyCamera hardware initialized!");

  pinMode(IRQ, INPUT_PULLUP);
  attachInterrupt(IRQ, [] { Serial.println("IRQ!"); }, FALLING);
}

void loop() {

  // Handle web server
  server.handleClient();

  static uint8_t loopn = 0;

  pycamera.setNeopixel(pycamera.Wheel(loopn));
  loopn += 8;

  pycamera.readButtons();
  pycamera.captureFrame();

  // ===== DISPLAY TEXT =====
  pycamera.fb->setCursor(0, 100);
  pycamera.fb->setTextSize(4);
  pycamera.fb->setTextColor(pycamera.color565(255, 255, 255));
  pycamera.fb->print("Hello World");

  // Frame size display
  pycamera.fb->setCursor(0, 200);
  pycamera.fb->setTextSize(2);
  pycamera.fb->setTextColor(pycamera.color565(255, 255, 255));
  pycamera.fb->print("Size:");

  switch (pycamera.photoSize) {
    case FRAMESIZE_QQVGA: pycamera.fb->print("160x120"); break;
    case FRAMESIZE_QVGA:  pycamera.fb->print("320x240"); break;
    case FRAMESIZE_HVGA:  pycamera.fb->print("480x320"); break;
    case FRAMESIZE_VGA:   pycamera.fb->print("640x480"); break;
    case FRAMESIZE_SVGA:  pycamera.fb->print("800x600"); break;
    case FRAMESIZE_XGA:   pycamera.fb->print("1024x768"); break;
    case FRAMESIZE_HD:    pycamera.fb->print("1280x720"); break;
    case FRAMESIZE_SXGA:  pycamera.fb->print("1280x1024"); break;
    case FRAMESIZE_UXGA:  pycamera.fb->print("1600x1200"); break;
    case FRAMESIZE_QXGA:  pycamera.fb->print("2048x1536"); break;
    case FRAMESIZE_QSXGA: pycamera.fb->print("2560x1920"); break;
    default: pycamera.fb->print("Unknown"); break;
  }

  // ===== ACCEL DATA =====
  float x_ms2, y_ms2, z_ms2;
  if (pycamera.readAccelData(&x_ms2, &y_ms2, &z_ms2)) {
    pycamera.fb->setCursor(0, 220);
    pycamera.fb->setTextSize(2);
    pycamera.fb->setTextColor(pycamera.color565(255, 255, 255));
    pycamera.fb->print("This is Teerath");
  }

  pycamera.blitFrame();

  // ===== BUTTON CONTROLS =====
  if (pycamera.justPressed(AWEXP_BUTTON_UP)) {
    for (int i = 0; i < sizeof(validSizes) / sizeof(framesize_t) - 1; ++i) {
      if (pycamera.photoSize == validSizes[i]) {
        pycamera.photoSize = validSizes[i + 1];
        break;
      }
    }
  }

  if (pycamera.justPressed(AWEXP_BUTTON_DOWN)) {
    for (int i = sizeof(validSizes) / sizeof(framesize_t) - 1; i > 0; --i) {
      if (pycamera.photoSize == validSizes[i]) {
        pycamera.photoSize = validSizes[i - 1];
        break;
      }
    }
  }

  if (pycamera.justPressed(AWEXP_BUTTON_RIGHT)) {
    pycamera.specialEffect = (pycamera.specialEffect + 1) % 7;
    pycamera.setSpecialEffect(pycamera.specialEffect);
  }

  if (pycamera.justPressed(AWEXP_BUTTON_LEFT)) {
    pycamera.specialEffect = (pycamera.specialEffect + 6) % 7;
    pycamera.setSpecialEffect(pycamera.specialEffect);
  }

  if (pycamera.justPressed(AWEXP_BUTTON_OK)) {
    ringlight_i =
      (ringlight_i + 1) % (sizeof(ringlightcolors_RGBW) / sizeof(uint32_t));
    pycamera.setRing(ringlightcolors_RGBW[ringlight_i]);
  }

  if (pycamera.justPressed(AWEXP_BUTTON_SEL)) {
    if (ringlightBrightness >= 250)
      ringlightBrightness = 0;
    else
      ringlightBrightness += 50;

    pycamera.ring.setBrightness(ringlightBrightness);
    pycamera.setRing(ringlightcolors_RGBW[ringlight_i]);
  }

  if (pycamera.justPressed(SHUTTER_BUTTON)) {
    if (pycamera.takePhoto("IMAGE", pycamera.photoSize)) {
      pycamera.speaker_tone(100, 50);
    }
  }

  delay(50);
}
