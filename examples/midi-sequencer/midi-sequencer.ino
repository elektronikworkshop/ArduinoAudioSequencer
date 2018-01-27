/** MIDI Sequencer
 * 
 *           LCD SDA: A4
 *           LCD SCL: A5
 * 
 * Encoder channel A: 2
 * Encoder channel B: 3
 *    Encoder button: 4
 * 
 *      Enter button: 5
 *       Exit button: 6
 * 
 *         Pitch pot: A0
 *      Velocity pot: A1
 * 
 *           MIDI TX: TX
 * 
 * To find the address of your LC-display, use the i2c-scanner tool from the
 * examples.
 * 
 * NOTE: In case you upgraded your sequencer with a MIDI input: Don't forget
 * to temporarily unplug the MIDI RX when uploading your sketch!
 */
#include <EwMidiSequencer.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>

#include "CustomChars.h"
#include "Button.h"

const uint8_t LcdI2cAddr = 0x3F;
const uint8_t LcdWidth = 16;
const uint8_t LcdHeight = 2;

const uint8_t EncPinA = 2;
const uint8_t EncPinB = 3;
const uint8_t EncButtonPin = 4;
const uint8_t EnterButtonPin = 5;
const uint8_t ExitButtonPin = 6;

const uint8_t PotBPin = A0;
const uint8_t PotAPin = A1;

Sequencer sequencer;

class UI
{
public:
  static const uint8_t MaxScreens = 8;
  
  class Screen
  {
  public:
    Screen(UI *ui)
      : m_ui(ui)
      , m_lastBlinkMs(0)
      , m_lastBlinkOn(true)
    { }

    virtual void init() = 0;
    virtual void run() = 0;
    
  protected:
    UI *ui()
    {
      return m_ui;
    }
    void drawTrack(uint8_t track,
                   Sequencer::Note::Field field,
                   uint8_t seq = Sequencer::CurrentSeq)
    {
      auto &s = sequencer.data(seq);
      auto &t = s.m_tracks[track];

      ui()->m_lcd.setCursor(0, 1);
      auto &lcd = ui()->m_lcd;
      switch (field) {
        case Sequencer::Note::Field::Velocity:
          for (uint8_t i = 0; i < LcdWidth; i++) {
            lcd.write(t.m_notes[i].m_velocity >> 4);        
          }
          break;
        case Sequencer::Note::Field::Pitch:
          for (uint8_t i = 0; i < LcdWidth; i++) {
            lcd.write(t.m_notes[i].m_pitch + 0x10);        
          }
          break;
      }
    }
    static uint8_t newIndex(uint8_t oldIndex, int16_t delta, uint8_t N)
    {
      if (delta < 0) {
        delta = oldIndex + delta;
        while (delta < 0) {
          delta += N;
        }
        oldIndex = delta;
      } else {
        oldIndex += delta;
      }
      oldIndex %= N;
      return oldIndex;
    }
    static uint8_t analogTo7Bit(uint8_t pin)
    {
      auto v = analogRead(pin);
      v = v > 1022 ? 1022 : v;
      return map(v, 0, 1022, 0, 127);
    }
    bool changeBlink()
    {
      if (millis() - m_lastBlinkMs > 500) {
        m_lastBlinkMs = millis();
        m_lastBlinkOn = !m_lastBlinkOn;
        return true;
      }
      return false;
    }
    bool getBlink() const
    {
      return m_lastBlinkOn;
    }
  private:
    UI *m_ui;
    long m_lastBlinkMs;
    bool m_lastBlinkOn;
  };

