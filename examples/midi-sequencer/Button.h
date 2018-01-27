

class Button
{
public:
  enum {
    FlagNone     = 0x00,
    FlagNoPullup = 0x01 << 0,
    FlagLevel    = 0x01 << 1,
    FlagRising   = 0x01 << 2,
  };
  
  Button(uint8_t pinNr,
         uint8_t flags = FlagNone)
    : m_pinNr(pinNr)
    , m_flags(flags)
    , m_state(HIGH)
  { }

  void begin()
  {
    pinMode(m_pinNr, (m_flags & FlagNoPullup ? INPUT : INPUT_PULLUP));
    m_state = digitalRead(m_pinNr);
  }

  uint8_t state()
  {
    uint8_t s = digitalRead(m_pinNr);
    bool r = false;
    
    if ((m_flags & FlagLevel) == 0) {
      if (m_flags & FlagRising) {
        r = m_state == LOW and s == HIGH;
      } else {
        r = m_state == HIGH and s == LOW;
      }
      m_state = s;
    } else {
      if (m_flags & FlagRising) {
        r = s == HIGH;
      } else {
        r = s == LOW;
      }
    }
    return r;
  }
private:
  uint8_t m_pinNr;
  uint8_t m_flags;
  uint8_t m_state;
};

