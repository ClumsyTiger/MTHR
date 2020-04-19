#define UTIL_PRINTING_TOOLS
#include "system.h"
#include "pcb.h"
#include "thread.h"


const StackSize Thread::def_stack_size = 4096;

const Time Thread::inf_priority  = 0,
           Thread::low_priority  = 1,
           Thread::norm_priority = 2,
           Thread::high_priority = 4;



Thread::Thread( PCB* pcb )
{
   if( !pcb ) DELETE_THIS_AND_RETURN;

   this->pcb = pcb;
   pcb->t = this;
}

Thread::Thread( StackSize stack_size, Time quant )
{
   LOCK;
   pcb = new PCB(runnable, quant, stack_size);
   UNLOCK;

   if( !pcb ) DELETE_THIS_AND_RETURN;

   pcb->t = this;
}

Thread::~Thread()
{
   LOCK;
   if( pcb )
   {
      // ako thread i dalje radi, treba ga sacekati, thread ne bi trebalo da moze da brise sam sebe !
      waitToComplete();

      pcb->t = nullptr;
      delete pcb; pcb = nullptr;
   }
   UNLOCK;
}



void Thread::runnable()
{
   LOCK;
   if( SystemInitialized() && System::running && System::running->t )
      System::running->t -> run_wrapper();

   // changing context
   UNLOCK_SW;
}

void Thread::run_wrapper()
{
   UNLOCK;   // u paru sa LOCK-om iz Thread::runnable()

   run();

   LOCK;     // u paru sa UNLOCK_SW iz Thread::runnable()
   // finalizing Thread state
   if( System::running )   // ovde bi bio problem da je neki thread obrisao ovaj (u destruktoru se System::running postavi na nullptr)
   {
      System::running->state = PCB::finished;
      System::activecnt--;

      System::running->timestamp   = 0;
      System::running->timeexpired = false;
      System::running->sem4del->signalAll();

      PCB::signal(System::running->parent, PCB::notifyparent_id);   // prvo se obavestava parent, da bi bili sigurni da ce se obavestiti
      PCB::signal(System::running->id,     PCB::finished_id    );   // moze da se desi da finished signal obrise thread i odmah promeni kontekst
      //System::running = nullptr;
   }
}

void Thread::run()
{
   #ifdef DEBUG
      LOCK;
      cout << "RUN" << getRunningId() << "# Hello World!" << endl;
      UNLOCK;
   #endif
}



void Thread::start()                { LOCK;   if( pcb ) pcb->start_thread();       UNLOCK; }
void Thread::waitToComplete() const { LOCK;   if( pcb ) pcb->wait_to_complete();   UNLOCK; }


ThreadId Thread::getId() const  { return pcb->getid();          }
ThreadId Thread::getRunningId() { return PCB::get_running_id(); }


Thread* Thread::getThreadById(ThreadId id)
{
   LOCK;
      PCB* pcb = PCB::getpcb(id);
      Thread* thread = pcb ? pcb->t : nullptr;
   UNLOCK;

   return thread;
}



void Thread::signal(SignalId id) { LOCK; if( pcb ) PCB::signal(pcb->id, id); UNLOCK; }

void Thread::registerHandler      (SignalId id, SignalHandler h)                    { LOCK; if( pcb ) pcb->register_handler(id, h);   UNLOCK; }
void Thread::unregisterAllHandlers(SignalId id)                                     { LOCK; if( pcb ) pcb->unregister_handlers(id);   UNLOCK; }
void Thread::swap                 (SignalId id, SignalHandler h1, SignalHandler h2) { LOCK; if( pcb ) pcb->swap_handlers(id, h1, h2); UNLOCK; }

void Thread::blockSignal  (SignalId id) { LOCK; if( pcb ) pcb->block_signal(id); UNLOCK; }
void Thread::unblockSignal(SignalId id) { LOCK; if( pcb ) pcb->allow_signal(id); UNLOCK; }

void Thread::blockSignalGlobally  (SignalId id) { LOCK; PCB::block_signal_g(id); UNLOCK; }
void Thread::unblockSignalGlobally(SignalId id) { LOCK; PCB::allow_signal_g(id); UNLOCK; }
