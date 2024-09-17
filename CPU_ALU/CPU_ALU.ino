// Board: "ESP 32 Dev Module"

#include "CAN_CPU.h"
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"

#define BUTTON_PIN 0

#define CTRL_BIT_G1 0
#define CTRL_BIT_RB 1
#define CTRL_BIT_RA 2
#define CTRL_BIT_PM 3
#define CTRL_BIT_RC 4
#define CTRL_BIT_G2 5

#define DATA_BITS 4
#define CTRL_BITS 6

unsigned char demo_mode=0;

// CAN driver
CAN_CPU* can;
EnoughTimePassed etp_ping(500);  // ping intervall
uint8_t boards_alive=0, boards_pong=0;

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

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU ALU BOARD");
  // Setting up LEDs:
  led_alu = new LED_ALU();
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  // Start CAN
  can = new CAN_CPU(BOARD_ID_ALU);
  // Initializing registers, gates, and flags:
  reset();
  can->send_message(RESET_CMD_BYTE);
  led_alu->update(bus,ra,rb,rc,ctrl,neg,overflow);
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
      led_alu->update(10,10,10,10,0b100101,1,0);
    else
      led_alu->update(5,5,5,5,0b011010,0,1);
    demo_flip_flag = !demo_flip_flag;
  }
  else if (!demo_mode)  // Normal ALU mode
  {
    // Send ping for other boards:
    if (etp_ping.enough_time())
    {
      boards_alive = boards_pong;
      Serial.printf("Boards alive = %d\n", boards_alive);
      boards_pong = 1;
      can->send_message(BOARDS_CMD_BYTE, boards_alive);
      can->send_message(PING_CMD_BYTE);
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
          if (Cmd == DATA_CMD_BYTE)
            data = Para;
          else if (Cmd == CTRL_CMD_BYTE)
            ctrl = Para;
          else if (Cmd == PING_CMD_BYTE)
          {
            if (Para == BOARD_ID_ALU)
              can->send_message(PONG_CMD_BYTE, BOARD_ID_ALU);
          }
          else if (Cmd == PONG_CMD_BYTE)  
            boards_pong = boards_pong | Para;
          break;
        case 1:
          can->get_message(Cmd);
          Serial.printf("CMD: 0x%02X", Cmd);
          if (Cmd == RESET_CMD_BYTE)
            reset();
          else if (Cmd == PING_CMD_BYTE)
            can->send_message(PONG_CMD_BYTE, BOARD_ID_ALU);
          break;
        default:
          can->clear_message();
          break;
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
