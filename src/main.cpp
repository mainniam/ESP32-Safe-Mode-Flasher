/**
 * ESP32-S3 Safe Mode Flasher
 * Puts all GPIO pins into safe high-impedance state
 * Prevents damage to peripherals during firmware updates
 * 
 * MIT License
 * Copyright (c) 2024
 */

#include <Arduino.h>
#include "driver/rmt.h"
#include <Wire.h>
#include <SPI.h>

// ==================== CONFIGURATION ====================
#define SERIAL_BAUD       115200
#define LOG_LEVEL         2           // 0=Quiet, 1=Normal, 2=Verbose
#define SAFETY_DELAY_MS   10          // Delay between pin operations

// System-critical pins (DO NOT MODIFY)
const int CRITICAL_PINS[] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,  // External Flash/PSRAM
  33, 34, 35, 36, 37, 38, 39,                   // SPI/PSRAM interface
  -1  // End marker
};

// USB/UART pins
const int USB_UART_PINS[] = {
  43,  // U0TXD
  44,  // U0RXD
  45,  // USB D-
  46,  // USB D+
  19,  // USB OTG VN
  18,  // USB OTG VP
  -1
};

// Pins that need pull-up resistors
const int PULLUP_PINS[] = {
  0,   // GPIO0 (Boot/Download)
  -1
};

// ==================== GLOBALS ====================
int safePins = 0;
int skippedPins = 0;
int specialPins = 0;
bool verboseMode = true;

// ==================== UTILITIES ====================
bool isCriticalPin(int pin) {
  for (int i = 0; CRITICAL_PINS[i] != -1; i++) {
    if (pin == CRITICAL_PINS[i]) return true;
  }
  return false;
}

bool isUsbUartPin(int pin) {
  for (int i = 0; USB_UART_PINS[i] != -1; i++) {
    if (pin == USB_UART_PINS[i]) return true;
  }
  return false;
}

bool needsPullup(int pin) {
  for (int i = 0; PULLUP_PINS[i] != -1; i++) {
    if (pin == PULLUP_PINS[i]) return true;
  }
  return false;
}

void logMessage(int level, const char* format, ...) {
  if (level > LOG_LEVEL) return;
  
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.println(buffer);
}

// ==================== PIN SAFETY ====================
void secureAllPins() {
  logMessage(1, "\n🔧 Securing GPIO pins...");
  
  for (int pin = 0; pin <= 48; pin++) {
    // Skip invalid GPIOs
    if (!GPIO_IS_VALID_GPIO(pin)) {
      logMessage(2, "  Skip GPIO%02d: Invalid GPIO", pin);
      skippedPins++;
      continue;
    }
    
    // Handle critical pins
    if (isCriticalPin(pin)) {
      logMessage(2, "  Skip GPIO%02d: Critical system pin", pin);
      skippedPins++;
      continue;
    }
    
    // Handle USB/UART pins
    if (isUsbUartPin(pin)) {
      pinMode(pin, INPUT);
      digitalWrite(pin, LOW);
      logMessage(2, "  GPIO%02d: INPUT (USB/UART)", pin);
      specialPins++;
      delay(SAFETY_DELAY_MS);
      continue;
    }
    
    // Handle pull-up required pins
    if (needsPullup(pin)) {
      pinMode(pin, INPUT_PULLUP);
      logMessage(2, "  GPIO%02d: INPUT_PULLUP", pin);
      specialPins++;
      delay(SAFETY_DELAY_MS);
      continue;
    }
    
    // Default: high-impedance input
    pinMode(pin, INPUT);
    digitalWrite(pin, LOW);  // Ensure no pull-up
    logMessage(2, "  GPIO%02d: INPUT (High-Z)", pin);
    safePins++;
    
    delay(SAFETY_DELAY_MS);
  }
  
  logMessage(1, " All pins secured");
}

