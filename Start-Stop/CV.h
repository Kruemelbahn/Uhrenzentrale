//=== CV-Definitions for Uhrenzentrale_StartStop ===

// give CV a unique name
enum { ID_DEVICE = 0, ID_RESERVE_1, LN_ADR_ONOFF, ID_CLOCKCMD_ID, ID_CV_DEVIDER, ID_RESERVE_4, VERSION_NUMBER, SOFTWARE_ID };

//========================================================

//=== declaration of var's =======================================
#define PRODUCT_ID SOFTWARE_ID
static const uint8_t DEVICE_ID = 1;							  // CV1: Device-ID
static const uint8_t SW_VERSION = 2;						  // CV7: Software-Version
static const uint8_t CLOCKCOMMANDSTARTSTOP = 15;  // CV8: Software-ID

static const uint16_t MAX_LN_ADR = 2048;

static const uint8_t MAX_CV = 8;

uint16_t ui16a_CV[MAX_CV] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX };  // ui16a_CV is a copy from eeprom

struct _CV_DEF // uses 9 Byte of RAM for each CV
{
  uint8_t ID;
  uint16_t DEFLT;
  uint16_t MIN;
  uint16_t MAX;
  uint8_t TYPE;
  boolean RO;
};

enum CV_TYPE { UI8 = 0, UI16 = 1, BINARY = 2 };

const struct _CV_DEF cvDefinition[MAX_CV] =
{ // ID               default value          minValue							  maxValue							type     					r/o
   { ID_DEVICE,       DEVICE_ID,             1,										  126,									CV_TYPE::UI8,     false}  // normally r/o
  ,{ ID_RESERVE_1,		0,                     0,                     0,                    CV_TYPE::UI8,     true}  // special handling for max value requested
  ,{ LN_ADR_ONOFF,		671,                   0,                     MAX_LN_ADR,           CV_TYPE::UI8,     false}  // special handling for max value requested
  ,{ ID_CLOCKCMD_ID,  11,                    11,								    11,						        CV_TYPE::UI8,     true}   // Software-ID from ClockCommander r/o
  ,{ ID_CV_DEVIDER,   2,                     2,									    2,						        CV_TYPE::UI8,     true}   // CV for Devider used by ClockCommander r/o
  ,{ ID_RESERVE_4,    0,                     0,										  0,										CV_TYPE::UI8,     true}   // (not used)
  ,{ VERSION_NUMBER,  SW_VERSION,            0,										  UINT8_MAX,						CV_TYPE::UI8,     false}  // normally r/o
  ,{ SOFTWARE_ID,     CLOCKCOMMANDSTARTSTOP, CLOCKCOMMANDSTARTSTOP, CLOCKCOMMANDSTARTSTOP,CV_TYPE::UI8,     true}   // always r/o
};

//=== naming ==================================================
const __FlashStringHelper* GetSwTitle() { return F("UZ-Start/Stop"); }
//========================================================
const __FlashStringHelper* GetCVName(uint8_t ui8_Index)
{
  // each string should have max. 10 chars
  const __FlashStringHelper* cvName[MAX_CV] = { F("DeviceID"),
                                                F("Reserve"),
                                                F("Adr LN-0/1"),
                                                F("CLK-CMD-ID"),
                                                F("DeviderCV"),
                                                F("Reserve"),
                                                F("Version"),
                                                F("SW-ID")
  };

  if (ui8_Index < MAX_CV)
    return cvName[ui8_Index];
  return F("???");
}

//=== functions ==================================================
boolean AlreadyCVInitialized() { return (ui16a_CV[SOFTWARE_ID] == CLOCKCOMMANDSTARTSTOP); }
