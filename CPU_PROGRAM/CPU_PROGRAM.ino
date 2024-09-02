// Board: "ESP 32 Dev Module"

#include <CAN.h>
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"
#include "DISP_CPU.h"
// #include <SPI.h>
// #include <TFT_eSPI.h>      // Hardware-specific library

#define DATA_CMD_BYTE 0x00
#define CTRL_CMD_BYTE 0x01
#define PROG_CMD_BYTE 0x02
#define RESET_CMD_BYTE 0xFF

#define BUTTON_PIN 0
// #define DATA_BUTTON_PINS 12,13,14,15
// #define CTRL_BUTTON_PINS 16,17,18,19,23,25

#define CTRL_BIT_G1 0
#define CTRL_BIT_RA 1
#define CTRL_BIT_RB 2
#define CTRL_BIT_PM 3
#define CTRL_BIT_RC 4
#define CTRL_BIT_G2 5

#define DATA_BITS 4
#define CTRL_BITS 6
#define COUNTER_BITS 4

unsigned char demo_mode=1;

unsigned char Data = 0;
unsigned char Counter = 0;
unsigned char Ccount = 0;
unsigned char Cset = 0;

NoBounceButtons nbb;
unsigned char button;
// unsigned char DataButtonPins[DATA_BITS] = {DATA_BUTTON_PINS};
// unsigned char CtrlButtonPins[CTRL_BITS] = {CTRL_BUTTON_PINS};
// unsigned char DataButtons[DATA_BITS];
// unsigned char CtrlButtons[CTRL_BITS];

EnoughTimePassed etp_demo(1000);  // Blink period (ms) in demo mode
unsigned char demo_flip_flag=0;

LED_PROGRAM *led_program;
DISP_PROGRAM *disp_program;

// TFT_eSPI *tft;

void reset()
{
  Data = 0;
  Counter = 0;
  Ccount = 0;
  Cset = 0;
}

void CAN_send_reset()
{
  Serial.print("Sending reset command via CAN bus ... ");
  CAN.beginPacket(0x12);
  CAN.write(RESET_CMD_BYTE);
  CAN.endPacket();
  Serial.println("done.");
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU PROGRAM MEMORY BOARD");
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  // for (int n=0; n<DATA_BITS; n++)
  //   DataButtons[n] = nbb.create(DataButtonPins[n]);
  // for (int n=0; n<CTRL_BITS; n++)
  //   CtrlButtons[n] = nbb.create(CtrlButtonPins[n]);
  // Setting up LEDs:
  led_program = new LED_PROGRAM();
  // Setting up the display:
  disp_program = new DISP_PROGRAM();
  // start the CAN bus at 1 Mbps
  if (!CAN.begin(1E6))
  {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  // Initializing registers, gates, and flags:
  reset();
  //CAN_send_reset(); // *** COMMENTED OUT FOR DEBUGGING ONLY ***
  led_program->update(Data, Counter, Ccount, Cset);
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
  disp_program->check();
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
            CAN_send_reset();
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
      led_program->update(10,10,1,0);
    else
      led_program->update(5,5,0,1);
    demo_flip_flag = !demo_flip_flag;
  }
  else if (!demo_mode && etp_demo.enough_time())  // Regular MANUAL CONTROL mode
  {
      if ((Counter++)==15)
        Counter = 0;
      // Cset = disp_program->get_cset(Counter);
      // Creset = disp_program->get_creset(Counter);
  //   for (int n=0; n<DATA_BITS; n++)
  //     if (nbb.action(DataButtons[n])==NBB_CLICK)
  //     {
  //       Serial.println("Data button pressed ...");
  //       nbb.reset(DataButtons[n]);
  //       Data = Data ^ (1<<n);
  //       CAN.beginPacket(0x12);
  //       CAN.write(DATA_CMD_BYTE);
  //       CAN.write(Data);
  //       CAN.endPacket();
  //       Serial.println("Data=");
  //       Serial.println(Data);
  //     }

  //   for (int n=0; n<CTRL_BITS; n++)
  //     if (nbb.action(CtrlButtons[n])==NBB_CLICK)
  //     {
  //       Serial.println("Ctrl button pressed ...");
  //       nbb.reset(CtrlButtons[n]);
  //       Ctrl = Ctrl ^ (1<<n);
  //       CAN.beginPacket(0x12);
  //       CAN.write(CTRL_CMD_BYTE);
  //       CAN.write(Ctrl);
  //       CAN.endPacket();
  //       Serial.println("Ctrl=");
  //       Serial.println(Ctrl);
  //     }

    // CAN bus receive section:
    int packetSize = CAN.parsePacket();
    if (packetSize)
    {
      // received a packet
      Serial.print("Received ");
      Serial.print("packet with id 0x");
      Serial.print(CAN.packetId(), HEX);
      if (!CAN.packetRtr())
      {
        Serial.print(" and length ");
        Serial.println(packetSize);
        if (packetSize==1)
        {
          unsigned char Cmd = (char)CAN.read();
          Serial.printf("CMD: 0x%02X", Cmd);
          if (Cmd==RESET_CMD_BYTE)
            reset();
        }
        else
          while (CAN.available())
          {
            char c = (char)CAN.read();
            Serial.printf("0x%02X ", c);
          }
        Serial.println();
      }
      Serial.println();
    }

  //   // Update LEDs:
    disp_program->setrow(Counter);
    led_program->update(Data, Counter, Ccount, Cset); // Overflow and Negative not done yet ...
  }
}
