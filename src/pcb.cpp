#define UTIL_PRINTING_TOOLS
#include "system.h"
#include "schedule.h"
#include "delegate.h"
#include "pcb.h"
#include "thread.h"
#include "ksemaph.h"


const StackSize PCB::def_stack_size = 4096;

const Time PCB::inf_quant = 0,
           PCB::def_quant = 2;


const Flags PCB::start    = 0,
            PCB::ready    = 1,
            PCB::running  = 2,
            PCB::blocked  = 3,
            PCB::finished = 4;

const ThreadId PCB::main_id    = 0,
               PCB::idle_id    = 1,
               PCB::unknown_id = ~0UL;

const SignalMask PCB::def_signal_mask = ~0U;

const SignalId PCB::terminate_id    = 0,
               PCB::notifyparent_id = 1,
               PCB::finished_id     = 2;


ThreadId PCB::idcnt = 0;
SignalMask PCB::gsignal_mask = ~0U;



PCB::PCB(Runnable run, Time quant, StackSize stack_size)
{
   if( !SystemInitialized() )   // inicijalizuje sistem ako vec nije, korisno za globalne podatke u vise fajlova (nemaju definisan redosled inicijalizacije)
      System::init();

   // pocetna inicijalizacija zbog destruktora
   t = nullptr;   // wrapper klasa pcb-ja se postavi na nullptr
   stack     = nullptr;
   blockedon = nullptr;
   sem4del   = nullptr;
   signal_queue    = nullptr;
   signal_delegate = nullptr;



   // thread identification
   id = idcnt++;

   // thread resources
   if( run )
   {
      LOCK;
         this->run        = run;
         this->stack      = new uns2[stack_size];
         this->stack_size = stack_size;
      UNLOCK;

      if( !stack )
         DELETE_THIS_AND_RETURN;
   }
   else
   {
      this->run        = nullptr;
      this->stack      = nullptr;   // ako se pravi PCB main thread-a, ne treba da mu mi alociramo i dealociramo stek
      this->stack_size = 0;         // velicina steka nam je tada nepoznata, tj. 0
   }



   // general purpouse registers
   ax = bx = cx = dx = si = di = 0;   // na pocetku su mogli da imaju undefined vrednosti



   // code segment, program counter and program status word
   if( run )
   {
      cs = FP_SEG(run);
      pc = FP_OFF(run);

      // PSW flags:                0 0 0 0 O D I T      S Z 0 A 0 P 0 C
      // + overflow + direction + interrupt + trap      + sign + zero + auxiliary carry + parity + carry
      psw = 0x200;   // na pocetku je ukljucen interrupt flag u psw-u
   }
   else
   {
      //  __<dno steka>___   // brojevi relativno u odnosu na BP'
      //   |    ...      |
      //+20| stack_size H|
      //+18| stack_size  |
      //+16| quant      H|
      //+14| quant       |
      //+12| run        H|
      //+10| run         |
      // +8| this*      H|
      // +6| this*       |
      //  ________________ call
      // +4| CS          |
      // +2| PC          |
      //  ________________ called
      // +0| BP          | « BP' « SP
      // -2| ...         |

      // ako se pravi pcb bez metode run, onda se pravi sa kopijom svih registara cpu-a, jer se smatra da se pravi nad trenutnim kontekstom
      REG temp_reg;

      asm mov ax, [bp+4];   // code segment
      asm mov temp_reg, ax; cs = temp_reg;

      asm mov ax, [bp+2];   // program counter
      asm mov temp_reg, es; pc = temp_reg;

      asm pushf; asm pop ax;
      asm mov temp_reg, ax; psw = temp_reg;   // flags
   }



   // stack segment, base pointer and stack pointer
   if( stack )
   {
      #define PCB_START_STACK_WORDCNT 18
      #define PCB_START_STACK_SAFETY  6

      ss = FP_SEG(stack + stack_size - PCB_START_STACK_WORDCNT);   // stack segment
      bp = FP_OFF(stack + stack_size - PCB_START_STACK_WORDCNT);   // offset od stack segmenta
      sp = bp;
   }
   else
   {
      #define PCB_ARG_WORDCNT 6   // 1W + 1W   +   1W = 6B   (PC, CS   +   first word before CS)   // SP before entering called routine
      REG temp_reg;

      asm mov temp_reg, ss; ss = temp_reg;

      asm mov ax, [bp];   // old base pointer, before entering called routine
      asm mov temp_reg, ax; bp = temp_reg;

      asm mov ax, bp;    // old stack pointer, before entering called routine
      asm add ax, PCB_ARG_WORDCNT;
      asm mov temp_reg, ax; bp = temp_reg;
   }



   // data segment and extra segment
   {
      REG temp_reg;

      asm mov temp_reg, ds; ds = temp_reg;
      es = 0;
   }



   // stack
   if( stack )
   {
      StackSize top = stack_size-1;   // vrh steka, odatle krece popunjavanje pocetnog konteksta (ovako prekidna rutina stavlja stvari na stek)
      //__<dno steka>___    // _________   // (brojevi u velicini od 2B)
      stack[top--] = -1;    // 0| ff  |
      stack[top--] = -1;    // 1| ff  |
      stack[top--] = -1;    // 2| ff  |
      stack[top--] = -1;    // 3| ff  |
      stack[top--] = -1;    // 4| ff  |
      stack[top--] = -1;    // 5| ff  | « BP // ovaj deo je tu za svaki slucaj, da bi se lakse detektovao stack underflow


      stack[top--] = psw;   // 6| PSW |
      stack[top--] = cs;    // 7| CS  |
      stack[top--] = pc;    // 8| PC  |

      stack[top--] = ax;    // 9| AX  |
      stack[top--] = bx;    //10| BX  |
      stack[top--] = cx;    //11| CX  |
      stack[top--] = dx;    //12| DX  |

      stack[top--] = es;    //13| ES  |
      stack[top--] = ds;    //14| DS  |
      stack[top--] = si;    //15| SI  |
      stack[top--] = di;    //16| DI  |

      REG oldBP = FP_OFF(stack + stack_size - PCB_START_STACK_SAFETY);
      stack[top--] = oldBP; //17| BP  | « BP' « SP
   }





   // alokacije
   LOCK;
      // thread join and delete
      sem4del = new KSemaphore(0);

      // signals
      signal_queue    = new List<SignalId>(true);   // signal queue jeste vlasnik svojih kontejnera signal id-jeva
      signal_delegate = new Delegate*[SIGNAL_TABLE_SIZE];

      // ukoliko je alociran niz pointera na delegate, mogu se alocirati i sami delegati
      if( signal_delegate )
      {
         // zasto ne lock samo oko new-a ? zato sto nema puno alokacija (samo 16)
         for( int2 i = 0; i < SIGNAL_TABLE_SIZE; i++ )
         {
            if( System::running ) signal_delegate[i] = System::running->signal_delegate[i]->copy();
            else                  signal_delegate[i] = new Delegate();
         }
      }

   UNLOCK;


   // provera da li su alokacije uspesne
   if( !sem4del || !signal_queue || !signal_delegate )
      DELETE_THIS_AND_RETURN;

   for( int i = 0; i < SIGNAL_TABLE_SIZE; i++ )
      if( !signal_delegate[i] )   // ako bar jedan od delegata nije napravljen, to je greska
         DELETE_THIS_AND_RETURN;



   // tek sada treba dodati pcb u globalnu listu pcb-jeva
   LOCK;
      if( SystemInitialized() )
      {
         System::pcbs->pushs(this);   // smatra se da niko nece praviti pcb-jeve (ili thread-ove) pre podizanja sistema - ako se to desi, donji blok resava problem
      }
      else
      {
         UNLOCK;
         DELETE_THIS_AND_RETURN;   // moze posle unlock-a, jer konstruktor poziva samo jedna nit (pcb koji se konstruise nije deljen!)
      }
   UNLOCK;





   // scheduler stuff
   this->quant = quant;

   if( run && id != main_id && id != idle_id ) state = start;   // ako je navedena funkcija i id nije main ni idle, state je start (nije jos pokrenut thread)
   else if( !run || id == main_id ) state = running;           // ako se pravi main thread, odmah mu se stavlja da je running (ili ako se kontekst niti pravi unutar same niti, mada to ne treba raditi osim za main)
   else if(         id == idle_id ) state = ready;             // ako se pravi idle thread, odmah mu se stavlja da je ready

   timestamp = ( state == running ) ? System::time() + quant : 0;

   LOCK;   // mora lock jer se radi nad deljenim resursom
      if( state != start && state != finished )
         System::activecnt++;   // sistemska nit se odmah pravi da bude running, tj. active (active su sve niti koje nisu u stanju start ili finished)
   UNLOCK;


   // context locking
   context_lockcnt = 0;
   blockedon = nullptr;
   timeexpired = false;


   LOCK;
      // thread join and delete
      parent      = ( System::running ) ? System::running->id          : unknown_id;

      // signals
      signal_queue->setmap(map_signal);
      signal_queue->setprint(print_signal);
      signal_queue->setdelete(delete_signal);

      signal_mask = ( System::running ) ? System::running->signal_mask : def_signal_mask;
      if( !System::running )
         signal_delegate[0]->add(pcb_default_signal0);   // default signal handler za signal 0, treba ga dodati samo onim thread-ovima koji nemaju parent-a
   UNLOCK;






   // debugging
   #ifdef DEBUG
      LOCK;
      cout << "New thread:\n" << *this << "\n";
      UNLOCK;
   #endif
}

