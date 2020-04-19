// ____________________________________________________________________________________________________________________________________________
// DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE
// ____________________________________________________________________________________________________________________________________________
// DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE
// ____________________________________________________________________________________________________________________________________________
// DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE...DELEGATE

#ifndef _DELEGATE_H_
#define _DELEGATE_H_

#include "utility.h"


// nije thread safe u smislu da ne moze vise thread-ova da ga koristi i da pritom jedan od njih pozove destruktor
class Delegate
{
// internal state
private:
   List<SignalHandler>* hlist;


// methods
private:
   Delegate(List<SignalHandler>* _hlist);

public:
   Delegate();
   ~Delegate();

public:
   bool add(SignalHandler h);
   bool swap(SignalHandler h1, SignalHandler h2);

   uns4 signal();
   uns4 size();

   Delegate* copy();
   void clear();

   ostream& print(ostream& os = cout) const;
   friend ostream& operator <<(ostream& os, const Delegate& d);
};


// signalhandler list tools
bool match_signal_handler_container(SignalHandler* h, void* args);   // args je u stvari kontejner signalhandler-a koga trazimo
bool map_call_signal_handler(SignalHandler* h, void*);               // zove signal handler ako je ispravno prosledjen signal handler kontejner

void print_signal_handler(ostream& os, const SignalHandler* h);      // ispisuje signal handler na izlazu
SignalHandler* copy_signal_handler_container(SignalHandler* h);      // kopira kontejner signalhander-a (pokazivac na prosto polje tipa signalhandler)
void delete_signal_handler_container(SignalHandler* h);              // brise kontejner signalhandler-a



#endif//_DELEGATE_H_
