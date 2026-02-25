#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ================= CAMERA PINS (AI Thinker) =================
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

#define WIFI_SSID "ESP_CAM_AP"
#define WIFI_PASS "12345678"

#define BATTERY_PIN 33
#define R1 100000.0
#define R2 100000.0

WebServer server(80);

float readBatteryVoltage() {

  long total = 0;
  for(int i=0;i<10;i++){
    total += analogRead(BATTERY_PIN);
    delay(2);
  }

  float raw = total / 10.0;

  float adcVoltage = (raw / 4095.0) * 3.3;
  float batteryVoltage = adcVoltage * ((R1 + R2) / R2);

  return batteryVoltage;
}
void handle_battery() {
  float voltage = readBatteryVoltage();
  server.send(200, "text/plain", String(voltage, 2));
}
void handle_rssi() {
  int rssi = WiFi.RSSI();
  Serial.print("RSSI request: ");
  Serial.println(rssi);
  server.send(200, "text/plain", String(rssi));
}
void handle_move() {

  if (!server.hasArg("dir")) {
    server.send(400, "text/plain", "Missing dir");
    return;
  }

  String dir = server.arg("dir");

  Serial.print("Joystick direction: ");
  Serial.println(dir);

  // ===== MOTOR CONTROL HOOKS =====
  if (dir == "up") {
    moveForward();
  }
  else if (dir == "down") {
    moveBackward();
  }
  else if (dir == "left") {
    turnLeft();
  }
  else if (dir == "right") {
    turnRight();
  }
  else if (dir == "stop") {
    stopMotors();
  }

  server.send(200, "text/plain", "OK");
}

// ================= STREAM HANDLER =================
void handle_stream() {

  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {

    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) continue;

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.println();
    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);

    delay(60); // stable FPS
  }
}

// ================= ROOT PAGE =================
void handle_root() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  background:#001a33;
  color:white;
  text-align:center;
  font-family:Arial;
}
img {
  width:95%;
  max-width:480px;
  border-radius:12px;
  margin-top:20px;
}
</style>
<div id="telemetry" style="
position: fixed;
top: 10px;
left: 10px;
background: rgba(0,0,0,0.4);
padding: 8px;
border-radius: 10px;
font-size:14px;">
Battery: <span id="battery">--</span> V<br>
Signal: <span id="rssi">--</span> dBm
</div>
<script>
// ===== RSSI UPDATE =====
function updateRSSI() {
  fetch('/rssi')
    .then(r => r.text())
    .then(data => {
      document.getElementById("rssi").innerHTML = data;
    });
}

setInterval(updateRSSI, 2000);
updateRSSI();

// ===== JOYSTICK =====
let stick = document.getElementById("stick");
let container = document.getElementById("joystickContainer");

let center = 80;
let active = false;

container.addEventListener("touchstart", e => active = true);
container.addEventListener("touchend", e => {
  active = false;
  stick.style.left = "45px";
  stick.style.top = "45px";
  fetch('/move?dir=stop');
});

container.addEventListener("touchmove", e => {

  if (!active) return;

  let rect = container.getBoundingClientRect();
  let touch = e.touches[0];

  let x = touch.clientX - rect.left - center;
  let y = touch.clientY - rect.top - center;

  let angle = Math.atan2(y, x);
  let distance = Math.min(60, Math.sqrt(x*x + y*y));

  let stickX = center + distance * Math.cos(angle);
  let stickY = center + distance * Math.sin(angle);

  stick.style.left = (stickX - 35) + "px";
  stick.style.top = (stickY - 35) + "px";

  // Direction logic
  if (Math.abs(x) > Math.abs(y)) {
    if (x > 20) fetch('/move?dir=right');
    else if (x < -20) fetch('/move?dir=left');
  } else {
    if (y > 20) fetch('/move?dir=down');
    else if (y < -20) fetch('/move?dir=up');
  }

});
function updateBattery() {
  fetch('/battery')
    .then(response => response.text())
    .then(data => {
      document.getElementById("battery").innerHTML = 
        "Battery: " + data + " V";
    });
}

setInterval(updateBattery, 2000); // every 2 seconds
updateBattery();
</script>
</head>
<body>

<h2>ESP32-CAM Live Stream</h2>

<img src="/stream">
<div id="joystickContainer" style="
position: fixed;
bottom: 40px;
left: 50%;
transform: translateX(-50%);
width: 160px;
height: 160px;
background: rgba(255,255,255,0.1);
border-radius: 50%;
">

<div id="stick" style="
width: 70px;
height: 70px;
background: #3aa0ff;
border-radius: 50%;
position: absolute;
top: 45px;
left: 45px;">
</div>

</div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}
// ================= RSSI =================
// void handle_rssi() {
//   int rssi = WiFi.RSSI();
//   server.send(200, "text/plain", String(rssi));
// }

// ================= MOTOR CONTROL =================
void moveUp() {
  Serial.println("MOTOR: UP");
}

void moveDown() {
  Serial.println("MOTOR: DOWN");
}

void moveLeft() {
  Serial.println("MOTOR: LEFT");
}

void moveRight() {
  Serial.println("MOTOR: RIGHT");
}

// void stopMotors() {
//   Serial.println("MOTOR: STOP");
// }

// Handle joystick direction from web
void handle_joystick() {
  String dir = server.arg("dir");

  if (dir == "up") moveUp();
  else if (dir == "down") moveDown();
  else if (dir == "left") moveLeft();
  else if (dir == "right") moveRight();
  else stopMotors();

  server.send(200, "text/plain", "OK");
}
// ================= SETUP =================
void setup() {

  Serial.begin(115200);

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
  config.frame_size = FRAMESIZE_QQVGA;  // safest size
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // allows reading up to ~3.6V
  pinMode(BATTERY_PIN, INPUT);

  server.on("/", handle_root);
  server.on("/move", handle_move);
  server.on("/stream", HTTP_GET, handle_stream);
  server.on("/battery", handle_battery);
  server.on("/rssi", handle_rssi);

  server.begin();

  Serial.print("Go to: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}

void moveForward() {
  Serial.println("MOTOR: Forward");
  // TODO: Add motor driver code
}

void moveBackward() {
  Serial.println("MOTOR: Backward");
}

void turnLeft() {
  Serial.println("MOTOR: Left");
}

void turnRight() {
  Serial.println("MOTOR: Right");
}

void stopMotors() {
  Serial.println("MOTOR: Stop");
}