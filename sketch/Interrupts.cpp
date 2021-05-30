#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "pins_arduino.h"
#include "Interrupts.h"

#define CLOCK_SPEED long(16000000)

static int g_prescalers[5] = { 1, 8, 64, 256, 1024 };

class InterruptImpl
{
public:
  template<typename T>
  InterruptImpl(
    volatile uint8_t *pControl1Reg,
    volatile uint8_t *pControl2Reg,
    volatile uint8_t *pMaskReg,
    volatile T * pCountReg,
    volatile T * pCompareAReg,
    volatile T * pCompareBReg)
    : InterruptImpl(sizeof(T), pControl1Reg, pControl2Reg, pMaskReg, pCountReg, pCompareAReg, pCompareBReg)
  {}

  // Create an interupt implementation which stores pointers to the target registers
  InterruptImpl(
    int byteWidth,
    volatile uint8_t *pControl1Reg,
    volatile uint8_t *pControl2Reg,
    volatile uint8_t *pMaskReg,
    volatile void *pCountReg,
    volatile void *pCompareAReg,
    volatile void *pCompareBReg)
    : m_TCCRA(pControl1Reg)
    , m_TCCRB(pControl2Reg)
    , m_TIMSK(pMaskReg)
    , m_width(byteWidth)
  {
    if (byteWidth == 1) {
      m_u8.OCRA = (volatile uint8_t*)pCompareAReg;
      m_u8.OCRB = (volatile uint8_t*)pCompareAReg;
      m_u8.TCNT = (volatile uint8_t*)pCountReg;
    }
    else if (byteWidth == 2) {
      m_u16.OCRA = (volatile uint16_t*)pCompareAReg;
      m_u16.OCRB = (volatile uint16_t*)pCompareAReg;
      m_u16.TCNT = (volatile uint16_t*)pCountReg;      
    }

    m_maxCompareValue = 0;
    for (int i = 0; i < m_width; ++i)
      m_maxCompareValue |= 0xFF << (8 * i);

    *m_TCCRA = 0; // Clear the control registers
    *m_TCCRB = 0;
  }

  bool attach(float frequency, InteruptService callback) {
    detach();

    m_callback = callback;

    unsigned int prescaler = 0;
    unsigned int prescalerIndex = 0;
    unsigned long requiredComparison = 0;

    // Determine the prescaler and compare register value
    for (int i = 0; i < sizeof(g_prescalers) / sizeof(g_prescalers[0]); ++i) {
      prescalerIndex     = i + 1;
      prescaler          = g_prescalers[i];
      requiredComparison = (CLOCK_SPEED / (prescaler * frequency)) - 1;
      if (requiredComparison < m_maxCompareValue)
        break;
      requiredComparison = 0;
      prescaler = 0;
    }    

    if (requiredComparison == 0 || prescaler == 0)
    {
      Serial.println("Invalid prescale or comp value");
      return false;
    }

    Serial.print("Comp Reg: ");
    Serial.print(requiredComparison);
    Serial.print(" Prescaler: ");
    Serial.print(prescaler);
    Serial.print(" Prescaler Idx: ");
    Serial.println(prescalerIndex);

    switch (m_width)
    {
    case 1:
      *m_u8.TCNT = 0;
      *m_u8.OCRA = (uint8_t)requiredComparison;
      *m_u8.OCRB = (uint8_t)requiredComparison;
      break;
    case 2:
      *m_u16.TCNT = 0;
      *m_u16.OCRA = (uint16_t)requiredComparison;
      *m_u16.OCRB = (uint16_t)requiredComparison;
      break;
    }

    *m_TCCRB &= 0b11111000; // Clear prescaler control bits
    *m_TCCRB |= 0b00000111 & (uint8_t)prescalerIndex; // Set prescaler index
    *m_TIMSK |= 0b00000010; // Enable A comparison interrupt
    *m_TCCRB |= (1 << WGM12); // Enable CTC mode
  }

  void detach() {
    m_callback = nullptr;
    *m_TCCRA = 0; // Clear the control registers
    *m_TCCRB = 0;
    *m_TIMSK &= 0b11111101; // Disable A comparison interrupt
  }

  // Functor operator to invoke the stored callback
  void operator ()(){
    if (m_callback != nullptr)
      m_callback();
  }

private:
  volatile InteruptService m_callback;

  int m_width;
  unsigned int m_maxCompareValue;

  union {
    struct {
      volatile uint8_t *OCRA;
      volatile uint8_t *OCRB;
      volatile uint8_t *TCNT;
    } m_u8;

    struct {
      volatile uint16_t *OCRA;
      volatile uint16_t *OCRB;
      volatile uint16_t *TCNT;      
    } m_u16;
  };

  volatile uint8_t *m_TCCRA;
  volatile uint8_t *m_TCCRB;
  volatile uint8_t *m_TIMSK;
};

InterruptImpl g_timers[3] = {
  { &TCCR0A, &TCCR0B, &TIMSK0, &TCNT0, &OCR0A, &OCR0B },
  { &TCCR1A, &TCCR1B, &TIMSK1, &TCNT1, &OCR1A, &OCR1B },
  { &TCCR3A, &TCCR3B, &TIMSK3, &TCNT3, &OCR3A, &OCR3B },
};

#define AttachISR(vec, index) ISR(vec) { g_timers[index](); }

AttachISR(TIMER0_COMPA_vect, 0);
AttachISR(TIMER1_COMPA_vect, 1);
AttachISR(TIMER3_COMPA_vect, 2);

bool Interrupts::attach(int timer, float frequency, InteruptService callback)
{
  noInterrupts();
  bool result = g_timers[timer].attach(frequency, callback);
  interrupts();
  return result;
}

void Interrupts::detach(int timer)
{
  noInterrupts();
  g_timers[timer].detach();
  interrupts();  
}
