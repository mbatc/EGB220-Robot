#ifndef ExampleModule_h__
#define ExampleModule_h__

class ExampleModule
{
public:
  ExampleModule(int theValue);

  int GetValue();

protected:
  int m_value;
};

#endif // ExampleModule_h__
