// _____________________________________________________________________________________________________________________________________________
// KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT
// _____________________________________________________________________________________________________________________________________________
// KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT
// _____________________________________________________________________________________________________________________________________________
// KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT...KEVENT

#ifndef _KEVENT_H_
#define _KEVENT_H_

#include "utility.h"
#include "system.h"



// sa sentry-jem smo sigurni da ce se funkcija registrovati u nizu KEvent-ova, kao i da se to nece uraditi iz same prekidne rutine (ne postoji problem sa volatile)
#define PREPAREENTRY(idx, callold)                                                  \
                                                                                    \
void interrupt eventintroutine##idx(...)                                            \
{                                                                                   \
   if( SystemInitialized() ) KEvent::signalevent(idx);                              \
   if( callold             ) KEvent::calloldintr(idx);                              \
}                                                                                   \
bool eventintroutine##idx##_sentry = KEvent::regnewintr(idx, eventintroutine##idx);



class KEvent
{
// declarations
   friend class Event;
   friend class System;
   friend bool KEventInitialized();


// internal state
private:
   static KEvent*    kevent[IVT_SIZE];   // array of all events
   static IntRoutine oldivt[IVT_SIZE];   // old ivt
   static IntRoutine regivt[IVT_SIZE];   // registered ivt
   static volatile bool initialized;

private:
   Event* event;
   ivtno idx;

   PCB* owner;
   KSemaphore* sem;


// methods
private:
   static void firstinit();   // zove se samo jednom, ako niz event-ova nije inicijalizovan
   static void rstevent();    // ako treba vratiti stare interrupt rutine (ne resetuje registrovane interrupt rutine, jer bi smo ih u suprotnom izgubili)

public:
   static bool regnewintr(ivtno idx, IntRoutine intr);   // nije namenjena da se zove iz interrupt rutine (zbog toga sto nedostaje volatile na puno mesta)
   static void signalevent(ivtno idx);   // namenjena da se zove iz interrupt rutine
   static void calloldintr(ivtno idx);   // namenjena da se zove iz interrupt rutine


private:
   KEvent(ivtno ivtno);   // pravi objekat ako vec ne postoji i pamti ga u event listi(ako je objekat vec napravljen na poziciji idx, sledeca nit koja bi ga pravila na poziciji idx ne moze nikako da se blokira na njemu)
   ~KEvent();             // unistava objekat iz event liste

   ostream& print(ostream& os = cout) const;
   friend ostream& operator <<(ostream& os, const KEvent& e);

private:
   void wait();     // zove ih Event (korisnicka wrapper klasa)
   void signal();   // zove ih Event (korisnicka wrapper klasa)
};


bool KEventInitialized();



#endif//_KEVENT_H_
