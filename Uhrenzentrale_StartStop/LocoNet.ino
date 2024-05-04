//=== LocoNet for Uhrenzentrale_StartStop ===
#include <LocoNet.h>

//=== declaration of var's =======================================
LocoNetFastClockClass  FastClock;

static  lnMsg          *LnPacket;
static  LnBuf          LnTxBuffer;

unsigned long ul_LastFastClockTick = 0;

/*
 *  https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf
 *  https://wiki.rocrail.net/doku.php?id=loconet:ln-pe-de
 *  http://embeddedloconet.sourceforge.net/SV_Programming_Messages_v13_PE.pdf
 *  remark: this software didnot follow '2.2.6) Standard SV/EEPROM Locations'
 */
// default OPC's are defined in 'utility\ln_opc.h'
// A3 is already used  and defined as OPC_LOCO_F912
// A8 is already used by FRED  and defined as OPC_DEBUG
// A8 is already used by FREDI and defined as OPC_FRED_BUTTON
//#define OPC_FRED_BUTTON     0xA8
// AF is already used by FREDI and defined as OPC_SELFTEST resp. as OPC_FRED_ADC
// ED is already used  and defined as OPC_IMM_PACKET
// EE is already used  and defined as OPC_IMM_PACKET_2
#define SV2_Format_1	0x01
#define SV2_Format_2	0x02

#define PIN_TX   7
#define PIN_RX   8

uint8_t ui8_WaitForTelegram = 0;
unsigned long ul_WaitStartForTelegram = 0;
uint8_t ui8_TrackState = 0;

//=== functions for receiving telegrams from SerialMonitor =======
#if defined TELEGRAM_FROM_SERIAL
static const int MAX_LEN_LNBUF = 64;
uint8_t ui8_PointToBuffer = 0;

uint8_t ui8a_receiveBuffer[MAX_LEN_LNBUF];
uint8_t ui8a_resultBuffer[MAX_LEN_LNBUF];

void ClearReceiveBuffer()
{
  ui8_PointToBuffer = 0;
  for(uint8_t i = 0; i < MAX_LEN_LNBUF; i++)
    ui8a_receiveBuffer[i] = 0;
}

uint8_t Adjust(uint8_t ui8_In)
{
  uint8_t i = ui8_In;
  if((i >= 48) && (i <= 57))
    i -= 48;
  else
    if((i >= 65) && (i <= 70))
      i -= 55;
    else
      if((i >= 97) && (i <= 102))
        i -= 87;
      else
        i = 0;
  return i;
}

uint8_t AnalyzeBuffer()
{
  uint8_t i = 0;
  uint8_t ui8_resBufSize = 0;
  while(uint8_t j = ui8a_receiveBuffer[i++])
  {
    if(j != ' ')
    {
      j = Adjust(j);
      uint8_t k = ui8a_receiveBuffer[i++];
      if(k != ' ')
        ui8a_resultBuffer[ui8_resBufSize++] = j * 16 + Adjust(k);
      else
        ui8a_resultBuffer[ui8_resBufSize++] = j;
    }
  }

  // add checksum:
  uint8_t ui8_checkSum(0xFF);
  for(i = 0; i < ui8_resBufSize; i++)
    ui8_checkSum ^= ui8a_resultBuffer[i]; 
  bitWrite(ui8_checkSum, 7, 0);     // set MSB zero
  ui8a_resultBuffer[ui8_resBufSize++] = ui8_checkSum;

  return ui8_resBufSize;
}

#endif

//=== functions ==================================================
void InitLocoNet()
{
  // First initialize the LocoNet interface, specifying the TX Pin
  LocoNet.init(PIN_TX);

#if defined TELEGRAM_FROM_SERIAL
  ClearReceiveBuffer();
#endif

  LocoNet.reportPower(1);
}

void HandleLocoNetMessages()
{
#if defined TELEGRAM_FROM_SERIAL
  LnPacket = NULL;
  if (Serial.available())
  {
    byte byte_Read = Serial.read();
    if(byte_Read == '\n')
    {
      if(AnalyzeBuffer() > 0)
        LnPacket = (lnMsg*) (&ui8a_resultBuffer);
      ClearReceiveBuffer();
    }
    else
    {
      if(ui8_PointToBuffer < MAX_LEN_LNBUF)
        ui8a_receiveBuffer[ui8_PointToBuffer++] = byte_Read;
    }
  }
#else
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive();
#endif
  if(LnPacket)
  {
    LocoNet.processSwitchSensorMessage(LnPacket);

    uint8_t ui8_msgLen = getLnMsgSize(LnPacket);
    uint8_t ui8_msgOPCODE = LnPacket->data[0];
		//====================================================
		if (ui8_msgLen == 16)
		{
      if (ui8_msgOPCODE == OPC_PEER_XFER) // E5
      {
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
        Printout('R');
#endif
        HandleE5MessageFormat2();
        HandleE5MessageFormat2FromClockCommander();
      }
		} // if(ui8_msgLen == 16)
		//====================================================
	} // if(LnPacket)
}  

void HandleE5MessageFormat2FromClockCommander()
{
  if (LnPacket->data[4] == SV2_Format_2)  // telegram with Message-Format '2'
  {
    if ((LnPacket->data[6] == GetCV(ID_DEVICE)) &&	    // 7-bit
        (LnPacket->data[7] == GetSWIDClockCommander())) // 7-bit : telegram for us
    {
      // SV_ADRL und SV_ADRH
      uint8_t ui8_LSBAdr(((LnPacket->data[5] & 0x04) << 5) + (LnPacket->data[8] & 0x7F));
      uint8_t ui8_MSBAdr(((LnPacket->data[5] & 0x08) << 4) + (LnPacket->data[9] & 0x7F));
      uint16_t ui16_Address((ui8_MSBAdr << 8) + ui8_LSBAdr);

      if (LnPacket->data[3] == 0x41)  // reply to SV write: 1 byte of data from D1 was written
      {
        if (ui16_Address == (uint16_t)(GetCVinClockCommander()))
        { // check D1:
          uint8_t ui8_D1(((LnPacket->data[10] & 0x01) << 7) + (LnPacket->data[11] & 0x7F));
          SetE5StateReceived(ui8_D1 == GetDevider());
        }
      } // if (LnPacket->data[3] == 0x41)
    } // if (TelegramE5ForUs())
  } // if(LnPacket->data[4] == SV2_Format_2)
}

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
void Printout(char ch)
{
  if(LnPacket)
  {
    // print out the packet in HEX
    Serial.print(ch);
    Serial.print(F("X: "));
    uint8_t ui8_msgLen = getLnMsgSize(LnPacket); 
    for (uint8_t i = 0; i < ui8_msgLen; i++)
    {
      uint8_t ui8_val = LnPacket->data[i];
      // Print a leading 0 if less than 16 to make 2 HEX digits
      if(ui8_val < 16)
        Serial.print('0');
      Serial.print(ui8_val, HEX);
      Serial.print(' ');
    }
    Serial.println();
  } // if(LnPacket)
}
void PrintAdr(uint16_t ui16_AdrFromTelegram)
{
  Serial.print(F("Adr received from LN "));
  Serial.println(ui16_AdrFromTelegram);
}
void PrintOutput(boolean bOutput)
{
  Serial.print(F("Output "));
  Serial.println(bOutput ? 1 : 0);
}
void PrintDirection(boolean bDirection)
{
  Serial.print(F("Direction "));
  Serial.println(bDirection ? 1 : 0);
}
#endif
