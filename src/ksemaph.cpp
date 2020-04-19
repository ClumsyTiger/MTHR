#include "system.h"
#include "pcb.h"
#include "list.h"
#include "ksemaph.h"

#include "schedule.h"
#include "semaphor.h"


List<PCB>* KSemaphore::glist;
SemaphId KSemaphore::idcnt;


KSemaphore::KSemaphore(int4 init)
{
   if( !SystemInitialized() )   // inicijalizuje sistem ako vec nije, korisno za globalne podatke u vise fajlova (nemaju definisan redosled inicijalizacije)
      System::init();


   // ksemaphore identification
   id = idcnt++;

   // pocetna inicijalizacija zbog destruktora
   semaphore = nullptr;   // wrapper klasa ksemaphore-a se postavi na nullptr
   list      = nullptr;



   LOCK;
   list = new List<PCB>(false);   // semaphore doesn't own its local pcbs or its global pcbs (only the system pcb list owns its pcbs)
   UNLOCK;

   if( !list ) DELETE_THIS_AND_RETURN;

   counter = init;
   list->setmatch(match_pcb_id);
   list->setprint(print_pcb_id);
}

KSemaphore::~KSemaphore()
{
   LOCK;
      if( semaphore )
      {
         semaphore->ksemaphore = nullptr;
         delete semaphore; semaphore = nullptr;
      }

      if( list )
      {
         if( SystemInitialized() )   // ukoliko je sistem inicijalizovan, onda ce on moci da isprazni scheduler u koji ce signalAll() potencijalno da dodaje pcb-jeve
            signalAll();

         delete list; list = nullptr;
      }
   UNLOCK;
}



void KSemaphore::block(Time timeout)
{
   LOCK;
      if( !System::running )
      {
         UNLOCK;
         return;
      }

      System::running->state = PCB::blocked;
      System::running->timestamp = (timeout > 0) ? System::time() + timeout : 0;
      System::running->blockedon = this;
      System::running->timeexpired = false;

      list->pushb((PCB*)System::running);

      if( timeout > 0 )
         glist->pushs((PCB*)System::running);

      switch_context_uc();   // dispatch unconditionally (without unlock) - saves lockcnt

   UNLOCK;
}

void KSemaphore::unblock(PCB* pcb)
{
   LOCK;

      if( !pcb )
      {
         // ukoliko se ne zada argument, odblokira se prva nit koja je cekala na semaforu (head)
         // izbaci se iz semaforove lokalne liste blokiranih pcb-jeva
         pcb = list->popf();
         if( pcb ) pcb->timeexpired = false;   // ta nit se odblokirala ali joj nije isteklo max vreme cekanja na semaforu
      }
      else if( pcb->blockedon == this )
      {
         // ukoliko se zada argument, odblokira se nit sa datim id-jem koja ceka na ovom semaforu (ako takva postoji)
         list->find( &(pcb->id) );             // pretraga na osnovu id-ja pcb-ja
         pcb = list->pop();                    // izbaci se iz semaforove lokalne liste blokiranih pcb-jeva
         if( pcb ) pcb->timeexpired = true;    // ta nit se odblokirala jer joj je isteklo max vreme cekanja na semaforu
      }
      else
      {
         // taj pcb nije blokiran na ovom semaforu, ne moze da ga odblokira ovaj semafor
         pcb = nullptr;
      }


      // ako nije pronadjen takav pcb, nista se ne radi
      if( !pcb )
      {
         UNLOCK;
         return;
      }


      // ako je pronadjen odblokira se i ubaci u scheduler
      pcb->state = PCB::ready;
      pcb->blockedon = nullptr;

      Scheduler::put(pcb);

      // ako se nalazi u globalnoj listi blokiranih thread-ova, treba i odatle da se izbaci
      if( pcb->timestamp != 0 )
      {
         glist->find( &(pcb->id) );
         glist->pop();
      }


   UNLOCK;
}

bool KSemaphore::wait(Time timeout)
{
   LOCK;   // zasto ne asm xchg ax, a; ? - zato sto bi to bilo gubljenje vremena na blokiranje niti
   counter--;
   if( counter < 0 )
   {
      #ifdef DEBUG
         if( System::running ) cout << "Thread<" << System::running->id << "> blocked on     Semaphore<" << id << ">\n";
         else                  cout << "Thread<"    "?"                    "> blocked on     Semaphore<" << id << ">\n";
      #endif

      block(timeout);

      #ifdef DEBUG
         if( System::running ) cout << "Thread<" << System::running->id << "> unblocked from Semaphore<" << id << ">\n";
         else                  cout << "Thread<"    "?"                    "> unblocked from Semaphore<" << id << ">\n";
      #endif
   }
   bool timeexpired = (System::running) ? System::running->timeexpired : false;
   UNLOCK;

   return !timeexpired;
}

uns4 KSemaphore::signal(uns4 n)
{
   LOCK;
      if( n == 0 ) n++;
      counter += n;

      PCB* pcb;
      int unblockcnt;
      for( unblockcnt = 0; unblockcnt < n && !list->isempty(); unblockcnt++ )
         unblock();

   UNLOCK;
   return unblockcnt;
}

uns4 KSemaphore::signalAll() { LOCK; uns4 unblockcnt = ( list->size() > 0 ) ? signal(list->size()) : 0; UNLOCK; return unblockcnt; }



int4 KSemaphore::val() const
{
   LOCK;
      int4 _cnt = counter;
   UNLOCK;

   return _cnt;
}



ostream& KSemaphore::print_list(ostream& os)
{
   LOCK;
      if( !list )
      {
         UNLOCK;
         return os;
      }

      list->print(os, false);


   UNLOCK;
   return os;
}

ostream& KSemaphore::print_glist(ostream& os, bool drawframe)
{
   LOCK;
      if( !glist )
      {
         UNLOCK;
         return os;
      }

      if( drawframe ) os << "======" << "globally=blocked(" << System::time() << ")" << "========" "\n";

         if( !drawframe ) os << "globally blocked: ";
         glist->print(os, false);
         os << "ISPIS OK" << endl;   ///////////////////////////////////////////////////////

      if( drawframe ) os << "\n" "----------------------------------" << endl;

   UNLOCK;
   return os;
}


ostream& KSemaphore::print(ostream& os, bool drawframe) const
{
   LOCK;

      if( drawframe ) os << "======" << "Semaphore" << "<" << id << ">" << "================" "\n";

         if( glist ) os << "glist: ";  glist->print(os, false) << "\n";
         if( list )  os << "list:  ";  list->print(os, false)  << "\n";

      if( drawframe ) os << "----------------------------------" << endl;

   UNLOCK;
   return os;
}

ostream& operator <<(ostream& os, const KSemaphore& k)
{
   return k.print(os);
}



int2 compare_ksemaphore_id(KSemaphore* k1, KSemaphore* k2)   // poredi dva ksemafora na osnovu id-jeva
{
   if( !k1 || !k2 )       return INCOMP;

   if( k1->id <  k2->id ) return LESS;
   if( k1->id == k2->id ) return EQUAL;
                          return GREATER;
}

bool match_ksemaphore_id(KSemaphore* k, void* args)   // args je u stvari id ksemafora koga trazimo
{
   if( !k || !args ) return false;
   if( k->id == *((SemaphId*)args) ) return true;

   return false;
}



void print_ksemaphore_id(ostream& os, const KSemaphore* k)   // ispisuje ksemafor id
{
   if( k ) k->print(os);
}
