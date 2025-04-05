//=== LCDPanel for Uhrenzentrale ===
#if defined LCD

#include <Wire.h>
#include <LCDPanel.h>

//=== declaration of var's =======================================
LCDPanel displayPanel = LCDPanel();

uint8_t ui8_DisplayPanelPresent = 0;  // ui8_DisplayPanelPresent: 1 if I2C-LCD-Panel is found

/* mode:
  0   after init "Uhrenzentrale" is displayed
  1   "Betrieb?" is displayed
  2   "Inbetriebnahme?" is displayed
  7   "FastClock" is displayed
 
 10   view current clock state

 20   (edit) mode for CV1
 21   (edit) mode for CV2
 22   (edit) mode for CV3
 23   (edit) mode for CV4
 24   (edit) mode for CV5
 25   (edit) mode for CV6
 26   (edit) mode for CV7
 27   (edit) mode for CV8
 28   (edit) mode for CV9
 
 60   (edit) mode for hour
 61   (edit) mode for minute

200   confirm display CV's
210   confirm display I2C-Scan
211   I2C-Scan

220   display IP-Address
221   display MAC-Address

*/
uint8_t ui8_DisplayPanelMode = 0;  // ui8_DisplayPanelMode for paneloperation

uint8_t ui8_ButtonMirror = 0;
static const uint8_t MAX_MODE = 19 + GetCVCount();

uint16_t ui16_EditValue = 0;
boolean b_Edit = false;

boolean b_IBN = false;

boolean b_MirrorStateOdd = false;
boolean b_MirrorRunning = false;
uint8_t ui8_DeviderMirror = 0;

uint8_t ui8_CursorX = 0;

//=== functions ==================================================
boolean IBNbyDisplayPanel() { return b_IBN; }

void SetDisplayPanelMode(uint8_t ui8_Mode) { ui8_DisplayPanelMode = ui8_Mode; }

void CheckAndInitDisplayPanel()
{
  boolean b_DisplayPanelDetected(displayPanel.detect_i2c(MCP23017_ADDRESS) == 0);
  if(!b_DisplayPanelDetected && ui8_DisplayPanelPresent)
    // LCD was present is now absent:
    b_IBN = false;
    
  if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  {
    // LCD (newly) found:
    // set up the LCD's number of columns and rows: 
    displayPanel.begin(16, 2);

#if defined DEBUG
    Serial.println(F("LCD-Panel found..."));
    Serial.print(F("..."));
    Serial.println(GetSwTitle());
#endif

    if(AlreadyCVInitialized())
      OutTextTitle();

    ui8_DisplayPanelMode = ui8_ButtonMirror = 0;
    ui16_EditValue = 0;
    b_Edit = b_IBN = false;

  } // if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  ui8_DisplayPanelPresent = (b_DisplayPanelDetected ? 1 : 0);
}

void SetDisplayPanelModeBetrieb()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("Betrieb?"));
  ui8_DisplayPanelMode = 1;
}

void SetDisplayPanelModeIBN()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("Inbetriebnahme?"));
  ui8_DisplayPanelMode = 2;
}

void SetDisplayPanelModeCV()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("CV?"));
  ui8_DisplayPanelMode = 200;
}

void SetDisplayPanelModeScan()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("I2C-Scan?"));
  ui8_DisplayPanelMode = 210;
}

void SetDisplayPanelModeFastClock()
{
  ui8_DisplayPanelMode = 7;
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("FastClock"));
}

#if defined ETHERNET_BOARD
void SetDisplayPanelModeIpAdr()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("IP-Adresse"));
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  displayPanel.print(Ethernet.localIP());
  ui8_DisplayPanelMode = 220;
}

void SetDisplayPanelModeMacAdr()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("MAC-Adresse"));
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  uint8_t *mac(GetMACAddress());
  for(uint8_t i = 0; i < 6; i++)
  {
    hexout(displayPanel, (uint8_t)(mac[i]), 2);
    if((i == 1) || (i == 3))
      displayPanel.print('.');
  }

  ui8_DisplayPanelMode = 221;
}
#endif

