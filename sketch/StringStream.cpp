#include "StringStream.h"

StringStream::StringStream(String s = "")
  : m_string(s)
  , m_position(0)
{}

int StringStream::available() {
  return m_string.length() - m_position;
}

int StringStream::read() {
  return available() > 0 ? m_string[m_position++] : -1;
}

int StringStream::peek() {
  return available() > 0 ? m_string[m_position] : -1;
}

void StringStream::flush() {
  m_string = "";
  m_position = 0;
}

size_t StringStream::write(uint8_t c) {
  m_string += (char)c;
  return 1;
}

String const &StringStream::str() const {
  return m_string;
}