PCB::~PCB()
{
   // debugging
   #ifdef DEBUG
      LOCK;
      cout << "Deleting thread:\n" << *this << "\n";
      UNLOCK;
   #endif



   LOCK;
      if( t )
      {
         t->pcb = nullptr;
         delete t; t = nullptr;
      }

      if( stack ) { delete[] stack; stack = nullptr; }
      if( sem4del ) { delete sem4del; sem4del = nullptr; }   // delete od semafora vraca sve blokirane niti u Scheduler!
      if( signal_queue ) { delete signal_queue; signal_queue = nullptr; }
      if( signal_delegate )
      {
         for( int i = 0; i < SIGNAL_TABLE_SIZE; i++ )
            if( signal_delegate[i] )
               delete signal_delegate[i];

         delete[] signal_delegate; signal_delegate = nullptr;
      }

      LOCK;
         if( SystemInitialized() && state != start && state != finished )   // ako je nit active, smanjuje activecnt
            System::activecnt--;
      UNLOCK;
      state = finished;



      if( SystemInitialized() )
      {
         System::pcbs->find( (void*)&id );
         System::pcbs->pop();

         // ukoliko ne postoji running thread ili postoji, ali je to thread koji treba obrisati, mora da se uradi promena konteksta (osim ako je main thread u pitanju)
         if( !System::running || System::running->id == id )
         {
            if( id != PCB::main_id && id != PCB::idle_id )   // samo main i idle niti imaju pravo da obrisu svoj pcb i da im se ne promeni kontekst
            {
               System::running = nullptr;
               switch_context_uc();                          // ako je nit uspela nekako sama sebe da obrise, mora da se uradi odmah context switch, da se ne bi radilo sa neispravnim kontekstom
            }
         }
      }
   UNLOCK;
}