void InitScan()
{
  displayPanel.setCursor(8, 0);  // set the cursor to column x, line y
  displayPanel.print(' ');       // clear '?'
  ui16_EditValue = -1;
  NextScan();
}

void NextScan()
{
  ++ui16_EditValue;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y

  if (ui16_EditValue > 127)
  {
    OutTextFertig();
    return;
  }

  uint8_t error(0);
  do
  {
    Wire.beginTransmission((uint8_t)(ui16_EditValue));
    error = Wire.endTransmission();

    if ((error == 0) || (error == 4))
    {
      displayPanel.print(F("0x"));
      hexout(displayPanel, (uint8_t)(ui16_EditValue), 2);
      displayPanel.print((error == 4) ? '?' : ' ');
      break;
    }

    ++ui16_EditValue;
    if (ui16_EditValue > 127)
    { // done:
      OutTextFertig();
      break;
    }
  } while (true);
}

void OutTextFertig() { displayPanel.print(F("fertig")); }

void OutTextTitle()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(GetSwTitle()); 
    
  displayPanel.setCursor(0, 1);
  displayPanel.print(F("Version "));
  displayPanel.print(GetCV(VERSION_NUMBER));
#if not defined DEBUG
  #if not defined TELEGRAM_FROM_SERIAL
    displayPanel.setCursor(12, 1);  // set the cursor to column x, line y
    displayPanel.print(DIRECT_CLOCK_PULSE ? F("Takt") : F(" DCC"));
  #endif
#endif
#if defined DEBUG
  displayPanel.print(F(" Deb"));
#endif
#if defined TELEGRAM_FROM_SERIAL
  displayPanel.print(F("Sim"));
#endif
}

void OutTextClockStatus()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  if(ENABLE_LN_FC_SLAVE)
  {
    // FastClockSlave:
    displayPanel.print(F("FC-Slave Status"));
  }
  else
  {
    // (FastClock)Master:
    if(!FASTCLOCK_MAIN_SCR)
      displayPanel.print(F("Devider 10:"));
    else
      displayPanel.print(F("--:--   10:"));
    displayPanel.print(GetDevider());
  }
  DisplayClockState();
}

void displayTime(int iLine)
{
  displayPanel.setCursor(0, iLine);  // set the cursor to column x, line y
  uint8_t ui8_Hour;
  uint8_t ui8_Minute;
  GetFastClock(&ui8_Hour, &ui8_Minute);

  if (ui8_Hour < 23)
    decout(displayPanel, ui8_Hour, 2);
  else
    displayPanel.print("??");
  displayPanel.print(':');
  if (ui8_Minute < 60)
    decout(displayPanel, ui8_Minute, 2);
  else
    displayPanel.print("??");
}

void DisplayClockState()
{
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(ENABLE_LN_FC_SLAVE)
  {
    uint16_t ui16_iCount(0);
    uint8_t ui8_Rate(0);
    uint8_t ui8_Sync(0);
    if(!GetClockState(&ui16_iCount, &ui8_Rate, &ui8_Sync))
      displayPanel.print(F("---"));
    else
    {
      decout(displayPanel, ui16_iCount, 4);
      displayPanel.print(F("-1:"));
      // in Slave-mode, rate is coming direct from FastClock-Master
      decout(displayPanel, ui8_Rate, 2);  // Rate: 0 = Freeze clock, 1 = normal, 10 = 10:1 etc. max is 0x7F
      displayPanel.print('-');
      displayPanel.print(ui8_Sync? '1' : '0');
      displayPanel.print('-');
      uint8_t ui8_FCHour(0);
      uint8_t ui8_FCMinute(0);
      GetFastClock(&ui8_FCHour, &ui8_FCMinute);
      displayPanel.print((ui8_FCMinute % 2) ? F("Odd ") : F("Even"));
    }
  }
  else
  {
    // FastClock-Master:
    if(IsClockRunning())
    {
      displayPanel.print(F("Running: Stop?"));
      boolean b_StateOdd(GetStateOdd());
			if (ENABLE_LN_FC_SLAVE)
			{
				displayPanel.setCursor(12, 1);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? F("Odd ") : F("Even"));
			}
			else
			{
				displayPanel.setCursor(15, 0);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? 'O' : 'E');
			}
    }
    else
      displayPanel.print(F("Stopped: Run? "));
  }
}