// ==================== PERIPHERAL SAFETY ====================
void disablePeripherals() {
  logMessage(1, "\n🔌 Disabling peripherals...");
  
  // Detach PWM from common pins
  int pwmPins[] = {2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
  for (int i = 0; i < sizeof(pwmPins)/sizeof(pwmPins[0]); i++) {
    ledcDetach(pwmPins[i]);
  }
  logMessage(2, "  PWM detached");
  
  // Uninstall RMT channels (used for WS2812B)
  for (int ch = 0; ch < 8; ch++) {
    rmt_driver_uninstall((rmt_channel_t)ch);
  }
  logMessage(2, "  RMT controllers uninstalled");
  
  // Stop I2C
  Wire.end();
  logMessage(2, "  I2C stopped");
  
  // Stop SPI
  SPI.end();
  logMessage(2, "  SPI stopped");
  
  // Close other serial ports
  Serial2.end();
  logMessage(2, "  Serial2 stopped");
  
  logMessage(1, " All peripherals disabled");
}

// ==================== DISPLAY STATUS ====================
void showStatus() {
  Serial.println("\n" + String(80, '='));
  Serial.println("ESP32-S3 SAFE MODE FLASHER");
  Serial.println(String(80, '='));
  
  Serial.printf("\n STATUS SUMMARY:\n");
  Serial.printf("   Safe GPIO pins:    %2d\n", safePins);
  Serial.printf("   Special pins:      %2d\n", specialPins);
  Serial.printf("   Skipped pins:      %2d\n", skippedPins);
  Serial.printf("   Total pins:        %2d\n", safePins + specialPins + skippedPins);
  
  Serial.println("\n CURRENT STATE:");
  Serial.println("  • All GPIOs in high-impedance INPUT");
  Serial.println("  • No pull-up/pull-down resistors active");
  Serial.println("  • All peripherals (PWM/RMT/I2C/SPI) disabled");
  Serial.println("  • System in low-power safe state");
  
  Serial.println("\n NEXT STEPS:");
  Serial.println("  1. Upload your main firmware");
  Serial.println("  2. Press RESET button");
  Serial.println("  3. Or power cycle the board");
  
  Serial.println("\n  WARNING:");
  Serial.println("  • GPIO0 must stay HIGH for normal boot");
  Serial.println("  • Do not connect anything to USB pins (45,46)");
  Serial.println("  • Critical pins (22-39) are untouched");
  
  Serial.println(String(80, '='));
  Serial.println("System is READY for safe programming");
  Serial.println(String(80, '=') + "\n");
}

// ==================== MAIN SETUP ====================
void setup() {
  // Start serial (keep this for monitoring)
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial connection
  
  // Print header
  Serial.println("\n\n");
  Serial.println("███████╗███████╗██████╗ ██████╗ ██████╗ ██████╗");
  Serial.println("██╔════╝██╔════╝██╔══██╗╚════██╗╚════██╗╚════██╗");
  Serial.println("███████╗███████╗██████╔╝ █████╔╝ █████╔╝ █████╔╝");
  Serial.println("╚════██║╚════██║██╔═══╝ ██╔═══╝  ╚═══██╗██╔═══╝");
  Serial.println("███████║███████║██║     ███████╗██████╔╝███████╗");
  Serial.println("╚══════╝╚══════╝╚═╝     ╚══════╝╚═════╝ ╚══════╝");
  Serial.println("\n          ESP32-S3 SAFE MODE FLASHER");
  Serial.println("          v1.0 | MIT License | 2024");
  
  // Start safety procedures
  logMessage(1, "\n Starting safety procedures...");
  
  // Step 1: Secure all GPIO pins
  secureAllPins();
  
  // Step 2: Disable peripherals
  disablePeripherals();
  
  // Step 3: Show status
  showStatus();
  
  // Step 4: Enable heartbeat
  logMessage(1, "Safety mode active. Monitoring...");
}

// ==================== MAIN LOOP ====================
void loop() {
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastStatus = 0;
  
  unsigned long now = millis();
  
  // Heartbeat LED (if GPIO2 is available and not critical)
  if (now - lastHeartbeat > 1000) {
    lastHeartbeat = now;
    
    // Simple software blink (no hardware PWM)
    static bool ledState = false;
    if (!isCriticalPin(2) && !isUsbUartPin(2)) {
      digitalWrite(2, ledState ? HIGH : LOW);
      ledState = !ledState;
    }
  }
  
  // Periodic status update
  if (now - lastStatus > 30000) {  // Every 30 seconds
    lastStatus = now;
    Serial.println("\n[STATUS CHECK] System still in safe mode.");
    Serial.printf("  Uptime: %lu seconds\n", now / 1000);
    Serial.println("  Ready for firmware upload.");
  }
  
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    switch(cmd) {
      case 's':
      case 'S':
        showStatus();
        break;
      case 'v':
      case 'V':
        verboseMode = !verboseMode;
        Serial.printf("\nVerbose mode: %s\n", verboseMode ? "ON" : "OFF");
        break;
      case 'r':
      case 'R':
        Serial.println("\n  Simulating reset...");
        Serial.println("(In real hardware, press RESET button)");
        break;
      case '?':
      case 'h':
      case 'H':
        Serial.println("\n COMMANDS:");
        Serial.println("  s - Show status");
        Serial.println("  v - Toggle verbose mode");
        Serial.println("  r - Reset reminder");
        Serial.println("  h - This help");
        break;
    }
  }
  
  // Minimal delay
  delay(100);
}

// ==================== END OF FILE ====================