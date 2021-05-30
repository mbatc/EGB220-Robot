#ifndef Bluetooth_h__
#define Bluetooth_h__

#include "SoftwareSerial.h"
#include "SerialCommands.h"
#include "StringStream.h"

class SerialCommands;

class Bluetooth : public Print
{
public:
  Bluetooth(int rx, int tx, Commands *pCommands);

  void update();

  virtual size_t write(uint8_t data);

protected:
  SoftwareSerial m_serial;
  
  // Buffer of data to send 
  StringStream m_sendBuffer;

  // Buffer of data to recieve
  StringStream m_recvBuffer;

  // Communication states
  bool m_commandReady = false;
  bool m_recieving = false;

  // The command set available through the bluetooth interface
  SerialCommands m_commands;
};

extern Bluetooth bt;

#endif