void DisplayCV(uint16_t ui16_Value)
{
  displayPanel.noCursor();
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("CV"));
  displayPanel.setCursor(3, 0);
  uint8_t ui8CvNr(ui8_DisplayPanelMode - 19);
  if (ui8CvNr < 10)
    displayPanel.print(' ');
  displayPanel.print(ui8CvNr);
  --ui8CvNr;

  // show shortname:
  displayPanel.setCursor(6, 0);
  displayPanel.print(GetCVName(ui8CvNr));

  displayPanel.setCursor(0, 1);
  if (b_Edit)
    displayPanel.print('>');
  if (IsCVBinary(ui8CvNr))
    binout(displayPanel, ui16_Value);
  else
    decout(displayPanel, ui16_Value, GetCountOfDigits(ui8CvNr));
  if (!b_Edit && !CanEditCV(ui8CvNr))
  {
    displayPanel.setCursor(14, 1);
    displayPanel.print(F("ro"));
  }
  if (b_Edit && CanEditCV(ui8CvNr))
  {
    displayPanel.setCursor(ui8_CursorX, 1);
    displayPanel.cursor();
  }
}

uint8_t GetCountOfDigits(uint8_t ui8CvNr)
{
  uint16_t ui16MaxCvValue(GetCVMaxValue(ui8CvNr));
  if (ui16MaxCvValue > 9999)
    return 5;
  if (ui16MaxCvValue > 999)
    return 4;
  if (ui16MaxCvValue > 99)
    return 3;
  if (ui16MaxCvValue > 9)
    return 2;
  return 1;
}

uint16_t GetFactor(uint8_t ui8CvNr)
{
  uint16_t ui16_faktor(0);
  uint8_t ui8_Position(GetCountOfDigits(ui8CvNr));
  switch (ui8_Position - ui8_CursorX)
  {
    case 0: ui16_faktor = 1;
      break;
    case 1: ui16_faktor = 10; 
      break;
    case 2: ui16_faktor = 100; 
      break;
    case 3: ui16_faktor = 1000; 
      break;
    case 4: ui16_faktor = 10000; 
      break;
  }
  return ui16_faktor;
}

void DisplayDate(uint8_t iValue)
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  if(ui8_DisplayPanelMode == 60)
    displayPanel.print(F("Stunde"));
  if(ui8_DisplayPanelMode == 61)
    displayPanel.print(F("Minute"));


  displayPanel.setCursor(0, 1);
  if(b_Edit)
    displayPanel.print('>');
  else
    iValue = GetFastClockTime(ui8_DisplayPanelMode - 60);
  
  if(iValue < GetMaxForDateTime())
    displayPanel.print(iValue);
}

