#include "event.h"
#include "kevent.h"


Event::Event(ivtno ivtno)
{
   LOCK;
   kevent = new KEvent(ivtno);
   UNLOCK;

   if( !kevent ) DELETE_THIS_AND_RETURN;

   kevent->event = this;
}

Event::~Event()
{
   LOCK;
      if( kevent )
      {
         kevent->event = nullptr;
         delete kevent; kevent = nullptr;
      }
   UNLOCK;
}




void Event::wait() { kevent->wait(); }
void Event::signal() { kevent->signal(); }
