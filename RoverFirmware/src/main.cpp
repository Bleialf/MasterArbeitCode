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
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"

const char *ssid = "RTG-Control-Unit";
const char *password = "19dawson91";

void startCameraServer();

#pragma region
typedef unsigned char u8;

#define OV2640_MAXLEVEL_SHARPNESS 6

const static u8 OV2640_SHARPNESS_AUTO[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0x20, 0x20,
        0x00, 0x00, 0x00};

const static u8 OV2640_SHARPNESS_MANUAL[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0x00, 0x20,
        0x00, 0x00, 0x00};

const static u8 OV2640_SHARPNESS_LEVEL0[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xc0, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL1[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xc1, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL2[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xc2, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL3[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xc4, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL4[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xc8, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL5[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xd0, 0x1f,
        0x00, 0x00, 0x00};
const static u8 OV2640_SHARPNESS_LEVEL6[] =
    {
        0xFF, 0x00, 0xff,
        0x92, 0x01, 0xff,
        0x93, 0xdf, 0x1f,
        0x00, 0x00, 0x00};

const static u8 *OV_SETTING_SHARPNESS[] =
    {
        OV2640_SHARPNESS_LEVEL0,
        OV2640_SHARPNESS_LEVEL1,
        OV2640_SHARPNESS_LEVEL2,
        OV2640_SHARPNESS_LEVEL3,
        OV2640_SHARPNESS_LEVEL4,
        OV2640_SHARPNESS_LEVEL5,
        OV2640_SHARPNESS_LEVEL6};

static int table_mask_write(const u8 *ptab)
{
  u8 address;
  u8 value, orgval;
  u8 mask;
  const u8 *pdata = ptab;

  if (NULL == pdata)
    return -1;
  sensor_t *s = esp_camera_sensor_get();

  while (1)
  {
    address = *pdata++;
    value = *pdata++;
    mask = *pdata++;
    if ((0 == address) && (0 == value) && (0 == mask))
    {
      break;
    }

    s->set_reg(s, address, mask, value);
  }

  return 0;
}

int change_sharpness(int sharpness)
{
  if (sharpness > OV2640_MAXLEVEL_SHARPNESS)
  {
    return -1;
  }

  if (sharpness < 0)
  {
    table_mask_write(OV2640_SHARPNESS_AUTO);
  }
  else
  {
    table_mask_write(OV2640_SHARPNESS_MANUAL);
    table_mask_write(OV_SETTING_SHARPNESS[sharpness]);
  }

  return 0;
}

#pragma endregion

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
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
    config.jpeg_quality = 1;
    config.fb_count = 2;
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

  // CAMERA SETTINGS
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, -2);      // -2 to 2
  s->set_saturation(s, -2);    // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 4);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  // s->set_ae_level(s, 2); // -2 to 2
  // s->set_aec_value(s, 400); // 0 to 1200
  s->set_gain_ctrl(s, 0);                  // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);                   // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)6); // 0 to 6
  s->set_bpc(s, 1);                        // 0 = disable , 1 = enable
  s->set_wpc(s, 1);                        // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);                    // 0 = disable , 1 = enable (makes much lighter and noisy)
  s->set_lenc(s, 0);                       // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);                    // 0 = disable , 1 = enable
  s->set_vflip(s, 0);                      // 0 = disable , 1 = enable
  s->set_dcw(s, 0);                        // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable
  s->set_vflip(s, 1);                      // flip it back
  s->set_framesize(s, FRAMESIZE_UXGA);

  s->set_reg(s, 0xff, 0xff, 0x01); // banksel
  s->set_reg(s, 0x11, 0xff, 03);   // frame rate

  s->set_reg(s, 0xff, 0xff, 0x00); // banksel
  s->set_reg(s, 0x86, 0xff, 1);    // disable effects

  s->set_reg(s, 0xd3, 0xff, 5); // clock

  s->set_reg(s, 0x42, 0xff, 0x4f); // image quality (lower is bad)
  s->set_reg(s, 0x44, 0xff, 1);    // quality

  change_sharpness(0);
  delay(1200);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();

  http.begin("http://192.168.1.10:5001/image"); // Specify the URL

  Serial.println("Image conversion successful...Sending");
  int httpCode = http.POST(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  Serial.print("Bufferlänge");
  Serial.println(fb->len);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    return;
  }

  // if (httpCode > 0)
  // { // Check for the returning code

  //   String payload = http.getString();
  //   int sleeptime = payload.toInt();
  //   Serial.println(httpCode);
  //   Serial.println(sleeptime);
  //   Serial.println(payload);
  // }

  else
  {
    Serial.println("Error on HTTP request");
  }

  // http.end();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}