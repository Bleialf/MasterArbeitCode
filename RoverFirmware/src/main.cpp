#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_http_client.h"

HTTPClient http;
//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"

struct RTCData{
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t ap_mac[6];// 6 bytes, 11 in total
};


RTC_DATA_ATTR bool rtcAvailable;
RTC_DATA_ATTR RTCData rtcstate;

const char *ssid = "RTG-Control-Unit";
const char *password = "19dawson91";

//We will use static ip
IPAddress ip( 192,168,1,123 );// pick your own suitable static IP address
IPAddress gateway( 192,168,1,1 ); // may be different for your network
IPAddress subnet( 255, 255, 255, 0 ); // may be different for your network (but this one is pretty standard)
IPAddress dns(192,168,1,1);


void startCameraServer();
void printmem();
void printTimes();
void printAvg();
void printMac(uint8_t* data);



void setup()
{
  unsigned long start = millis();
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound())
  {
    Serial.println("PSRAM FOUND");
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 4;
    config.fb_count = 1;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  s->set_framesize(s, FRAMESIZE_UXGA);
  s->set_vflip(s, 1);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.persistent( false );
  WiFi.config( ip,dns, gateway, subnet );

if( rtcAvailable ) {
  // The RTC data was good, make a quick connection
  Serial.println("Using rtcvalues");
  Serial.print("Channel: ");
  Serial.println(rtcstate.channel);
  Serial.print("Bssid: ");
  printMac(rtcstate.ap_mac);

  WiFi.begin( ssid, password, rtcstate.channel, rtcstate.ap_mac );
}
else {
  // The RTC data was not valid, so make a regular connection
  Serial.println("No RTCValues");
  WiFi.begin( ssid, password );
}


  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 50)
  {
    delay(100);
    counter++;
  }

if (WiFi.status() != WL_CONNECTED){
  Serial.printf("NoConnection Sleeping for %dseconds\n", 10);
  rtcAvailable = false;
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(1000000 * 10);
  esp_deep_sleep_start();
}

  Serial.println("Saving Wifistate to RTC-Mem");
  Serial.print("Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("Bssid: ");

  rtcstate.channel = WiFi.channel();
  memcpy( rtcstate.ap_mac, WiFi.BSSID(), 6 );
  printMac(rtcstate.ap_mac);
  rtcAvailable = true;

  Serial.println("");
  Serial.println("WiFi connected");

  camera_fb_t *fb = NULL;
  for (int i = 0; i < 3; i++){
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
  }

  printmem();


  http.begin("http://192.168.1.107:5001/image"); // Specify the URL

  Serial.println("Image conversion successful...");
  Serial.printf("Sending %dBytes\n", fb->len);
  int httpCode = http.POST(fb->buf, fb->len);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    return;
  }

  int sleeptime = 10;
  if (httpCode > 0)
  { // Check for the returning code

    String payload = http.getString();
    sleeptime = payload.toInt();
  }
  else
  {
    Serial.println("Error on HTTP request");
    Serial.println("Sleeping 10 Seconds");
  }
  http.end();

  unsigned long dif = millis() - start;
  Serial.printf("Whole Operation took %dms\n", dif);
  Serial.printf("Whole Operation took %dms\n", dif);

  http.begin("http://192.168.1.107:5001/time");

  http.POST(String(dif));
  http.end();

  Serial.printf("Sleeping for %dseconds\n", sleeptime);
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(1000000 * sleeptime);
  esp_deep_sleep_start();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}

void printmem(){
   Serial.printf("Total Heap: %d\n", ESP.getHeapSize());
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
}

void printMac(uint8_t* data){
for(int i=0; i<6; i++){
    Serial.printf(" %02X ", data[i]);
 }
}