void HandleDisplayPanel()
{
  CheckAndInitDisplayPanel();

  if(ui8_DisplayPanelPresent != 1)
    return;

  uint8_t ui8_bs = 1;
  if(displayPanel.readButtonA5() == BUTTON_A5)
    ui8_bs = 0;
  displayPanel.setBacklight(ui8_bs);

  uint8_t ui8_buttons = displayPanel.readButtons();  // reads only 5 buttons (A0...A4)
  if(!ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = 0;
    return;
  }

  if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = ui8_buttons;
    if(ui8_DisplayPanelMode == 0)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to Betrieb?
        SetDisplayPanelModeBetrieb();  // mode = 1
      }
      return;
    } // if(ui8_DisplayPanelMode == 0)
    //------------------------------------
    if(ui8_DisplayPanelMode == 1)
    {
      // actual ui8_DisplayPanelMode = "Betrieb?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Title
        OutTextTitle();
        ui8_DisplayPanelMode = 0;
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetDisplayPanelModeIBN(); // mode = 2
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current clock state
        OutTextClockStatus();
        ui8_DisplayPanelMode = 10;
        b_MirrorStateOdd = !GetStateOdd();  // force to refresh display
        b_MirrorRunning = !IsClockRunning();
        return;
      }
    } // if(ui8_DisplayPanelMode == 1)
    //------------------------------------
    if(ui8_DisplayPanelMode == 2)
    {
      // actual ui8_DisplayPanelMode = "IBN?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Status
        SetDisplayPanelModeBetrieb();  // mode = 1
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to ask CV / I²C-Scan
        SetDisplayPanelModeCV();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to FastClock
        SetDisplayPanelModeFastClock();
      }
      return;
    } // if(ui8_DisplayPanelMode == 2)
    //------------------------------------
    if(ui8_DisplayPanelMode == 7)
    {
      // actual ui8_DisplayPanelMode = "FastClock"
      if (ui8_buttons & BUTTON_UP)
      { // switch to IBN
        SetDisplayPanelModeIBN(); // mode = 2
        return;
      }
      if ((ui8_buttons & BUTTON_RIGHT) && ENABLE_LN_FC_MASTER)
      { // switch to display current FastClock
        ui8_DisplayPanelMode = 60;
        DisplayDate(GetFastClockTime(0));
        return;
      }
    } // if(ui8_DisplayPanelMode == 7)
    //------------------------------------
    if(ui8_DisplayPanelMode == 10)
    {
      // actual ui8_DisplayPanelMode = view current clock state
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display "Betrieb?"
        SetDisplayPanelModeBetrieb();  // mode = 1
        return;
      }
      if(ENABLE_LN_FC_MASTER)
      {
        if (ui8_buttons & BUTTON_SELECT)
        {
          if (ui8_buttons & BUTTON_RIGHT)
          {
            // send FastClock (start/stop)-telegramm again (undocumented feature)
            if (b_ClockIsRunning)
              SendFastClockTelegram(OPC_SL_RD_DATA, ui8_FCHour, ui8_FCMinute, GetDevider()); // 0xE7
            else
              SendFastClockTelegram(OPC_SL_RD_DATA, ui8_FCHour, ui8_FCMinute, 0); // 0xE7, Devider(Rate) = 0 indicates that clock has stopped 
          }
          else
          {
            InvertClockRunning();
            b_MirrorRunning = !IsClockRunning();
          }
          return;
        }
        if(ENABLE_DEVIDER_CHANGE)
        {
          if (ui8_buttons & BUTTON_UP)
            SetDevider(GetDevider() + 1);
          else if (ui8_buttons & BUTTON_DOWN)
            SetDevider(GetDevider() - 1);
        }
      } // if(ENABLE_LN_FC_MASTER)
      return;
    } // if(ui8_DisplayPanelMode == 10)
    //------------------------------------
    if ((ui8_DisplayPanelMode >= 20) && (ui8_DisplayPanelMode <= MAX_MODE))
    {
      uint8_t ui8CurrentCvNr(ui8_DisplayPanelMode - 20);
      // actual ui8_DisplayPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for current CV
        if (b_Edit)
        {
          // save current value
          boolean b_OK(CheckAndWriteCVtoEEPROM(ui8CurrentCvNr, ui16_EditValue));
          DisplayCV(GetCV(ui8CurrentCvNr));
          displayPanel.setCursor(10, 1);  // set the cursor to column x, line y
          displayPanel.print(b_OK ? F("stored") : F("failed"));
          displayPanel.setCursor(ui8_CursorX, 1);
        }
        return;
      }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        if (b_Edit)
        {
          // leave edit-mode (without save)
          b_Edit = false;
          DisplayCV(GetCV(ui8CurrentCvNr));
          return;
        }
        SetDisplayPanelModeCV();
        b_IBN = false;
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if (!b_Edit && CanEditCV(ui8CurrentCvNr))
        {
          // enter edit-mode
          ui16_EditValue = GetCV(ui8CurrentCvNr);
          b_Edit = true;
          ui8_CursorX = 0;
          DisplayCV(ui16_EditValue);
        }
        if (b_Edit)
        { // move cursor only in one direction
          ++ui8_CursorX;

          // rollover:
          if (IsCVBinary(ui8CurrentCvNr))
          {
            if (ui8_CursorX == 9)
              ui8_CursorX = 1;
          }
          else
          {
            if (ui8_CursorX == (GetCountOfDigits(ui8CurrentCvNr) + 1))
              ui8_CursorX = 1;
          }

          displayPanel.setCursor(ui8_CursorX, 1);
          displayPanel.cursor();
        }
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      {
        if (b_Edit)
        { // Wert vergrößern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue += GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue < ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue;  // Überlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue |= (1 << (8 - ui8_CursorX));

          if (ui16_EditValue > GetCVMaxValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue;  // Überlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to next CV
        ++ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode >= MAX_MODE)
          ui8_DisplayPanelMode = MAX_MODE;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20)); // ui8_DisplayPanelMode has changed :-)
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        if (b_Edit)
        { // Wert verkleinern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue -= GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue > ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue; // Unterlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue &= ~(1 << (8 - ui8_CursorX));

          if (ui16_EditValue < GetCVMinValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          if (ui16_EditValue == UINT16_MAX) // 0 -> 255/65535
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to previous CV
        --ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode < 20)
          ui8_DisplayPanelMode = 20;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20)); // ui8_DisplayPanelMode has changed :-)
        return;
      }
    } // if((ui8_DisplayPanelMode >= 20) && (ui8_DisplayPanelMode <= MAX_MODE))
    //------------------------------------
    if((ui8_DisplayPanelMode == 60) || (ui8_DisplayPanelMode == 61))
    {
      // actual ui8_DisplayPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for date/time
        if(b_Edit)
        {
          if(ui8_DisplayPanelMode == 60) SetFastClockHour(ui16_EditValue);
          if(ui8_DisplayPanelMode == 61) SetFastClockMinute(ui16_EditValue);
          displayPanel.setCursor(8, 1);  // set the cursor to column x, line y
          displayPanel.print(F("stored"));
        }
        return;
      }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current date/time
        if(b_Edit)
        {
          b_Edit = false;
          DisplayDate(GetFastClockTime(ui8_DisplayPanelMode - 60));
          return;
        }
        SetDisplayPanelModeFastClock();
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if(!b_Edit)
        {
          ui16_EditValue = GetFastClockTime(ui8_DisplayPanelMode - 60);
          b_Edit = true;
          DisplayDate(ui16_EditValue);
        }
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      {
        if(b_Edit)
        { // Wert vergrößern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          ++ui16_EditValue;
          if (ui16_EditValue >= GetMaxForDateTime())  // 24 or 60 depending on ui8_DisplayPanelMode (60 for hour, 61 for minute)
            ui16_EditValue = 0;
          DisplayDate((uint8_t)(ui16_EditValue));
          return;
        }
        // switch to next CV
        ++ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode > 61)
          ui8_DisplayPanelMode = 61;
        DisplayDate(GetFastClockTime(ui8_DisplayPanelMode - 60));
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        if (b_Edit)
        { // Wert verkleinern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (ui16_EditValue == 0)
            ui16_EditValue = GetMaxForDateTime() - 1;
          else
            --ui16_EditValue;
          DisplayDate((uint8_t)(ui16_EditValue));
          return;
        }
        // switch to previous CV
        --ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode < 60)
          ui8_DisplayPanelMode = 60;
        DisplayDate(GetFastClockTime(ui8_DisplayPanelMode - 60));
        return;
      }
    } // if((ui8_DisplayPanelMode == 60) || (ui8_DisplayPanelMode == 61))
    //------------------------------------
    if(ui8_DisplayPanelMode == 200)
    {
      // actual ui8_DisplayPanelMode = "CV?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to I²C-Scan
        SetDisplayPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "IBN?"
        SetDisplayPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current CV's
        ui8_DisplayPanelMode = 20;
        DisplayCV(GetCV(ID_DEVICE));
        b_IBN = true;
      }
      return;
    } // if(ui8_DisplayPanelMode == 200)
    //------------------------------------
    if(ui8_DisplayPanelMode == 210)
    {
      // actual ui8_DisplayPanelMode = "I²C-Scan?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "CV?"
        SetDisplayPanelModeCV();
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "Inbetriebnahme?"
        SetDisplayPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display I²C-Addresses
        ui8_DisplayPanelMode = 211;
        InitScan();
      }
