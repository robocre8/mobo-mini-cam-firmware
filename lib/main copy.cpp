#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

WiFiUDP Udp;
const int UDP_PORT = 8888;

#include "command_functions.h"
#include "udp_comm.h"

// ---------------- WIFI CONFIG ----------------
const char* WIFI_SSID = "samukospot";
const char* WIFI_PASS = "samuko312";

//---------------------------------------------------------------------------------------------
// Timing variables in esp_timer_get_timeeconds
// please do not adjust any of the values as it can affect important operations
uint64_t udpCommTime, udpCommTimeInterval = 5000;
uint64_t sensorReadTime, sensorReadTimeInterval = 50000;
uint64_t ctrlTime, ctrlTimeInterval = 50000;
//---------------------------------------------------------------------------------------------


void setup()
{
  // Serial.begin(57600);
  Serial.begin(115200);
  // Serial.begin(460800);
  // Serial.begin(921600);
  // Serial.setTimeout(2);

  // ---------------- WIFI CONNECT ----------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. ESP32 IP: ");
  // Serial.println(WiFi.localIP());
  // Initialize mDNS
  if (!MDNS.begin("mobo-mini")) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  WiFi.setSleep(false);
  // ---------------- UDP START ----------------
  Udp.begin(UDP_PORT);

  Wire.begin();
  tof_sensor_init();
  gripper.begin();

  bool connect_success = controller.begin(); // takes about 3 to 4 secs to connect
  if (!connect_success) {
    Serial.println("Error Connecting to EPMC. Probably due to Supported NumOfMotor Mismatch");
    while(true);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  digitalWrite(LED_PIN, HIGH);
  delay(1500);
  digitalWrite(LED_PIN, LOW);

  gripper.open();

  controller.clearDataBuffer();
  controller.writeSpeed(0.0, 0.0);
  controller.setCmdTimeout(0); // set motor command velocity timeout

  // Initialize timing markers
  uint64_t now_us = esp_timer_get_time();
  udpCommTime = now_us;
  sensorReadTime = now_us;
  ctrlTime = now_us;
}

void loop()
{
  uint64_t now_us = esp_timer_get_time();

  // udp comm loop
  if( (now_us - udpCommTime) > udpCommTimeInterval ) {
    recieve_and_send_data();
    udpCommTime = now_us;
  }

  // sensor read loop
  if( (now_us - sensorReadTime) > sensorReadTimeInterval ) {
    tof_dist_mm = tof_sensor_get_dist_mm();

    float tl=0.0, tr=0.0, wl=0.0, wr=0.0;
    float x=0.0, y=0.0, t=0.0, v=0.0, w=0.0, d=0.0;
    controller.readPos(tl, tr);
    controller.readVel(wl, wr);

    d = R*(tr+tl)/2.0;
    t = R*(tr-tl)/L;
    x = d*cos(t);
    y = d*sin(t);
    v = R*(wr+wl)/2.0;
    w = R*(wr-wl)/2.0;

    odom_data[0] = x;
    odom_data[1] = y;
    odom_data[2] = t;
    odom_data[3] = v;
    odom_data[4] = w;
    odom_data[5] = d;

    sensorReadTime = now_us;
  }

  // gripper control loop
  if( (now_us - ctrlTime) > ctrlTimeInterval ) {
    gripper.open(gripper_dist_mm);

    float wl=0.0, wr=0.0;
    wl = computeWL(cmd_vel[0], cmd_vel[1]);
    wr = computeWR(cmd_vel[0], cmd_vel[1]);

    Serial.println(wl);
    Serial.println(wl);

    controller.writeSpeed(wl, wr);

    ctrlTime = now_us;
  }

}