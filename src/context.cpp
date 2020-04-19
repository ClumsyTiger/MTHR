#define UTIL_PRINTING_TOOLS
#include "system.h"
#include "schedule.h"
#include "pcb.h"
#include "ksemaph.h"


// deklaracije u context.cpp-u
void           switch_context();
void interrupt switch_context_uc();



// prekidna rutina, zbog toga sto pise interrupt se na stek stavlja PSW, CS i PC (2B, 2B, 2B, tim redom) ? samo to ?
void interrupt timer_tick()
{
   // poziv stare prekidne rutine koja se nalazila na ulazu 8, a sad je na ulazu 96
   asm int IVT_OLD_TIMER;   // ulaz 96 - prvi slobodan ulaz u iv tabeli
   if( !SystemInitialized() ) return;


   // prosao je jedan tick
   System::currtime++;
   extern void tick();
   tick();



   // izbacivanje pcb-jeva iz globalne liste semafora ako su prosli timestamp do kada moraju da cekaju
   for( PCB* blocked = KSemaphore::glist->peekf();  blocked && System::currtime >= blocked->timestamp;  )
   {
      blocked->blockedon-> unblock(blocked);
      blocked = KSemaphore::glist->peekf();
   }


   // provera da li treba uslovno promeniti kontekst
   if( !System::running || System::running->quant != PCB::inf_quant )
      if( System::currtime >= System::running->timestamp )
         switch_context();
}



// uslovna promena konteksta
void switch_context()
{
   PUSHF; INTD;   // treba da zabrani interrupte, da bi se funkcija ponasala kao int rutina
      if( !SystemInitialized() ) { POPF; return; }
      System::should_switch_context = true;

      // ustanovljeno da treba da se vrsi promena konteksta, i dozvoljena je ili ne postoji trenutni running thread
      if( System::should_switch_context && (!System::running || !System::context_lockcnt) )
         switch_context_uc();

   POPF;
}



