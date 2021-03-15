#include "SerialCommands.h"
#include "Commands.h"

char const * SerialCommands::callToken = "call";
char const * SerialCommands::setToken  = "set";
char const * SerialCommands::getToken  = "get";
char const * SerialCommands::typeToken = "type";

SerialCommands::SerialCommands(Commands *pCommands)
  : m_pCommands(pCommands)
{}

ResultType SerialCommands::write(uint8_t byte)
{
  ResultType result = RT_NotReady;
  if (m_commandBuffer.size() > 0 && (byte == '\0' || byte == '\n' || byte == '\r')) {
    m_commandBuffer.add('\0');
    result = execute((char const*)m_commandBuffer.data());
    m_commandBuffer.clear();
  }
  else {
    m_commandBuffer.add(byte);
  }
  return result;
}

ResultType SerialCommands::execute(char const * command)
{
  Seeker cmdSeeker(command);
  cmdSeeker.skipSet(whitespace);
  if (cmdSeeker.startsWith(callToken)) {
    return executeCall(&cmdSeeker);
  }
  else if (cmdSeeker.startsWith(setToken)) {
    return executeSet(&cmdSeeker);
  }
  else if (cmdSeeker.startsWith(getToken)) {
    return executeGet(&cmdSeeker);
  }
  else if (cmdSeeker.startsWith(typeToken)) {
    return executeType(&cmdSeeker);
  }

  return RT_Failure;
}

uint32_t SerialCommands::returnType()
{
  return m_convertedType;
}

void* SerialCommands::returnBuffer()
{
  return m_conversionBuffer;
}

ResultType SerialCommands::executeCall(Seeker *pCmd)
{
  ResultType result = RT_Failure;
  
  if (pCmd->readToken() && m_pCommands->call(pCmd->getToken())) {
    result = RT_Call;
  }

  return result;
}

ResultType SerialCommands::executeSet(Seeker *pCmd)
{
  bool success = false;
  if (pCmd->readToken()) {
    uint32_t id = m_pCommands->getVariableType(pCmd->getToken());
    if (id == TypeID<int>()) {
      success = m_pCommands->set<int>(pCmd->getToken(), pCmd->readInt());
    }
    else if (id == TypeID<float>()) {
      success = m_pCommands->set<float>(pCmd->getToken(), pCmd->readFloat());
    }
    else if (id == TypeID<bool>()) {
      success = m_pCommands->set<bool>(pCmd->getToken(), pCmd->readInt() != 0);
    }
  }

  return success ? RT_Set : RT_Failure;
}

ResultType SerialCommands::executeGet(Seeker *pCmd)
{
  if (pCmd->readToken()) {
    uint32_t id = m_pCommands->getVariableType(pCmd->getToken());
    if (id == TypeID<int>()) {
      return setReturnVar<int>(pCmd);
    }
    else if (id == TypeID<float>()) {
      return setReturnVar<float>(pCmd);
    }
    else if (id == TypeID<bool>()) {
      return setReturnVar<bool>(pCmd);
    }
  }

  return RT_Failure;
}

ResultType SerialCommands::executeType(Seeker *pCmd)
{
  if (pCmd->readToken()) {
    int id = m_pCommands->getVariableType(pCmd->getToken());
    if (id >= 0) {
      setReturnVar<int>(id);
      return RT_Type;
    }
  }

  return RT_Failure;
}
