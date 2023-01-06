// Board: "ESP 32 Dev Module"

#include <CAN.h>
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"

#define BUTTON_PIN 0

#define DATA_CMD_BYTE 0x00
#define CTRL_CMD_BYTE 0x01
#define RESET_CMD_BYTE 0xFF

#define CTRL_BIT_G1 0
#define CTRL_BIT_RB 1
#define CTRL_BIT_RA 2
#define CTRL_BIT_PM 3
#define CTRL_BIT_RC 4
#define CTRL_BIT_G2 5

#define DATA_BITS 4
#define CTRL_BITS 6

unsigned char demo_mode=0;

LED_ALU *led_alu;
NoBounceButtons nbb;
unsigned char button;
EnoughTimePassed etp_demo(1000);  // Blink period (ms) in demo mode
unsigned char demo_flip_flag=0;

unsigned char data = 0;
unsigned char ctrl = 0;
unsigned char bus = 0;

unsigned char ra, rb, rc;
bool eg1, era, erb, epm, erc, eg2;
unsigned char neg, overflow;

void reset()
{
  ra = rb = rc = 0;
  eg1 = era = erb = epm = erc = eg2 = false;
  neg = overflow = 0;
  bus = data = ctrl =0;
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
  Serial.println("CPU ALU BOARD");
  // Setting up LEDs:
  led_alu = new LED_ALU();
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  // Starting CAN bus at 1 Mbps:
  if (!CAN.begin(1E6))
  {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  // Initializing registers, gates, and flags:
  reset();
  CAN_send_reset();
  led_alu->update(bus,ra,rb,rc,ctrl,neg,overflow);
  // Debug output:
  if (demo_mode)
    Serial.println("Setup Finished in TEST MODE.");
  else
    Serial.println("Setup Finished in NORMAL MODE.");
}

void loop()
{
  // Call function to update status of all buttons:
  nbb.check();
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
      led_alu->update(10,10,10,10,0b100101,1,0);
    else
      led_alu->update(5,5,5,5,0b011010,0,1);
    demo_flip_flag = !demo_flip_flag;
  }
  else if (!demo_mode)  // Normal ALU mode
  {
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
        if (packetSize==2)
        {
          unsigned char Cmd = (char)CAN.read();
          unsigned char Para = (char)CAN.read();
          Serial.printf("CMD: 0x%02X  PARA: 0x%02X", Cmd, Para);
          if (Cmd==DATA_CMD_BYTE)
            data = Para;
          else if (Cmd==CTRL_CMD_BYTE)
            ctrl = Para;
        }
        else if (packetSize==1)
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
    // ctrl to register/gate/ALU control settings:
    era = (ctrl>>CTRL_BIT_RA) & 1;
    erb = (ctrl>>CTRL_BIT_RB) & 1;
    erc = (ctrl>>CTRL_BIT_RC) & 1;
    eg1 = (ctrl>>CTRL_BIT_G1) & 1;
    eg2 = (ctrl>>CTRL_BIT_G2) & 1;
    epm = (ctrl>>CTRL_BIT_PM) & 1;
    // Logic for whole CPU core:
    // Determine bus state:
    if (eg1 && !eg2)
      bus = data;
    else if (!eg1 && eg2)
      bus = rc;
    else
      bus = 0;
    // Load registers from bus if enabled:
    if (era)
      ra = bus;
    if (erb)
      rb = bus;
    // Determine ALU output and load into register if enabled:
    if (erc)
      rc = (ra+rb)*(!epm) + (ra-rb)*epm;
    // Determine overflow flag:
    if ((ra+rb>15) && epm==false)
      overflow = 1;
    else
      overflow=0;
    // Determine neg flag:
    if ((rb>ra) && epm==true)
      neg = 1;
    else
      neg = 0;
    // Update LEDs:
    led_alu->update(bus,ra,rb,rc,ctrl,neg,overflow); // Overflow and Negative not done yet ...
  }
}


