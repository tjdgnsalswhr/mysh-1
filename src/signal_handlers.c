#include "signal_handlers.h"

void catch_sigint(int signalNo)
{
  exit(0);
  // TODO: File this!
}

void catch_sigtstp(int signalNo)
{
  exit(0);
  // TODO: File this!
}
