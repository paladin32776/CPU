// Board: "ESP 32 Dev Module"

#include "CAN_CPU.h"
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"

#define BUTTON_PIN 0
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

unsigned char demo_mode=0;

// CAN driver
CAN_CPU* can;
uint8_t boards_alive=0;

unsigned char Data = 0;
unsigned char Ctrl = 0;

NoBounceButtons nbb;
unsigned char button;
unsigned char DataButtonPins[DATA_BITS] = {DATA_BUTTON_PINS};
unsigned char CtrlButtonPins[CTRL_BITS] = {CTRL_BUTTON_PINS};
unsigned char DataButtons[DATA_BITS];
unsigned char CtrlButtons[CTRL_BITS];

EnoughTimePassed etp_demo(1000);  // Blink period (ms) in demo mode
unsigned char demo_flip_flag=0;

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
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  for (int n=0; n<DATA_BITS; n++)
    DataButtons[n] = nbb.create(DataButtonPins[n]);
  for (int n=0; n<CTRL_BITS; n++)
    CtrlButtons[n] = nbb.create(CtrlButtonPins[n]);
  // Setting up LEDs:
  led_manual = new LED_MANUAL();
  // Start CAN
  can = new CAN_CPU(BOARD_ID_MANUAL);
  // Initializing registers, gates, and flags:
  reset();
  can->send_message(RESET_CMD_BYTE);
  led_manual->update(Data,Ctrl);
  // Debug output:
  if (demo_mode)
    Serial.println("Setup Finished in DEMO MODE.");
  else
    Serial.println("Setup Finished in NORMAL MODE.");
}

void loop()
{
  // Call function to update status of all buttons:
  nbb.check();
  can->check();
  // Check for buttons pressed:
  if (nbb.action(button)>0)
  {
    switch (nbb.action(button))
    {
        case NBB_CLICK:
          if (demo_mode==1)
          {
            demo_mode=0;
            reset();
            can->send_message(RESET_CMD_BYTE);
          }
          break;
        case NBB_LONG_CLICK:
          if (demo_mode==0)
            demo_mode=1;
          break;
        default:
          break;
        nbb.reset(button);
    }
  }
  if (demo_mode && etp_demo.enough_time()) // Demo mode, just blinking some leds
  {
    Serial.println(demo_flip_flag);
    if (!demo_flip_flag)
      led_manual->update(10,0b101010);
    else
      led_manual->update(5,0b010101);
    demo_flip_flag = !demo_flip_flag;
  }
  else if (!demo_mode)  // Regular MANUAL CONTROL mode
  {
    for (int n=0; n<DATA_BITS; n++)
      if (nbb.action(DataButtons[n])==NBB_CLICK)
      {
        Serial.println("Data button pressed ...");
        nbb.reset(DataButtons[n]);
        Data = Data ^ (1<<n);
        can->send_message(DATA_CMD_BYTE, Data);
        Serial.printf("Data=%d\n", Data);
      }

    for (int n=0; n<CTRL_BITS; n++)
      if (nbb.action(CtrlButtons[n])==NBB_CLICK)
      {
        Serial.println("Ctrl button pressed ...");
        nbb.reset(CtrlButtons[n]);
        Ctrl = Ctrl ^ (1<<n);
        if ((boards_alive & BOARD_ID_CONTROL) == 0)
          can->send_message(CTRL_CMD_BYTE, Ctrl);
        Serial.printf("Ctrl=%d\n", Ctrl);
      }

    // CAN bus receive section:
    if (can->message_available())
    {
      // received a packet
      Serial.print("Received ");
      uint8_t Cmd, Para, Para2;
      switch (can->message_length())
      {
        case 2:
          can->get_message(Cmd, Para);
          Serial.printf("CMD: 0x%02X  PARA: 0x%02X", Cmd, Para);
          if (Cmd == BOARDS_CMD_BYTE)
          {
            boards_alive = Para;
            Serial.printf("\nBoards alive = %d\n", boards_alive);
          }
          break;
        case 1:
          can->get_message(Cmd);
          Serial.printf("CMD: 0x%02X", Cmd);
          if (Cmd == RESET_CMD_BYTE)
            reset();
          else if (Cmd == PING_CMD_BYTE)
            can->send_message(PONG_CMD_BYTE, BOARD_ID_MANUAL);
          break;
        default:
          can->clear_message();
          break;
      }
      Serial.println();
    }

    // Update LEDs:
    led_manual->update(Data, Ctrl); // Overflow and Negative not done yet ...
  }
}