#if defined ETHERNET_BOARD
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to display IP-Address
        SetDisplayPanelModeIpAdr();
      }
#endif
      return;
    } // if(ui8_DisplayPanelMode == 210)
    //------------------------------------
    if(ui8_DisplayPanelMode == 211)
    {
      // actual ui8_DisplayPanelMode = "I²C-Scan"
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "I²C-Scan?"
        SetDisplayPanelModeScan();
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        NextScan();
        return;
      }
    } // if(ui8_DisplayPanelMode == 211)
    //------------------------------------
#if defined ETHERNET_BOARD
    if(ui8_DisplayPanelMode == 220)
    {
      // actual ui8_DisplayPanelMode = "IP-Address"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "I²C-Scan ?"
        SetDisplayPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to "MAC-Address"
        SetDisplayPanelModeMacAdr();
      }
      return;
    } // if(ui8_DisplayPanelMode == 220)
    if(ui8_DisplayPanelMode == 221)
    {
      // actual ui8_DisplayPanelMode = "MAC-Address"
      if (ui8_buttons & BUTTON_FCT_BACK)
      { // any key returns to "IP-Address"
        SetDisplayPanelModeIpAdr();
      }
      return;
    } // if(ui8_DisplayPanelMode == 221)
#endif
  } // if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  //========================================================================
  if(ui8_DisplayPanelMode == 7)
  { // display FastClock time in bottom row:
    displayTime(1);
  } // if(ui8_DisplayPanelMode == 7) 
  //========================================================================
  if(ui8_DisplayPanelMode == 10)
  {
    if(FASTCLOCK_MAIN_SCR)
      displayTime(0);        

    uint8_t ui8_Devider(GetDevider());
    if(ui8_Devider != ui8_DeviderMirror)
    {
			if(ENABLE_LN_FC_SLAVE)
				displayPanel.setCursor(7, 1);  // set the cursor to column x, line y
			else
				displayPanel.setCursor(11, 0);  // set the cursor to column x, line y
			displayPanel.print(ui8_Devider);
      ui8_DeviderMirror = ui8_Devider;
    }
    
    boolean b_StateOdd(GetStateOdd());
    if(b_StateOdd != b_MirrorStateOdd)
    {
      b_MirrorStateOdd = b_StateOdd;
			if (ENABLE_LN_FC_SLAVE)
			{
				displayPanel.setCursor(12, 1);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? F("Odd ") : F("Even"));
			}
			else
			{
				displayPanel.setCursor(15, 0);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? 'O' : 'E');
			}
    }

    if(!ENABLE_LN_FC_SLAVE)
    {
      boolean b_Running(IsClockRunning());
      if(b_Running != b_MirrorRunning)
      {
        b_MirrorRunning = b_Running;
        DisplayClockState();
      }
    }
  }
  //========================================================================
}

uint8_t GetMaxForDateTime()
{
  if(ui8_DisplayPanelMode == 60) return 24;
  return 60;
}

#endif
