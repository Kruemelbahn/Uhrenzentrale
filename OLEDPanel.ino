//=== OLEDPanel for Uhrenzentrale ===
#if defined OLED

#include <OLEDPanel.h>

//=== declaration of var's =======================================
//--- for use of OLED
#define PCF8574_ADDR_OLED_BUTTON (0x21 << 1)
#define PCF8574A_ADDR_OLED_BUTTON (PCF8574_ADDR_OLED_BUTTON + 0x18)

#define CTRL_BUTTON 5
#define BUTTON_KEYPAD 0x80

OLEDPanel displayPanel;

uint8_t ui8_DisplayPanelPresent = 0;  // ui8_DisplayPanelPresent: 1 if I2C-OLED-Panel is found
//--- end OLED

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

// used for binary-edit:
boolean b_EditBinary = false;
uint8_t ui8_CursorX = 0;

//=== functions originally from GlobalOutPrint.ino ===============
void binout(uint8_t ui8_Out)
{
  if(ui8_Out<128)
    displayPanel.print('0');
  if(ui8_Out<64)
    displayPanel.print('0');
  if(ui8_Out<32)
    displayPanel.print('0');
  if(ui8_Out<16)
    displayPanel.print('0');
  if(ui8_Out<8)
    displayPanel.print('0');
  if(ui8_Out<4)
    displayPanel.print('0');
  if(ui8_Out<2)
    displayPanel.print('0');
  displayPanel.print(ui8_Out, BIN);
}

void decout(uint16_t ui16_Out, uint8_t ui8CountOfDigits)
{
  // 5 digits, 0..99999
  if ((ui8CountOfDigits > 4) && (ui16_Out < 10000))
    displayPanel.print('0');
  if ((ui8CountOfDigits > 3) && (ui16_Out < 1000))
    displayPanel.print('0');
  if ((ui8CountOfDigits > 2) && (ui16_Out < 100))
    displayPanel.print('0');
  if ((ui8CountOfDigits > 1) && (ui16_Out < 10))
    displayPanel.print('0');
  displayPanel.print(ui16_Out, DEC);
}

void hexout(uint16_t ui16_Out, uint8_t ui8CountOfDigits)
{
  // 4 digits, 0..FFFF
  if((ui8CountOfDigits > 3) && (ui16_Out < 0x1000))
    displayPanel.print('0');
  if((ui8CountOfDigits > 2) && (ui16_Out < 0x0100))
    displayPanel.print('0');
  if((ui8CountOfDigits > 1) && (ui16_Out < 0x0010))
    displayPanel.print('0');
  displayPanel.print(ui16_Out, HEX);
}

//=== functions ==================================================
boolean IBNbyDisplayPanel() { return b_IBN; }

void SetDisplayPanelMode(uint8_t ui8_Mode) { ui8_DisplayPanelMode = ui8_Mode; }

void CheckAndInitDisplayPanel()
{
  //---OLED-----------------
  // OLED & buttons: type of display-controler and I²C-address is defined in "lcd.h"

  boolean b_DisplayPanelDetected(displayPanel.detect_i2c(PCF8574_ADDR_OLED_BUTTON) == 0);
  bool bAddrA(false);
  if(!b_DisplayPanelDetected)
  {
    b_DisplayPanelDetected = (displayPanel.detect_i2c(PCF8574A_ADDR_OLED_BUTTON) == 0);
    bAddrA = true;
  }
  
  if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  {
    if(bAddrA)
      displayPanel.setKeyAddr(PCF8574A_ADDR_OLED_BUTTON, false);	// 'init' will be done with call of 'begin'

    // OLED (newly) found:
    displayPanel.begin();

#if defined DEBUG
    Serial.println(F("OLED-Panel found..."));
    Serial.print(F("..."));
    Serial.println(GetSwTitle());
    if(bAddrA)
      Serial.println(F("Buttons found on PCF8574A_ADDR_OLED_BUTTON..."));
    else
      Serial.println(F("Buttons found on PCF8574_ADDR_OLED_BUTTON..."));
#endif

    if(AlreadyCVInitialized())
      OutTextTitle();

    ui8_DisplayPanelMode = ui8_ButtonMirror = ui8_CursorX = 0;
		ui16_EditValue = 0;
    b_EditBinary = b_Edit = false;
  } // if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  ui8_DisplayPanelPresent = (b_DisplayPanelDetected ? 1 : 0);
}

void DisplayFirstLine()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.printc(0, GetSwTitle());
}

void DisplayLine(uint8_t iLine, const __FlashStringHelper *fText)
{
  displayPanel.clearLine(iLine);
  displayPanel.setCursor(0, iLine);  // set the cursor to column x, line y
  displayPanel.print(fText);
}

