#include "system.h"
#include "lock.h"


void Lock::lock()   { System::lock();   }
void Lock::unlock() { System::unlock(); }

void Lock::unlock_and_switch() { System::unlock_and_switch(); }

