#pragma once

typedef void (*InteruptService)();

class Interrupts
{
public:
  static bool attach(int timer, float frequency, InteruptService callback);
  static void detach(int timer);
};
