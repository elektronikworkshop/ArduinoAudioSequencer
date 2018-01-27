#include "EwMidiSequencer.h"

/* https://www.arduino.cc/en/Tutorial/Midi */

/** See https://github.com/FortySevenEffects/arduino_midi_library */
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

Sequencer* theSequencer = nullptr;

/* make sure this function will be inlined within the
 *  interrupt handler
 */
void
timer1ISR(void)
{
  TCNT1 = theSequencer->m_timer1Preload;
  theSequencer->nextstep();
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  timer1ISR();
}

Sequencer::Sequencer()
  : m_bpm(120)
  , m_sequence(0)
  , m_step(0)
{
  theSequencer = this;
}

/* Fot timer1 prescaler etc. see
 *  https://oscarliang.com/arduino-timer-and-interrupt-tutorial/
 */
 
void
Sequencer::begin()
{
  MIDI.begin();
  
  // bpm must be set
  bpm(m_bpm);
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  
  TCNT1 = m_timer1Preload;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts  


  pinMode(LED_BUILTIN, OUTPUT);
}

const uint32_t ClockHz = 16000000UL;
const uint32_t Timer1Prescale = 256UL;
const uint32_t MicroSecondsPerSecond = 1000000UL;

const uint16_t MicroSecondsPerTimer1Tick = (MicroSecondsPerSecond * Timer1Prescale) / ClockHz;

uint16_t bpm_to_timer1_preload(uint8_t bpm)
{
  uint32_t us = 60UL * MicroSecondsPerSecond;

  us = us / bpm / 4;
  
  uint16_t ticks = us / MicroSecondsPerTimer1Tick;

  return 65536 - ticks;
}

void
Sequencer::bpm(uint8_t bpm)
{
  m_bpm = bpm;
  m_timer1Preload = bpm_to_timer1_preload(m_bpm);
}

void
Sequencer::nextstep()
{
  m_step++;
  m_step = m_step >= NumSteps ? 0 : m_step;

  auto *track = m_sequences[m_sequence].m_tracks;
  for (uint8_t t = 0; t < NumTracks; t++, track++) { 
    auto &n = track->m_notes[m_step];
    if (n.m_velocity) {
      MIDI.sendNoteOn(n.m_pitch, n.m_velocity, t + 1);
    }
  }

#if 1
  /* blink LED_BUILTIN with the beat using fast port toggle */
  if ((m_step & 0x03) == 0x00) {
    PORTB |= 0x20;
  } else {
    PORTB &= ~0x20;
  }
#endif
}

