#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// =====================
// CAMERA PINS (AI Thinker)
// =====================
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5

#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// =====================
#define FLASH_LED_PIN 4
bool flashState = false;

#define WIFI_SSID "ESP_CAM_AP_Trent"
#define WIFI_PASS "12345678"

WebServer server(80);

// =====================
// MOTOR FUNCTION STUBS
// =====================
void moveUp()    { Serial.println("UP"); }
void moveDown()  { Serial.println("DOWN"); }
void moveLeft()  { Serial.println("LEFT"); }
void moveRight() { Serial.println("RIGHT"); }
void stopMotors(){ Serial.println("STOP"); }

// =====================
// MJPEG STREAM HANDLER
// =====================
void handle_stream() {
  WiFiClient client = server.client();

  String response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) continue;

    server.sendContent("--frame\r\n");
    server.sendContent("Content-Type: image/jpeg\r\n\r\n");
    client.write(fb->buf, fb->len);
    server.sendContent("\r\n");

    esp_camera_fb_return(fb);

    if (!client.connected()) break;
  }
}

// =====================
// WEB PAGE
// =====================
void handle_root() {

  String buttonColor = flashState ? "#4da6ff" : "#001f4d";

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color:#000d26;
      text-align:center;
      font-family:Arial;
      color:white;
    }
    img {
      display:block;
      margin:auto;
      max-width:90%;
      border-radius:10px;
    }
    .flashBtn {
      margin-top:20px;
      padding:25px 40px;
      font-size:24px;
      border:none;
      border-radius:15px;
      color:white;
      cursor:pointer;
    }
    .joyBtn {
      width:80px;
      height:80px;
      font-size:24px;
      border-radius:50%;
      border:none;
      margin:10px;
      background:#003366;
      color:white;
    }
  </style>
  </head>
  <body>

  <h2>ESP32-CAM Live Stream</h2>

  <img src="/stream">

  <form action="/toggle">
    <button class="flashBtn" style="background-color:)rawliteral" 
    + buttonColor + R"rawliteral(">
      )rawliteral" + String(flashState ? "Turn Flash OFF" : "Turn Flash ON") + R"rawliteral(
    </button>
  </form>

  <br><br>

  <!-- JOYSTICK -->
  <div style="margin-top:40px;">
    <form action="/up"><button class="joyBtn">↑</button></form><br>
    <form action="/left" style="display:inline;"><button class="joyBtn">←</button></form>
    <form action="/stop" style="display:inline;"><button class="joyBtn">■</button></form>
    <form action="/right" style="display:inline;"><button class="joyBtn">→</button></form><br>
    <form action="/down"><button class="joyBtn">↓</button></form>
  </div>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// =====================
// ROUTES
// =====================
void handle_toggle() {
  flashState = !flashState;
  digitalWrite(FLASH_LED_PIN, flashState ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handle_up()    { moveUp();    server.sendHeader("Location", "/"); server.send(303); }
void handle_down()  { moveDown();  server.sendHeader("Location", "/"); server.send(303); }
void handle_left()  { moveLeft();  server.sendHeader("Location", "/"); server.send(303); }
void handle_right() { moveRight(); server.sendHeader("Location", "/"); server.send(303); }
void handle_stop()  { stopMotors();server.sendHeader("Location", "/"); server.send(303); }

// =====================
void setup() {

  Serial.begin(115200);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d7 = CAM_PIN_D7;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    return;
  }

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.println("Connect to: ESP_CAM_AP");
  Serial.print("Go to: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handle_root);
  server.on("/stream", HTTP_GET, handle_stream);
  server.on("/toggle", handle_toggle);
  server.on("/up", handle_up);
  server.on("/down", handle_down);
  server.on("/left", handle_left);
  server.on("/right", handle_right);
  server.on("/stop", handle_stop);

  server.begin();
}

void loop() {
  server.handleClient();
}