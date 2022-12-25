#include <CAN.h>
#include "LED_CPU.h"
#include "NoBounceButtons.h"

#define TEST_MODE 0

#define DATA_CMD_BYTE 0x00
#define CTRL_CMD_BYTE 0x01
#define DATA_BUTTON_PINS 12,13,14,15
#define CTRL_BUTTON_PINS 16,17,18,19,23,25

#define CTRL_BIT_G1 0
#define CTRL_BIT_RA 1
#define CTRL_BIT_RB 2
#define CTRL_BIT_PM 3
#define CTRL_BIT_RC 4
#define CTRL_BIT_G2 5

#define DATA_BITS 4
#define CTRL_BITS 6

NoBounceButtons nbb;

unsigned char Data = 0;
unsigned char Ctrl = 0;

unsigned char DataButtonPins[DATA_BITS] = {DATA_BUTTON_PINS};
unsigned char CtrlButtonPins[CTRL_BITS] = {CTRL_BUTTON_PINS};
unsigned char DataButtons[DATA_BITS];
unsigned char CtrlButtons[CTRL_BITS];

LED_MANUAL *led_manual;

void reset()
{
  Data = 0;
  Ctrl = 0;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU MANUAL CONTROL BOARD");
  // Setup buttons:
    for (int n=0; n<DATA_BITS; n++)
      DataButtons[n] = nbb.create(DataButtonPins[n]);
    for (int n=0; n<CTRL_BITS; n++)
      CtrlButtons[n] = nbb.create(CtrlButtonPins[n]);
  // Set up LEDs:
  led_manual = new LED_MANUAL();
  // Initializing registers, gates, and flags:
  reset();
  led_manual->update(Data,Ctrl);
  // start the CAN bus at 1 Mbps
  if (!CAN.begin(1E6))
  {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  Serial.println("Setup Finished.");
}

void loop()
{
  if (TEST_MODE) // Test mode, just blinking some leds
  {
    led_manual->update(10,0b101010);
    delay(1000);
    led_manual->update(5,0b010101);
    delay(1000);
  }
  else // Regular MANUAL CONTROL mode
  {
    nbb.check();
    for (int n=0; n<DATA_BITS; n++)
      if (nbb.action(DataButtons[n])==NBB_CLICK)
      {
        Serial.println("Data button pressed ...");
        nbb.reset(DataButtons[n]);
        Data = Data ^ (1<<n);
        Serial.println("CAN.beginPacket(0x12);");
        CAN.beginPacket(0x12);
        Serial.println("CAN.write(DATA_CMD_BYTE);");
        CAN.write(DATA_CMD_BYTE);
        Serial.println("CAN.write(Data);");
        CAN.write(Data);
        Serial.println("CAN.endPacket();");
        CAN.endPacket();
        Serial.println("Data=");
        Serial.println(Data);
      }

    for (int n=0; n<CTRL_BITS; n++)
      if (nbb.action(CtrlButtons[n])==NBB_CLICK)
      {
        Serial.println("Ctrl button pressed ...");
        nbb.reset(CtrlButtons[n]);
        Ctrl = Ctrl ^ (1<<n);
        CAN.beginPacket(0x12);
        CAN.write(CTRL_CMD_BYTE);
        CAN.write(Ctrl);
        CAN.endPacket();
        Serial.println("Ctrl=");
        Serial.println(Ctrl);
      }
    // Update LEDs:
    led_manual->update(Data, Ctrl); // Overflow and Negative not done yet ...
  }
}


