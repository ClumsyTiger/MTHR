// _____________________________________________________________________________________________________________________________________________
// USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN
// _____________________________________________________________________________________________________________________________________________
// USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN
// _____________________________________________________________________________________________________________________________________________
// USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN...USER-MAIN

#include <iostream.h>
#include "thread.h"
#include "semaphor.h"
#include "event.h"



Semaphore *semAtoB, *semBtoA;

// Thread A
class ThreadA : public Thread
{
public:
   ThreadA( Time quant = Thread::norm_priority, StackSize stack_size = Thread::def_stack_size ) : Thread(stack_size, quant)
   {}
   virtual ~ThreadA() { waitToComplete(); }

protected:
   virtual void run();
};



void ThreadA::run()
{
   for( int i = 0; i < 10; i++ )
   {
      //semBtoA->wait(20);

      LOCK;
      cout << "A# " << i << endl;
      UNLOCK;

      for( int k = 0; k < 10000; k++ )
         for( int j = 0; j < 30000; j++ )
            ;

      //semAtoB->signal();
   }

   LOCK;
   cout << "A> Happy End"<< endl;
   UNLOCK;
}
//ThreadA A1, A2;



// Thread B
class ThreadB : public Thread
{
public:
   ThreadB( Time quant = Thread::norm_priority, StackSize stack_size = Thread::def_stack_size ) : Thread(stack_size, quant)
   {}
   virtual ~ThreadB() { waitToComplete(); }

protected:
   virtual void run();
};



void ThreadB::run()
{
   for( int i = 0; i < 10; i++ )
   {
      //semAtoB->wait(20);

      LOCK;
      cout << "B# " << i << endl;
      UNLOCK;

      for( int k = 0; k < 10000; k++ )
         for( int j = 0; j < 30000; j++ )
            ;

      //semBtoA->signal();
   }

   LOCK;
   cout << "B> Happy End"<< endl;
   UNLOCK;
}
//ThreadB B1, B2;



// Thread C
class ThreadC : public Thread
{
public:
   ThreadC( Time quant = Thread::norm_priority, StackSize stack_size = Thread::def_stack_size ) : Thread(stack_size, quant)
   {}
   virtual ~ThreadC() { waitToComplete(); }

protected:
   virtual void run();
};


PREPAREENTRY(9, true);
void ThreadC::run()
{
   Event* event9 = new Event(9);
   assert( event9 );

   for( int i = 0; i < 10; i++ )
   {
      LOCK;
      cout << "C# " << i << endl;
      UNLOCK;
      event9->wait();

    //for( int k = 0; k < 10000; k++ )
       //for( int j = 0; j < 30000; j++ )
          //;

   }

   LOCK;
   delete event9;
   UNLOCK;

   LOCK;
   cout << "C> Happy End"<< endl;
   UNLOCK;
}
//ThreadC C1, C2;



// korisnik mora da definise ovu funkciju
void tick() {}

// user pravi ovaj main
int userMain(int argc, char* argv[])
{
   semAtoB = new Semaphore(0);
   semBtoA = new Semaphore(0);
   ThreadA* A = new ThreadA(20);
   ThreadB* B = new ThreadB(20);
   ThreadC* C = new ThreadC(20);


   A->start();
   B->start();
   C->start();


//   for( int i = 0; i < 25; i++ )
//   {
//      LOCK;
//      cout << "MAIN# " << i << endl;
//      UNLOCK;
//
//      for( int j = 0; j < 30000; j++ )
//         for( int k = 0; k < 30000; k++ )
//            ;
//   }

   LOCK;
   delete A;
   delete B;
   delete C;
   delete semAtoB;
   delete semBtoA;
   UNLOCK;


   LOCK;
   cout << "MAIN> Happy End" << endl;
   UNLOCK;

   return 0;
}
