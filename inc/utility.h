// ___________________________________________________________________________________________________________________________________________________
// UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY
// ___________________________________________________________________________________________________________________________________________________
// UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY
// ___________________________________________________________________________________________________________________________________________________
// UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY...UTILITY

#ifndef _UTILITY_H_
#define _UTILITY_H_



// debagovanje
//#define DEBUG
//#define DIAGNOSTICS

// assert with a custom message
#include <assert.h>
#define crash_if(p, msg) \
(                        \
   (p) ? (void) __assertfail( msg ": %s, file %s, line %d" _ENDL,   #p, __FILE__, __LINE__ )   \
       : (void) 0        \
)



// stvari koje fale u C++92
typedef unsigned int    bool;
//                      char;

typedef signed   char   int1;
typedef signed   int    int2;
typedef signed   long   int4;

typedef unsigned char   uns1;
typedef unsigned int    uns2;
typedef unsigned long   uns4;

typedef          float  flo4;
typedef          double flo8;

#define nullptr 0L
#define true  1
#define false 0


// stvari za sortiranje liste
#define GREATER 1
#define EQUAL   0
#define LESS   -1
#define INCOMP -2



// stvari iz <dos.h> header-a
#define FP_SEG( fp )( (unsigned )( void _seg * )( void far * )( fp ))
#define FP_OFF( fp )( (unsigned )( fp ))

#define MK_FP( seg,ofs )( (void _seg * )( seg ) +( void near * )( ofs ))


// zabranjuje i dozvoljava prekide
#define INTE; { asm sti; }
#define INTD; { asm cli; }

// stavlja i skida PSW sa steka
#define PUSHF; { asm pushf; }
#define POPF;  { asm popf;  }



// thread safe standardna biblioteka
#include "lock.h"
void* far operator new(uns2 size);
void  far operator delete(void* block);

class ostream;
template< class T > class List;


#define DELETE_THIS                    \
{                                      \
   crash_if(true, "delete_this pozvan"); \
   LOCK;                               \
   delete this; (void*)this = nullptr; \
   UNLOCK;                             \
}

#define REPLACE_THIS(arg)              \
{                                      \
   crash_if(true, "replace_this pozvan"); \
   LOCK;                               \
   delete this; (void*)this = arg;     \
   UNLOCK;                             \
}

#define DELETE_THIS_AND_RETURN         \
{                                      \
   crash_if(true, "delete_this_and_return pozvan"); \
   DELETE_THIS;                        \
   return;                             \
}

#define REPLACE_THIS_AND_RETURN(arg)   \
{                                      \
   crash_if(true, "replace_this_and_return pozvan"); \
   REPLACE_THIS(arg);                  \
   return;                             \
}



// id-jevi
typedef uns4 ThreadId;    // id thread-a
typedef uns4 SemaphId;    // id semafora
typedef uns1 SignalId;    // id signala

// pcb
typedef void (*Runnable)();   // thread code
typedef uns2* Stack;          // stek
typedef uns4 StackSize;       // velicina steka

// sistem
typedef uns2 REG;         // registar
typedef uns4 Time;        // jedan tick tajmerske rutine <-> 55ms
typedef uns2 Flags;       // flegovi

// interrupti
typedef uns1 ivtno;
typedef void interrupt (*IntRoutine) (...);   // interrupt routine
#define IVT_SIZE 256                          // velicina interrupt tabele

// signali
typedef void (*SignalHandler)();   // signal handler
#define SIGNAL_TABLE_SIZE 16       // velicina signal tabele
typedef uns2 SignalMask;           // maska za signale



#endif//_UTILITY_H_


// korisne stvari za printing, izvan header guard-a
#if defined(UTIL_PRINTING_TOOLS) && !defined(UTIL_PRINTING_TOOLS_INCLUDED)
   #define UTIL_PRINTING_TOOLS_INCLUDED

   #include <iostream.h>
   #include <iomanip.h>

   #define UTIL_PRETTYHEX setw(4) << setfill('0') << setbase(16)
   #define UTIL_NORMPRINT setw(0) << setfill(' ') << setbase(10)

   ostream& show_allocations(ostream& os = cout, bool drawframe = true);
#endif
