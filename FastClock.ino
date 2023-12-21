//=== FastClock === usable for all ====================================
#include <Wire.h>
#include <PCF8574.h>

#include <Adafruit_LEDBackpack.h>

//=== global stuff =======================================

//=== declaration of var's =======================================
#define FC_PHASE_A_PORT 2
#define FC_PHASE_B_PORT 3
#if not defined CS_SD_CARD
	#define CS_SD_CARD 4
#endif

#define SLAVE_CLOCK_ADDRESS_MODULE  0x3D
#define SLAVE_CLOCK_ADDRESS_LEDDISPLAY  0x70

#define FC_PHASE_A  0x01  // on even minutes, control-LED is on
#define FC_PHASE_B  0x02  // on odd minutes, control-LED is off

PCF8574 SlaveClock(SLAVE_CLOCK_ADDRESS_MODULE);
Adafruit_7segment SlaveClockLED = Adafruit_7segment();

boolean b_SlaveClockModulePresent = false;
boolean b_SlaveClockLEDDisplayPresent = false;
boolean b_FCButtonMirror = false;
boolean b_FastClockReceived = false;
boolean b_FastClockIsRunning = false;
uint8_t ui8_FCHour;
uint8_t ui8_FCMinute;
uint8_t ui8_FCOut;
uint8_t ui8_FCDay;
uint8_t ui8_FCRate;
uint8_t ui8_FCSync;
uint16_t ui16_FCCount;

boolean GetFastClockState() { return b_FastClockReceived; }

uint8_t GetClockPhase() { return ui8_FCOut; }

boolean isFastClockRunning() { return b_FastClockIsRunning; }

uint8_t GetClockRate() { return ui8_FCRate; }

unsigned long ul_LastSetFastClock = 0;

//=== functions for FastClock =======
// FastClock is independent of any other functionality (except LocoNet)

// initialize any external modules for displaying (FastClock-)Time 
void InitFastClock()
{
  ui8_FCRate = 0;
	ui16_FCCount = 0;
	ul_LastSetFastClock = 0;
  b_FastClockIsRunning = false;

#if defined DEBUG
    Serial.println(F("FastClock..."));
#endif

  if(!ENABLE_LN || !ENABLE_LN_FC_MODUL)
    return;
  
  Wire.begin();
  
  Wire.beginTransmission(SLAVE_CLOCK_ADDRESS_MODULE);
  if(Wire.endTransmission() == 0)
  {
    b_SlaveClockModulePresent = true;
#if defined DEBUG
    Serial.println(F("...Slave Clock found"));
#endif
  }
#if defined FAST_CLOCK_LOCAL
  if(!b_SlaveClockModulePresent)
  {
    pinMode(FC_PHASE_A_PORT, OUTPUT);
    pinMode(FC_PHASE_B_PORT, OUTPUT);
  #if not defined ETHERNET_BOARD
    pinMode(CS_SD_CARD, INPUT_PULLUP);
  #endif
  }
#endif

  Wire.beginTransmission(SLAVE_CLOCK_ADDRESS_LEDDISPLAY);
  if(Wire.endTransmission() == 0)
  {
    b_SlaveClockLEDDisplayPresent = true;
    SlaveClockLED.begin(SLAVE_CLOCK_ADDRESS_LEDDISPLAY);
#if defined DEBUG
    Serial.println(F("...LED-Slave Clock found"));
#endif
  }
}

void HandleFastClock()
{
	if (b_FastClockIsRunning && ENABLE_LN_FC_SLAVE && !ENABLE_LN_FC_INTERN)
	{ // if ENABLE_LN_FC_INTERN is true, time will be calculated with the help of 'notifyFastClockFracMins'
		// when FastClock is running: last SetFastClock (coming from EF/E7-telegram) not received since more then 60s 
    unsigned long ulWaitingFoNextTelegram(60000);
    if (ui8_FCRate)
      ulWaitingFoNextTelegram /= ui8_FCRate;
		if ((millis() - ul_LastSetFastClock) > ulWaitingFoNextTelegram)
			PollFastClock();
	}

  boolean bButton(false);
  if(b_SlaveClockModulePresent)
  {
    uint8_t ui8_FCIn = SlaveClock.read8();	// local button for clock-(test)impuls on FastClock-Module
    if((ui8_FCIn & 0x04) == 0)
		{
      bButton = true;
#if defined DEBUG
			Serial.println(F("FastClock Testbutton pressed"));
#endif
		}
  }
#if defined FAST_CLOCK_LOCAL
  else
  {
    uint8_t ui8_FCIn = digitalRead(CS_SD_CARD); // local button for clock-(test)impuls direct connected to atmege328
    if(ui8_FCIn == LOW)
		{
			bButton = true;
#if defined DEBUG
			Serial.println(F("Local Testbutton pressed"));
#endif
}
	}
#endif
  if(!bButton)
  {
    b_FCButtonMirror = false;
    return;    // button NOT pressed
  }
  else
  {
    if(!b_FCButtonMirror)
    {
      b_FCButtonMirror = true;
      if(ui8_FCOut == FC_PHASE_A)
        ui8_FCOut = FC_PHASE_B;
      else
        ui8_FCOut = FC_PHASE_A;
      ActualizeSlaveClock();
    }
  }
}

