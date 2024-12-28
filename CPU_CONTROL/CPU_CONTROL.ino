// Board: "ESP 32 Dev Module"

#include "CAN_CPU.h"
#include "LED_CPU.h"
#include "NoBounceButtons.h"
#include "EnoughTimePassed.h"
#include "DISP_CPU.h"

#define BUTTON_PIN 0
#define COUNT_BUTTON_PINS 18,17,16

#define CTRL_BIT_G1 5
#define CTRL_BIT_RA 3
#define CTRL_BIT_RB 4
#define CTRL_BIT_PM 2
#define CTRL_BIT_RC 1
#define CTRL_BIT_G2 0

#define DATA_BITS 4
#define CTRL_BITS 6
#define COUNTER_BITS 4
#define COUNT_BUTTONS 3

unsigned char demo_mode=0;

// CAN driver
CAN_CPU* can;
uint8_t boards_alive=0;

unsigned char Data = 0;
unsigned char Ctrl = 0;
unsigned char Counter = 0;
unsigned char Ccount = 0;
unsigned char Cset = 0;
unsigned char Creset = 0;
unsigned char Pcount = 0;
unsigned char Pset = 0;

bool CounterChanged = true;

NoBounceButtons nbb;
unsigned char button;
unsigned char CountButtons[COUNT_BUTTONS];
unsigned char CountButtonPins[COUNT_BUTTONS] = {COUNT_BUTTON_PINS};

unsigned char autocount=0; // 0=manual, 1=auto count slow, 2=auto count fast
EnoughTimePassed etp_slow_autocount(500);  // slow count period (ms), autocount=1
EnoughTimePassed etp_fast_autocount(50);  // fast count period (ms), autocount=2

EnoughTimePassed etp_demo(1000);  // Blink period (ms) in demo mode
unsigned char demo_flip_flag=0;

LED_CONTROL *led_control;
DISP_CONTROL *disp_control;

// TFT_eSPI *tft;
      
void reset()
{
  Data = 0;
  Counter = 0;
  Ctrl = disp_control->get_ctrl(Counter);
  Cset = disp_control->get_cset(Counter);
  Creset = disp_control->get_creset(Counter);
  Pcount = disp_control->get_pcount(Counter);
  Pset = disp_control->get_pset(Counter);
  Ccount = 0;
  CounterChanged = true;
  // Serial.printf("Reset: Data=%d, Counter=%d, Ctrl=%d, Cset=%d, Creset=%d, Pcount=%d, Pset=%d\n", Data, Counter, Ctrl, Cset, Creset, Pcount, Pset);
}

void count()
{
  if (Cset)
  {
    if (Counter==0 && Data==0)
      autocount = 0;  
    Counter = Data;
  }
  else if (Creset)
    Counter = 0;
  else
    Counter = (Counter+1)%16;
  CounterChanged = true;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU CONTROL UNIT BOARD");
  // Setting up buttons:
  button = nbb.create(BUTTON_PIN);
  for (int n=0; n<COUNT_BUTTONS; n++)
    CountButtons[n] = nbb.create(CountButtonPins[n]);
  // Setting up LEDs:
  led_control = new LED_CONTROL();
  // Setting up the display:
  disp_control = new DISP_CONTROL();
  // Start CAN
  can = new CAN_CPU(BOARD_ID_CONTROL);
  // Initializing registers, gates, and flags:
  reset();
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
  disp_control->check();
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
      led_control->update(10,0b101010,10,1,0,1,0,1);
    else
      led_control->update(5,0b010101,5,0,1,0,1,0);
    demo_flip_flag = !demo_flip_flag;
  }
  else if (!demo_mode)  // Regular MANUAL CONTROL mode
  {

    // Counter button handling
    if (nbb.action(CountButtons[0]) == NBB_CLICK)
    {
      count();
      nbb.reset(CountButtons[0]);
    }
    if (nbb.action(CountButtons[1]) == NBB_CLICK)
    {
      // Serial.printf("CountButtons[1] clicked.\n");
      if (autocount==0)
        autocount=1;
      else
        autocount=0;
      nbb.reset(CountButtons[1]);
    }
    if (nbb.action(CountButtons[2]) == NBB_CLICK)
    {
      // Serial.printf("CountButtons[2] clicked.\n");
      if (autocount==0)
        autocount=2;
      else
        autocount=0;
      nbb.reset(CountButtons[2]);
    }

    // Handling of Ccount LED:
    if (autocount>0)
      Ccount = 1;
    else
      Ccount = 0;
      
    // Auto count handling
    if ((autocount==1 && etp_slow_autocount.enough_time()) || (autocount==2 && etp_fast_autocount.enough_time()))
      count();

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
          // if (Cmd<0xF0 || Cmd==RESET_CMD_BYTE)
          //   Serial.printf("Rx CMD: 0x%02X  PARA: 0x%02X\n", Cmd, Para);
          if (Cmd == DATA_CMD_BYTE)
          {
            Data = Para;
            led_control->update(Data, Ctrl, Counter, Ccount, Cset, Creset, Pcount, Pset);
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
            can->send_message(PONG_CMD_BYTE, BOARD_ID_CONTROL);
          break;
        default:
          can->clear_message();
          break;
      }
      // Serial.println();
    }

    if (CounterChanged)
    {
      // Get bits from memory
      Ctrl = disp_control->get_ctrl(Counter);
      Cset = disp_control->get_cset(Counter);
      Creset = disp_control->get_creset(Counter);
      Pcount = disp_control->get_pcount(Counter);
      Pset = disp_control->get_pset(Counter);
      // CAN transmit to other boards
      // Serial.printf("Before CAN send:\nData=%d, Counter=%d, Ctrl=%d, Cset=%d, Creset=%d, Pcount=%d, Pset=%d\n", Data, Counter, Ctrl, Cset, Creset, Pcount, Pset);
      can->send_message(CTRL_CMD_BYTE, Ctrl + (Pcount<<6) + (Pset<<7));
      //Update LEDs:
      disp_control->setrow(Counter);
      led_control->update(Data, Ctrl, Counter, Ccount, Cset, Creset, Pcount, Pset); 
      CounterChanged = false;
    }
  }
}
