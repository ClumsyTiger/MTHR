#include <dos.h>
#define UTIL_PRINTING_TOOLS
#include "system.h"
#include "schedule.h"
#include "list.h"
#include "pcb.h"
#include "thread.h"
#include "ksemaph.h"
#include "kevent.h"


System* System::sys = nullptr;
bool System::initialized = false;
bool System::initialized_ivt = false;

volatile Time System::currtime = 0;
volatile bool System::should_switch_context = false;
volatile uns4 System::context_lockcnt = 0;

IntRoutine System::ivtbak[IVT_SIZE];
IntRoutine System::ivt   [IVT_SIZE];


List<PCB>* System::pcbs = nullptr;
uns4 System::activecnt = 0;

PCB* System::main = nullptr;
PCB* System::idle = nullptr;
volatile PCB* System::running  = nullptr;



void SystemIdleThread() { while( true ); }



System::System()
{
   LOCK;
      // pocela inicijalizacija
      initialized = false;
      should_switch_context = false;   // interrupt rutina context switch-a ne bi trebalo da je bila namestena pre podizanja sistema, ali za svaki slucaj



      // resursi koji se moraju inicijalizovati sto pre
      currtime   = 0;          // resetovanje trenutnog vremena (od kako se sistem podigao)

      PCB::idcnt = 0;          // resetovanje brojaca id-jeva niti
      KSemaphore::idcnt = 0;   // resetovanje brojaca id-jeva semafora
      activecnt  = 0;          // resetovanje broja aktivnih niti

      PCB::gsignal_mask = PCB::def_signal_mask;  // resetovanje tabele svih dozvoljenih signala (0-15)



      // resursi koji se moraju uspesno alocirati
      pcbs = new List<PCB>(true);                 // lista svih ikada napravljenih pcb-jeva, jeste owner pcb-jeva
      KSemaphore::glist = new List<PCB>(false);   // globalna lista na kojoj niti cekaju neko max vreme i onda se deblokiraju; nije owner pcb-jeva

      // provera da li su alokacije uspele
      if( !pcbs || !KSemaphore::glist )
      {
         UNLOCK;
         DELETE_THIS_AND_RETURN;
      }



      PUSHF; INTD;   // mora da ne bi interrupt napravljen sa PREPAREENTRY() izvrsio user interrupt funkciju
         initialized = true;   // mora da se ne bi napravila rekurzija u PCB konstruktoru, jer on ispituje da li je system initialized


         #ifdef DEBUG
            Thread* _main = new Thread( new PCB(nullptr, 40) );                   // main nit
            Thread* _idle = new Thread( new PCB(SystemIdleThread, 1) );           // idle nit
         #else
            Thread* _main = new Thread( new PCB(nullptr) );                       // main nit
            Thread* _idle = new Thread( new PCB(SystemIdleThread, 1) );           // idle nit
         #endif

         initialized = false;
      POPF;

      main = _main ? _main->pcb : nullptr;
      idle = _idle ? _idle->pcb : nullptr;
      running = main;



      // provera da li su alokacije uspele
      if( !main || !idle )
      {
         UNLOCK;
         DELETE_THIS_AND_RETURN;
      }



      // inicijalizacija alociranih resursa
      pcbs->setcompare(compare_pcb_id);
      pcbs->setmatch  (match_pcb_id);
      pcbs->setdelete (delete_pcb);

      KSemaphore::glist->setcompare(compare_pcb_timestamp);
      KSemaphore::glist->setmatch(match_pcb_id);
      KSemaphore::glist->setprint(print_pcb_id);


      // inicijalizacija kopije iv tabele
      PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
         initivt();

         extern void interrupt timer_tick();
         setivt(96, System::getivt(8));
         setivt( 8, (IntRoutine) timer_tick);
      POPF;



      // zavrsena inicijalizacija
      initialized = true;
   UNLOCK;
}

