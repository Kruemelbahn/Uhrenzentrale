/*
 * Uhrenzentrale
 
used I²C-Addresses:
  - 0x20  LCD-Panel ODER:
  
  - 0x78  OLED-Panel mit
  - 0x21  Button am OLED-Panel

  - 0x3D FastClock-Interface
  - 0x70 FastClock-LED-Display
  
discrete In/Outs used for functionalities:
  -  0    (used  USB)
  -  1    (used  USB)
	-  2 Out used   by (non I²C) FastClock
	-  3 Out used   by (non I²C) FastClock
	-  4 In  used   by (non I²C) Fastclock
			 Out [CS]   (used  by ETH-Shield for SD-Card-select, Remark: memory using SD = 4862 Flash, 791 RAM))
	-  5
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9
  - 10
  - 11
  - 12
  - 13
  - 14 Out used   by DCC+
  - 15 Out used   by DCC-
  - 16
  - 17     
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2019 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor
// note: activating SerialMonitor in Arduino-IDE
//       will create a reset in software on board!

//#define DEBUG_CV 1 // enables CV-Output to serial port during program start (saves 180Bytes of code :-)
//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define USE_SD_CARD_ON_ETH_BOARD 1      // enables pin 4 for cardselect of SD-card on Ethernet-Board
//#define ETHERNET_BOARD 1                // needs ~14k of code, ~320Bytes of RAM
//#define ETHERNET_WITH_LOCOIO 1
//#define ETHERNET_PAGE_SERVER 1
//#define ETHERNET_CLOCKCOMMANDER 1

//#define FAST_CLOCK_LOCAL 1  //use local ports as slaveclockmodule as an alternative/additional for local I2C-Module

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
// instead from LocoNet-Port (which is inactive then)

#define LCD  // used in GlobalOutPrint.ino, LCDPanel.ino
//#define OLED    // used in OLEDPanel.ino

#if defined LCD
  #if defined OLED
    #error LCD and OLED defined
  #endif
#endif
#if !defined LCD
  #if !defined OLED
    #error Neither LCD nor OLED defined
  #endif
#endif

#include "CV.h"

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)
#define ENABLE_LN_FC_MODUL    (GetCV(ADD_FUNCTIONS_1) & 0x04)
#define ENABLE_LN_FC_INTERN   (GetCV(ADD_FUNCTIONS_1) & 0x08)
#define ENABLE_LN_FC_INVERT   (GetCV(ADD_FUNCTIONS_1) & 0x20)
#define ENABLE_LN_FC_SLAVE    (ENABLE_LN_FC_MODUL)
#define ENABLE_LN_FC_MASTER   (!ENABLE_LN_FC_SLAVE)
#define ENABLE_LN_FC_JMRI     (GetCV(ADD_FUNCTIONS_1) & 0x10)

#define ENABLE_DEVIDER_CHANGE (GetCV(ADD_FUNCTIONS_2) & 0x01)
#define FASTCLOCK_MAIN_SCR    ((GetCV(ADD_FUNCTIONS_2) & 0x02) && ENABLE_LN_FC_MASTER)
#define DIRECT_CLOCK_PULSE    (GetCV(ADD_FUNCTIONS_2) & 0x10)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID 13  // NMRA: DIY
#define DEVELOPER_ID 58     // NMRA: my ID, should be > 20 (1 = FREMO)

//========================================================
//=== declaration of var's ===============================
unsigned long ul_MillisAtStart = 0;

//========================================================

#include <LocoNet.h>  // requested for notifyPower, notifySwitchRequest, notifyFastClock

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  ReadCVsFromEEPROM();

  CheckAndInitDisplayPanel();

  InitLocoNet();

  if (ENABLE_LN_FC_MODUL)
    InitFastClock();

#if defined ETHERNET_BOARD
  InitEthernet();
#endif

  InitClockCommander();

  ul_MillisAtStart = millis();
}

void loop()
{
  // light the Heartbeat LED
  oHeartbeat.beat();
  // generate blinken
  Blinken();

  //=== do LCD handling ==============
  // can be connected every time
  // panel only necessary for setup CV's (or some status informations):
  HandleDisplayPanel();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

#if defined ETHERNET_BOARD
  //=== react on Ethernet requests ===
  HandleEthernetRequests();
#endif

  //=== do FastClock handling ===
  if (ENABLE_LN_FC_MODUL)
    HandleFastClock();

  //=== do general clock-handling ==========
  uint8_t ui8_WaitTime(GetCV(ID_WAITTIME));
  if (ui8_WaitTime && ul_MillisAtStart && ((millis() - ul_MillisAtStart) > (ui8_WaitTime * 1000)))
  {
    ul_MillisAtStart = 0;
    SetDisplayPanelMode(10);
    OutTextClockStatus();
  }
  HandleClockCommander();

#if defined DEBUG
#if defined DEBUG_MEM
  ViewFreeMemory();  // shows memory usage
  ShowTimeDiff();    // shows time for 1 cycle
#endif
#endif
}

//=== will be called from LocoNet-Class
void notifyPower(uint8_t State)
{
  // LocoNET-Hack:
  // on Power-On (OPC_GPON) we will answer the current State using B1-Telegram:
  if (State == 1)
    sendClockState();
}

// Address: Switch Address.
// Output: Value 0 for Coil Off, anything else for Coil On
// Direction: Value 0 for Closed/GREEN, anything else for Thrown/RED
// state: Value 0 for no input, anything else for activated
// Sensor: Value 0 for 'Aux'/'thrown' anything else for 'switch'/'closed'
void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction)
{  // OPC_SW_REQ (B0) received:
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  PrintAdr(Address);
#endif
  if (Output)  // "Ein"
  {
    uint16_t ui16_Adr(readAddressFromClock_OnOff());
    if (ui16_Adr && (Address == ui16_Adr))
      SetClockMode(Direction ? true : false);
  }
}

/*=== will be called from LocoNetFastClockClass
      if telegram is OPC_SL_RD_DATA [0xE7] or OPC_WR_SL_DATA [0xEF] and clk-state != IDLE ==================*/
void notifyFastClock(uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync)
{
#if defined DEBUG
  Serial.print(F("notifyFastClock "));
  Serial.print(Hour);
  Serial.print(":");
  decout(Serial, Minute, 2);
  Serial.print("[Rate=");
  Serial.print(Rate);
  Serial.print("][Sync=");
  Serial.print(Sync);
  Serial.println("]");
#endif
  SetFastClock(Rate, Day, Hour, Minute, Sync);
}

void notifyFastClockFracMins(uint16_t FracMins)
{
  HandleFracMins(FracMins);
}
