/*
 * Uhrenzentrale_StartStop
 
used I²C-Addresses:
  - 0x20 LCD-Panel
  
discrete In/Outs used for functionalities:
  -  0    (used  USB)
  -  1    (used  USB)
  -  2
  -  3 Out used   LED grün (Start)
  -  4 In  used   Taster grün (Start)
  -  5 In  used   Taster rot (Stop)
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9 
  - 10 Out used   LED rot (Stop)
  - 11
  - 12
  - 13
  - 14
  - 15
  - 16
  - 17     
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
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
                  // please comment out also includes in system.ino

//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define FAST_CLOCK_LOCAL 1  //use local ports for slave clock if no I2C-clock is found.

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
                                // instead from LocoNet-Port (which is inactive then)

#define LCD     // used in GlobalOutPrint.ino

#include "CV.h"

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)

//#define LN_SHIELD             (1)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID  13   // NMRA: DIY
#define DEVELOPER_ID  58      // NMRA: my ID, should be > 27 (1 = FREMO, see https://groups.io/g/LocoNet-Hackers/files/LocoNet%20Hackers%20DeveloperId%20List_v27.html)

//========================================================

#include <LocoNet.h>  // requested for notifySwitchOutputsReport

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
  
  CheckAndInitLCDPanel();

  InitLocoNet();

  InitStartStop();
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
  HandleLCDPanel();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

  //=== do general clock-handling ==========
  HandleControls();

#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

//=== will be called from LocoNet-Class
void notifySwitchOutputsReport(uint16_t Address, uint8_t Output, uint8_t Direction)
{ // OPC_SW_REP (B1) received, (LnPacket->data[2] & 0x40) == 0x00:
 handleReceivedB1(Address, Output, Direction);
}