//bool PCB::transition_between_states(volatile PCB* pcb, Flags curr_state, Flags next_state)
//{
//   LOCK;
//      if( !pcb || pcb->state != curr_state )
//      {
//         UNLOCK;
//         return false;
//      }
//
//      // ready, running i blocked <=> active state
//
//      // start -> ready <-> running -> finished
//      //           ↑  blocked  ↓
//
//
//      bool state_changed = true;
//      if(      curr_state == PCB::start   && next_state == PCB::ready    )   // start -> ready
//      {
//      }
//
//      else if( curr_state == PCB::ready   && next_state == PCB::running  )   // ready -> running
//      {
//         pcb->state      = next_state;
//         pcb->timestamp  = System::currtime + pcb->quant;   // da li je PCB::inf_quant se proverava u tick-u, ovde nije bitno
//         System::running = pcb;
//      }
//      else if( curr_state == PCB::running && next_state == PCB::ready    )   // running -> ready
//      {
//         pcb->state       = next_state;
//         pcb->timestamp   = 0;
//         pcb->timeexpired = false;   // moramo da znamo da li se nit odblokirala jer joj je istekao max wait period  ili zbog poziva signal-a, zato ovo polje ne sme da se resetuje u ready->running
//
//         if( pcb != System::idle )
//            Scheduler::put((PCB*)pcb);   // postaje ready
//      }
//
//      else if( curr_state == PCB::running && next_state == PCB::blocked  )   // running -> blocked
//      {
//      }
//      else if( curr_state == PCB::blocked && next_state == PCB::ready    )   // blocked -> ready
//      {
//      }
//
//      else if( curr_state == PCB::running && next_state == PCB::finished )   // running -> finished
//      {
//      }
//      else
//      {
//         state_changed = false;
//      }
//
//   UNLOCK;
//   return state_changed;
//}