void ActualizeSlaveClock()
{
  if(b_SlaveClockModulePresent)
    SlaveClock.write8(ui8_FCOut | 0xFC);
#if defined FAST_CLOCK_LOCAL
  else
  {
    digitalWrite(FC_PHASE_A_PORT, ui8_FCOut == FC_PHASE_A);
    digitalWrite(FC_PHASE_B_PORT, ui8_FCOut == FC_PHASE_B);
  }
#endif
}

boolean IncFastClock(uint8_t ui8Increment)
{
  ui8_FCMinute += ui8Increment;
  if (ui8_FCMinute > 59)
  {
    ui8_FCMinute -= 60;
    ui8_FCHour++;
    if (ui8_FCHour > 23)
      ui8_FCHour -= 24;
  }
  SetFastClock(ui8_FCRate, ui8_FCDay, ui8_FCHour, ui8_FCMinute, ui8_FCSync);
}

// Get (FastClock) Time for display etc.
boolean GetFastClock(uint8_t *ui8_Hour, uint8_t *ui8_Minute)
{
  *ui8_Hour = ui8_FCHour;
  *ui8_Minute = ui8_FCMinute;

  if(!b_FastClockReceived)
    return false;

  return true;
}

void SetFastClock( uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync )
{
  b_FastClockReceived = true;
	ui16_FCCount++;

	ui8_FCRate = Rate;
	ui8_FCSync = Sync;

  // if Rate is Zero assume, Fastclock isn't runnnung / has stopped
  b_FastClockIsRunning = (Rate != 0);

	ul_LastSetFastClock = millis();

  // if Sync is Zero assume function-call not coming from FastClock-Master by telegram but is calculated time
  if((Sync == 0) && !ENABLE_LN_FC_INTERN) // received automagically from 
    return;                               // 'LocoNetFastClockClass::process66msActions'
                                          // after first FastClock-Telegramm 'E7' is received
  if(!b_FastClockIsRunning)
    return;

	if(ui8_FCMinute != Minute)
  {
    // update slave clock(s) on I2C-bus:
    if((Minute % 2) == 0)
      // Minute is even
      ui8_FCOut = (ENABLE_LN_FC_INVERT ? FC_PHASE_B : FC_PHASE_A);
    else
      // Minute is odd
      ui8_FCOut = (ENABLE_LN_FC_INVERT ? FC_PHASE_A : FC_PHASE_B);
    ActualizeSlaveClock();

    if(b_SlaveClockLEDDisplayPresent)
    { // update 4-digit-7-SEG-LED
      uint16_t time = Hour * 100 + Minute;
      SlaveClockLED.print(time);
      SlaveClockLED.drawColon(true);
      SlaveClockLED.writeDisplay();
    } // if(b_SlaveClockLEDDisplayPresent)
  }

  ui8_FCHour = Hour;
  ui8_FCMinute = Minute;
	ui8_FCDay = Day;

#if defined DEBUG
 	Serial.print("Rate: "); Serial.print(Rate, DEC);
	Serial.print(" Day: "); Serial.print(Day, DEC);
	Serial.print(" Hour: "); Serial.print(Hour, DEC);
	Serial.print(" Min: "); Serial.print(Minute, DEC);
	Serial.print(" Sync: "); Serial.println(Sync, DEC);
#endif
}

boolean isTimeForProcessActions(unsigned long *timeMark, unsigned long timeInterval)
{
  unsigned long timeNow = millis();
  if ( timeNow - *timeMark >= timeInterval)
  {
    *timeMark += timeInterval;
    return true;
  }
  return false;
}

boolean GetClockState(uint16_t *iCount, uint8_t *ui8_Rate, uint8_t *ui8_Sync)
{
	*iCount = ui16_FCCount;
	*ui8_Rate = ui8_FCRate;
	*ui8_Sync = ui8_FCSync;

	return b_FastClockReceived;
}
