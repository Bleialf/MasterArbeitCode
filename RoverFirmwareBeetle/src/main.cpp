#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Buffer to store captured jpegs and framebuffers
uint8_t* jpeg_buffer;

uint32_t jpegLength;
uint8_t vid, pid;
uint8_t temp;

#define JPEG_BUF_SIZE 110000
#define CAM_MOSFET_PIN 16
#define Debug

// ArduCAM driver handle with CS pin defined as pin 5 for ESP32 dev kits
const int CS = 5;
ArduCAM myCAM(OV5642, CS);

// Initializes the I2C interface to the camera
//  The code after Wire.begin() isn't strictly necessary, but it verifies that the I2C interface is
//  working correctly and that the expected OV2640 camera is connected.
void arducam_i2c_init()
{
  // Initialize I2C bus
  Wire.begin();
}

// Initializes the SPI interface to the camera
//  The code after SPI.begin() isn't strictly necessary, but it implements some workarounds to
//  common problems and verifies that the SPI interface is working correctly
void arducam_spi_init()
{
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  // initialize SPI:
  SPI.begin();

  // // Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  // delay(100);
  // myCAM.write_reg(0x07, 0x00);
}

// Initializes the camera driver and hardware with desired settings. Should run once during setup()
void arducam_init()
{
  arducam_spi_init();
  arducam_i2c_init();
  // set to JPEG format, this works around issues with the color data when sampling in RAW formats
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);

  // Specify the smallest possible resolution
  myCAM.OV5642_set_JPEG_size(OV5642_1280x960);
  myCAM.OV5642_set_Compress_quality(4);
  Serial.println(F("Camera initialized."));
}

// Capture a photo on the camera. This method only takes the photograph, call arducam_transfer() to
// read get the data
void arducam_capture()
{
  // Make sure the buffer is emptied before each capture
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  // Start capture
  myCAM.start_capture();
  // Wait for indication that it is done
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    ;
  delay(10);
  myCAM.clear_fifo_flag();
}

uint32_t read_fifo_burst(ArduCAM myCAM, uint8_t buf[], uint32_t buf_len)
{
  uint32_t length = 0;
  length = myCAM.read_fifo_length();

  if (length >= MAX_FIFO_SIZE) // 512 kb
  {
    Serial.println(F("ACK CMD Over size. END"));
    return 0;
  }
  if (length == 0) // 0 kb
  {
    Serial.println(F("ACK CMD Size is 0."));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode
  for (int index = 0; index < length; index++)
  {
    buf[index] = SPI.transfer(0x00);
    delayMicroseconds(5);
  }
  myCAM.CS_HIGH();
  // is_header = false;
  return length;
}

void getImage()
{
  memset(jpeg_buffer, 0, JPEG_BUF_SIZE);
#ifdef Debug
  Serial.println(F("Cleared Memory"));
  #endif
  // Take the photo
  arducam_capture();
  jpegLength = read_fifo_burst(myCAM, jpeg_buffer, JPEG_BUF_SIZE);
}



HTTPClient http;

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

struct RTCData{
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t ap_mac[6];// 6 bytes, 11 in total
};


RTC_DATA_ATTR bool rtcAvailable;
RTC_DATA_ATTR RTCData rtcstate;

const char *ssid = "camhotspot";
const char *password = NULL;

//We will use static ip
IPAddress ip( 192,168,179,123 );// pick your own suitable static IP address
IPAddress gateway(  192,168,179,1 ); // may be different for your network
IPAddress subnet( 255, 255, 255, 0 ); // may be different for your network (but this one is pretty standard)
IPAddress dns(192,168,179,1);


void printmem();
void printTimes();
void printAvg();
void printMac(uint8_t* data);



void setup()
{
  pinMode(CAM_MOSFET_PIN, OUTPUT);
  digitalWrite(CAM_MOSFET_PIN, HIGH);
  unsigned long start = millis();
#ifdef Debug

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();
  #endif
  arducam_init();

  printmem();

  jpeg_buffer = (uint8_t*)malloc(JPEG_BUF_SIZE);

  WiFi.persistent( false );
  WiFi.config( ip,dns, gateway, subnet );

if( rtcAvailable ) {
  // The RTC data was good, make a quick connection
  #ifdef Debug
  Serial.println(F("Using rtcvalues"));
  Serial.print(F("Channel: "));
  Serial.println(rtcstate.channel);
  Serial.print(F("Bssid: "));
  printMac(rtcstate.ap_mac);
  #endif
  WiFi.begin( ssid, password, rtcstate.channel, rtcstate.ap_mac );
}
else {
  // The RTC data was not valid, so make a regular connection
  Serial.println(F("No RTCValues"));
  WiFi.begin( ssid, password );
}


  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 50)
  {
    delay(100);
    counter++;
  }

if (WiFi.status() != WL_CONNECTED){
  #ifdef Debug
  Serial.printf("NoConnection Sleeping for %dseconds\n", 10);
  #endif
  rtcAvailable = false;
  digitalWrite(CAM_MOSFET_PIN, LOW);
  WiFi.disconnect();
  delay(100);
  esp_sleep_enable_timer_wakeup(1000000 * 10);
  esp_deep_sleep_start();
}
#ifdef Debug
  Serial.println(F("Saving Wifistate to RTC-Mem"));
  Serial.print("Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("Bssid: ");
  #endif

  rtcstate.channel = WiFi.channel();
  memcpy( rtcstate.ap_mac, WiFi.BSSID(), 6 );
  printMac(rtcstate.ap_mac);
  rtcAvailable = true;

#ifdef Debug
  Serial.println("");
  Serial.println("WiFi connected");
  #endif

  getImage();
  digitalWrite(CAM_MOSFET_PIN, LOW);

  printmem();


  http.begin(F("http://192.168.179.2:5001/image")); // Specify the URL

#ifdef Debug
  Serial.println(F("Image conversion successful..."));
  Serial.printf("Sending %dBytes\n", jpegLength);
#endif

  int httpCode = http.POST(jpeg_buffer, jpegLength);

  int sleeptime = 10;
   if (httpCode > 0)
   { // Check for the returning code

     String payload = http.getString();
     sleeptime = payload.toInt();
  }
  else
  {
    #ifdef Debug
    Serial.println(F("Error on HTTP request"));
    Serial.println(F("Sleeping 10 Seconds"));
    #endif
  }
  http.end();

  unsigned long dif = millis() - start;
  #ifdef Debug
  Serial.printf("Whole Operation took %dms\n", dif);
  #endif

  http.begin(F("http://192.168.179.2:5001/time"));

  http.POST(String(dif));
  http.end();

#ifdef Debug
  Serial.printf("Sleeping for %dseconds\n", sleeptime);
  #endif
  WiFi.disconnect();
  delay(10);
  esp_sleep_enable_timer_wakeup(1000000 * sleeptime);
  esp_deep_sleep_start();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}

void printmem(){
#ifdef Debug

   Serial.printf("Total Heap: %d\n", ESP.getHeapSize());
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
  // Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

  #endif
}

void printMac(uint8_t* data){
  #ifdef Debug
for(int i=0; i<6; i++){
    Serial.printf(" %02X ", data[i]);
 }
 #endif
}