ostream& PCB::print(ostream& os, bool verbose) const
{
   LOCK;

      // thread identification
      os << "======" << "Thread" << "<";

         if     ( id == PCB::main_id ) os << "M";
         else if( id == PCB::idle_id ) os << "I";
         else                          os << id;

      os << ">" << "===================" "\n";


      if( verbose )
      {
         // thread resources
         os << "run:   " << UTIL_PRETTYHEX << FP_SEG(run)   << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(run)   << "\n";
         os << "stack: " << UTIL_PRETTYHEX << FP_SEG(stack) << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(stack) << " [" << UTIL_NORMPRINT << stack_size << "]" << "\n";

         // thread state
         os << "\n";
         os << "ax:" << UTIL_PRETTYHEX << ax  << " "
            << "bx:" << UTIL_PRETTYHEX << bx  << " "
            << "cx:" << UTIL_PRETTYHEX << cx  << " "
            << "dx:" << UTIL_PRETTYHEX << dx  << " "
            << "si:" << UTIL_PRETTYHEX << si  << " "
            << "di:" << UTIL_PRETTYHEX << di  << "\n";

         os << "pc:" << UTIL_PRETTYHEX << pc  << " "
            << "sw:" << UTIL_PRETTYHEX << psw << "\n";

         os << "bp:" << UTIL_PRETTYHEX << bp  << " "
            << "sp:" << UTIL_PRETTYHEX << sp  << "\n";

         os << "cs:" << UTIL_PRETTYHEX << cs  << " "
            << "ss:" << UTIL_PRETTYHEX << ss  << " "
            << "ds:" << UTIL_PRETTYHEX << ds  << " "
            << "es:" << UTIL_PRETTYHEX << es  << "\n";

         os << UTIL_NORMPRINT << "\n";
      }


      // scheduler data
      os << "quant: " << quant << "\n";
      os << "state: " << ((state  == PCB::start   ) ? "start" :
                          (state  == PCB::ready   ) ? "ready" :
                          (state  == PCB::running ) ? "running" :
                          (state  == PCB::blocked ) ? "blocked" :
                          (state  == PCB::finished) ? "finished" :
                                                      "?"
                         );

      if( timestamp > 0 ) os << " " "until " << timestamp << " (time: " << System::time() << ")" "\n";
      else                os <<                                                                  "\n";

      os << "\n";


      // context locking
      if( blockedon )   os << "blocked on: semaph<" << blockedon->id << "> lockcnt: " << context_lockcnt-1 << "\n";
      if( timeexpired ) os << "> max block time expired"                                                      "\n";

      // thread join and delete
      if( sem4del ) { os << "sem4del<" << sem4del->id << ">: "; sem4del->print_list(os) << "\n"; }
      else          { os << "sem4del<"    "?"            ">"                               "\n"; }

      if( parent != unknown_id ) os << "parent: " << parent << "\n";
      else                       os << "parent: "    "?"       "\n";

      os << "\n";


      // signals
      if( signal_queue ) { os << "signals: "; signal_queue->print(os, false) << "\n"; }

      for( uns2 i = 0; i < SIGNAL_TABLE_SIZE; i++ )
         if( signal_delegate[i] && signal_delegate[i]->size() > 0 )
            { os << "delegate[" << i << "]: "; signal_delegate[i]->print(os) << "\n"; }

      {
         os << "mask:  ";
         for( int2 i = sizeof(SignalMask)*8 - 1; i >= 0; i-- )
         {
            if( i%8 == 7 ) os << " ";
            os << ((signal_mask & (1 << i)) ? 1 : 0);
         }
         os << "\n";
      }

      {
         os << "mask_g:";
         for( int2 i = sizeof(SignalMask)*8 - 1; i >= 0; i-- )
         {
            if( i%8 == 7 ) os << " ";
            os << ((gsignal_mask & (1 << i)) ? 1 : 0);
         }
         os << "\n";
      }

      os << "----------------------------------" << endl;

   UNLOCK;
   return os;
}
ostream& operator <<(ostream& os, const PCB& pcb)
{
   #ifdef DIAGNOSTICS
      return pcb.print(os, true);
   #else
      return pcb.print(os);
   #endif
}



