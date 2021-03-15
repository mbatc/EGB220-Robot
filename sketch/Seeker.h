#ifndef Seeker_h__
#define Seeker_h__

#include "Str.h"

enum SeekOrigin : uint8_t {
  SO_Start,
  SO_Current,
  SO_End
};

class Seeker
{
public:
  Seeker(char const * str, uint32_t initialPos = 0);

  uint32_t seekTo(char const * set);
  uint32_t seekToSet(char const * str);
  uint32_t skipSet(char const * set);

  int   readInt();
  float readFloat();
  bool  readToken();

  char const * getToken();
  char const * tell();

  bool startsWith(char const * str, bool seekPast = true);
  bool startsWithSet(char const * set, bool seekPast = true);

  uint32_t seek(uint32_t pos, SeekOrigin origin = SO_Current);
  uint32_t length();

protected:
  uint32_t setCaret(char const * newCaret);

  Str m_lastToken;

  char const *m_start = nullptr;
  char const *m_caret = nullptr;
  int         m_length = -1;
};

#endif // Seeker_h__
