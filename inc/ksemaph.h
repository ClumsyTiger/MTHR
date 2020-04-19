// ____________________________________________________________________________________________________________________________________________
// KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE
// ____________________________________________________________________________________________________________________________________________
// KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE
// ____________________________________________________________________________________________________________________________________________
// KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE...KSEMAPHORE

#ifndef _KSEMAPH_H_
#define _KSEMAPH_H_

#include "utility.h"
#include "pcb.h"


class KSemaphore
{
// declarations
private:
   friend class Semaphore;
   friend class System;
   friend class PCB;
   friend class Thread;
   friend class KEvent;

   friend void interrupt timer_tick();
   friend void           switch_context   ();
   friend void interrupt switch_context_uc();

   friend int2 compare_ksemaphore_id(KSemaphore* k1, KSemaphore* k2);   // poredi dva ksemafora na osnovu id-jeva
   friend bool match_ksemaphore_id(KSemaphore* k, void* args);          // args je u stvari id ksemafora koga trazimo

   friend void print_ksemaphore_id(ostream& os, const KSemaphore* k);   // ispisuje ksemafor id

   friend void pcb_default_signal0();


// internal state
private:
   static SemaphId idcnt;

private:
   Semaphore* semaphore;      // wrapper klasa za kernelsem
   SemaphId   id;             // id ksemafora

private:
   static List<PCB>* glist;   // lista niti koje su se zablokirale na ksemaforu na konacno vreme
   List<PCB>* list;           // lista niti koje su se zablokirale na ksemaforu na neograniceno vreme

   int4 counter;              // koliko niti moze da prodje, tj. se zablokiralo na ksemaforu


// methods
private:
   void block(Time timeout = 0);
   void unblock(PCB* pcb = nullptr);
   void destroy();

private:
   KSemaphore(int4 init = 1);
   ~KSemaphore();

   bool wait(Time timeout = 0);
   uns4 signal(uns4 n = 0);
   uns4 signalAll();

   int4 val() const;


   ostream& print_list(ostream& os = cout);
   static ostream& print_glist(ostream& os = cout, bool drawframe = true);

   ostream& print(ostream& os = cout, bool drawframe = true) const;
   friend ostream& operator <<(ostream& os, const KSemaphore& k);
};



// comparators and matchers
int2 compare_ksemaphore_id(KSemaphore* k1, KSemaphore* k2);   // poredi dva ksemafora na osnovu id-jeva
bool match_ksemaphore_id(KSemaphore* k, void* args);          // args je u stvari id ksemafora koga trazimo

void print_ksemaphore_id(ostream& os, const KSemaphore* k);   // ispisuje id ksemafora na izlazu


#endif//_KSEMAPH_H_
