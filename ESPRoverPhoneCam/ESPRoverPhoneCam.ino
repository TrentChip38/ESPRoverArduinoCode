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
<div id="battery" style="
position: fixed;
top: 10px;
right: 10px;
color: white;
font-size: 16px;
background: rgba(0,0,0,0.4);
padding: 5px 10px;
border-radius: 8px;">
Battery: -- V
</div>
<script>
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

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
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
  server.on("/stream", HTTP_GET, handle_stream);
  server.on("/battery", handle_battery);

  server.begin();

  Serial.print("Go to: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}