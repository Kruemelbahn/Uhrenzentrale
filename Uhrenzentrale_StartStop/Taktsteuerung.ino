//=== Taktsteuerung for Uhrenzentrale_StartStop ===
#include <Bounce.h>
#include <LocoNetKS.h>

//=== declaration of var's =======================================
#if defined LN_SHIELD
  #define CLOCK_LEDon      14
  #define CLOCK_LEDoff     15
  #define CLOCK_ONButton   16
  #define CLOCK_OFFButton  17
#else
  #define CLOCK_LEDon       3
  #define CLOCK_LEDoff     10
  #define CLOCK_ONButton    4
  #define CLOCK_OFFButton   5
#endif
#define DeviderDIP1   13 
#define DeviderDIP2   12 
#define DeviderDIP3   11 

const uint8_t ui8_ArrayDevider[8] = { 0, 10, 15, 20, 25, 30, 35, 40 };

uint8_t GetDevider()
{
  uint8_t ui8_Index =  ((digitalRead(DeviderDIP1) == 0)
                      | (digitalRead(DeviderDIP2) == 0) << 1
                      | (digitalRead(DeviderDIP3) == 0) << 2);
  return ui8_ArrayDevider[ui8_Index];
}

// instanciate Bounce-Object for 20ms
Bounce bouncerOFF = Bounce(CLOCK_OFFButton, 20);
Bounce bouncerON  = Bounce(CLOCK_ONButton, 20);

uint8_t ui8_LEDStatusOFF = 0;
uint8_t ui8_LEDStatusON = 0;

uint8_t ui8_SendE5State = 0;

static const int MAX_LEN_LNBUF = 64;
uint8_t ui8a_sendBuffer[MAX_LEN_LNBUF];

unsigned long ul_waitTime = 0;
uint8_t ui8FlashCount = 0;

boolean b_Error = false;

//=== functions ==================================================
uint8_t GetSWIDClockCommander() { return GetCV(ID_CLOCKCMD_ID); }    // CV8: Software-ID für Clock-Command-Station!
uint8_t GetCVinClockCommander() { return GetCV(ID_CV_DEVIDER); }

void SetE5StateReceived(boolean bStateOK)
{
  if (ui8_SendE5State == 5)
    ui8_SendE5State = (bStateOK ? 6 : 7);
}

void InitStartStop()
{
  pinMode(CLOCK_LEDon, OUTPUT);            // PD3 - Pin5   - lights green
  pinMode(CLOCK_LEDoff, OUTPUT);           // PB2 - Pin16  - lights red
  pinMode(CLOCK_ONButton, INPUT_PULLUP);   // PD4 - Pin6
  pinMode(CLOCK_OFFButton, INPUT_PULLUP);  // PD5 - Pin11

  pinMode(DeviderDIP1, INPUT_PULLUP);
  pinMode(DeviderDIP2, INPUT_PULLUP);
  pinMode(DeviderDIP3, INPUT_PULLUP);

  ui8_LEDStatusOFF = 0;
  ui8_LEDStatusON = 0;
  ui8_SendE5State = 0;
  ul_waitTime = 0;
  ui8FlashCount = 0;
  b_Error = false;
}

void HandleControls()
{
  uint16_t ui16_Adr(GetCV(LN_ADR_ONOFF));
  
  // check buttons // (Start/Stop)
  bouncerON.update();
  if(ui16_Adr && bouncerON.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
    LocoNetKS.sendSwitchState(ui16_Adr, true, true, OPC_SW_REQ);   // Telegramm B0

  bouncerOFF.update();
  if(ui16_Adr && bouncerOFF.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
    LocoNetKS.sendSwitchState(ui16_Adr, true, false, OPC_SW_REQ);   // Telegramm B0

  // Handle buttons and state for sending devider
  if (GetDevider())
  {
    switch (ui8_SendE5State)
    {
      case 0: // ON unbetätigt, warten auf OFF betätigt
        if (digitalRead(CLOCK_ONButton) && bouncerOFF.fallingEdge())
          ui8_SendE5State = 1;
        break;
      case 1: // OFF betätigt, warten auf ON betätigt
        if (digitalRead(CLOCK_OFFButton))
          ui8_SendE5State = 0;  // OFF unbetätigt
        else
        {
          if (bouncerON.fallingEdge())
            ui8_SendE5State = 2;
        }
        break;
      case 2: // ON und OFF betätigt, warten auf OFF unbetätigt
        if (digitalRead(CLOCK_ONButton))
          ui8_SendE5State = 0; // ON unbetätigt
        else
        {
          if (bouncerOFF.risingEdge())
            ui8_SendE5State = 3;
        }
        break;
      case 3: // OFF unbetätigt und ON betätigt, warten auf ON unbetätigt
        if (!digitalRead(CLOCK_OFFButton))
          ui8_SendE5State = 0; // OFF betätigt 
        else
        {
          if (bouncerON.risingEdge())
            ui8_SendE5State = 4;
        }
        break;
      case 4: // Bedienfolge eingehalten: Devider senden
        LnPacket = (lnMsg*)(&ui8a_sendBuffer);
        if (LnPacket)
        {
          memset(ui8a_sendBuffer, 0x00, MAX_LEN_LNBUF);
          LnPacket->data[2] = GetCV(ID_DEVICE);
          LnPacket->data[3] = 0x01;                     // write (=send) 1 byte
          LnPacket->data[6] = GetCV(ID_DEVICE);         // Sender und Empfänger müssen übeeinstimmen!
          LnPacket->data[7] = GetSWIDClockCommander();  // SOFTWARE_ID des Empfängers

          sendE5Telegram(GetCVinClockCommander(), 0, GetDevider(), 0, 0, 0);
          ul_waitTime = millis();
          ui8_SendE5State = 5;
        }
        else
          ui8_SendE5State = 0;
        break;
      case 5: // wait for Response:
        if ((millis() - ul_waitTime) > 2000) // wir warten maximal 2s
          ui8_SendE5State = 7;
        break;
      case 6: // Response OK
        ui8FlashCount = 5;
        ul_waitTime = millis();
        ui8_SendE5State = 0;
        b_Error = false;
        break;
      case 7: // Response failed
        ui8FlashCount = 5;
        ul_waitTime = millis();
        ui8_SendE5State = 0;
        b_Error = true;
        break;
    } //switch (ui8_SendE5State)
  } // if (GetDevider() && GetCVinClockCommander())

  // handle LEDs
  if (ui8FlashCount > 0)
  {
    if ((millis() - ul_waitTime) > 1000) // wir warten maximal 1s
    {
      --ui8FlashCount;
      ul_waitTime = millis();
    }
    digitalWrite(CLOCK_LEDon, Blinken05Hz());
    digitalWrite(CLOCK_LEDoff, b_Error? !Blinken05Hz() : Blinken05Hz());
  }
  else
  {
    b_Error = false;
    digitalWrite(CLOCK_LEDon, ui8_LEDStatusON ? true : false);
    digitalWrite(CLOCK_LEDoff, ui8_LEDStatusOFF ? true : false);
  }
}

void handleReceivedB1(uint16_t Address, uint8_t Output, uint8_t Direction)
{
  UNREFERENCED_PARAMETER(Direction);

  uint16_t ui16_Adr(GetCV(LN_ADR_ONOFF));
  if (ui16_Adr && (Address == ui16_Adr))
  {
    ui8_LEDStatusON = Output;
    ui8_LEDStatusOFF = !Output;
  }
}
