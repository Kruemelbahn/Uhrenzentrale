//=== Routines for Clock-Controlling [DCC-Signal], adapted for Uhrenzentrale ===
/*
 * ISR-Routine and handling are taken from "SimpleDCC_without_Lib.ino"
 * containing in: http://sourceforge.net/projects/pgahtow/files/Arduino%20%28v1.0%29%20libaries/DCCInterfaceMaster_Railcom.zip
 * from webpage of Philipp Gahtow: http://pgahtow.de/wiki/index.php?title=DCC#Download
 * 
 * details for DCC-format can be found in NMRA S9.2 and RP9.2.1
 *
 * DCC-Format for clock-telegram as follows:
 * 
 * used chapter      : C. Instruction Packets for Multi Function Digital Decoders
 * with              : {preamble} 0 [ AAAAAAAA ] 0 {instruction-bytes} 0 EEEEEEEE 1 
 * AAAAAAAA          : 11xxxxxx x xxxxxxxx = containing address 250 ==> 11000000 0 11111010
 * instruction-bytes : CCCDDDDD
 * CCCDDDDD          : Function Group One Instruction (100) ==> 1000000z with z as the state of F1 
 * 
 * in complete       : {preamble} 0 11000000 0 11111010 0 1000000z 0 EEEEEEEE 1 
 * length            : 4, including errorbyte
 */
 
//=== declaration of var's =======================================
// definitions for Ports
//    because they are inverted to each other: handle them together!
//    (they are connected directly to the powersection and further to the 4-way distributor
//     for dircekt clock-connection [also using RJ12])
#define DCC1 PC0
#define DCC2 PC1

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 0x8D  // 58usec pulse length 
#define TIMER_LONG  0x1B  // 116usec pulse length 

uint8_t last_timer = TIMER_SHORT;  // store last timer value
   
boolean bShortPulse = false;  // used for short or long pulse
boolean every_second_isr = false;  // pulse up or down

// definitions for state machine 
#define PREAMBLE 0    
#define SEPERATOR 1
#define SENDBYTE  2

uint8_t state = PREAMBLE;
uint8_t preamble_count = 16;
uint8_t outbyte = 0;
uint8_t cbit = 0x80;

// buffer for command
struct Message {
   uint8_t data[7];
   uint8_t len;
   boolean active;
} ;

#define MAXMSG  2 // currently use only two messages - the idle msg and the clock msg

struct Message msg[MAXMSG] =
{ 
//    data[7]                        len  active
  { { 0xFF,    0, 0xFF, 0, 0, 0, 0}, 3,   true},    // idle msg
  { { 0xC0, 0xFA,    0, 0, 0, 0, 0}, 4,   false}    // clock msg with functions
}; // clock msg is filled later with in assemble_dcc_msg() with function (F1) together with XOR data as errorbyte
                                
int msgIndex = 0;  
int byteIndex = 0;

//=== ISR functions =============================================
//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.
void SetupTimer2()
{
  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timer clock = 16MHz/8 = 2MHz oder 0,5usec
  TCCR2A = 0;
  TCCR2B = 0<<CS22 | 1<<CS21 | 0<<CS20; 

  //Timer2 Overflow Interrupt Enable   
  TIMSK2 = 1<<TOIE2;

  //load the timer for its first cycle
  TCNT2 = TIMER_SHORT; 
}

//Timer2 overflow interrupt vector handler
ISR(TIMER2_OVF_vect)
{
	if (IsDCCout())
	{
		//Capture the current timer value TCTN2. This is how much error we have
		//due to interrupt latency and the work in this function
		//Reload the timer and correct for latency.  
		// for more info, see http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/

		unsigned char latency;

		// for every second interupt just toggle signal
		// DCC ausschalten:
		PORTC &= ~((1 << DCC1) | (1 << DCC2));
		if (every_second_isr)
		{
			// DCC1 einschalten:
			PORTC |= (1 << DCC1);
			every_second_isr = false;

			// set timer to last value
			latency = TCNT2;
			TCNT2 = latency + last_timer;

		}
		else
		{  // != every second interrupt, advance bit or state
			 // DCC2 einschalten:
			PORTC |= (1 << DCC2);
			every_second_isr = true;

			switch (state)
			{
  			case PREAMBLE:
  				bShortPulse = true; // short pulse
  				preamble_count--;
  				if (preamble_count == 0)
  				{  // advance to next state
  					state = SEPERATOR;
  					// get next message
  					msgIndex++;
  					while ((msgIndex < MAXMSG) && !msg[msgIndex].active)
  						msgIndex++;
  					if (msgIndex >= MAXMSG)
  						msgIndex = 0;
  					byteIndex = 0; //start msg with byte 0
  				}
  				break;
  
  			case SEPERATOR:
  				bShortPulse = false; // long pulse
  				// then advance to next state
  				state = SENDBYTE;
  				// goto next byte ...
  				cbit = 0x80;  // send this bit next time first         
  				outbyte = msg[msgIndex].data[byteIndex];
  				break;
  			
  			case SENDBYTE:
  				if (outbyte & cbit)
  					bShortPulse = true;  // send short pulse
  				else
  					bShortPulse = false;  // send long pulse
  				cbit = cbit >> 1;
  				if (cbit == 0)
  				{  // last bit sent, is there a next byte?
  					byteIndex++;
  					if (byteIndex >= msg[msgIndex].len)
  					{
  						// this was already the XOR byte then advance to preamble
  						state = PREAMBLE;
  						preamble_count = 16;
  					}
  					else
  					{
  						// send separtor and advance to next byte
  						state = SEPERATOR;
  					}
  				}
  				break;
			} // switch (state)

			if (bShortPulse)
			{  // if data==1 then short pulse
				latency = TCNT2;
				TCNT2 = latency + TIMER_SHORT;
				last_timer = TIMER_SHORT;
			}
			else
			{  // long pulse
				latency = TCNT2;
				TCNT2 = latency + TIMER_LONG;
				last_timer = TIMER_LONG;
			} // if (bShortPulse)
		} // else if (every_second_isr)
	} // if(IsDCCout())
}

//=== functions ==================================================
void InitClockDCC()
{
  // set to output directly:
  DDRC |= (1 << DCC1) | (1 << DCC2);

  // Start the timer 2 
  SetupTimer2();

#if defined DEBUG
	if(IsDCCout())
		Serial.println(F("...send DCC as output-signals"));
	else
		Serial.println(F("...send direct output-signal"));
#endif
}

void HandleClockDCC()
{
	if (IsDCCout())
	{
		uint8_t functionData(0x80);
		if (GetStateOdd())
			++functionData;

		noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
		msg[1].active = IsClockRunning();
		msg[1].data[2] = functionData;
    // add XOR byte 
		msg[1].data[3] = msg[1].data[0] ^ msg[1].data[1] ^ msg[1].data[2];
		interrupts();
	}
}
