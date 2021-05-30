#include "Bluetooth.h"

Bluetooth::Bluetooth(int rx, int tx, Commands *pCommands)
  : m_serial(rx, tx)
  , m_commands(pCommands, &m_recvBuffer, &m_serial)
{
  // Open the serial communication with the bluetooth module
  m_serial.begin(9600);
}

void Bluetooth::update()
{
  while (m_serial.available()) {
    if (!m_recieving) {
      Serial.print("Recieving: ");
    }

    m_recieving = true; // Signal we are recieving a command
    char c = m_serial.read();
    if (c == '\0')
    {
      if (m_recvBuffer.available() > 1) {
        Serial.println("Executing command");
        m_commands.execute();
      }
      m_recvBuffer.flush();
      m_recieving = false; // Signal we have finished revieving the command
      Serial.println();
      Serial.println("Recieve Done");
    } // New line signals the end of a command 
    else {
      m_recvBuffer.write(c);
      Serial.write(c);
    }
  }

  // Only send a response if we are not recieving and data
  if (m_sendBuffer.available())
  {
    Serial.print("Sending: ");

    // Send all the data available
    while (m_sendBuffer.available()) {
      Serial.write(m_sendBuffer.peek());
      m_serial.write(m_sendBuffer.read());
    }
    Serial.println();
    m_sendBuffer.flush(); // Free the memory
    m_serial.write('\0'); // Write 0 to indicate the end of a message
  }
}

size_t Bluetooth::write(uint8_t data) { m_sendBuffer.write(data); }
