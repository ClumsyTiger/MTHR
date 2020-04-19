#include <dos.h>
#define UTIL_PRINTING_TOOLS
#include "utility.h"
#include "ksemaph.h"
#include "kevent.h"
#include "event.h"


KEvent*    KEvent::kevent[IVT_SIZE];   // array of all events
IntRoutine KEvent::oldivt[IVT_SIZE];   // old ivt
IntRoutine KEvent::regivt[IVT_SIZE];   // registered ivt
volatile bool KEvent::initialized = false;





void KEvent::firstinit()
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
   if( !KEventInitialized() )
   {
      for( int i = 0; i < IVT_SIZE; i++ )
      {
         kevent[i] = nullptr;
         oldivt[i] = nullptr;
         regivt[i] = nullptr;
      }

      initialized = true;
   }
   POPF;
}

void KEvent::rstevent()
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
      if( !KEventInitialized() )
      {
         POPF;
         return;
      }

      for( int i = 0; i < IVT_SIZE; i++ )
      {
         if( kevent[i] )
         {
            delete kevent[i];
            System::setivt(i, oldivt[i]);
            oldivt[i] = nullptr;    // ovo se radi zbog prekidne rutine koja poziva oldivt u sebi
                                    // ne brise se regivt, jer su se tu registrovale nase interrupt funkcije, inace bi smo ih izgubili
         }
      }
   POPF;
}



// samo se u ovoj funkciji (pored konstruktora) radi pocetna inicijalizacija listi null-ovima,
// jer se ne ocekuje da se ova funkcija ikada zove iz prekidne rutine! (vec samo putem nekih sentry-ja)
bool KEvent::regnewintr(ivtno idx, IntRoutine intr)
{
   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
      if( !KEventInitialized() )
         firstinit();

      regivt[idx] = intr;
   POPF;

   return true;
}

volatile uns4 called_old_cnt = 0;   ///////////////////////////////////////////////////////
volatile uns4 called_cnt = 0;   ///////////////////////////////////////////////////////
// ova funkcija se zove iz interrupt rutine napravljene sa PREPAREENTRY
void KEvent::signalevent(ivtno idx)
{
   if( !KEventInitialized() )
      return;

   PUSHF; INTD;   // treba da zabrani interrupte, zato sto se moze zvati i izvan interrupt rutine (treba da se spreci onda da to radi interrupt rutina)
      if( kevent[idx] ) { called_cnt++; kevent[idx]->signal(); }   /////////////////////////////////////
   POPF;
}

// ova funkcija se zove iz interrupt rutine napravljene sa PREPAREENTRY
void KEvent::calloldintr(ivtno idx)
{
   if( !KEventInitialized() )
      return;

   PUSHF; INTD;   // treba da zabrani interrupte, zato sto se moze zvati i izvan interrupt rutine (treba da se spreci onda da to radi interrupt rutina)
      if( oldivt[idx] )
      {
         called_old_cnt++;   ///////////////////////////////////////////
         (oldivt[idx])();
      }
   POPF;
}










KEvent::KEvent(ivtno idx)
{
   if( !SystemInitialized() )   // inicijalizuje sistem ako vec nije, korisno za globalne podatke u vise fajlova (nemaju definisan redosled inicijalizacije)
      System::init();

   // pocetna inicijalizacija zbog destruktora
   event = nullptr;   // wrapper klasa kevent-a se postavi na nullptr
   owner = nullptr;
   sem   = nullptr;
   this->idx = idx;



   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
      // ukoliko nije uradjen first init, uradi se ovde
      if( !KEventInitialized() )
         firstinit();

      // ukoliko postoji vec event sa tim id-jem ne moze da se napravi novi
      // ukoliko ne postoji registrovana prekidna rutina, nema poente praviti novi event
      if( kevent[idx] || !regivt[idx] )
      {
         DELETE_THIS;

         POPF;
         return;
      }
   POPF;



   LOCK;
   owner = (PCB*)System::running;   // ne mora da se proverava da li postoji, zato sto se event uvek pravi unutar postojeceg thread-a
   sem = new KSemaphore(0);   // cim se pozove wait iz owner niti odmah se zablokira na semaforu
   UNLOCK;

   if( !owner || !sem ) DELETE_THIS_AND_RETURN;


   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
      kevent[idx] = this;
      oldivt[idx] = System::getivt(idx);
      System::setivt(idx, regivt[idx]);   // kada se event napravi, on registruje nasu interrupt rutinu u iv tabelu
   POPF;



   // debugging
   #ifdef DEBUG
      LOCK;
      cout << "New event:\n" << *this << "\n";
      UNLOCK;
   #endif
}