void DisplayClearLowerLines()
{
  displayPanel.clearLine(4);
  displayPanel.clearLine(5);
  displayPanel.clearLine(6);
  displayPanel.clearLine(7);
}

void SetDisplayPanelModeBetrieb()
{ // mode: 1
	DisplayFirstLine();
  DisplayLine(3, F("Betrieb?"));
  ui8_DisplayPanelMode = 1;
}

void SetDisplayPanelModeIBN()
{ // mode: 2
	DisplayFirstLine();
  DisplayLine(3, F("Inbetriebnahme?"));
  ui8_DisplayPanelMode = 2;
}

void SetDisplayPanelModeFastClock()
{ // mode: 7
	DisplayFirstLine();
  DisplayLine(3, F("FastClock"));
  ui8_DisplayPanelMode = 7;
  b_Edit = false;
}

void SetDisplayPanelModeCV()
{ // mode: 200
	DisplayClearLowerLines();
	displayPanel.print(14, 3, ' '); // clear '?'
  DisplayLine(4, F("CV?"));
  ui8_DisplayPanelMode = 200;
}

void SetDisplayPanelModeScan()
{ // mode: 210
	DisplayClearLowerLines();
	displayPanel.print(14, 3, ' '); // clear '?'
  DisplayLine(4, F("I2C-Scan?"));
  ui8_DisplayPanelMode = 210;
}

#if defined ETHERNET_BOARD
void SetDisplayPanelModeIpAdr()
{ // mode: 220
  DisplayLine(4, F("IP-Adresse"));
  DisplayLine(5, Ethernet.localIP());
  ui8_DisplayPanelMode = 220;
}

void SetDisplayPanelModeMacAdr()
{ // mode: 221
  DisplayLine(4, F("MAC-Adresse"));
  displayPanel.setCursor(0, 5);  // set the cursor to column x, line y
  uint8_t *mac(GetMACAddress());
  for(uint8_t i = 0; i < 6; i++)
  {
    hexout((uint8_t)(mac[i]), 2);
    if((i == 1) || (i == 3))
      displayPanel.print('.');
  }
  ui8_DisplayPanelMode = 221;
}
#endif

void InitScan()
{
   displayPanel.print(8, 4, ' '); // clear '?'	
   ui16_EditValue = -1;
   NextScan();
}

void NextScan()
{
   ++ui16_EditValue;
   displayPanel.setCursor(0, 5);  // set the cursor to column x, line y
   
   if(ui16_EditValue > 127)
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
       hexout((uint8_t)(ui16_EditValue), 2);
       displayPanel.print((error == 4) ? '?' : ' ');
       break;
     }

     ++ui16_EditValue;
     if(ui16_EditValue > 127)
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
	displayPanel.printOuterFrame();
	displayPanel.printc(2, GetSwTitle()); 
	displayPanel.printc(4, F("Version "));
  displayPanel.print(GetCV(VERSION_NUMBER), DEC);
#if not defined DEBUG
  #if not defined TELEGRAM_FROM_SERIAL
    displayPanel.printc(5, DIRECT_CLOCK_PULSE ? F("Takt") : F("DCC"));
  #endif
#endif
#if defined DEBUG
  displayPanel.printc(6, F("Debug"));
#endif
#if defined TELEGRAM_FROM_SERIAL
  displayPanel.printc(7, F("Simulation"));
#endif
  ui8_DisplayPanelMode = 0;
}

void displayValue3(uint8_t y, uint8_t iValue)
{
  displayPanel.setCursor(0, y);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  decout((uint8_t)(iValue), 3);
}

void displayValue4(uint8_t y, uint16_t iValue)
{
  displayPanel.setCursor(0, y);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  decout((uint8_t)(iValue), 4);
}

void OutTextClockStatus()
{
  displayPanel.print(7, 3, ' ');  // clear '?'
  DisplayClearLowerLines();
  displayPanel.setCursor(0, 4);  // set the cursor to column x, line y
  if(ENABLE_LN_FC_SLAVE)
  {
    // FastClockSlave:
    displayPanel.print(F("FC-Slave Status"));
  }
  else
  {
    // (FastClock)Master:
    displayPanel.print(F("Devider 10:"));
    displayPanel.print(GetDevider());

  }
  DisplayClockState();
}