void PCB::start_thread()
{
   LOCK;
      if( state == PCB::start )
      {
         state = PCB::ready;
         Scheduler::put(this);
         System::activecnt++;
      }
   UNLOCK;
}

void PCB::wait_to_complete() const
{
   LOCK;
      // nit pozivalac: A
      // pozvana nit:   B
      // ako A postoji, A razlicita nit od B i nit B je aktivna (nije u stanju start ili finished), tada se A blokira na B, dok B ne zavrsi rad
      while( System::running && System::running->id != id   &&   state != PCB::start && state != PCB::finished )
         sem4del->wait();
   UNLOCK;
}



ThreadId PCB::getid() const
{
   LOCK;
      ThreadId id = this->id;
   UNLOCK;

   return id;
}

ThreadId PCB::get_running_id()
{
   LOCK;
      // niti koje nisu main i idle nece moci da dohvate main ili idle (sto je zabranjeno) na ovaj nacin, jer nece moci da pozovu ovu funkciju u kontekstu main ili idle (mogu samo da je zovu iz svog konteksta)
      ThreadId id = ( System::running ) ? System::running->id : unknown_id;
   UNLOCK;

   return id;
}



PCB* PCB::getpcb(ThreadId id)
{
   LOCK;
      if( id >= idcnt || id == PCB::idle_id )   // ne moze se dohvatiti idle nit, to je zabranjeno
      {
         UNLOCK;
         return nullptr;
      }

      PCB* pcb = System::pcbs->find( (void*)&id );

   UNLOCK;
   return pcb;
}

PCB* PCB::get_running_pcb()
{
   LOCK;
      PCB* running = (PCB*) System::running;
   UNLOCK;

   return running;
}






