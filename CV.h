//=== CV-Definitions for Uhrenzentrale ===

// give CV a unique name
enum {  ID_DEVICE = 0, ID_DEVIDER, ID_WAITTIME, ID_RESERVE_2, ID_RESERVE_3, ID_RESERVE_4, VERSION_NUMBER, SOFTWARE_ID, ADD_FUNCTIONS_1
      , ADD_FUNCTIONS_2, LN_ADR_ONOFF
#if defined ETHERNET_BOARD
      , IP_BLOCK_3
      , IP_BLOCK_4
#endif
};

//=== declaration of var's =======================================
#define PRODUCT_ID SOFTWARE_ID
static const uint8_t DEVICE_ID = 1;							// CV1: Device-ID
static const uint8_t SW_VERSION = 8;						// CV7: Software-Version
static const uint8_t CLOCKCOMMANDSTATION = 11;  // CV8: Software-ID

static const uint16_t MAX_LN_ADR = 2048;

#if defined ETHERNET_BOARD
  static const uint8_t MAX_CV = 13;
#else
  static const uint8_t MAX_CV = 11;
#endif

uint16_t ui16a_CV[MAX_CV] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX
														, UINT16_MAX
#if defined ETHERNET_BOARD
                            , UINT16_MAX, UINT16_MAX
#endif
                            };  // ui16a_CV is a copy from eeprom

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
{ // ID               default value        minValue							maxValue							type     r/o
   { ID_DEVICE,       DEVICE_ID,           1,										126,									CV_TYPE::UI8,     false}  // normally r/o
  ,{ ID_DEVIDER,      30,                  10,									99,										CV_TYPE::UI8,     false}  // Devider
  ,{ ID_WAITTIME,     3,                   0,										9,										CV_TYPE::UI8,     false}  // Wait time before automatic switching to controlmode
  ,{ ID_RESERVE_2,    0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
  ,{ ID_RESERVE_3,    0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
  ,{ ID_RESERVE_4,    0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
  ,{ VERSION_NUMBER,  SW_VERSION,          0,										UINT8_MAX,						CV_TYPE::UI8,     false}  // normally r/o
  ,{ SOFTWARE_ID,     CLOCKCOMMANDSTATION, CLOCKCOMMANDSTATION, CLOCKCOMMANDSTATION,	CV_TYPE::UI8,     true}   // always r/o
  ,{ ADD_FUNCTIONS_1, 0x00,                0,										UINT8_MAX,						CV_TYPE::BINARY,  false}  // additional functions 1 (FastClock-Slave)
  ,{ ADD_FUNCTIONS_2, 0x10,                0,										UINT8_MAX,						CV_TYPE::BINARY,  false}  // additional functions 2
  ,{ LN_ADR_ONOFF,		671,                 0,                   MAX_LN_ADR,           CV_TYPE::UI16,    false}  // special handling for max value requested
#if defined ETHERNET_BOARD
  ,{ IP_BLOCK_3,      2,                   0,										UINT8_MAX,						false,     false}  // IP-Address part 3
  ,{ IP_BLOCK_4,      106,                 0,										UINT8_MAX,						false,     false}  // IP-Address part 4
#endif
};

//=== naming ==================================================
const __FlashStringHelper* GetSwTitle() { return F("Uhrenzentrale"); }
//========================================================
const __FlashStringHelper *GetCVName(uint8_t ui8_Index)
{ 
  // each string should have max. 10 chars
  const __FlashStringHelper *cvName[MAX_CV] = { F("DeviceID"),
                                                F("Devid.10:n"),
                                                F("WaitTime"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Version"),
                                                F("SW-ID"),
                                                F("Cfg FC-Slv"),
                                                F("Config 1"),
                                                F("Adr LN-0/1")
#if defined ETHERNET_BOARD
                                              , F("IP-Part 3")
                                              , F("IP-Part 4")
#endif
                                                };
                                    
  if(ui8_Index < MAX_CV)
    return cvName[ui8_Index];
  return F("???");
}

//=== functions ==================================================
boolean AlreadyCVInitialized() { return (ui16a_CV[SOFTWARE_ID] == CLOCKCOMMANDSTATION); }