void DisplayCV(uint16_t ui16_Value)
{
  displayPanel.noCursor();
	displayPanel.print(2, 4, ' ');  // clear '?'
  displayPanel.setCursor(3, 4);
  uint8_t ui8CvNr(ui8_DisplayPanelMode - 19);
  if(ui8CvNr < 10)
    displayPanel.print(' ');
  displayPanel.print(ui8CvNr, DEC);
  --ui8CvNr;

  // show shortname:
  displayPanel.clearToEOL(6, 4);
  displayPanel.setCursor(6, 4);
  displayPanel.print(GetCVName(ui8CvNr));

  displayPanel.clearLine(5);
  displayPanel.setCursor(0, 5);

  if(b_Edit)
    displayPanel.print('>');

  if (IsCVBinary(ui8CvNr))
    binout(ui16_Value);
  else
    displayPanel.print(ui16_Value, DEC);
  if(!b_Edit && !CanEditCV(ui8CvNr))
  {
    displayPanel.setCursor(14, 5);
    displayPanel.print(F("ro"));
  }
  if(b_EditBinary)
  {
    displayPanel.cursor(ui8_CursorX, 5);
  }
}

void DisplayClockState()
{
  displayPanel.setCursor(0, 5);  // set the cursor to column x, line y
  if(ENABLE_LN_FC_SLAVE)
  {
    // FastClockSlave:
    uint16_t ui16_iCount(0);
    uint8_t ui8_Rate(0);
    uint8_t ui8_Sync(0);
    if(!GetClockState(&ui16_iCount, &ui8_Rate, &ui8_Sync))
      displayPanel.print(F("---"));
    else
    {
      displayPanel.print(ui16_iCount);
      displayPanel.print(F("-1:"));
      displayPanel.print(ui8_Rate);
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
    // (FastClock)Master:
    if(IsClockRunning())
      displayPanel.print(F("Running: Stop?"));
    else
      displayPanel.print(F("Stopped: Run? "));
  }
}

void DisplayDate(uint8_t iValue)
{
  DisplayClearLowerLines();
  displayPanel.setCursor(0, 4);  // set the cursor to column x, line y
  if(ui8_DisplayPanelMode == 60)
    displayPanel.print(F("Stunde"));
  if(ui8_DisplayPanelMode == 61)
    displayPanel.print(F("Minute"));

  displayPanel.setCursor(0, 5);
  if(b_Edit)
    displayPanel.print('>');
  else
    iValue = GetFastClockTime(ui8_DisplayPanelMode - 60);
  displayPanel.print(iValue, DEC);
}

void HandleDisplayPanel()
{
  CheckAndInitDisplayPanel();

  displayPanel.refresh();
  
  uint8_t ui8_buttons(displayPanel.readButtons());  // reads all eight buttons

  if(!ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = 0;
    return;
  }

  if((ui8_buttons && (ui8_ButtonMirror != ui8_buttons)))
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
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetDisplayPanelModeIBN(); // mode = 2
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current clock state
        OutTextClockStatus();
        ui8_DisplayPanelMode = 10;
        b_MirrorStateOdd = !GetStateOdd();  // force to refresh display
        b_MirrorRunning = !IsClockRunning();
      }
      return;
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
        SetDisplayPanelModeCV();  // mode = 200
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
        DisplayDate((uint8_t)GetFastClockTime(0));
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
            IncMinute();
            b_MirrorStateOdd = !b_MirrorStateOdd; // force to display new state
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
      }
      return;
    } // if(ui8_DisplayPanelMode == 10)
    //------------------------------------
    if((ui8_DisplayPanelMode >= 20) && (ui8_DisplayPanelMode <= MAX_MODE))
    {
      // actual ui8_DisplayPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for current CV
        if(b_Edit)
        {
          // save current value
          boolean b_OK = CheckAndWriteCVtoEEPROM(ui8_DisplayPanelMode - 20, ui16_EditValue);
          DisplayCV(GetCV(ui8_DisplayPanelMode - 20));
          displayPanel.setCursor(10, 5);  // set the cursor to column x, line y
          displayPanel.print(b_OK ? F("stored") : F("failed"));
          displayPanel.setCursor(ui8_CursorX, 1);
        }
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        if(b_Edit)
        {
          // leave edit-mode (without save)
          b_Edit = b_EditBinary = false;
          DisplayCV(GetCV(ui8_DisplayPanelMode - 20));
          return;
        }
        SetDisplayPanelModeCV();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if(!b_Edit && CanEditCV(ui8_DisplayPanelMode - 20))
        {
          // enter edit-mode
          ui16_EditValue = GetCV(ui8_DisplayPanelMode - 20);
          b_Edit = true;
          b_EditBinary = IsCVBinary(ui8_DisplayPanelMode - 20);
          ui8_CursorX = 1;
          DisplayCV(ui16_EditValue);
        }
        else if(b_EditBinary)
        { // move cursor only in one direction
          ++ui8_CursorX;
          if(ui8_CursorX == 9)
            ui8_CursorX = 1;  // rollover
          displayPanel.cursor(ui8_CursorX, 5);
        }
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to previous CV
        if(b_Edit)
        { // Wert verkleinern:
          if(b_EditBinary)
            ui16_EditValue &= ~(1 << (8 - ui8_CursorX));
          else
            --ui16_EditValue;
          if(ui16_EditValue == UINT8_MAX) // 0 -> 255
            ui16_EditValue = GetCVMaxValue(ui8_DisplayPanelMode - 20);  // Unterlauf
          DisplayCV(ui16_EditValue);
          return;
        } // if(b_Edit)
        --ui8_DisplayPanelMode;
        if(ui8_DisplayPanelMode < 20)
          ui8_DisplayPanelMode = 20;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20));
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to next CV
        if(b_Edit)
        { // Wert vergrößern:
          if(b_EditBinary)
            ui16_EditValue |= (1 << (8 - ui8_CursorX));
          else
            ++ui16_EditValue;
          if(ui16_EditValue > GetCVMaxValue(ui8_DisplayPanelMode - 20))
            ui16_EditValue = 0;  // Überlauf
          DisplayCV(ui16_EditValue);
          return;
        }
        ++ui8_DisplayPanelMode;
        if(ui8_DisplayPanelMode >= MAX_MODE)
          ui8_DisplayPanelMode = MAX_MODE;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20));
      }
			return;
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
          DisplayDate((uint8_t)GetFastClockTime(ui8_DisplayPanelMode - 60));
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
          DisplayDate((uint8_t)ui16_EditValue);
        }
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      { // switch to previous CV
        if(b_Edit)
        {
          --ui16_EditValue;
          if(ui16_EditValue == UINT8_MAX)
            ui16_EditValue = GetMaxForDateTime() - 1;
          DisplayDate((uint8_t)ui16_EditValue);
          return;
        }
        --ui8_DisplayPanelMode;
        if(ui8_DisplayPanelMode < 60)
          ui8_DisplayPanelMode = 60;
        DisplayDate((uint8_t)GetFastClockTime(ui8_DisplayPanelMode - 60));
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to next CV
        if(b_Edit)
        {
          ++ui16_EditValue;
          if(ui16_EditValue >= GetMaxForDateTime())
            ui16_EditValue = 0;
          DisplayDate((uint8_t)ui16_EditValue);
          return;
        }
        ++ui8_DisplayPanelMode;
        if(ui8_DisplayPanelMode > 61)
          ui8_DisplayPanelMode = 61;
        DisplayDate((uint8_t)GetFastClockTime(ui8_DisplayPanelMode - 60));
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
      }
      else if (ui8_buttons & BUTTON_DOWN)
      {
        NextScan();
      }
      return;
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
    //------------------------------------
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
    displayPanel.setCursor(0, 4);  // set the cursor to column x, line y
    uint8_t ui8_Hour;
    uint8_t ui8_Minute;
    if(GetFastClock(&ui8_Hour, &ui8_Minute))
    {
      if (ui8_Hour < 23)
        decout((uint8_t)ui8_Hour, 2);
      else
        displayPanel.print("??");
      displayPanel.print(':');
      if (ui8_Minute < 60)
        decout((uint8_t)ui8_Minute, 2);
      else
        displayPanel.print("??");
    }
    else
      displayPanel.print(F("--:--"));
  } // if(ui8_DisplayPanelMode == 7) 
  //========================================================================
  if(ui8_DisplayPanelMode == 10)
  {
		uint8_t ui8_Devider(GetDevider());
		if (ui8_Devider != ui8_DeviderMirror)
		{
			if (ENABLE_LN_FC_SLAVE)
				displayPanel.setCursor(7, 5);  // set the cursor to column x, line y
			else
				displayPanel.setCursor(11, 4);  // set the cursor to column x, line y
			displayPanel.print(ui8_Devider);
			ui8_DeviderMirror = ui8_Devider;
		}

		boolean b_StateOdd(GetStateOdd());
		if (b_StateOdd != b_MirrorStateOdd)
		{
			b_MirrorStateOdd = b_StateOdd;
			if (ENABLE_LN_FC_SLAVE)
			{
				displayPanel.setCursor(12, 5);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? F("Odd ") : F("Even"));
			}
			else
			{
				displayPanel.setCursor(15, 4);  // set the cursor to column x, line y
				displayPanel.print(b_StateOdd ? 'O' : 'E');
			}
		}

		if (!ENABLE_LN_FC_SLAVE)
		{
			boolean b_Running(IsClockRunning());
			if (b_Running != b_MirrorRunning)
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
  if(ui8_DisplayPanelMode == 60)
    return 24;
  return 60;
}

#endif // defined OLED