void PCB::signal(ThreadId tid, SignalId sid)
{
   LOCK;
      if( tid >= idcnt || sid >= SIGNAL_TABLE_SIZE )   // ako ne postoji trazeni thread ili signal
      {
         UNLOCK;
         return;
      }


      PCB* pcb = (PCB*) System::running;
      if( pcb && pcb->id == tid )                      // ako nit sama u sebi izazove signal, na tom mestu ce se i izvrsiti
      {
         if   // ako nit sama u sebi posalje terminate signal, sigurno nije blokirana ni na jednom semaforu
         (  ( sid != notifyparent_id && sid != finished_id   &&   pcb->isallowed_signal(sid) && PCB::isallowed_signal_g(sid) )   // svi signali osim notifyparent i finished se mogu slati ako nisu blokirani
         || (                           sid == finished_id   &&   pcb->state == finished )  )                                    // signali notify_parent i finished se mogu slati samo od strane sistema, tj. nakon sto je thread koji ih salje zavrsio
         {
            pcb->signal_delegate[sid]->signal();
         }

      }
      else if( (pcb = PCB::getpcb(tid)) != nullptr )   // inace se id signala postavi u signal queue druge niti<tid> kojoj se poslao signal
      {
         if( sid == terminate_id && pcb->blockedon )
            pcb->blockedon-> unblock(pcb);   // ako je nit kojoj se salje terminate bila blokirana, izbaci se iz semafora

         if   // signal se ubacuje u listu signala niti, obradice je delegat kada nit dodje na red u promeni konteksta
         (  ( sid != notifyparent_id && sid != finished_id   &&   pcb->isallowed_signal(sid) && PCB::isallowed_signal_g(sid) )   // svi signali osim notifyparent i finished se mogu slati ako nisu blokirani
         || ( sid == notifyparent_id                         &&   pcb->state == finished )  )                                    // signali notify_parent i finished se mogu slati samo od strane sistema, tj. nakon sto je thread koji ih salje zavrsio
         {
            pcb->signal_queue->pushb( new SignalId(sid) );
         }

      }

   UNLOCK;
}

void PCB::acknowledge()
{
   LOCK;
      if( System::running && System::running->signal_queue )
         System::running->signal_queue->map(nullptr, true);
   UNLOCK;
}



void PCB::register_handler(SignalId id, SignalHandler h)
{
   LOCK;   // zasto lock ? za svaki slucaj (mozda postoji neki nacin na koji bi signal delegate mogao da bude obrisan, a da ga je poceo koristiti drugi thread)
      if( id < SIGNAL_TABLE_SIZE )
         signal_delegate[id]->add(h);   // signal_delegate[id] mora da postoji unutar konteksta pcb-ja, dok se ne unisti pcb
   UNLOCK;
}

void PCB::unregister_handlers(SignalId id)
{
   LOCK;   // zasto lock ? za svaki slucaj (mozda postoji neki nacin na koji bi signal delegate mogao da bude obrisan, a da ga je poceo koristiti drugi thread)
      if( id < SIGNAL_TABLE_SIZE )
         signal_delegate[id]->clear();
   UNLOCK;
}

void PCB::swap_handlers(SignalId id, SignalHandler h1, SignalHandler h2)
{
   LOCK;   // zasto lock ? za svaki slucaj (mozda postoji neki nacin na koji bi signal delegate mogao da bude obrisan, a da ga je poceo koristiti drugi thread)
      if( id < SIGNAL_TABLE_SIZE )
         signal_delegate[id]->swap(h1, h2);
   UNLOCK;
}



void PCB::allow_signal(SignalId id)
{
   LOCK;   // zasto lock iako je lokalni podatak ? zato sto neka druga nit moze da zove block i unblock nad ovom niti
      if( id < SIGNAL_TABLE_SIZE )
         signal_mask |= (1U << id);
   UNLOCK;
}

void PCB::allow_signal_g(SignalId id)
{
   LOCK;
      if( id < SIGNAL_TABLE_SIZE )
         gsignal_mask |= (1U << id);
   UNLOCK;
}

void PCB::block_signal(SignalId id)
{
   LOCK;   // zasto lock iako je lokalni podatak ? zato sto neka druga nit moze da zove block i unblock nad ovom niti
      if( id < SIGNAL_TABLE_SIZE )
         signal_mask &= ~(1U << id);
   UNLOCK;
}

void PCB::block_signal_g(SignalId id)
{
   LOCK;
      if( id < SIGNAL_TABLE_SIZE )
         gsignal_mask &= ~(1U << id);
   UNLOCK;
}



bool PCB::isallowed_signal(SignalId id)
{
   LOCK;   // zasto lock iako je lokalni podatak ? zato sto druga nit moze da zove block i unblock nad ovom niti
      bool allowed = (signal_mask & (1U << id)) ? true : false;
   UNLOCK;

   return allowed;
}

bool PCB::isallowed_signal_g(SignalId id)
{
   LOCK;
      bool allowed = (gsignal_mask & (1U << id)) ? true : false;
   UNLOCK;

   return allowed;
}