System::~System()
{
   // preventing context switching
   LOCK;
      // restoring the old interrupt vector table (from backup)
      // !!! trebalo bi da se ne cuva cela lista, vec samo user izmene (kompleksno, jer moze nova sistemska rutina da obrgli user rutinu)
      KEvent::rstevent();
      rstivt();

      // preventing context switching after the previous system's timer routine has been restored
      should_switch_context = false;



      // deleting lists
      if( KSemaphore::glist ) { delete KSemaphore::glist; KSemaphore::glist = nullptr; }

      // clearing pcbs (and implicitly threads)
      #ifdef DEBUG
         if( running != main && running != idle ) Scheduler::put((PCB*)running);   // for debugging purpouses
      #endif

      if( pcbs ) { delete pcbs; pcbs = nullptr; }   // moraju prvo da se obrisu svi semafori jer oni stavljaju sve svoje pcb-jeve u Scheduler
      main = nullptr;      // deleted above
      idle = nullptr;      // deleted above
      running = nullptr;   // deleted above

      // emptying scheduler
      for( PCB* leftover; (leftover = Scheduler::get()) != nullptr;  )   // emptying scheduler
      {
         #ifdef DEBUG
            cout << "Leftover thread:" "\n" << *leftover;   // u ovom trenutku svi pcb-jevi su obrisani, ali to nije problem za ispis
         #endif
      }


      // deregistering system
      initialized = false;   // mora tacno na ovom mestu kaze da je sistem deinicijalizovan, jer inace ostanu pcb-jevi u globalnoj listi pcb-jeva (pdb destruktor prvo pita da li je sistem inicijalizovan pre nego sto proba da ukloni pcb iz liste!)
   UNLOCK;
}


void System::init()
{
   static bool first_init = true;

   LOCK;
      if( first_init )
      {
         sys = nullptr;
         first_init = false;
      }

      if( !sys )
      {
         #ifdef DEBUG or DIAGNOSTICS
            LOCK;
               cout <<
               "        _   __                          _ "     "\n"
               "       | | / /                         | |"     "\n"
               "       | |/ /   ___  _ __  _ __    ___ | |"     "\n"
               "       |    \\  / _ \\| '__|| '_ \\  / _ \\| |" "\n"
               "       | |\\  \\|  __/| |   | | | ||  __/| |"   "\n"
               "       \\_| \\_/ \\___||_|   |_| |_| \\___||_|" "\n"
               "                                          "     "\n"
               "                                          "     "\n";

               cout << "=================================================================" "\n" << endl;
            UNLOCK;
         #endif

         sys = new System();
      }
   UNLOCK;
}

void System::exit()
{
   LOCK;
      if( sys )
      {
         delete sys; sys = nullptr;


         #ifdef DEBUG or DIAGNOSTICS
            LOCK;
               cout << "\n"
               "     __ _   __                          _ "     "\n"
               "    / /| | / /                         | |"     "\n"
               "   / / | |/ /   ___  _ __  _ __    ___ | |"     "\n"
               "  / /  |    \\  / _ \\| '__|| '_ \\  / _ \\| |" "\n"
               " / /   | |\\  \\|  __/| |   | | | ||  __/| |"   "\n"
               "/_/    \\_| \\_/ \\___||_|   |_| |_| \\___||_|" "\n"
               "                                          "     "\n"
               "                                          "     "\n";

               cout << "=================================================================" "\n" << endl;
            UNLOCK;
         #endif
      }
   UNLOCK;
}



IntRoutine System::getivt(ivtno idx)
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( !initialized_ivt )
         initivt();

      ivt[idx] = getvect(idx);
   POPF;

   return ivt[idx];
}

void System::setivt(ivtno idx, IntRoutine intr)
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( !initialized_ivt )
         initivt();

      ivt[idx] = intr;
      setvect(idx, ivt[idx]);
   POPF;
}

bool System::callivt(ivtno idx)
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( !initialized_ivt )
         initivt();

      if( ivt[idx] )
         (ivt[idx])();
   POPF;

   return false;
}

void System::rstivt(ivtno idx)
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( !initialized_ivt )
      {
         POPF;
         return;
      }

      if( ivt[idx] )   // samo ako je korisnik zamenio neku rutinu sa svojom rutinom se vraca stara rutina
      {
         setvect(idx, ivtbak[idx]);
         ivt[idx] = nullptr;
      }

      initialized_ivt = false;
   POPF;
}