// trenutno mora da bude interrupt jer se na steku ocekuju registri ax, bx, ... onako kako ih poziv interrupt rutine postavi, a i zato sto ce da zakljuca interrupt-e
// bezuslovna promena konteksta
void interrupt switch_context_uc()
{
   static volatile PCB *curr, *next;

   System::should_switch_context = false;
   if( !SystemInitialized() ) return;
   // sada smeju da se zovu funkcije koje u sebi imaju LOCK i UNLOCK (ali ne UNLOCK_SW!), zato sto je should_context_switch = false
   // inace da nije mozda bi se greskom opet pozvao context switch unutar ovog context switch-a, sto nije dobro




   // start -> ready <-> running -> finished
   //           ↑  blocked  ↓

   // scheduler - currently running thread
   curr = System::running;
   if( curr )
   {
      curr->context_lockcnt = System::context_lockcnt;
      curr->timeexpired = false;   // moramo da znamo da li se nit odblokirala jer joj je istekao max wait period ili zbog poziva signal-a, zato ovo polje ne sme da se resetuje u prelazu stanja ready->running

      if( curr->state == PCB::running )
      {
         curr->state       = PCB::ready;
         curr->timestamp   = 0;

         if( curr != System::idle )
            Scheduler::put((PCB*)curr);   // postaje ready
      }
   }
   //PCB::transition_between_states(curr, PCB::running, PCB::ready);



   // scheduler - next running thread
   next = Scheduler::get();
   if( !next ) next = System::idle;   // samo ako ne postoji bas ni jedna nit koja je ready se uzima idle nit
   if( next )   // sistem moze da deinicializuje idle nit i da je postavi na nullptr
   {
      next->state     = PCB::running;
      next->timestamp = System::currtime + next->quant;   // da li je PCB::inf_quant se proverava u tick-u, ovde nije bitno
      System::running = next;
      System::context_lockcnt = next->context_lockcnt;
   }
   //PCB::transition_between_states(curr, PCB::ready, PCB::running);



   // ako ima potrebe da se stvarno radi promena konteksta (da se kopiraju registri)
   if( next != curr )
   {
      static volatile REG temp_reg;   // mora da bude static -- u 'longjmp'-u se stvari nepravilno skidaju sa steka tek nastalog PCB-ja


      // setjmp, saving current context
      if( curr != nullptr )
      {
                              asm mov temp_reg, ss; curr->ss  = temp_reg;   // cuvanje stack-related registara
                              asm mov temp_reg, bp; curr->bp  = temp_reg;
                              asm mov temp_reg, sp; curr->sp  = temp_reg;


         //_____...________                                                 // _________   // (brojevi relativno u odnosu na BP')
         asm mov ax, [bp+22]; asm mov temp_reg, ax; curr->psw = temp_reg;   //+22| PSW |
         asm mov ax, [bp+20]; asm mov temp_reg, ax; curr->cs  = temp_reg;   //+20| CS  |
         asm mov ax, [bp+18]; asm mov temp_reg, ax; curr->pc  = temp_reg;   //+18| PC  |

         asm mov ax, [bp+16]; asm mov temp_reg, ax; curr->ax  = temp_reg;   //+16| AX  |
         asm mov ax, [bp+14]; asm mov temp_reg, ax; curr->bx  = temp_reg;   //+14| BX  |
         asm mov ax, [bp+12]; asm mov temp_reg, ax; curr->cx  = temp_reg;   //+12| CX  |
         asm mov ax, [bp+10]; asm mov temp_reg, ax; curr->dx  = temp_reg;   //+10| DX  |

         asm mov ax, [bp+ 8]; asm mov temp_reg, ax; curr->es  = temp_reg;   // +8| ES  |
         asm mov ax, [bp+ 6]; asm mov temp_reg, ax; curr->ds  = temp_reg;   // +6| DS  |
         asm mov ax, [bp+ 4]; asm mov temp_reg, ax; curr->si  = temp_reg;   // +4| SI  |
         asm mov ax, [bp+ 2]; asm mov temp_reg, ax; curr->di  = temp_reg;   // +2| DI  |

         //                                                                 // +0| BP  | « BP'
         //                                                                 // ..|     | « SP
      }


      // longjmp, restoring next context
      if( next != nullptr )
      {
         temp_reg = next->ss;  asm mov ss, temp_reg;   // restauriranje stack-related registara
         temp_reg = next->bp;  asm mov bp, temp_reg;
         temp_reg = next->sp;  asm mov sp, temp_reg;


         //_____...________                                                 // _________   // (brojevi relativno u odnosu na BP')
         temp_reg = next->psw; asm mov ax, temp_reg; asm mov [bp+22], ax;   //+22| PSW |
         temp_reg = next->cs;  asm mov ax, temp_reg; asm mov [bp+20], ax;   //+20| CS  |
         temp_reg = next->pc;  asm mov ax, temp_reg; asm mov [bp+18], ax;   //+18| PC  |

         temp_reg = next->ax;  asm mov ax, temp_reg; asm mov [bp+16], ax;   //+16| AX  |
         temp_reg = next->bx;  asm mov ax, temp_reg; asm mov [bp+14], ax;   //+14| BX  |
         temp_reg = next->cx;  asm mov ax, temp_reg; asm mov [bp+12], ax;   //+12| CX  |
         temp_reg = next->dx;  asm mov ax, temp_reg; asm mov [bp+10], ax;   //+10| DX  |

         temp_reg = next->es;  asm mov ax, temp_reg; asm mov [bp+ 8], ax;   // +8| ES  |
         temp_reg = next->ds;  asm mov ax, temp_reg; asm mov [bp+ 6], ax;   // +6| DS  |
         temp_reg = next->si;  asm mov ax, temp_reg; asm mov [bp+ 4], ax;   // +4| SI  |
         temp_reg = next->di;  asm mov ax, temp_reg; asm mov [bp+ 2], ax;   // +2| DI  |

         //                                                                 // +0| BP  | « BP'
         //                                                                 // ..|     | « SP
      }

   }



   // printing debug information
   #ifdef DEBUG
      LOCK;   // treba li mozda da zabrani interrupte ? (da li je tacno da cout ponekad ukljuci interrupt ili je to legenda)
      PUSHF; INTD;

         cout << "\n\n\n" "=====================================context(" << System::time() << ")====== ";

            if      ( curr == System::main ) { cout << "M "; }
            else if ( curr == System::idle ) { cout << "I "; }
            else if ( curr )                 { cout << "T" << curr->id; }
            else                             { cout << "T?"; }

         cout << " to ";

            if      ( next == System::main ) { cout << "M "; }
            else if ( next == System::idle ) { cout << "I "; }
            else if ( next )                 { cout << "T" << next->id; }
            else                             { cout << "T?"; }

         cout << "\n";


         extern volatile uns4 called_cnt;   ///////////////////////////////////////////////////////////////////////////////
         extern volatile uns4 called_old_cnt;   ///////////////////////////////////////////////////////////////////////////////
         cout << "CALLEDCNT: " << called_cnt << "\n" << "CALLEDOLDCNT: " << called_old_cnt << "\n";   ///////////////////////////////////////////////////////////////////////////////


         if( next != curr )
         {
               if( curr ) { cout << "Trenutni kontekst:\n" << *(PCB*) curr; }
               else       { cout << "Trenutni kontekst:\n"    "?";          }

            cout << "\n";
         }

            if( next ) { cout << "Sledeci kontekst:\n"  << *(PCB*) next; }
            else       { cout << "Sledeci kontekst:\n"     "?";          }

         cout << "\n";

            KSemaphore::print_glist(cout, false) << "\n";
            //System::showsched(cout, false);
            cout << "activecnt: " << System::activecnt << "\n";

          //static volatile REG psw;
          //asm pushf; asm pop psw;
          //cout << "(psw I-flag: " << (psw & 0x200) << ")\n";

         cout << "-------------------------------------------------------" << endl;

      POPF;
      UNLOCK;
   #endif



   // acknowledging incoming signals
   LOCK;
      PUSHF; INTE;   // dozvoljavamo interrupt-e, ali ne promenu konteksta, jer acknowledge moze da traje duze, pa da ne bi blokirao prekide (npr. sa tastature)
         PCB::acknowledge();   // izvrsavamo sve signale koji su nam pristigli a koji nisu maskirani
      POPF;
   UNLOCK;   // ovde moze da se uradi context switch samo ako se desio event i trenutni thread je idle thread !!
}
