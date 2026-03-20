#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include <ESPmDNS.h>

// ---------------- WIFI ----------------
const char* ssid = "samukospot";
const char* password = "samuko312";

#define FLASH_LED_PIN 4

// ---------------- CAMERA PINS (AI Thinker) ----------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0

#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ------------------------------------------------------------

// static esp_err_t stream_handler(httpd_req_t *req)
// {
//     camera_fb_t * fb = NULL;
//     esp_err_t res = ESP_OK;

//     httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");

//     while(true)
//     {
//         fb = esp_camera_fb_get();
//         if (!fb) {
//             return ESP_FAIL;
//         }

//         char buf[64];
//         int len = snprintf(buf, sizeof(buf),
//             "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
//             fb->len);

//         httpd_resp_send_chunk(req, buf, len);
//         httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
//         httpd_resp_send_chunk(req, "\r\n", 2);

//         esp_camera_fb_return(fb);
//     }

//     return res;
// }

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    // TURN FLASH LED ON when streaming starts
    digitalWrite(FLASH_LED_PIN, HIGH);

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");

    while(true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            res = ESP_FAIL;
            break;
        }

        char buf[64];
        int len = snprintf(buf, sizeof(buf),
            "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            fb->len);

        if (httpd_resp_send_chunk(req, buf, len) != ESP_OK)
        {
            esp_camera_fb_return(fb);
            break;
        }

        if (httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len) != ESP_OK)
        {
            esp_camera_fb_return(fb);
            break;
        }

        if (httpd_resp_send_chunk(req, "\r\n", 2) != ESP_OK)
        {
            esp_camera_fb_return(fb);
            break;
        }

        esp_camera_fb_return(fb);

        delay(1);
    }

    // TURN FLASH LED OFF when streaming stops
    digitalWrite(FLASH_LED_PIN, LOW);

    return res;
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.ctrl_port = 32768;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = stream_handler,
            .user_ctx = NULL,
            // .is_websocket = true
        };

        httpd_register_uri_handler(server, &stream_uri);
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(FLASH_LED_PIN, OUTPUT);

    // -------- CAMERA CONFIG --------
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    // config.pixel_format = PIXFORMAT_RGB888;

    config.frame_size = FRAMESIZE_QVGA;   // 320x240 (fast)
    config.jpeg_quality = 18;             // lighter compression
    config.fb_count = 2;  

    if (esp_camera_init(&config) != ESP_OK)
    {
        Serial.println("Camera init failed");
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    s->set_brightness(s, 0);
    s->set_saturation(s, 0);
    s->set_contrast(s, 0);

    // -------- WIFI --------
    WiFi.begin(ssid, password);

    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // Initialize mDNS
  if (!MDNS.begin("espcam")) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

    Serial.println();
    Serial.print("Camera Stream Ready: http://");
    // Serial.print(WiFi.localIP());
    // Serial.print("mobo-mini-cam");
    Serial.println(":81/stream");

    digitalWrite(FLASH_LED_PIN, LOW);   // LED OFF
    delay(500);
    digitalWrite(FLASH_LED_PIN, HIGH);  // LED ON
    delay(1500);
    digitalWrite(FLASH_LED_PIN, LOW);   // LED OFF
    

    startCameraServer();
}

void loop()
{
}
