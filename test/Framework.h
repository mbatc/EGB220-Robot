#ifndef Framework_h__
#define Framework_h__

#include <functional>

int  Tests_Run();
int  Tests_Add(char const * name, std::function<void()> func);
void Tests_Fail(char const * message, char const * file, int line);

#define Test_Fail(message) { Tests_Fail("Assertion Failed: " message, __FILE__, __LINE__); return; }
#define Test_Assert(condition) { if (!(condition)) Test_Fail("Assertion Failed: " #condition), __FILE__, __LINE__); }

#define Test(name)\
void name();\
static int __Framework_Test_ ## name ## _Status = Tests_Add(#name, name);\
static void name()\

#endif // Framework_h__
