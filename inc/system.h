// _____________________________________________________________________________________________________________________________________________
// SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM
// _____________________________________________________________________________________________________________________________________________
// SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM
// _____________________________________________________________________________________________________________________________________________
// SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM...SYSTEM

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "utility.h"
#include "ksemaph.h"
#include "list.h"


// lokacije standardnih interrupt rutina
#define IVT_DIVIDE_ERROR  0
#define IVT_SINGLE_STEP   1
#define IVT_NON_MASKABLE  2
#define IVT_BREAKPOINT    3
#define IVT_OVERFLOW      4
#define IVT_TIMER         8
#define IVT_KEYBOARD      9
#define IVT_OLD_TIMER    96



class System
{
// declarations
   friend bool SystemInitialized();
   friend int main(int argc, char* argv[]);   // kernel main

   friend class Lock;
   friend class PCB;
   friend class Thread;
   friend class KSemaphore;
   friend class KEvent;

   friend void interrupt timer_tick();
   friend void           switch_context   ();
   friend void interrupt switch_context_uc();

   friend void pcb_default_signal0();

   #ifdef DEBUG
      friend void systemtest0();
   #endif


// internal state
private:
   static System* sys;
   static bool initialized;
   static bool initialized_ivt;

   static IntRoutine ivtbak[IVT_SIZE];
   static IntRoutine ivt   [IVT_SIZE];

   static volatile Time currtime;
   static volatile bool should_switch_context;
   static volatile uns4 context_lockcnt;


   static List<PCB>* pcbs;
   static uns4 activecnt;

   static PCB* main;
   static PCB* idle;
   static volatile PCB* running;



// methods
private:
   System();
   ~System();

private:
   static void init();
   static void exit();

   static void  initivt();
   static void  rstivt ();

   static IntRoutine getivt (ivtno idx);
   static void       setivt (ivtno idx, IntRoutine intr);
   static bool       callivt(ivtno idx);
   static void       rstivt (ivtno idx);

private:
   static uns4 getthreadcnt();   //vraca broj napravljenih thread-ova (u globalnoj listi)
   static uns4 getactivecnt();   //vraca broj aktivnih thread-ova (koji su pokrenuti sa start)

   static PCB* getrunn();
   static PCB* getmain();
   static PCB* getidle();

public:
   static ostream& showivt  (ostream& os = cout, uns2 n = IVT_SIZE);
   static ostream& showtypes(ostream& os = cout);
   static ostream& showsched(ostream& os = cout, bool drawframe = true);
   static Time     time();

public:
   static void lock();
   static void unlock();
private:
   static void unlock_and_switch();
   static void reset_lock();
};

// provera da li je sistem inicijalizovan
bool SystemInitialized();


#endif//_SYSTEM_H_
