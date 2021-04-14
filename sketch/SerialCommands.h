#ifndef SerialCommands_h__
#define SerialCommands_h__

#include "Commands.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "../test/FakeArduino.h"
#endif

// class Commands; // Pre-declare the Commands class

/**
 * Stream Commands Spec:
 * 
 * General Syntax - 
 *  action identified [value]
 *
 * To call a command function:
 *   call myFunc
 *
 * To set a variable:
 *   set myVar 1
 *
 * To get a variable:
 *   get myVar
 *
 * To get a variable type:
 *   type myVar
 *   
 * To list all variables:
 *   lsvar
 *
 * To list all commands:
 *   lscmd
 *   
 * Gets will add data to the SerialCommands read buffer.
 * This can be read from using the read() function.
 */

enum ResultType {
  RT_Failure,
  RT_None,
  RT_Call,
  RT_Set,
  RT_Get,
  RT_Type,
  RT_ListCommands,
  RT_ListVariables,
  RT_Count,
};

class Stream;

class SerialCommands
{
public:
  static char const * callToken;
  static char const * setToken;
  static char const * getToken;
  static char const * typeToken;
  static char const * lsCmdToken;
  static char const * lsVarToken;

  /**
   * Create a SerialCommands interface using a set of Commands.
   */
  SerialCommands(Commands *pCommands, Stream *pIn, Stream *pOut);

  /**
   * Update the Stream commands.
   * Reads data from the stream and writes responses.
   */
  ResultType execute();

protected:
  void readToken(Stream *pStream);
  
  ResultType executeCall();
  ResultType executeSet();
  ResultType executeGet();
  ResultType executeType();

  ResultType respondSet();
  ResultType respondCall();
  ResultType respondType();
  ResultType respondListCmd();
  ResultType respondListVar();
  ResultType respondFailure(char const *msg);

  template<typename T>
  ResultType respondGet() {
    T val;
    if (m_pCommands->get<T>(m_lastToken.c_str(), &val))
      return respondGet<T>(val, m_pCommands->getVariableTypeName(m_lastToken.c_str()));
    return respondFailure("Unknown Variable");
  }

  template<typename T>
  ResultType respondGet(T const & value, char const *typeName) {
    m_pOut->print("OK+GET\n");
    m_pOut->print(m_lastToken.c_str());
    m_pOut->print(" ");
    m_pOut->print(typeName);
    m_pOut->print(" ");
    m_pOut->print(value);
    m_pOut->write('\0');
    return RT_Get;
  }
  
  String  m_lastToken; // Last token read from the Stream input. User internally
  
  Commands *m_pCommands = nullptr; // The set of commands available
  Stream *m_pIn         = nullptr;
  Stream *m_pOut        = nullptr;
};

#endif // SerialCommands_h__
