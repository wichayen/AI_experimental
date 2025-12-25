//  ******************************************************************************/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

char str_temperature[8];
char str_pressure[8];
char str_altitude[8];
char str_humidity[8];

//  ******************************************************************************/
#include <TFT_eSPI.h>  // Graphics and font library

TFT_eSPI tft = TFT_eSPI();       // Invoke library

//  ******************************************************************************/
#include "USB.h" // Essential for USB.begin() and underlying TinyUSB initialization
#include "tusb.h"  // Provides all TinyUSB definitions and weak callback declarations

// Define your custom VID and PID
// IMPORTANT: For production, get a proper VID from USB-IF.
// For testing/personal projects, 0x1234 is often used as a placeholder.
#define MY_VID 0xCAFE
#define MY_PID 0x0300

// Define the Vendor Specific USB Interface Number
// If you have other interfaces (like CDC), this should be incremented.
// For a standalone vendor device, 0 is often used.
#define VENDOR_INTERFACE_NUM 0

// Define the custom Bulk IN and Bulk OUT Endpoint Addresses
// These must be unique and valid for your ESP32-S3.
// EP_OUT_NUM (e.g., 1) is for host to device (OUT)
// EP_IN_NUM (e.g., 2) is for device to host (IN)
#define VENDOR_EP_OUT_ADDR (0x01) // EP 1 OUT (Host to Device)
#define VENDOR_EP_IN_ADDR  (0x81) // EP 1 IN (Device to Host - 0x80 | EP_NUM, so EP 1)

// Max packet size for bulk endpoints
#define VENDOR_EP_MAX_PACKET_SIZE 64 // Standard for Full Speed USB

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    .bDeviceClass       = TUSB_CLASS_VENDOR_SPECIFIC, // This is key for Vendor Generic
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = MY_VID,
    .idProduct          = MY_PID,
    .bcdDevice          = 0x0100, // Device release number

    .iManufacturer      = 0x01, // Index of manufacturer string
    .iProduct           = 0x02, // Index of product string
    .iSerialNumber      = 0x03, // Index of serial number string

    .bNumConfigurations = 0x01
};

// Total length of configuration descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN) // Use TUD_CONFIG_DESC_LEN etc. from TinyUSB

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface Descriptor: Vendor Specific
    // Interface number, alternate setting, endpoint count, class, subclass, protocol, string index
    // Note: The TUD_VENDOR_DESCRIPTOR macro includes the two endpoint descriptors
    TUD_VENDOR_DESCRIPTOR(VENDOR_INTERFACE_NUM, 4, VENDOR_EP_OUT_ADDR, VENDOR_EP_IN_ADDR, VENDOR_EP_MAX_PACKET_SIZE)
};

// String Descriptors
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "Hong",                         // 1: Manufacturer
  "Hong USB Device",              // 2: Product
  "123456",                       // 3: Serials, should use chip ID
  "Hong WebUSB"                   // 4: Vendor Interface
};

// Global buffer for string descriptors (must be static/global)
static uint16_t desc_str_h[32]; // Max 31 chars + header

/******************************************************************
 * TinyUSB Callbacks - Only define the ones critical for your custom device
 ******************************************************************/

// Invoked when received GET DEVICE DESCRIPTOR
const uint8_t * tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
const uint8_t * tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // Unused parameter
  return desc_configuration;
}

