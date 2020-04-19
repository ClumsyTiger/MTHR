#include <malloc.h>
#include <iostream.h>
#include "utility.h"


static uns4 alive_objects = 0;
static uns4 alloc_so_far  = 0;



void* operator new(uns2 size)
{
   PUSHF; INTD;   // sigurnije je ovako, ali bi mozda moglo da se napravi preko lock-a (problem je u interrupt rutini kada se new-uju stvari ?)
      void* block = malloc(size);
      crash_if( block == nullptr, "out of memory" );

      alloc_so_far += size;
      alive_objects++;
   POPF;

   return block;
}


void operator delete(void* block)
{
   PUSHF; INTD;   // sigurnije je ovako, ali bi mozda moglo da se napravi preko lock-a (problem je u interrupt rutini kada se new-uju stvari ?)
      if( block )
      {
         free(block);
         alive_objects--;
      }
   POPF;
}



ostream& show_allocations(ostream& os, bool drawframe)
{
   LOCK;
      if( drawframe )
         os << "======allocations=================" "\n";

      os << "allocated so far: " << alloc_so_far  << "\n";
      os << "alive objects: "    << alive_objects << "\n";

      if( drawframe )
         os << "----------------------------------" << endl;

   UNLOCK;
   return os;
}

#ifdef DEBUG
   // testtiranje utility-ja
   void utilitytest0()
   {
      LOCK;
         cout << "===============================================utility=test======" "\n";

            cout << "allocated so far: " << alloc_so_far << " words" "\n";

            cout << "alive objects: " << alive_objects << "\n";

         cout << "\n";

            int* a   = new int(6);
            cout << "int* a   = new int(6): " << alive_objects << "\n";

            delete a;
            cout << "delete a: " << alive_objects << "\n";

         cout << "\n";

            int* arr = new int[6];
            cout << "int* arr = new int[6]: " << alive_objects << "\n";

            delete[] arr;
            cout << "delete[] arr: " << alive_objects << "\n";

         cout << "-----------------------------------------------------------------" << endl;
      UNLOCK;
   }
#endif
