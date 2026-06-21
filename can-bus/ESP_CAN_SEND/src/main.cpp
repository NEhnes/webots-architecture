// libs
#include <SPI.h>
#include <Arduino.h>
#include <mcp2515.h>

struct can_frame canMsg; // struct to hold CAN message
MCP2515 mcp2515(5); // SPI CS pin is GPIO 5

#define MAX_RETRIES 3
#define CAN_ACK_ID 0x037  // CAN ID for acknowledgment

void setup() {
  Serial.begin(115200);
  Serial.println("Setup begin - SENDER");
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, CS
  delay(500);
  mcp2515.reset();
  delay(500);

  uint8_t status = mcp2515.getStatus();
  Serial.print("MCP2515 Status: ");
  Serial.println(status, HEX);

  MCP2515::ERROR result = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  if (result != MCP2515::ERROR_OK) {
    Serial.println("ERROR: setBitrate failed!");
    Serial.println(result);

    if (result == MCP2515::ERROR_FAIL) {
      Serial.println("ERROR_FAIL");
    } else if (result == MCP2515::ERROR_ALLTXBUSY) {
      Serial.println("ERROR_ALLTXBUSY");
    } else if (result == MCP2515::ERROR_FAILINIT) {
      Serial.println("ERROR_FAILINIT");
    } else if (result == MCP2515::ERROR_FAILTX) {
      Serial.println("ERROR_FAILTX");
    } else if (result == MCP2515::ERROR_NOMSG) {
      Serial.println("ERROR_NOMSG");
    }

    while(1); // Halt
  } else {
    Serial.println("Bitrate set to 500kbps");
  }

  delay(2000); // Wait before changing mode

  result = mcp2515.setNormalMode();
  if (result != MCP2515::ERROR_OK) {
    Serial.println("ERROR: setNormalMode failed!");
    Serial.println(result);
    while(1); // Halt
  } else {
    Serial.println("MCP2515 in Normal Mode");
  }

  Serial.println("Setup complete");
}

void loop() {

  int VRX = analogRead(13);
  int VRY = analogRead(12);

  int value = 1234; // Your integer value to send

  // Prepare CAN message
  canMsg.can_id  = 0x036;  // CAN ID
  canMsg.can_dlc = 8;      // Data length code (number of bytes)

  // // below is from original example code
  // canMsg.data[0] = (value >> 8) & 0xFF; // MSB of value
  // canMsg.data[1] = value & 0xFF;        // LSB of value

  // this is my new stuff for joystick
  memcpy(&canMsg.data[0], &VRX, sizeof(VRX)); // Copy VRX into data bytes 0-3
  memcpy(&canMsg.data[4], &VRY, sizeof(VRY)); // Copy VRY into data bytes 4-7

  bool messageSent = false;
  int retries = 0;

  Serial.print("Sending CAN message, first byte:");
  Serial.println(canMsg.data[0]); // Print first byte for verification

  while (!messageSent && retries < MAX_RETRIES) {
    
    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      // Serial.print("Value sent: ");
      // Serial.println(value);

      // Wait for acknowledgment
      unsigned long startTime = millis();
      bool ackReceived = false;
      
      while (millis() - startTime < 500) { // Wait up to 500ms for an ACK
        if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
          if (canMsg.can_id == CAN_ACK_ID) {
            ackReceived = true;
            break;
          }
        }
      }

      if (ackReceived) {
        Serial.println("ACK received");
        messageSent = true;
      } else {
        Serial.println("ACK not received, retrying...");
        retries++;
      }
    } else {
      Serial.println("Error sending message, retrying...");
      retries++;
    }
  }

  if (!messageSent) {
    Serial.println("Failed to send message after retries");
  }

  delay(40); // 25hz
}