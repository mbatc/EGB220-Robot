#define _CRT_SECURE_NO_WARNINGS
#include "Seeker.h"
#include <stdio.h>

Seeker::Seeker(char const * str, uint32_t initialPos /*= 0*/)
  : m_start(str)
  , m_caret(str + initialPos)
{}

uint32_t Seeker::seekTo(char const * str)
{
  return setCaret(strstr(m_caret, str));
}

uint32_t Seeker::seekToSet(char const * set)
{
  return setCaret(strpbrk(m_caret, set));
}

uint32_t Seeker::skipSet(char const * set)
{
  char const * cur = m_caret;
  while (*cur != 0 && strchr(set, *cur) != nullptr)
    ++cur;
  return setCaret(cur);
}

int Seeker::readInt()
{
  int val;
  int dist = sscanf(m_caret, "%d", &val);
  if (dist > 0)
    seek(dist);
  return val;
}

float Seeker::readFloat()
{
  float val;
  int dist = sscanf(m_caret, "%f", &val);
  if (dist > 0)
    seek(dist);
  return val;
}

bool Seeker::readToken()
{
  skipSet(whitespace);
  char const * last = tell();
  seekToSet(whitespace);
  m_lastToken = Str(last, tell() - last);
  return m_lastToken.length() > 0;
}

char const * Seeker::getToken()
{
  return m_lastToken.data();
}

char const * Seeker::tell()
{
  return m_caret;
}

bool Seeker::startsWith(char const * str, bool seekPast)
{
  int len = strlen(str);
  bool found = strncmp(m_caret, str, len) == 0;
  if (found && seekPast)
    seek(len);
  return found;
}

bool Seeker::startsWithSet(char const * set, bool seekPast)
{
  bool found = strchr(set, *m_caret) != nullptr;
  if (found && seekPast)
    seek(1);
  return found;
}

uint32_t Seeker::seek(uint32_t pos, SeekOrigin origin /*= SO_Current*/)
{
  switch (origin)
  {
  case SO_Current: return setCaret(m_caret + pos);
  case SO_Start:   return setCaret(m_start + pos);
  case SO_End:     return setCaret(m_start + length() - pos);
  }
}

uint32_t Seeker::length()
{
  if (m_length == -1)
  {
    char const *pos = m_caret;
    while (*pos != '\0') ++pos;
    m_length = pos - m_start;
  }
  return m_length;
}

uint32_t Seeker::setCaret(char const * newCaret)
{
  if (newCaret == nullptr)
    return seek(0, SO_End);

  uint32_t dist = uint32_t(newCaret - m_caret);
  m_caret = newCaret;
  return dist;
}