bool PCB::isblocked_signal(SignalId id)
{
   LOCK;   // zasto lock iako je lokalni podatak ? zato sto druga nit moze da zove block i unblock nad ovom niti
      bool blocked = (signal_mask & (1U << id)) ? false : true;
   UNLOCK;

   return blocked;
}

bool PCB::isblocked_signal_g(SignalId id)
{
   LOCK;
      bool blocked = (gsignal_mask & (1U << id)) ? false : true;
   UNLOCK;

   return blocked;
}






// pcb list tools
int2 compare_pcb_id(PCB* p1, PCB* p2)   // poredi dva pcb-ja na osnovu id-jeva
{
   if( !p1 || !p2 )       return INCOMP;

   if( p1->id <  p2->id ) return LESS;
   if( p1->id == p2->id ) return EQUAL;
                          return GREATER;
}

int2 compare_pcb_timestamp(PCB* p1, PCB* p2)   // poredi dva pcb-ja na osnovu waituntil polja
{
   if( !p1 || !p2 )                     return INCOMP;

   if( p1->timestamp <  p2->timestamp ) return LESS;
   if( p1->timestamp == p2->timestamp ) return EQUAL;
                                        return GREATER;
}

bool match_pcb_id(PCB* p, void* args)   // args je u stvari id pcb-ja koga trazimo
{
   if( !p || !args ) return false;
   if( p->id == *((ThreadId*)args) ) return true;

   return false;
}


void print_pcb(ostream& os, const PCB* p)   // ispisuje pcb na izlazu
{
   #ifdef DIAGNOSTICS
      if( p ) p->print(os, true);
   #else
      if( p ) p->print(os);
   #endif
}

void print_pcb_id(ostream& os, const PCB* p) { if( p ) os << p->id; }   // ispisuje id pcb-ja na izlazu
void delete_pcb(PCB* p)                      { if( p ) delete p;    }   // zove destruktor pcb-a



// signal list tools
bool map_signal(SignalId* s, void*)
{
   if( !s || *s > SIGNAL_TABLE_SIZE ) return true;   // treba vratiti true, da bi map izbacio taj pogresan request iz signal_queue-a

   PCB* pcb = (PCB*) PCB::get_running_pcb();
   if( !pcb || pcb->signal_delegate[*s] ) return true;

   if( pcb->isallowed_signal(*s) && !PCB::isallowed_signal_g(*s) )
   {
      pcb->signal_delegate[*s]->signal();
      return false;
   }

   return false;   // request je blokiran, cuva se u signal_queue-u da bi se mozda obradio kasnije
}

void print_signal(ostream& os, const SignalId* s) { if( s ) os << *s; }   // ispisuje signal na izlazu
void delete_signal(SignalId* s)                   { if( s ) delete s; }   // zove destruktor signala (pokazivac na uns2)




// default signal 0
void pcb_default_signal0()
{
   LOCK;
      PCB* pcb = (PCB*) System::running;
      if( pcb && pcb->id != PCB::main_id )   // main thread ne moze da se ubije, jer on mora da deinicijalizuje sistem
      {
         PCB::signal(pcb->parent, PCB::notifyparent_id);   // prvo se obavestava parent, da bi bili sigurni da ce se obavestiti
         PCB::signal(pcb->id,     PCB::finished_id    );   // moze da se desi da finished signal obrise thread - potencijalno rekurzija (a i zbog toga sto nije regularan izlazak, mozda ne bi trebalo da se rade zavrsne radnje)

         if( pcb->state != PCB::start && pcb->state != PCB::finished )
            System::activecnt--;
         pcb->state = PCB::finished;   // da se ne bi cekao wait_to_complete
         pcb->~PCB();   // ovde se nit unistava i postavlja running na nullptr, oslobadjajuci resurse, nakon toga se radi switch_context_uc()
      }
   UNLOCK;   // nikad se nece doci do ove instrukcije (osim ako se radi o main thread-u), stavljena zbog simetrije
}
