// Refactored Arduino code for ESP32-S3 with TinyUSB Vendor Generic device and ILI9488 TFT

// =============================================================================
// LIBRARIES
// =============================================================================
#include <TFT_eSPI.h> // Graphics and font library for the ILI9488 display
#include "USB.h"       // Essential for USB.begin() and underlying TinyUSB initialization
#include "tusb.h"      // Provides all TinyUSB definitions and weak callback declarations

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
void printLine(int line, const char* text);
void clearScreen();
void updateStatusRects(const uint8_t* status_bytes);
void handleUsbCommand(uint8_t* buf, uint32_t count);

// =============================================================================
// GLOBAL OBJECTS
// =============================================================================
TFT_eSPI tft = TFT_eSPI(); // Invoke TFT library
int line = 0;              // Global line counter for simple text display

// =============================================================================
// USB DEVICE CONFIGURATION
// =============================================================================

// --- Custom VID/PID ---
// IMPORTANT: For production, get a proper VID from USB-IF.
#define MY_VID 0xCAFE
#define MY_PID 0x0300

// --- USB Interface and Endpoints ---
#define VENDOR_INTERFACE_NUM      0
#define VENDOR_EP_OUT_ADDR        0x01 // EP 1 OUT (Host to Device)
#define VENDOR_EP_IN_ADDR         0x81 // EP 1 IN (Device to Host)
#define VENDOR_EP_MAX_PACKET_SIZE 64   // Max packet size for bulk endpoints

// --- USB Command Protocol ---
#define CMD_UPDATE_STATUS 0x31
#define CMD_CLEAR_SCREEN  0x32
#define CMD_XXX           0x33 // Placeholder for a potential command
#define CMD_DRAW_BITMAP   0x41

// =============================================================================
// USB DESCRIPTORS
// (These describe the device's capabilities to the host)
// =============================================================================

// --- Device Descriptor ---
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    .bDeviceClass       = TUSB_CLASS_VENDOR_SPECIFIC,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = MY_VID,
    .idProduct          = MY_PID,
    .bcdDevice          = 0x0100, // Device release number
    .iManufacturer      = 0x01,   // Index of manufacturer string
    .iProduct           = 0x02,   // Index of product string
    .iSerialNumber      = 0x03,   // Index of serial number string
    .bNumConfigurations = 0x01
};

// --- Configuration Descriptor ---
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)
uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface Descriptor: Vendor Specific
    // Includes the two endpoint descriptors
    TUD_VENDOR_DESCRIPTOR(VENDOR_INTERFACE_NUM, 4, VENDOR_EP_OUT_ADDR, VENDOR_EP_IN_ADDR, VENDOR_EP_MAX_PACKET_SIZE)
};

// --- String Descriptors ---
char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 }, // 0: Supported language is English (0x0409)
  "Hong",                         // 1: Manufacturer
  "Hong USB Device",              // 2: Product
  "123456",                       // 3: Serials (should use chip ID in production)
  "Hong WebUSB"                   // 4: Vendor Interface
};

// Global buffer for string descriptors
static uint16_t desc_str_h[32];

// =============================================================================
// TINYUSB CALLBACKS
// =============================================================================

// Invoked when received GET DEVICE DESCRIPTOR
const uint8_t* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
const uint8_t* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // Unused parameter
  return desc_configuration;
}

