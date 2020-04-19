// _______________________________________________________________________________________________________________________________________
// PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB
// _______________________________________________________________________________________________________________________________________
// PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB
// _______________________________________________________________________________________________________________________________________
// PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB...PCB

#ifndef _PCB_H_
#define _PCB_H_

#define UTIL_PRINTING_TOOLS
#include "utility.h"
#include "delegate.h"


// the (living) embodiment of a Thread
// bilo ko moze da koristi pcb, ali samo thread moze da ga napravi
class PCB
{
// declarations
   friend class Thread;

   friend class Lock;
   friend class System;
   friend class KSemaphore;
   friend class KEvent;

   friend void interrupt timer_tick();
   friend void           switch_context   ();
   friend void interrupt switch_context_uc();

   friend int2 compare_pcb_id       (PCB* p1, PCB* p2);
   friend int2 compare_pcb_timestamp(PCB* p1, PCB* p2);
   friend bool match_pcb_id         (PCB* p, void* args);

   friend void print_pcb   (ostream& os, const PCB* p);
   friend void print_pcb_id(ostream& os, const PCB* p);
   friend void delete_pcb(PCB* p);

   friend bool map_signal(SignalId* s, void*);
   friend void pcb_default_signal0();


// interface constants
private:
   static const StackSize def_stack_size;  // = 4096     // default stack size
   static const Time inf_quant, def_quant; // = [0, 2]   // default time quant for scheduling, jedan kvant je 55ms (milisekundi!), ukupno ~19quant/s

private:
   static const Flags start, ready, running, blocked, finished;   // = [0, 1, 2, 3, 4]
   static const ThreadId main_id, idle_id, unknown_id;            // = [0, 1, ~0UL]

   static const SignalMask def_signal_mask;   // = ~0U
   static const SignalId terminate_id, notifyparent_id, finished_id;   // = [0, 1, 2]   // specijalni signali


// internal state
private:
   static ThreadId idcnt;

private:
   // thread identification
   ThreadId id;                  // id thread-a
   Thread* t;                    // thread objekat

   // thread resources
   Runnable run;                 // 'kod' od koga thread pocinje
   Stack stack;                  // stek thread-a
   StackSize stack_size;         // velicina tog steka

   // thread state
   REG ax, bx, cx, dx, si, di;   // registri opste namene
   REG pc, psw;                  // program counter i program status word
   REG bp, sp;                   // base pointer i stack pointer
   REG cs, ss, ds, es;           // segmentni registri, ne moze im se direktno dodeljivati vrednost, vec mora preko nekog drugog registra (npr. ax)

   // scheduler stuff
   Time  quant;                  // broj jedinica vremena koje su dodeljene thread-u za izvrsavanje
   Flags state : 3;              // trenutno stanje thread-a
   Time  timestamp;              // vreme do kada se menja kontekst/vreme do kada je blokiran thread

   // context locking
   KSemaphore* blockedon;        // semafor na kome je blokiran thread
   uns4 context_lockcnt;         // broj puta koliko se u thread-u zakljucao context switch pre bezuslovnog context switch-a (blokiran na semaforu)
   bool timeexpired;             // isteklo max vreme cekanja na semaforu pa je zato thread odblokiran

   // thread join and delete
   KSemaphore* sem4del;          // semafor na kome niti cekaju koje pozovu destruktor pcb-ja (ako thread nije zavrsio rad)
 //KSemaphore* sem4join;         // semafor na kome trenutna nit ceka ako pozove wait (onda ceka svu svoju direktnu decu)
   ThreadId parent;              // id thread-a koji je napravio ovaj thread
 //uns4 childcnt;                // broj direktne dece koju ovaj thread ima

   // signals
   List<SignalId>* signal_queue; // red pozvanih signala
   Delegate** signal_delegate;   // svi delegati za obradu signala trenutnog pcb-ja
   static SignalMask gsignal_mask; // globalna maska za signale
   SignalMask signal_mask;       // lokalna maska za signale pcb-ja


// methods
private:
   PCB(Runnable run, Time quant = def_quant, StackSize stack_size = def_stack_size);   // ako je run = nullptr, onda se pravi kontekst trenutnog thread-a
   ~PCB();

   //static bool setjmp (PCB* pcb);
   //static void longjmp(PCB* pcb);

   //static bool transition_between_states(volatile PCB* pcb, Flags prev_state, Flags next_state);

   ostream& print(ostream& os = cout, bool verbose = false) const;
   friend ostream& operator <<(ostream& os, const PCB& pcb);

private:
   void start_thread();
   void wait_to_complete() const;

   ThreadId        getid() const;
   static ThreadId get_running_id();

   static PCB* getpcb(ThreadId id);
   static PCB* get_running_pcb();

// ni jedna od nestatickih funkcija nije 100% thread safe ako se zove iz tudjeg konteksta (nije problem, jer userMain nema pristup pcb-ju)
//    + moze da se pozove neka nestaticka funkcija iz thread-a X nad thread-om Y
//    + cim se udje u telo te funkcije a pre lock-a se desi promena konteksta na thread Y, Y se zavrsi, promena konteksta na thread Z, Z obrise Y, vrati se na X
//    + X radi nad obrisanim podacima, jer nema nacina da sazna da li je thread unisten
// da li moze da se proveri da li je this* != null ? - ne, this* se ne menja
private:
   // u postavci pise da treba da ova funkcija bude non-static, ali to nije thread-safe ako se nit obrise!
   static void signal(ThreadId tid, SignalId sid);   // salje signal niti sa navedenim id-jem, ako nit sama sebi salje signal odmah ga i obradi
   static void acknowledge();                        // running thread obradjuje svoje signale

   void register_handler   (SignalId id, SignalHandler h);
   void unregister_handlers(SignalId id);
   void swap_handlers      (SignalId id, SignalHandler h1, SignalHandler h2);

   void        allow_signal  (SignalId id);
   static void allow_signal_g(SignalId id);
   void        block_signal  (SignalId id);
   static void block_signal_g(SignalId id);

   bool        isallowed_signal  (SignalId id);
   static bool isallowed_signal_g(SignalId id);
   bool        isblocked_signal  (SignalId id);
   static bool isblocked_signal_g(SignalId id);
};



// pcb list tools
int2 compare_pcb_id       (PCB* p1, PCB* p2);        // poredi dva pcb-ja na osnovu id-jeva
int2 compare_pcb_timestamp(PCB* p1, PCB* p2);        // poredi dva pcb-ja na osnovu timestamp polja
bool match_pcb_id         (PCB* p, void* args);      // args je u stvari id pcb-ja koga trazimo

void print_pcb   (ostream& os, const PCB* p);        // ispisuje pcb na izlazu
void print_pcb_id(ostream& os, const PCB* p);        // ispisuje id pcb-ja na izlazu
void delete_pcb(PCB* p);                             // zove destruktor pcb-ja


// signal list tools
bool map_signal(SignalId* s, void*);                 // poziva delegate[s] ali samo ako nije disabled (sam map ce da izbaci posle request iz signal_queue-a)

void print_signal(ostream& os, const SignalId* s);   // ispisuje signal na izlazu
void delete_signal(SignalId* s);                     // zove destruktor signala (pokazivac na uns2)


// default signal 0
void pcb_default_signal0();



#endif//_PCB_H_
