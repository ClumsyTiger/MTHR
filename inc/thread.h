// ____________________________________________________________________________________________________________________________________
// THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD
// ____________________________________________________________________________________________________________________________________
// THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD
// ____________________________________________________________________________________________________________________________________
// THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD...THREAD

#ifndef _THREAD_H_
#define _THREAD_H_

#include "utility.h"
#include "usermain.h"   // samo zato da bi bio realizovan ceo interfejs koji je trazen u projektu
class PCB;


// klasa koja je vidljiva korisniku, obuhvata pcb
class Thread
{
// declarations
   friend class PCB;
   friend class System;


// internal state
private:
   PCB* pcb;

public:
   static const StackSize def_stack_size;   // = 4096   // default stack size
   static const Time inf_priority, low_priority, norm_priority, high_priority;   // = [0, 1, 2, 4]   // thread priority


// methods
private:
   Thread( PCB* pcb );   // used for creating main thread
   static void runnable();
   virtual void run_wrapper();   // ovu funkciju ne treba pozivati

protected:
   Thread( StackSize stack_size = Thread::def_stack_size, Time quant = Thread::norm_priority );   // da ne bi mogla osnovna klasa da se pravi
   virtual void run();

public:
   virtual ~Thread();   // osnovna klasa bi trebalo da moze da se unisti, ako je neko uspeo da je napravi

   void start();
   void waitToComplete() const;


   ThreadId getId() const;
   static ThreadId getRunningId();
   static Thread*  getThreadById(ThreadId id);

public:
   void signal(SignalId id);

   void registerHandler(SignalId id, SignalHandler h);
   void unregisterAllHandlers(SignalId id);
   void swap(SignalId id, SignalHandler h1, SignalHandler h2);

   void blockSignal  (SignalId id);
   void unblockSignal(SignalId id);

   static void blockSignalGlobally  (SignalId id);
   static void unblockSignalGlobally(SignalId id);
};



#endif//_THREAD_H_
