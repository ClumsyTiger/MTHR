#include "ksemaph.h"
#include "semaphor.h"


Semaphore::Semaphore( KSemaphore* ksemaphore, int1 )   // cpp_null_ambiguity_resolve
{
   if( !ksemaphore ) DELETE_THIS_AND_RETURN;

   this->ksemaphore = ksemaphore;
   ksemaphore->semaphore = this;
}

Semaphore::Semaphore( int4 init )
{
   LOCK;
      ksemaphore = new KSemaphore(init);
   UNLOCK;

   if( !ksemaphore ) DELETE_THIS_AND_RETURN;

   ksemaphore->semaphore = this;
}

Semaphore::~Semaphore()
{
   LOCK;
   if( ksemaphore )
   {
      ksemaphore->semaphore = nullptr;
      delete ksemaphore; ksemaphore = nullptr;
   }
   UNLOCK;
}



bool Semaphore::wait(Time timeout) { return ksemaphore->wait(timeout); }
uns4 Semaphore::signal(uns4 n) { return ksemaphore->signal(n); }

int4 Semaphore::val() const { return ksemaphore->val(); }