// Invoked when received GET STRING DESCRIPTOR request
const uint16_t * tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid; // Unused parameter
  uint8_t chr_count;

  if (index == 0) { // Language ID
    memcpy(&desc_str_h[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    // Convert ASCII string into UTF-16
    if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

    const char* str = string_desc_arr[index]; // <--- FIXED: Changed string_cc_arr to string_desc_arr
    chr_count = strlen(str);
    if (chr_count > 31) chr_count = 31; // Max 31 chars for our buffer

    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str_h[i + 1] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  desc_str_h[0] = (uint16_t)((TUSB_DESC_STRING << 8 ) | (2 * chr_count + 2));

  return desc_str_h;
}

//  ******************************************************************************/
// The real situation (important)
// With Arduino-ESP32 core 3.2.0, custom TinyUSB Vendor devices using raw descriptors + tud_vendor_rx_cb() are NOT reliably supported.

// // Invoked when data is received on the vendor OUT endpoint
// void tud_vendor_rx_cb(uint8_t itf) {

//   Serial.println("tud_vendor_rx_cb");
//   Serial.print("received data");
//   // Check if it's our vendor interface
//   if (itf == VENDOR_INTERFACE_NUM) {
//     uint8_t buffer[VENDOR_EP_MAX_PACKET_SIZE];
//     uint32_t len = tud_vendor_read(buffer, sizeof(buffer));

//     if (len > 0) {
//       Serial.print("Received from host (Vendor OUT): ");
//       for (uint32_t i = 0; i < len; i++) {
//         Serial.print("0x");
//         if (buffer[i] < 0x10) Serial.print("0");
//         Serial.print(buffer[i], HEX);
//         Serial.print(" ");
//       }
//       Serial.println();

//       // Loopback logic: Write the received data back to the IN endpoint
//       // Ensure the device is mounted and the IN endpoint is writable before writing
//       if (tud_mounted() && tud_vendor_n_write_available(itf) >= len) { // Check if enough space is available
//         tud_vendor_write(buffer, len);
//         tud_vendor_flush(); // Ensure data is sent
//         Serial.println("Echoed data back to host (Vendor IN).");
//       } else {
//         Serial.println("Warning: Cannot echo data back (not mounted or IN endpoint not writable).");
//       }
//     }
//   }
// }

//  ******************************************************************************/
void setup() {
  unsigned status;

  Serial.begin(115200); // Standard serial for debugging (e.g., via USB-JTAG or separate UART)

  //  USB initialization
  // !!! IMPORTANT ARDUINO IDE SETTINGS !!!
  // Tools -> USB CDC On Boot: "Disabled"
  // Tools -> USB Mode: "USB-OTG (TinyUSB)"
  Serial.println("Starting Vendor Generic USB Device...");
  USB.begin();
  Serial.println("USB Stack initialized. Waiting for host connection...");
  Serial.print("Configured VID: 0x"); Serial.println(MY_VID, HEX);
  Serial.print("Configured PID: 0x"); Serial.println(MY_PID, HEX);

  //  BME280 initialization
  Serial.println(F("BME280 initialization"));
  Wire.begin(40, 41);  // SDA = GPIO 40, SCL = GPIO 41
  status = bme.begin(0x76, &Wire);  // or 0x77 if your sensor uses 

  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  //  LCD initialization
  tft.init();
  tft.setRotation(0);            // 0–3, try 1 or 3 for landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Hello ILI9488!");

  // tft.fillRect(50, 50, 100, 100, TFT_RED);
  // tft.drawRect(40, 40, 120, 120, TFT_GREEN);
  tft.fillRect(160-50+10, 50+30, 80, 80, TFT_BLUE);
  tft.fillRect(160-50+10, 150+30, 80, 80, TFT_RED);
  tft.fillRect(160-50+10, 250+30, 80, 80, TFT_GREEN);
  tft.fillRect(160-50+10, 350+30, 80, 80, TFT_YELLOW);

  //  read BME280 and show in LCD
  printValues();
  
}

int line = 0;

void printLine(int line, const char* text) {
    tft.setCursor(0, line * 16);       // 16 pixels per line
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.println(text);
}

void clearScreen() {
    tft.fillScreen(TFT_BLACK);
    line = 0;  // Reset line counter if needed
}


uint16_t pixel_dat16[10]; // 10 pixels in RGB565 format

void pushRGB888toScreen(uint8_t* rgb888_buf, int x, int y) {
  for (int i = 0; i < 10; i++) {
    uint8_t r = rgb888_buf[i * 3];
    uint8_t g = rgb888_buf[i * 3 + 1];
    uint8_t b = rgb888_buf[i * 3 + 2];

    // Convert RGB888 → RGB565
    pixel_dat16[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  // Now push as RGB565
  tft.pushImage(x, y, 10, 1, pixel_dat16);
}


void echo_all(uint8_t buf[], uint32_t count)
{
    uint16_t i;
    uint16_t x, y;
    uint8_t pixel_dat[40];
	  uint16_t pixel_dat16[20];
    uint16_t line = 0;  // Initialize line
    
    Serial.println("echo_all");

    switch (buf[0]) {
        case 0x31:
            // Echo to WebUSB
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();

            if(buf[10] == 1){
              tft.fillRect(160-50+10, 50+30, 80, 80, TFT_BLUE);
            } else {
              tft.fillRect(160-50+10, 50+30, 80, 80, TFT_BLACK);
            }

            if(buf[11] == 1){
              tft.fillRect(160-50+10, 150+30, 80, 80, TFT_RED);
            } else {
              tft.fillRect(160-50+10, 150+30, 80, 80, TFT_BLACK);
            }

            if(buf[12] == 1){
              tft.fillRect(160-50+10, 250+30, 80, 80, TFT_GREEN);
            } else {
              tft.fillRect(160-50+10, 250+30, 80, 80, TFT_BLACK);
            }

            if(buf[13] == 1){
              tft.fillRect(160-50+10, 350+30, 80, 80, TFT_YELLOW);
            } else {
              tft.fillRect(160-50+10, 350+30, 80, 80, TFT_BLACK);
            }

            break;

        case 0x32:
            // Echo to WebUSB
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();
            break;

        case 0x33:
            // Echo to WebUSB
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();
            break;

        case 0x41:
              // Echo to WebUSB
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();

            x = (((uint16_t)buf[2]) << 8) | ((uint16_t)buf[1]);
            y = (((uint16_t)buf[4]) << 8) | ((uint16_t)buf[3]);

            for (i = 0; i < 40; i++) {
              pixel_dat[i] = buf[5+i];
            }

            for (i = 0; i < 20; i++)
            {
              pixel_dat16[i] = (((uint16_t)buf[(i * 2) + 6]) << 8) | ((uint16_t)buf[(i * 2) + 5]);
            }

            // Draw 20x20 image at (x, y)
            // tft.myPushImage(x, y, 20, 1, pixel_dat16);
            tft.pushImage(x, y, 20, 1, pixel_dat16);
            // pushRGB888toScreen(pixel_dat, x, y);

            break;
        
        case 0x51: // read BME280 and update values
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();

            printValues();
            break;

        case 0x52: // send temperature data back to USB host
            tud_vendor_write(str_temperature, strlen(str_temperature));
            tud_vendor_write_flush();

            Serial.print("USB Temperature = ");
            Serial.println(str_temperature);
            break;

        case 0x53: // send pressure data back to USB host
            tud_vendor_write(str_pressure, strlen(str_pressure));
            tud_vendor_write_flush();
            break;

        case 0x54: // send altitude data back to USB host
            tud_vendor_write(str_altitude, strlen(str_altitude));
            tud_vendor_write_flush();
            break;

        case 0x55: // send humidity data back to USB host
            tud_vendor_write(str_humidity, strlen(str_humidity));
            tud_vendor_write_flush();
            break;

        default:
            // Echo to WebUSB
            tud_vendor_write(buf, count);
            tud_vendor_write_flush();
            break;
    }

}


unsigned long lastPrintTime = 0;
const unsigned long printInterval = 3000; // 3 second

void loop() {
  // The loopback is handled within tud_vendor_rx_cb.
  // The heartbeat message is removed for a cleaner loopback test.
  // If you need other background tasks, add them here.

  // delay(10); // Small delay to prevent busy-looping
  if (tud_vendor_available()) {
    Serial.println("tud_vendor_available");

    uint8_t buf[64];
    uint32_t count = tud_vendor_read(buf, sizeof(buf));

    // echo back
    echo_all(buf, count);
  }

  // Call printValues() every xxx second
  unsigned long currentMillis = millis();
  if (currentMillis - lastPrintTime >= printInterval) {
    lastPrintTime = currentMillis;
    printValues();
  }
}

void clearTextArea() {
  // Clear only the top text area (adjust height if needed)
  tft.fillRect(0, 0, 320, 80, TFT_BLACK);
}

void printValues() {
    float temperature = bme.readTemperature();
    float pressure    = bme.readPressure() / 100.0F;
    float altitude    = bme.readAltitude(SEALEVELPRESSURE_HPA);
    float humidity    = bme.readHumidity();

    // Convert float → string
    dtostrf(temperature, 6, 1, str_temperature); // width, decimals
    dtostrf(pressure,    6, 1, str_pressure);
    dtostrf(altitude,    6, 1, str_altitude);
    dtostrf(humidity,    6, 1, str_humidity);

    // ---------- Serial output (unchanged) ----------
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(altitude);
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.println();

    // ---------- LCD output ----------
    clearTextArea();

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    tft.setCursor(0, 0);
    tft.print("Temp: ");
    tft.print(temperature, 1);
    tft.println(" C");

    tft.print("Press: ");
    tft.print(pressure, 1);
    tft.println(" hPa");

    tft.print("Hum: ");
    tft.print(humidity, 1);
    tft.println(" %");

    tft.print("Alt: ");
    tft.print(altitude, 1);
    tft.println(" m");
}

