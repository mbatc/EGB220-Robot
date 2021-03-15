#include "Str.h"
#include <string.h>

static char const * const _empty = "\0";
char const *whitespace = " \n\t\r";

static bool _Compare(char const *rhs, char const *lhs, uint32_t count = ~uint32_t(0)) { return strncmp(rhs, lhs, count) == 0; }

char const * strnstr(char const *str, char const *needle, uint32_t count)
{
  char const * cursor = str;
  char const * end    = str + count;
  uint32_t needleLen = (uint32_t)strlen(needle);

  while (*cursor != 0 && cursor < end)
  {
    if (_Compare(cursor, needle, needleLen))
      return cursor;
    ++cursor;
  }

  return nullptr;
}

char const * strnpbrk(char const *str, char const *control, uint32_t count)
{
  char const * cursor = str;
  char const * end = str + count;
  uint32_t needleLen = (uint32_t)strlen(control);

  while (*cursor != 0 && cursor < end)
  {
    char c = *cursor;
    char const *_control = control;
    while (*_control != 0)
      if (*_control++ == c)
        return cursor;
  }

  return nullptr;
}

Str & Str::operator=(char const * str)
{
  *this = Str(str);
  return *this;
}

Str::Str(char const * str, int length)
{
  if (str)
  {
    m_buffer.reserve((length < 0 ? (uint32_t)strlen(str) : length) + 1);
    m_buffer.addRange(str, str + m_buffer.capacity() - 1);
    m_buffer.add('\0');
  }
}

Str::Str(List<char> str)
  : m_buffer(move(str))
{
  if (m_buffer.back() != 0)
    m_buffer.add(0);
}

List<Str> Str::split(Str const & seperator, bool isSet) const
{
  return split(seperator.data(), isSet);
}

List<Str> Str::split(char const * seperator, bool isSet) const
{
  List<Str> ls;

  uint32_t lastCaret = 0;
  uint32_t currentCaret = 0;
  uint32_t tokenLen = isSet ? 1 : (uint32_t)strlen(seperator);

  while (currentCaret = find(seperator, lastCaret, ~uint32_t(0), isSet) < length())
  {
    ls.add(substr(lastCaret, currentCaret));
    lastCaret = currentCaret + tokenLen;
  }

  if (lastCaret != currentCaret)
    ls.add(substr(lastCaret, currentCaret));

  return ls;
}

List<Str> Str::split(char seperator) const
{
  char buf[2] = { seperator, 0 };
  return split(buf);
}

bool Str::compare(const char * str) const
{
  return _Compare(data(), str);
}

bool Str::compare(Str const & seperator) const
{
  return compare(seperator.data());
}

char const & Str::at(uint32_t index) const
{
  return m_buffer[index];
}

char const & Str::operator[](uint32_t index) const
{
  return at(index);
}

Str Str::substr(uint32_t start, uint32_t length) const
{
  return Str(m_buffer.getElements(start, length));
}

Str Str::cat(Str const & rhs) const
{
  List<char> newBuffer = m_buffer.getElements(0, length());
  newBuffer.addRange(rhs.m_buffer.begin(), rhs.m_buffer.end());
  return newBuffer;
}

uint32_t Str::length() const
{
  return m_buffer.size() == 0 ? 0 : (m_buffer.size() - 1);
}

uint32_t Str::find(char c, uint32_t from, uint32_t to) const
{
  return utilMin(m_buffer.find(c), length());
}

uint32_t Str::find(char const * str, uint32_t from, uint32_t to, bool isSet) const
{
  char const *start = begin() + from;
  uint32_t    count = to - from;
  char const *end = isSet ? strnpbrk(start, str, count) : strnstr(start, str, count);
  return end ? uint32_t(end - begin()) : length();
}

uint32_t Str::find(Str const & seperator, uint32_t from, uint32_t to, bool isSet) const
{
  return find(seperator.data(), from, to);
}

Str Str::join(List<Str> const & strings, Str const & seperator)
{
  return join(strings, seperator.data());
}

Str Str::join(List<Str> const & strings, char const * seperator)
{
  Str ret;
  for (uint32_t i = 0; i < strings.size(); ++i)
  {
    ret = ret.cat(strings[i]);
    if (i < strings.size() - 1)
      ret = ret.cat(seperator);
  }

  return ret;
}

char const * Str::data() const
{
  return length() == 0 ? _empty : m_buffer.data();
}

char const * Str::begin() const
{
  return data();
}

char const * Str::end() const
{
  return data() + length();
}
