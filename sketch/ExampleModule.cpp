#include "ExampleModule.h"

ExampleModule::ExampleModule(int theValue)
  : m_value(theValue)
{}

int ExampleModule::GetValue()
{
  return m_value;
}
