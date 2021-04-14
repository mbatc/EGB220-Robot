#include "Commands.h"
#include "SerialCommands.h"

char const * SerialCommands::callToken = "call";
char const * SerialCommands::setToken  = "set";
char const * SerialCommands::getToken  = "get";
char const * SerialCommands::typeToken = "type";
char const * SerialCommands::lsCmdToken = "lscmd";
char const * SerialCommands::lsVarToken = "lsvar";

SerialCommands::SerialCommands(Commands *pCommands, Stream *pIn, Stream *pOut)
  : m_pCommands(pCommands)
  , m_pIn(pIn)
  , m_pOut(pOut)
{}

void SerialCommands::readToken(Stream *pStream)
{
  m_lastToken = ""; // Clear the token
  bool found = false;  
  while (true) {
    // Read bytes until we find a whitespace character
    if (!pStream->available()) {
      continue;
    }
    
    int c = pStream->read();
    if (strchr(whitespace, (char)c) != 0) {
      if (!found) {
        continue;
      }
      break;
    }

    // Append characters to the token
    m_lastToken += (char)c;
    found = true;
  }
}

ResultType SerialCommands::execute()
{
  bool onlyWhitespace = true;
  while (onlyWhitespace && m_pIn->available()) {
    int c = m_pIn->peek(); // check character without consuming it
    if (strchr(whitespace, (char)c) != 0) {
      m_pIn->read(); // next char
    }
    else {
      onlyWhitespace = false;
    }
  }
  
  if (onlyWhitespace)
    return RT_None;
  
  readToken(m_pIn);
  if (m_lastToken.equalsIgnoreCase(callToken)) {    
    return executeCall();
  }
  else if (m_lastToken.equalsIgnoreCase(setToken)) {
    return executeSet();
  }
  else if (m_lastToken.equalsIgnoreCase(getToken)) {
    return executeGet();
  }
  else if (m_lastToken.equalsIgnoreCase(typeToken)) {
    return executeType();
  }
  else if (m_lastToken.equalsIgnoreCase(lsCmdToken)) {
    return respondListCmd();
  }
  else if (m_lastToken.equalsIgnoreCase(lsVarToken)) {
    return respondListVar();
  }
  return respondFailure("Unknown Command Token");
}

ResultType SerialCommands::executeCall()
{
  readToken(m_pIn);
  
  if (m_pCommands->call(m_lastToken.c_str())) {
    return respondCall();
  }
  return respondFailure("Command Not Found");
}

ResultType SerialCommands::executeSet()
{  
  readToken(m_pIn);
  
  bool success = false;
  uint32_t id = m_pCommands->getVariableType(m_lastToken.c_str());
  if (id == TypeID<int>()) {
    success = m_pCommands->set<int>(m_lastToken.c_str(), (int)m_pIn->parseInt(SKIP_WHITESPACE));
  }
  else if (id == TypeID<float>()) {
    success = m_pCommands->set<float>(m_lastToken.c_str(), m_pIn->parseFloat(SKIP_WHITESPACE));
  }
  else if (id == TypeID<double>()) {
    success = m_pCommands->set<double>(m_lastToken.c_str(), m_pIn->parseFloat(SKIP_WHITESPACE));
  }
  else if (id == TypeID<bool>()) {
    success = m_pCommands->set<bool>(m_lastToken.c_str(), m_pIn->parseInt(SKIP_WHITESPACE) != 0);
  }

  return success ? respondSet() : respondFailure("Unknown Variable");
}

ResultType SerialCommands::executeGet()
{  
  readToken(m_pIn);
  uint32_t id = m_pCommands->getVariableType(m_lastToken.c_str());
  if (id == TypeID<int>()) {
    return respondGet<int>();
  }
  else if (id == TypeID<float>()) {
    return respondGet<float>();
  }
  else if (id == TypeID<double>()) {
    return respondGet<double>();
  }
  else if (id == TypeID<bool>()) {
    return respondGet<bool>();
  }
  
  return respondFailure("Unknown Variable");
}

ResultType SerialCommands::executeType()
{  
  readToken(m_pIn);
  return respondType();
}

ResultType SerialCommands::respondSet()
{
  m_pOut->print("OK+SET");
  m_pOut->write('\0');
  return RT_Set;
}

ResultType SerialCommands::respondCall()
{  
  m_pOut->print("OK+CALL");
  m_pOut->write('\0');
  return RT_Call;
}

ResultType SerialCommands::respondType()
{  
  m_pOut->print("OK+TYPE\n");
  m_pOut->print(m_pCommands->getVariableTypeName(m_lastToken.c_str()));
  m_pOut->write('\0');
  return RT_Type;
}

ResultType SerialCommands::respondListCmd()
{
  m_pOut->print("OK+LSCMD\n");
  m_pOut->print(m_pCommands->getCommandCount());
  m_pOut->print("\n");
  for (int i = 0; i < m_pCommands->getCommandCount(); ++i) {
    m_pOut->print(m_pCommands->getCommandName(i));
    m_pOut->write('\n');
  }
  m_pOut->write('\0');
  
  return RT_ListCommands;
}

ResultType SerialCommands::respondListVar()
{  
  m_pOut->print("OK+LSVAR\n");
  m_pOut->print(m_pCommands->getVariableCount());
  m_pOut->print("\n");
  for (int i = 0; i < m_pCommands->getVariableCount(); ++i) {
    m_pOut->print(m_pCommands->getVariableName(i));
    m_pOut->print(" ");
    m_pOut->print(m_pCommands->getVariableTypeName(i));
    m_pOut->write('\n');
  }
  m_pOut->write('\0');
  
  return RT_ListVariables;
}

ResultType SerialCommands::respondFailure(char const * msg)
{  
  m_pOut->print("ERR+");
  m_pOut->print(msg);
  m_pOut->write('\0');
  return RT_Failure;
}