KEvent::~KEvent()
{
   // debugging
   #ifdef DEBUG
      LOCK;
         cout << "Deleting event:\n" << *this << "\n";
      UNLOCK;
   #endif



   PUSHF; INTD;   // treba da zabrani interrupte, zato sto petlja sa kopijom interrupt tabele
      if( event )
      {
         event->kevent = nullptr;
         delete event; event = nullptr;
      }

      if( sem ) { delete sem; sem = nullptr; }


      if( SystemInitialized() )   // samo ako sistem i dalje radi treba vratiti staru prekidnu rutinu
         System::setivt(idx, oldivt[idx]);

      kevent[idx] = nullptr;
      oldivt[idx] = nullptr;    // ovo se radi zbog prekidne rutine koja poziva oldivt u sebi (inace ne bi moralo)
    //regivt[idx] = nullptr;    // ovo se ne radi zato sto bi u suprotnom izgubili referencu na globalnu funkciju napravljenu sa PREPAREENTRY!
   POPF;
}



ostream& KEvent::print(ostream& os) const
{
   LOCK;

      os << "======" << "Event" << "<" << (uns2)idx << ">" << "====================" "\n";

      os << "kevent[" << (uns2)idx << "]: ";

         if( kevent[idx] ) os << (uns2)(kevent[idx]->idx) << "\n";
         else              os << "?"                         "\n";


      os << "oldivt[" << (uns2)idx << "]: " << UTIL_PRETTYHEX << FP_SEG(oldivt[idx]) << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(oldivt[idx]) << UTIL_NORMPRINT << "\n";
      os << "regivt[" << (uns2)idx << "]: " << UTIL_PRETTYHEX << FP_SEG(regivt[idx]) << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(regivt[idx]) << UTIL_NORMPRINT;

         PUSHF; INTD;
            bool ivt_idx_set = ( getvect((uns2)idx) == regivt[idx] );
         POPF;

         if( ivt_idx_set ) os << " - set"      "\n";
         else              os << " - not set!" "\n";


      if( owner ) os << "owner: thread<" << owner->id << ">\n";
      else        os << "owner: thread<"    "?"          ">\n";

      if( sem )   { os << "event sem<" << sem->id << ">: "; sem->print_list(os) << "\n"; }
      else        { os << "event sem<" << "?"        ">"                           "\n"; }


      os << "----------------------------------" << endl;

   UNLOCK;
   return os;
}

ostream& operator <<(ostream& os, const KEvent& e)
{
   return e.print(os);
}



void KEvent::wait()
{
   PUSHF; INTD;   // ne treba da zabrani interrupte, zato sto ne bi trebalo da se poziva izvan interrupt rutine, ali za svaki slucaj (posto postoji u Event-u i javna je metoda)
   if( System::running == owner )   // iz nekog razloga, moze samo jedna nit da poseduje neki event
      sem->wait();
   POPF;
}

void KEvent::signal()
{
   LOCK;   // treba da se lock-uje, da bi se desio unlock i u slucaju da ova funkcija nije ugnjezdena ni u jedan lock (inace se ne bi pozvala funkcija za promenu konteksta, jer nju zove unlock)
      PUSHF; INTD;   // treba da zabrani interrupte, zato sto moze da se pozove izvan interrupt rutine, onda treba da zabrani da se desi interrupt da bi semafor ostao u konzistentnom stanju
         if( sem->signalAll() > 0 )                 // samo ako se nit zablokirala na semaforu od eventa ce se menjati kontekst kada se restaurira
            System::should_switch_context = true;   // da bi event bio preemptive, mora da se zameni kontekst nakon svih unlock-ova
      POPF;
   UNLOCK;   // mora da se unlock-uje
}



bool KEventInitialized()
{
   PUSHF; INTD;   // treba da zabrani interrupte, da bi ova funkcija mogla da se zove i unutar i izvan prekidne rutine
      bool inited = KEvent::initialized;
   POPF;

   return inited;
}