// Invoked when received GET STRING DESCRIPTOR request
const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) { // Language ID
    memcpy(&desc_str_h[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

    const char* str = string_desc_arr[index];
    chr_count = strlen(str);
    if (chr_count > 31) chr_count = 31;

    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str_h[i + 1] = str[i];
    }
  }

  desc_str_h[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return desc_str_h;
}

// Invoked when data is received from the host on the VENDOR OUT endpoint
void tud_vendor_rx_cb(uint8_t itf) {
  // Ensure this callback is for our vendor interface
  if (itf == VENDOR_INTERFACE_NUM) {
    uint8_t usb_rx_buf[VENDOR_EP_MAX_PACKET_SIZE];
    
    // Read the available data from the USB buffer
    uint32_t count = tud_vendor_read(usb_rx_buf, sizeof(usb_rx_buf));

    if (count > 0) {
      // Process the received command
      handleUsbCommand(usb_rx_buf, count);
    }
  }
}

// =============================================================================
// APPLICATION LOGIC & COMMAND HANDLING
// =============================================================================

/**
 * @brief Processes commands received over USB.
 * 
 * @param buf The buffer containing the command and its data.
 * @param count The number of bytes in the buffer.
 */
void handleUsbCommand(uint8_t* buf, uint32_t count) {
  // Echo the raw command back to the host for debugging/confirmation
  if (tud_mounted() && tud_vendor_n_write_available(VENDOR_INTERFACE_NUM) >= count) {
    tud_vendor_write(buf, count);
    tud_vendor_flush();
  }

  // --- Parse Command and Data ---
  // Protocol:
  // buf[0]    - Command ID
  // buf[1,2]  - uint16_t x (little-endian)
  // buf[3,4]  - uint16_t y (little-endian)
  // buf[5+]   - Command-specific payload
  uint8_t command = buf[0];
  uint16_t x = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
  uint16_t y = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8);

  switch (command) {
    case CMD_UPDATE_STATUS:
      printLine(line++, "Cmd: Update Status");
      // The status bytes for the rectangles start at index 10
      updateStatusRects(&buf[10]);
      break;

    case CMD_CLEAR_SCREEN:
      clearScreen();
      printLine(line++, "Cmd: Clear Screen");
      break;

    case CMD_XXX:
      clearScreen();
      printLine(line++, "Cmd: XXX");
      break;

    case CMD_DRAW_BITMAP:
      { // Use a block to declare variables inside a case
        // Expects a 20x1 bitmap (20 pixels)
        const int num_pixels = 20;
        uint16_t pixel_data[num_pixels];

        // Convert byte array (little-endian RGB565) to uint16_t array
        // Data starts at index 5
        for (int i = 0; i < num_pixels; i++) {
          pixel_data[i] = (uint16_t)buf[5 + (i * 2)] | ((uint16_t)buf[6 + (i * 2)] << 8);
        }
        tft.pushImage(x, y, num_pixels, 1, pixel_data);
      }
      break;

    default:
      printLine(line++, "Cmd: Unknown");
      break;
  }
}

// =============================================================================
// TFT DISPLAY HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Draws or clears four status rectangles based on input bytes.
 * 
 * @param status_bytes A pointer to 4 bytes, where each byte (0 or 1) 
 *                     controls the color of a rectangle.
 */
void updateStatusRects(const uint8_t* status_bytes) {
    const int RECT_X = 160 - 50;
    const int RECT_W = 100;
    const int RECT_H = 100;

    tft.fillRect(RECT_X, 50,  RECT_W, RECT_H, status_bytes[0] ? TFT_BLUE   : TFT_BLACK);
    tft.fillRect(RECT_X, 150, RECT_W, RECT_H, status_bytes[1] ? TFT_RED    : TFT_BLACK);
    tft.fillRect(RECT_X, 250, RECT_W, RECT_H, status_bytes[2] ? TFT_GREEN  : TFT_BLACK);
    tft.fillRect(RECT_X, 350, RECT_W, RECT_H, status_bytes[3] ? TFT_YELLOW : TFT_BLACK);
}

/**
 * @brief Prints a line of text to the TFT at a specific line number.
 */
void printLine(int line_num, const char* text) {
    tft.setCursor(0, line_num * 16); // 16 pixels per line
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.println(text);
}

/**
 * @brief Clears the entire TFT screen and resets the line counter.
 */
void clearScreen() {
    tft.fillScreen(TFT_BLACK);
    line = 0;
}

// =============================================================================
// MAIN SETUP & LOOP
// =============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Refactored Vendor Generic USB Device...");

  // --- IMPORTANT ARDUINO IDE SETTINGS ---
  // Tools -> USB CDC On Boot: "Disabled"
  // Tools -> USB Mode: "USB-OTG (TinyUSB)"

  // Initialize the USB stack
  USB.begin();
  Serial.println("USB Stack initialized. Waiting for host connection...");
  Serial.print("Configured VID: 0x"); Serial.println(MY_VID, HEX);
  Serial.print("Configured PID: 0x"); Serial.println(MY_PID, HEX);

  // Initialize the TFT display
  tft.init();
  tft.setRotation(0); // 0–3, try 1 or 3 for landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Hello ILI9488!");

  // Draw initial rectangles
  updateStatusRects((const uint8_t[]){1, 1, 1, 1});
}

void loop() {
  // All USB communication is handled by the TinyUSB callbacks (event-driven).
  // This loop can be used for other non-USB-related tasks.
  // A small delay prevents the loop from running at full speed unnecessarily.
  delay(10);
}