  class ScreenTrack
    : public Screen
  {
  public:
    ScreenTrack(UI *ui)
      : Screen(ui)
      , m_track(0)
      , m_pos(0)
    { }
    void track(uint8_t track)
    {
      m_track = track;
    }
    bool readPitch()
    {
      auto pitch = analogTo7Bit(PotAPin);
      if (pitch != m_pitch) {
        m_pitch = pitch;
        return true;
      }
      return false;
    }
    bool readVelocity()
    {
      auto velocity = analogTo7Bit(PotBPin);
      if (velocity != m_velocity) {
        m_velocity = velocity;
        return true;
      }
      return false;
    }
    void resetCursor()
    {
      auto &lcd = ui()->m_lcd;
      lcd.setCursor(m_pos, 1);
    }
    virtual void init()
    {
      auto &lcd = ui()->m_lcd;
      
      lcd.cursor();
      lcd.blink();
      
      readPitch();
      readVelocity();
      char buffer[LcdWidth + 1] = {' '};
      sprintf(buffer,
              "V%3u P%3u",
              m_velocity,
              m_pitch
              );
      lcd.setCursor(0, 0);
      lcd.write(buffer, LcdWidth);
      
      // second row shows sequence
      drawTrack(m_track, Sequencer::Note::Field::Velocity, Sequencer::CurrentSeq);

      resetCursor();
    }
    uint8_t velocity2char(uint8_t v)
    {
      return map(v, 1, 127, 1, 7);
    }
    virtual void run()
    {
      bool changes = false;

      if (ui()->m_exitButton.state()) {
        ui()->Pop();
        return;
      }

      if (ui()->m_enterButton.state()) {
        auto &s = sequencer.data();
        auto &t = s.m_tracks[m_track];
        uint8_t c = 0;
        if (t.m_notes[m_pos].m_velocity) {
          t.m_notes[m_pos].m_velocity = 0;
        } else if (m_velocity) {
          t.m_notes[m_pos].m_velocity = m_velocity;
          t.m_notes[m_pos].m_pitch = m_pitch;
          c = velocity2char(m_velocity);
        }
        ui()->m_lcd.setCursor(m_pos, 1);
        ui()->m_lcd.write(c);
        changes = true;
      }
      if (readPitch()) {
        char buffer[4];
        sprintf(buffer, "%3u", m_pitch);
        ui()->m_lcd.setCursor(6, 0);
        ui()->m_lcd.write(buffer, 3);
        changes = true;
      }
      if (readVelocity()) {
        char buffer[4];
        sprintf(buffer, "%3u", m_velocity);
        ui()->m_lcd.setCursor(1, 0);
        ui()->m_lcd.write(buffer, 3);
        changes = true;
      }
      auto delta = ui()->getEncDelta();
      if (delta) {
        m_pos = newIndex(m_pos, delta, Sequencer::NumSteps);
        changes = true;
      }
      if (changes) {
        resetCursor();
      }
    }
  private:
    uint8_t m_track;
    uint8_t m_pitch;
    uint8_t m_velocity;
    uint8_t m_pos;
  };  

