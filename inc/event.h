// _____________________________________________________________________________________________________________________________________________
// EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT
// _____________________________________________________________________________________________________________________________________________
// EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT
// _____________________________________________________________________________________________________________________________________________
// EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT...EVENT

#ifndef _EVENT_H_
#define _EVENT_H_

#include "utility.h"
#include "usermain.h"   // samo zato da bi bio realizovan ceo interfejs koji je trazen u projektu
#include "kevent.h";



class Event
{
// declarations
   friend class KEvent;
   friend class System;


// internal state
private:
   KEvent* kevent;


// methods
public:
   Event(ivtno ivtno);
   ~Event();

   void wait();

protected:
   void signal();
};


#endif//_EVENT_H_
