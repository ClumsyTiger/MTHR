// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE

#ifndef _SEMAPHOR_H_
#define _SEMAPHOR_H_

#include "utility.h"
#include "usermain.h"   // samo zato da bi bio realizovan ceo interfejs koji je trazen u projektu
class KSemaphore;


class Semaphore
{
// declarations
private:
   friend class KSemaphore;


// internal state
private:
   KSemaphore* ksemaphore;


// methods
private:
   Semaphore(KSemaphore* ksemaphore, int1 cpp_null_ambiguity_resolve);   // because this version of C++ doesn't differentiate between nullptr and long 0

public:
   Semaphore(int4 init = 1);
   ~Semaphore();

   bool wait(Time timeout = 0);
   uns4 signal(uns4 n = 0);

   int4 val() const;
};



#endif//_SEMAPHOR_H_