  class ScreenHome
    : public Screen
  {
  public:
    ScreenHome(UI *ui)
      :Screen(ui)
      , m_track(0)
      , m_editItem(0)
      , m_editing(false)
      , m_screenTrack(ui)
    { }
    virtual void init()
    {
      auto &lcd = ui()->m_lcd;
      
      lcd.noBlink();
      lcd.noCursor();

// commented out to make it possible to come back to home screen
// and switch the track quickly
//      m_editing = false;

      char buffer[LcdWidth + 1];
      sprintf(buffer,
              "%cS%1u %cT%1u %cBPM %3u",
              (m_editItem == 0 ? '>' : ' '),
              sequencer.sequence(),
              (m_editItem == 1 ? '>' : ' '),
              m_track,
              (m_editItem == 2 ? '>' : ' '),
              sequencer.bpm());
      lcd.setCursor(0, 0);
      lcd.write(buffer, LcdWidth);
      
      // second row shows sequence
      drawTrack(m_track, Sequencer::Note::Field::Velocity, Sequencer::CurrentSeq);
      lcd.cursor();
    }

    enum {
      EditItemSeq = 0,
      EditItemTrack,
      EditItemBpm,
      EditItemCount,
    };
    void setItemMarker(bool on)
    {
      auto &lcd = ui()->m_lcd;
      lcd.noCursor();
      const uint8_t poslut[EditItemCount] = {0, 4, 8};
      lcd.setCursor(poslut[m_editItem], 0);
      lcd.write(on ? '>' : ' ');
    }
    virtual void run()
    {
      bool change = false;
      
      if (ui()->m_enterButton.state()) {
        m_screenTrack.track(m_track);
        ui()->Push(&m_screenTrack);
      }
      if (ui()->m_encButton.state()) {
        if (m_editing) {
          m_editing = false;
          setItemMarker(true);  // in case we were blinking
          change = true;
        } else {
          m_editing = true;
        }
      }
      auto delta = ui()->getEncDelta();
      if (delta) {
        auto &lcd = ui()->m_lcd;

        if (m_editing) {
          switch (m_editItem) {
            case EditItemSeq:
            {
              char buffer[2];
              auto s = newIndex(sequencer.sequence(), delta, Sequencer::NumSeq);
              sprintf(buffer, "%1u", s);
              lcd.noCursor();
              lcd.setCursor(2, 0);
              lcd.write(buffer, 1);
              sequencer.sequence(s);
              
              drawTrack(m_track, Sequencer::Note::Field::Velocity, Sequencer::CurrentSeq);
              break;
            }
            case EditItemTrack:
            {
              char buffer[2];
              m_track = newIndex(m_track, delta, Sequencer::NumTracks);
              sprintf(buffer, "%1u", m_track);
              lcd.noCursor();
              lcd.setCursor(6, 0);
              lcd.write(buffer, 1);
        
              drawTrack(m_track, Sequencer::Note::Field::Velocity, Sequencer::CurrentSeq);
              break;
            }
            case EditItemBpm:
            {
              long bpm = sequencer.bpm() + delta;
              bpm = bpm > 200 ? 200 : bpm;
              bpm = bpm < 60 ? 60 : bpm;
          
              if (bpm != sequencer.bpm()) {
                sequencer.bpm(bpm);
              }
              char buffer[4];
              sprintf(buffer, "%3u", sequencer.bpm());
              lcd.noCursor();
              lcd.setCursor(13, 0);
              lcd.write(buffer, 3);
              break;
            }
          }
          change = true;
        } else {
          setItemMarker(false);
          m_editItem = newIndex(m_editItem, delta, EditItemCount);
          setItemMarker(true);
          change = true;
        }
      }
      
      if (m_editing and changeBlink()) {
        setItemMarker(getBlink());
        change = true;
      }
            
      static uint8_t prevStep = sequencer.step();
      uint8_t step = sequencer.step();
      if (prevStep != step or change) {
        ui()->m_lcd.setCursor(step, 1);
        prevStep = step;
      }
      if (change) {
         ui()->m_lcd.cursor();
      }
    }
  private:
    uint8_t m_track;
    uint8_t m_editItem;
    bool m_editing;
    ScreenTrack m_screenTrack;
  };

  UI()
    : m_lcd(LcdI2cAddr, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE)
    , m_enc(EncPinA, EncPinB)
    , m_encButton(EncButtonPin)
    , m_enterButton(EnterButtonPin)
    , m_exitButton(ExitButtonPin)
    , m_encPos(m_enc.read())
    , m_screens{nullptr}
    , m_screen(0)
    , m_screenHome(this)
  { }
  
  void begin()
  {
    m_lcd.begin (LcdWidth, LcdHeight);
    m_lcd.setBacklight(HIGH);
  
    for (int i = 0; i < 8; i++) {
      m_lcd.createChar(i, volChars[i]);
    }
    
    m_lcd.home ();
    m_lcd.print("EW LoFi");
    m_lcd.setCursor (0, 1);
    m_lcd.print(" MIDI Sequencer ");

    m_encButton.begin();
    m_enterButton.begin();
    m_exitButton.begin();
    
    m_screen = 0;
    m_screens[m_screen] = &m_screenHome;
    m_screens[m_screen]->init();
  }
  
  void run()
  {
    m_screens[m_screen]->run();
  }

  int8_t getEncDelta()
  {
    auto delta = m_enc.read() - m_encPos;
    if (delta >= 4 or delta <= 4) {
      delta /= 4;
      m_encPos += delta * 4;
    }
    return delta;
  }  

  void Push(Screen *screen)
  {
    m_screen++;
    m_screens[m_screen] = screen;
    screen->init();
  }
  void Pop()
  {
    if (m_screen) {
      m_screen--;
      m_screens[m_screen]->init();
    }
  }
private:
  LiquidCrystal_I2C m_lcd;
  Encoder m_enc;
  Button m_encButton;
  Button m_enterButton;
  Button m_exitButton;
  long m_encPos;
  Screen *m_screens[MaxScreens];
  uint8_t m_screen;
  ScreenHome m_screenHome;
};

UI ui;

void setup()
{
  sequencer.begin();
  ui.begin();
}

void loop()
{
  ui.run();
}
