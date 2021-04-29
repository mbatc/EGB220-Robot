#include "Bluetooth.h"

Bluetooth::Bluetooth(int rx, int tx, Commands *pCommands)
  : m_serial(rx, tx)
  , m_commands(pCommands, &m_recvBuffer, &m_serial)
{
  m_serial.begin(9600);
}

void Bluetooth::update()
{
  if (m_commandReady) {
    m_commands.execute();
    m_recvBuffer.flush();
    m_commandReady = false;
  }
  
  if (m_serial.available()) {
    m_commandReady |= m_serial.peek() == '\n'; // New line signals the end of a command 
    m_recvBuffer.write((char)m_serial.read());
  }
  else if (!m_recvBuffer.available() && m_sendBuffer.available())
  {
    while (m_sendBuffer.available())
      m_serial.write(m_sendBuffer.read());
    m_sendBuffer.flush();
    m_serial.write('\0');
  }
}

size_t Bluetooth::write(uint8_t data) { m_sendBuffer.write(data); }
