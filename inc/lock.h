// ____________________________________________________________________________________________________________________________________________
// LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...
// ____________________________________________________________________________________________________________________________________________
// LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...
// ____________________________________________________________________________________________________________________________________________
// LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...LOCK...

#ifndef _LOCK_H_
#define _LOCK_H_


class Lock
{
// declarations
   friend class System;
   friend class KSemaphore;
   friend class Thread;

// methods
public:
   static void Lock::lock();
   static void Lock::unlock();

private:
   static void Lock::unlock_and_switch();
};


#define LOCK      { Lock::lock();              }
#define UNLOCK    { Lock::unlock();            }
#define UNLOCK_SW { Lock::unlock_and_switch(); }



#endif//_LOCK_H_
