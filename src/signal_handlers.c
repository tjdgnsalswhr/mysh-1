#include "signal_handlers.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>


void catch_sigint(int signalNo)
{
  signal(SIGINT,SIG_IGN);
  printf("\nCTRL+C IS INVALID INPUT!!\n");
  // TODO: File this!
}

void catch_sigtstp(int signalNo)
{
  signal(SIGTSTP,SIG_IGN);
  printf("\nCTRL+Z IS INVALID INPUT!!\n");
  // TODO: File this!
}