void System::initivt()
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( initialized_ivt )
      {
         POPF;
         return;
      }

      for( int i = 0; i < IVT_SIZE; i++ )
      {
         ivtbak[i] = getvect(i);
         ivt[i]    = nullptr;
      }

      initialized_ivt = true;
   POPF;
}

void System::rstivt()
{
   // restoring the backup interrupt vector table
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto radi sa kopijom interrupt tabele
      if( !initialized_ivt )
      {
         POPF;
         return;
      }

      for( int i = 0; i < IVT_SIZE; i++ )
      {
         if( ivt[i] )   // samo ako je korisnik zamenio neku rutinu sa svojom rutinom se vraca stara rutina
         {
            setvect(i, ivtbak[i]);
            ivt[i] = nullptr;
         }
      }
   POPF;
}



uns4 System::getthreadcnt() { LOCK; uns4 tcnt = pcbs->size();      UNLOCK; return tcnt; }
uns4 System::getactivecnt() { LOCK; uns4 acnt = System::activecnt; UNLOCK; return acnt; }

PCB* System::getrunn() { LOCK; PCB* runn = (PCB*) running; UNLOCK; return runn; }
PCB* System::getmain() { LOCK; PCB* main = System::main;   UNLOCK; return main; }
PCB* System::getidle() { LOCK; PCB* idle = System::idle;   UNLOCK; return idle; }



ostream& System::showtypes(ostream& os)
{
   LOCK;
      os << "======Types=======================" "\n";
      os << sizeof(signed char     ) << "B " << "signed char     " << "\n";   // 1
      os << sizeof(signed int      ) << "B " << "signed int      " << "\n";   // 2
      os << sizeof(signed long     ) << "B " << "signed long     " << "\n";   // 4
      os << sizeof(signed long long) << "B " << "signed long long" << "\n";   // 4
      os << "\n";

      os << sizeof(unsigned char     ) << "B " << "unsigned char     " << "\n";   // 1
      os << sizeof(unsigned int      ) << "B " << "unsigned int      " << "\n";   // 2
      os << sizeof(unsigned long     ) << "B " << "unsigned long     " << "\n";   // 4
      os << sizeof(unsigned long long) << "B " << "unsigned long long" << "\n";   // 4
      os << "\n";

      os << sizeof(float ) << "B " << "float " << "\n";   // 4
      os << sizeof(double) << "B " << "double" << "\n";   // 8
      os << "\n";

      os << sizeof(void* ) << "B " << "void *" << "\n";   // 8

      os << "----------------------------------" << endl;
   UNLOCK;

   return os;
}

ostream& System::showivt(ostream& os, uns2 n)
{
   LOCK;
      os << "======Interrupt=Table=============" "\n";

      for( int i = 0; i < n; i++ )
      {
         // interrupt category
         if     ( i <= 4  ) os << "D";
         else if( i <= 31 ) os << "R";
         else               os << "A";

         // interrupt entry
         os << " " << setw(3) << i  << ": ";
         if( ivt[i] || ivtbak[i] )
         {
            cout << UTIL_PRETTYHEX << FP_SEG(ivt[i] ? ivt[i] : ivtbak[i]) << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(ivt[i] ? ivt[i] : ivtbak[i]) << UTIL_NORMPRINT;
            // dedicated
            if     ( i == IVT_DIVIDE_ERROR )   os << " - " "divide error" "\n";
            else if( i == IVT_SINGLE_STEP  )   os << " - " "single step"  "\n";
            else if( i == IVT_NON_MASKABLE )   os << " - " "non-maskable" "\n";
            else if( i == IVT_BREAKPOINT   )   os << " - " "breakpoint"   "\n";
            else if( i == IVT_OVERFLOW     )   os << " - " "overflow"     "\n";
            // reserved
            else if( i == IVT_TIMER        )   os << " - " "timer"        "\n";
            else if( i == IVT_KEYBOARD     )   os << " - " "keyboard"     "\n";
            // available
            else if( i == IVT_OLD_TIMER    )   os << " - " "old timer"    "\n";
            else                               os <<                      "\n";
         }
         else
         {
            os << "\n";
         }


         // vertical spacing
         if( i%10 == 9 && i != n-1 ) os << "\n";
      }
      os<< "\n";

      // legend
      os << "D-dedicated R-reserved A-available" "\n";
      os << "----------------------------------" << endl;
   UNLOCK;

   return os;
}

