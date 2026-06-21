#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// joystick raw values
int VRX, VRY;

// joystick states
enum class Direction
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NEUTRAL
};

// bitmaps for arrows
byte upArrow[8] = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000};

byte downArrow[8] = {
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000};

byte leftArrow[8] = {
    0b00010000,
    0b00110000,
    0b01110000,
    0b11111111,
    0b11111111,
    0b01110000,
    0b00110000,
    0b00010000};

byte rightArrow[8] = {
    0b00001000,
    0b00001100,
    0b00001110,
    0b11111111,
    0b11111111,
    0b00001110,
    0b00001100,
    0b00001000};

// MCP2515 CAN controller parameters
struct can_frame canMsg;
MCP2515 mcp2515(5); // CS pin is GPIO 5
#define CAN_ACK_ID 0x037 // CAN ID for acknowledgment

// function declarations
void draw(Direction _dir);
Direction getInput(int VRX, int VRY);

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup begin - RECEIVER");

  Serial.println("Initializing SPI...");
  Wire.begin(OLED_SDA, OLED_SCL);

  Serial.println("Initializing OLED...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) // address 0x3C for 128x64 OLED
  {
    Serial.println(F("SSD1306 allocation failed")); // init OLED
    for (;;); // loop forever if OLED fails
  }
  else
  {
    Serial.println("OLED initialized successfully");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  Serial.println("Initializing MCP2515...");
  mcp2515.reset();
  Serial.println(mcp2515.checkError());

  MCP2515::ERROR result = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  if (result != MCP2515::ERROR_OK)
  {
    Serial.print("ERROR: setBitrate failed! --- ");
    Serial.println(result);
    while (1)
      ; // loop if fail
  }

  result = mcp2515.setNormalMode();
  if (result != MCP2515::ERROR_OK)
  {
    Serial.println("ERROR: setNormalMode failed!");
    Serial.println(result);
    while (1)
      ; // loop if fail
  }

  Serial.println("Setup complete");
}

void loop()
{
  Serial.println("Waiting for CAN message...");
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK)
  {
    Serial.print("Message received with ID: 0x");
    Serial.println(canMsg.can_id, HEX);
    if (canMsg.can_id == 0x036) // verify sender ID
    {
      memcpy(&VRX, &canMsg.data[0], sizeof(VRX)); // data bytes 0-3 into VRX
      memcpy(&VRY, &canMsg.data[4], sizeof(VRY)); // data bytes 4-7 into VRY

      Serial.print("VRX: ");
      Serial.print(VRX);
      Serial.print(" | VRY: ");
      Serial.println(VRY);
      Serial.println("---------------------");

      // send acknowledgment
      canMsg.can_id = CAN_ACK_ID; // ACK ID
      canMsg.can_dlc = 0;         // no data needed
      mcp2515.sendMessage(&canMsg);
      Serial.println("ACK sent, ID: 0x037");
    }
  }

  Direction dir = getInput(VRX, VRY);

  draw(dir);

  delay(200);
}

void draw(Direction _dir)
{
  // clear display and set header text
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("ESP32 CAN BUS RECV");

  switch (_dir)
  {
  // draw symbol/text based on direction
  case Direction::UP:
    display.drawBitmap(56, 16, upArrow, 8, 8, SSD1306_WHITE);
    break;
  case Direction::DOWN:
    display.drawBitmap(56, 32, downArrow, 8, 8, SSD1306_WHITE);
    break;
  case Direction::LEFT:
    display.drawBitmap(48, 24, leftArrow, 8, 8, SSD1306_WHITE);
    break;
  case Direction::RIGHT:
    display.drawBitmap(64, 24, rightArrow, 8, 8, SSD1306_WHITE);
    break;
  case Direction::NEUTRAL:
  default:
    display.setCursor(20, 24);
    display.setTextSize(2);
    display.println("NEUTRAL");
    break;
  }

  display.display();
}

Direction getInput(int VRX, int VRY)   // vrx, vry inputs 0-4095
{
  // deadzone threshold
  const int DEADZONE = 500;
  const int CENTER = 2048;

  // calculate x and y relative to center
  int x = VRX - CENTER;
  int y = -(VRY - CENTER);

  // apply deadzone
  if (abs(x) < DEADZONE && abs(y) < DEADZONE)
  {
    Serial.println("NEUTRAL ACHIEVED");
    return Direction::NEUTRAL;
  }

  // determine which axis has more movement
  if (abs(x) > abs(y))
  {
    // horizontal movement is dominant
    Serial.println((x > 0) ? "RIGHT ACHIEVED" : "LEFT ACHIEVED");
    return (x > 0) ? Direction::RIGHT : Direction::LEFT;
  }
  else
  {
    // vertical movement is dominant
    Serial.println((y > 0) ? "UP ACHIEVED" : "DOWN ACHIEVED");
    return (y > 0) ? Direction::UP : Direction::DOWN;
  }
}