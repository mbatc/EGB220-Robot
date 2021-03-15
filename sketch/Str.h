#ifndef Str_h__
#define Str_h__

#include "List.h"

extern char const *whitespace;

char const * strnstr(char const *str, char const *needle, uint32_t count);
char const * strnpbrk(char const *str, char const *control, uint32_t count);

class Str
{
public:
  Str()                         = default;
  Str(Str && o)                 = default;
  Str(Str const & o)            = default;
  Str& operator=(Str && o)      = default;
  Str& operator=(Str const & o) = default;

  Str& operator=(char const * o);
  Str(char const *str, int length = -1);
  Str(List<char> str);

  Str substr(uint32_t start, uint32_t length) const;
  
  Str cat(Str const & rhs) const;

  uint32_t length() const;
  uint32_t find(char c, uint32_t from = 0, uint32_t to = ~uint32_t(0)) const;
  uint32_t find(char const * str, uint32_t from = 0, uint32_t to = ~uint32_t(0), bool isSet = false) const;
  uint32_t find(Str const & seperator, uint32_t from = 0, uint32_t to = ~uint32_t(0), bool isSet = false) const;

  static Str join(List<Str> const & strings, Str const & seperator);
  static Str join(List<Str> const & strings, char const * seperator);

  List<Str> split(Str const & seperator, bool isSet = false) const;
  List<Str> split(char const * seperator, bool isSet = false) const;
  List<Str> split(char seperator) const;

  bool compare(const char *str) const;
  bool compare(Str const & seperator) const;

  char const & at(uint32_t index) const;
  char const & operator[](uint32_t index) const;

  char const * data() const;
  char const * begin() const;
  char const * end() const;

protected:
  List<char> m_buffer;
};

#endif // Str_h__
