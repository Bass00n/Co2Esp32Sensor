// #include <HTTP_Method.h>
// #include <Uri.h>
// #include <WebServer.h>

/*****************
   Get CO2 value 
 *****************/
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "s8_uart.h"


/* BEGIN CONFIGURATION */
#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
  #define S8_RX_PIN 16         // Rx pin which the S8 Tx pin is attached to (change if it is needed)
  #define S8_TX_PIN 17         // Tx pin which the S8 Rx pin is attached to (change if it is needed)
#else
  #define S8_UART_PORT  2     // Change UART port if it is needed
#endif
/* END CONFIGURATION */


#ifdef USE_SOFTWARE_SERIAL
  SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
#else
  #if defined(ARDUINO_ARCH_RP2040)
    REDIRECT_STDOUT_TO(Serial)    // to use printf (Serial.printf not supported)
    UART S8_serial(S8_TX_PIN, S8_RX_PIN, NC, NC);
  #else
    HardwareSerial S8_serial(S8_UART_PORT);   
  #endif
#endif

WiFiMulti wifiMulti;
S8_UART *sensor_S8;
S8_sensor sensor;

const char* ssid = "scat";
const char* passwd = "6523@TOP";
String url = "http://java-develop.ru";


void setup() {

  // Configure serial port, we need it for debug
  Serial.begin(DEBUG_BAUDRATE);

  // Wait port is open or timeout
  int i = 0;
  while (!Serial && i < 50) {
    delay(10);
    i++;
  }
  
  // First message, we are alive
  Serial.println("");
  Serial.println("Init");

  // Initialize S8 sensor
  S8_serial.begin(S8_BAUDRATE);
  sensor_S8 = new S8_UART(S8_serial);

  // Check if S8 is available
  sensor_S8->get_firmware_version(sensor.firm_version);
  int len = strlen(sensor.firm_version);
  if (len == 0) {
      Serial.println("SenseAir S8 CO2 sensor not found!");
      while (1) { delay(1); };
  }

  // Show basic S8 sensor info
  Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
  printf("Firmware version: %s\n", sensor.firm_version);
  sensor.sensor_id = sensor_S8->get_sensor_ID();
  Serial.print("Sensor ID: 0x"); printIntToHex(sensor.sensor_id, 4); Serial.println("");


  wifiMulti.addAP(ssid, passwd);
  Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

  Serial.println("Setup done!");
  Serial.flush();
}


void loop() {


  // Get CO2 measure
  sensor.co2 = sensor_S8->get_co2();
  printf("CO2 value = %d ppm\n", sensor.co2);

  if(wifiMulti.run() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    printf("IP: %s\n", WiFi.localIP().toString());
    String payload = "CO2 value = ";
    payload.concat(sensor.co2);
    int httpCode = http.POST(payload);
    printf("Response code: %d\n", httpCode);
  } else {
    printf("Wifi not connected!\n");
  }
  
  printf("\n");

  // Wait 5 second for next measure
  delay(5000);
}
