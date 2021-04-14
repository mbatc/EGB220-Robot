#ifndef StringStream_h__
#define StringStream_h__

#include "Stream.h"

class StringStream : public Stream
{
public:
  StringStream(String s = "");

  virtual int available();
  virtual int read();
  virtual int peek();
  virtual void flush();
  virtual size_t write(uint8_t c);

  String const &str() const;

protected:
  String m_string;
  unsigned int m_position;
};

#endif // StringStream_h__
