#ifndef _EW_SEQUENCER_H_
#define _EW_SEQUENCER_H_

#include <Arduino.h>

/**
 * 
 * TODO: We could enhance the sequencer to
 *  a) Send out a MIDI clock
 *  b) Synchronize to a MIDI clock
 * see https://en.wikipedia.org/wiki/MIDI_beat_clock
 */
class Sequencer
{
public:
  /** Number of 16th steps per bar. */
  static const uint8_t NumSteps = 16;

  static const uint8_t NumTracks = 8;

  static const uint8_t NumSeq = 2;

  class Note
  {
  public:
    typedef enum {
      Velocity = 0,
      Pitch,
    } Field;
    uint8_t m_pitch;
    uint8_t m_velocity;
  };

  /** TODO: make track implicit of sequence
   */
  class Track
  {
  public:
    Note m_notes[NumSteps];    
  };

  class Sequence
  {
  public:
    Track m_tracks[NumTracks];
  };

  Sequencer();
  
  void begin();


  uint8_t bpm() const
  {
    return m_bpm;
  }
  void bpm(uint8_t bmp);
  uint8_t step() const
  {
    return m_step;
  }
  void sequence(uint8_t seq)
  {
    m_sequence = seq;
  }
  uint8_t sequence() const
  {
    return m_sequence;
  }
  static const uint8_t CurrentSeq = 0xFF;
  Sequence &data(uint8_t index = CurrentSeq)
  {
    if (index >= NumSeq) {
      return m_sequences[m_sequence];
    } else {
      return m_sequences[index];
    }
  }
private:
  friend void timer1ISR(void);
  void nextstep();

  uint8_t m_bpm;
  uint8_t m_sequence;
  uint8_t m_step;
  
  uint16_t m_timer1Preload;
  
  Sequence m_sequences[NumSeq];
};



#endif /* _EW_SEQUENCER_H_ */