// ovaj Scheduler radi kao stek, a ne kao queue! (radi po last in first out principu iz nekog razloga)
// - nije tacno!! ipak pamti koliko puta je uzeta ista nit i vracena u scheduler
ostream& System::showsched(ostream& os, bool drawframe)
{
   LOCK;
      List<PCB>* list = new List<PCB>(false);
      if( !list )
      {
         UNLOCK;
         return os;
      }
      list->setprint(print_pcb_id);


      if( drawframe ) os << "======Scheduler===================" "\n";
      else            os << "scheduler: ";

      // ispisivanje running niti
      if     ( running == main ) { os << "<"    "M"         << ">"; }
      else if( running == idle ) { os << "<"    "I"         << ">"; }
      else if( running )         { os << "<" << running->id << ">"; }
      else                       { os << "<" << "?"         << ">"; }


      // ispisivanje ostalih niti
      {
         for( PCB* pcb = Scheduler::get();  pcb;  )   // prihvatanje stvari iz scheduler-a (od ovoga i ovako ne zavisi ispravnost programa, samo efikasnost)
         {
            if     ( pcb == main ) { os << " "    "M";     }
            else if( pcb == idle ) { os << " "    "I";     }
            else                   { os << " " << pcb->id; }

            list->pushb(pcb);
            pcb = Scheduler::get();
         }
      }

      // vracanje niti u scheduler
      {
         for( PCB* pcb = list->popb();  pcb;  )
         {
            Scheduler::put(pcb);
            pcb = list->popb();   // pop back da bi se lista ponasala kao stek (jer se prvo radi push back)
         }
         delete list; list = nullptr;
      }


      if( drawframe ) os << "\n" "----------------------------------" << endl;
      else            os <<                                              endl;

   UNLOCK;

   return os;
}

Time System::time()
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto se menja u interrupt rutini
      Time time = currtime;
   POPF;

   return time;
}



bool SystemInitialized()
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto cita podatak koji moze da se promeni
      bool inited = System::initialized;
   POPF;

   return inited;
}



void System::lock()
{
   PUSHF; INTD;   // treba da zabrani interrupte, da bi se pravilno lock-ovao blok bez zabrane prekida
      System::context_lockcnt++;
   POPF;
}

void System::unlock()
{
   PUSHF; INTD;   // treba da zabrani interrupte, da bi se pravilno unlock-ovao blok bez zabrane prekida
      if( System::context_lockcnt > 0 && --System::context_lockcnt == 0 )
         if( SystemInitialized() && System::should_switch_context )
            switch_context();
   POPF;
}

void System::unlock_and_switch()
{
   PUSHF; INTD;   // treba da zabrani interrupte, da bi se pravilno smanjio lockcnt i odmah uradio context switch blok bez zabrane prekida
      if( System::context_lockcnt > 0 )
         System::context_lockcnt--;
      if( SystemInitialized() )
         switch_context_uc();   // uvek se bezuslovno uradi context switch na kraju
   POPF;
}





#ifdef DEBUG
   // testiranje sistema
   void systemtest0()
   {
      LOCK;
         cout << "================================================system=test======" "\n";


         for( int i = 0; i < 4; i++ )
         {
               System::exit();
               cout << "system: " << (System::initialized ? "inited" : "not inited") << endl;
               cout << "lockcnt: " << System::context_lockcnt-1 << endl;
               show_allocations();

            cout << endl;

            if( i == 3 ) break;


               System::init();
               show_allocations();

            cout << endl << endl << endl;
         }

         cout << "-----------------------------------------------------------------" << endl;
      UNLOCK;
   }
#endif
