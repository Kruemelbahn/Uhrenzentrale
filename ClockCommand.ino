//=== Routines for Uhrenzentrale ===

//=== declaration of var's =======================================
#define CLOCK_State         9

boolean b_DCCOut = true;
boolean b_ClockIsRunning = false;

// true  = odd minute, set Function 1 in telegram
// false = even minute, reset Function 1 in telegram
boolean b_MinuteOdd = false;
boolean b_MinuteOddMirror = false;

unsigned long ul_WaitDevider = 0;
uint8_t ui8_LastMinute = 0;

//=== FastClock-Time functions =============================================
uint8_t GetFastClockTime(uint8_t i)
{
	uint8_t ui8_FCHour(0);
	uint8_t ui8_FCMinute(0);
	GetFastClock(&ui8_FCHour, &ui8_FCMinute);

	if(i == 0)
    return ui8_FCHour;
  return ui8_FCMinute;
}

void SetFastClockHour(uint8_t i)
{
	uint8_t ui8_FCHour(0);
	uint8_t ui8_FCMinute(0);
	GetFastClock(&ui8_FCHour, &ui8_FCMinute);

	SetFastClock( GetDevider() / 10, 0, i, ui8_FCMinute, 1);
}

void SetFastClockMinute(uint8_t i)
{
	uint8_t ui8_FCHour(0);
	uint8_t ui8_FCMinute(0);
	GetFastClock(&ui8_FCHour, &ui8_FCMinute);

	SetFastClock( GetDevider() / 10, 0, ui8_FCHour, i, 1);
}

void IncMinute()
{
	uint8_t ui8_FCHour(0);
	uint8_t ui8_FCMinute(0);
	GetFastClock(&ui8_FCHour, &ui8_FCMinute);
	
	IncAndSetFastClock(&ui8_FCHour, &ui8_FCMinute);
}

void IncAndSetFastClock(uint8_t *ui8_Hour, uint8_t *ui8_Minute)
{
	++(*ui8_Minute);
	// adjust time
	if (*ui8_Minute > 59)
	{
		*ui8_Minute = 0;
		++(*ui8_Hour);
		if (*ui8_Hour > 23)
			*ui8_Hour = 0;
	}
	SetFastClock(GetDevider() / 10, 0, *ui8_Hour, *ui8_Minute, 1);
}

//=== functions ==================================================
boolean IsDCCout() { return b_DCCOut; }

boolean GetStateOdd() { return b_MinuteOdd; }

boolean IsClockRunning() { return b_ClockIsRunning; }

void InvertClockRunning()
{ 
  boolean bSendFirstStartTelegramm(!b_ClockIsRunning);
  boolean bSendFirstStopTelegramm(b_ClockIsRunning);

	b_ClockIsRunning = !b_ClockIsRunning;
	
  uint8_t ui8_FCHour(0);
  uint8_t ui8_FCMinute(0);
  GetFastClock(&ui8_FCHour, &ui8_FCMinute);

	if (b_ClockIsRunning)
	{ // (re)start:
    if(bSendFirstStartTelegramm)
 			SendFastClockTelegram(OPC_SL_RD_DATA, ui8_FCHour, ui8_FCMinute, GetDevider()); // 0xE7
		ul_WaitDevider = millis();
	}
	if (!b_ClockIsRunning)
	{ // stop:
    if(bSendFirstStopTelegramm)
 			SendFastClockTelegram(OPC_SL_RD_DATA, ui8_FCHour, ui8_FCMinute, 0); // 0xE7, Devider(Rate) = 0 indicates that clock has stopped 
		ul_WaitDevider = millis();
	}
  sendClockState();
}

boolean CanSendClockMsg()
{
  if(b_ClockIsRunning && (b_MinuteOdd != b_MinuteOddMirror))
  { // clock msg only, if clock is running and state has changed
    b_MinuteOddMirror = b_MinuteOdd;
    return true;
  }
  return false;
}

void SetClockMode(boolean b_mode) { if(b_mode != b_ClockIsRunning) InvertClockRunning(); }

uint8_t GetDevider() { return GetCV(ID_DEVIDER); }

void SetDevider(uint8_t ui8_CurrentDevider)
{
  CheckAndWriteCVtoEEPROM(ID_DEVIDER, ui8_CurrentDevider);
}

void InitClockCommander()
{
  pinMode(CLOCK_State, OUTPUT);
  
	b_MinuteOdd = false;
	ui8_LastMinute = 0;

#if defined DEBUG
	if (ENABLE_LN_FC_MASTER)
		Serial.println(F("...work as FastClock-Master"));
#endif

	b_DCCOut = !DIRECT_CLOCK_PULSE;
	if (ENABLE_LN_FC_SLAVE)
		b_DCCOut = false;
	InitClockDCC();
}

void HandleClockCommander()
{
	uint8_t ui8_FCHour(0);
	uint8_t ui8_FCMinute(0);
	boolean bFCState(GetFastClock(&ui8_FCHour, &ui8_FCMinute));
  // handle clock depending on devider
	// run clock?
	if (b_ClockIsRunning)
	{
    if(ENABLE_LN_FC_SLAVE)
    {
      if(ENABLE_LN_FC_MODUL)
      {
        // FastClock will do the job:
        if(bFCState)
        {
          if(ui8_LastMinute != ui8_FCMinute)
          {
            ui8_LastMinute = ui8_FCMinute;
            b_MinuteOdd = (ui8_FCMinute & 0x01 ? true: false);
          }
        }
      } // if(ENABLE_LN_FC_MODUL)
    } // if(ENABLE_LN_FC_SLAVE)
    else
    {
      // (FastClock)Master
      uint8_t ui8_Devider(GetDevider());
      if (!ui8_Devider)
        ui8_Devider = 10; // in case of Error
      unsigned long ul_Devider(600000 / ui8_Devider);
    
  		if ((millis() - ul_WaitDevider) > ul_Devider)
  		{
  			ul_WaitDevider = millis();
				IncAndSetFastClock(&ui8_FCHour, &ui8_FCMinute);
  			SendFastClockTelegram(OPC_SL_RD_DATA, ui8_FCHour, ui8_FCMinute, ui8_Devider); // 0xE7
  
				b_MinuteOdd = (ui8_FCMinute & 0x01 ? true : false);
			}
      // set led
      digitalWrite(CLOCK_State, b_MinuteOdd);
    } // else

		if (!b_DCCOut)
		{
			// set outputs directly - no DCC-out!
			// alles ausschalten:
			uint8_t ui8_PortC(PORTC);
			ui8_PortC &= 0xFC;
			ui8_PortC += (GetClockPhase() & 0x03);
			PORTC = ui8_PortC;
		}

	} // if(b_ClockIsRunning)

  // handle DCC-message
	HandleClockDCC();
}
