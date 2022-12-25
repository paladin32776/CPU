#include <CAN.h>
#include "LED_CPU.h"

#define TEST_MODE 0

#define DATA_CMD_BYTE 0x00
#define CTRL_CMD_BYTE 0x01
#define CTRL_BIT_G1 0
#define CTRL_BIT_RB 1
#define CTRL_BIT_RA 2
#define CTRL_BIT_PM 3
#define CTRL_BIT_RC 4
#define CTRL_BIT_G2 5

#define DATA_BITS 4
#define CTRL_BITS 6

LED_ALU *led_alu;

unsigned char Data = 0;
unsigned char Ctrl = 0;
unsigned char Bus = 0;

unsigned char RA, RB, RC;
bool eG1, eRA, eRB, ePM, eRC, eG2;
unsigned char NEG, OVERFLOW;

void reset()
{
  RA = RB = RC = 0;
  eG1 = eRA = eRB = ePM = eRC = eG2 = false;
  NEG = OVERFLOW = 0;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("CPU ALU BOARD");
  // Setting up LEDs:
  led_alu = new LED_ALU();
  // Initializing registers, gates, and flags:
  reset();
  led_alu->update(Bus,RA,RB,RC,Ctrl,NEG,OVERFLOW);
  // start the CAN bus at 1 Mbps
  if (!CAN.begin(1E6))
  {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop()
{
  if (TEST_MODE) // Test mode, just blinking some leds
  {
    led_alu->update(10,10,10,10,0b100101,1,0);
    delay(1000);
    led_alu->update(5,5,5,5,0b011010,0,1);
    delay(1000);
  }
  else // Regular ALU mode
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
            Data = Para;
          else if (Cmd==CTRL_CMD_BYTE)
            Ctrl = Para;
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
    // Ctrl to register/gate/ALU control settings:
    eRA = (Ctrl>>CTRL_BIT_RA) & 1;
    eRB = (Ctrl>>CTRL_BIT_RB) & 1;
    eRC = (Ctrl>>CTRL_BIT_RC) & 1;
    eG1 = (Ctrl>>CTRL_BIT_G1) & 1;
    eG2 = (Ctrl>>CTRL_BIT_G2) & 1;
    ePM = (Ctrl>>CTRL_BIT_PM) & 1;
    // Logic for whole CPU core:
    // Determine bus state:
    if (eG1 && !eG2)
      Bus = Data;
    else if (!eG1 && eG2)
      Bus = RC;
    else
      Bus = 0;
    // Load registers from bus if enabled:
    if (eRA)
      RA = Bus;
    if (eRB)
      RB = Bus;
    // Determine ALU output and load into register if enabled:
    if (eRC)
      RC = (RA+RB)*(!ePM) + (RA-RB)*ePM;
    // Determine OVERFLOW flag:
    if ((RA+RB>15) && ePM==false)
      OVERFLOW = 1;
    else
      OVERFLOW=0;
    // Determine NEG flag:
    if ((RB>RA) && ePM==true)
      NEG = 1;
    else
      NEG = 0;
    // Update LEDs:
    led_alu->update(Bus,RA,RB,RC,Ctrl,NEG,OVERFLOW); // Overflow and Negative not done yet ...
  }
}


