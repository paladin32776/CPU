// Board: "ESP 32 Dev Module"

#include "CAN_CPU.h"
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"
#include "DISP_CPU.h"

#define BUTTON_PIN 0

#define CTRL_BIT_PCOUNT 6
#define CTRL_BIT_PSET 7

#define DATA_BITS 4
#define CTRL_BITS 6
#define COUNTER_BITS 4

unsigned char demo_mode=0;

// CAN driver
CAN_CPU* can;
uint8_t boards_alive=0;

unsigned char Data = 0;
unsigned char Counter = 0;
unsigned char Pcount = 0;
unsigned char Pset = 0;

bool CounterChanged = true;
int PcountChanged = 0;
int PsetChanged = 0;

NoBounceButtons nbb;
unsigned char button;

EnoughTimePassed etp_demo(1000);  // Blink period (ms) in demo mode
unsigned char demo_flip_flag=0;

LED_PROGRAM *led_program;
DISP_PROGRAM *disp_program;

// TFT_eSPI *tft;

void reset()
{
  Counter = 0;
  Data = disp_program->get_data(Counter);
  Pcount = 0;
  Pset = 0;
  CounterChanged = true;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU PROGRAM MEMORY BOARD");
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  // Setting up LEDs:
  led_program = new LED_PROGRAM();
  // Setting up the display:
  disp_program = new DISP_PROGRAM();
  // Start CAN
  can = new CAN_CPU(BOARD_ID_PROGRAM);
  // Initializing registers, gates, and flags:
  reset();
  can->send_message(RESET_CMD_BYTE);
  led_program->update(Data, Counter, Pcount, Pset);
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
    }
    nbb.reset(button);
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
  else if (!demo_mode)  // Regular PROGRAM mode
  {

    // CAN bus receive section:
    if (can->message_available())
    {
      // received a packet
      // Serial.print("Received ");
      uint8_t Cmd, Para, Para2;
      switch (can->message_length())
      {
        case 2:
          can->get_message(Cmd, Para);
          // if (Cmd<0xF0)
          //   Serial.printf("Rx CMD: 0x%02X  PARA: 0x%02X\n", Cmd, Para);
          if (Cmd == CTRL_CMD_BYTE)
          {
            // PcountChange is +1 for rising edge, -1 for falling edge, and 0 for no change
            PcountChanged =  (int)bitRead(Para, CTRL_BIT_PCOUNT) - (int)Pcount;
            Pcount = bitRead(Para, CTRL_BIT_PCOUNT);
            PsetChanged = (int)bitRead(Para, CTRL_BIT_PSET) - (int)Pset;
            Pset = bitRead(Para, CTRL_BIT_PSET);
          }
          else if (Cmd == BOARDS_CMD_BYTE)
          {
            boards_alive = Para;
            // Serial.printf("\nBoards alive = %d\n", boards_alive);
          }
          break;
        case 1:
          can->get_message(Cmd);
          // if (Cmd<0xF0 || Cmd==RESET_CMD_BYTE)
          //   Serial.printf("Rx CMD: 0x%02X\n", Cmd);
          if (Cmd == RESET_CMD_BYTE)
            reset();
          else if (Cmd == PING_CMD_BYTE)
            can->send_message(PONG_CMD_BYTE, BOARD_ID_PROGRAM);
          break;
        default:
          can->clear_message();
          break;
      }
      // Serial.println();
    }

    if (PcountChanged==1)
    {
      Counter = (Counter+1)%16;
      PcountChanged = 0;
      Data = disp_program->get_data(Counter);
      CounterChanged = true;
    }
    if (PsetChanged==1)
    {
      Counter = Data;
      PsetChanged = 0;
      Data = disp_program->get_data(Counter);
      CounterChanged = true;
    }

    // Update LEDs:
    if ((PcountChanged!=0) || (PsetChanged!=0) || CounterChanged)
      led_program->update(Data, Counter, Pcount, Pset);
    // Update display:
    if (CounterChanged)
    {  
      disp_program->setrow(Counter);
      // CAN transmit to other boards
      can->send_message(DATA_CMD_BYTE, Data);
      // Reset CounterChanged flag
      CounterChanged = false;
    }
  }
}
