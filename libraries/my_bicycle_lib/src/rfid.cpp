#include "rfid.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include "reg_status.h"

#define PIN_CP  7
#define PIN_CLK 8
#define PIN_RX  9

#define DISABLE_CP_INT()    detachInterrupt(digitalPinToInterrupt(PIN_CP))
#define ENABLE_CP_INT()     attachInterrupt(digitalPinToInterrupt(PIN_CP), isr_cp, CHANGE)
#define DISABLE_CLOCK_INT() detachInterrupt(digitalPinToInterrupt(PIN_CLK)) 
#define ENABLE_CLOCK_INT()  attachInterrupt(digitalPinToInterrupt(PIN_CLK), isr_clock, FALLING)

//HINT: 1 char has 4 bits + 1 bit parity
//Description of sequence
//2   chars leading zeros
//1   char start charater (b11010)
//14  chars data
//1   char end character  (b11111)
//1   char LRC
//2   chars ending zeros
#define BUFFER_SIZE 21
volatile uint8_t rxBuffer[BUFFER_SIZE];

//sampling
static volatile uint8_t dataBit;
static volatile uint8_t dataByte;

static volatile bool has_rfid_rx;

bool has_rfid_data_available(){
  return (REG_STATUS & (1<<HAS_RFID_RX)) > 0;
}

//clock interrupt
void isr_clock (void)
{
  static uint8_t rxBit;

  //fetch rx bit
  rxBit = digitalRead(PIN_RX);

  if(dataByte < BUFFER_SIZE) {
    if(rxBit == 0) {
      rxBuffer[dataByte] |= (1<<dataBit);
    }
    else {
      rxBuffer[dataByte] &= ~(1<<dataBit);
    }

    if(++dataBit > 4) {
      dataBit = 0;
      ++dataByte;
    }
  }
}

//card present interrupt
void isr_cp (void)
{
  if(digitalRead(PIN_CP) == HIGH) {
    //rising edge
    DISABLE_CLOCK_INT();
    REG_STATUS |= (1<<HAS_RFID_RX);
    DISABLE_CP_INT();
  }
  else {
    //falling edge
    ENABLE_CLOCK_INT();
  }
}





static void rfid_reset(void)
{
  //reset state machine
  dataBit = 0;
  dataByte = 0;

  //enable card present interrupt (disables itself)
  ENABLE_CP_INT();
}


void rfid_init(void)
{
  has_rfid_rx = false;
  //define ports as input
  pinMode(PIN_CP, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  
  pinMode(PIN_RX, INPUT_PULLUP);
  
  ENABLE_CP_INT();
  
  //DDRC &= ~(1<<PIN_RX);
  //DDRD &= ~((1<<PIN_CP) | (1<<PIN_CLK));

  //enable pullups
  //PORTC |= (1<<PIN_RX);
  //PORTD |= ((1<<PIN_CP) | (1<<PIN_CLK));

  

  //Any logical change on INT0 generates an interrupt request.
  //The falling edge of INT1 generates an interrupt request.
  //EICRA |= (1<<ISC00) | (1<<ISC11);

  //power pin = output pin
  //DDRC |= (1<<PC3);
}


void rfid_start(void)
{
  rfid_reset();
  //power to the people
  //PORTC |= (1<<PC3);
}


void rfid_stop(void)
{
  DISABLE_CP_INT();
  //disable power
  //PORTC &= ~(1<<PC3);
}

#define START_SENTINEL  0x0b
#define END_SENTINEL    0x1f
//actually the scanner gives us 5 Bytes, but we need only the last 4
//if you need 5, you can easily change uint32_t* aId to uint64_t* aId
uint8_t rfid_get(Rfid_Tag* aId)
{
  uint8_t x;
  uint8_t rawDec;
  uint8_t result = RFID_ERROR;

  if((dataByte == BUFFER_SIZE)
    && (rxBuffer[0] == 0)
    && (rxBuffer[1] == 0)
    && (rxBuffer[2] == START_SENTINEL)
    && (rxBuffer[17] == END_SENTINEL)
    && (rxBuffer[19] == 0)
    && (rxBuffer[20] == 0))
  {
    result = RFID_OK;
    *aId = 0;

    for(uint8_t i=3; i<17; ++i) {
      //odd parity check
      x = rxBuffer[i];
      x ^= x >> 4;
      x ^= x >> 2;
      x ^= x >> 1;
      if(((~x) & 0x01) != 0) {
        result = RFID_ERROR;
        break;
      }

      rawDec = rxBuffer[i] & 0x0f;
      if(rawDec > 9) {
        result = RFID_ERROR;
        break;
      }
      
      *aId = (*aId) * 10 + rawDec;
    }
  }
  
  
  REG_STATUS &= ~(1<<HAS_RFID_RX);
  
  rfid_reset();

  return result;
}

