// _________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN
// _________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN
// _________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN

#include <iostream.h>
#define UTIL_PRINTING_TOOLS
#include "system.h"



#define MAIN_SAMPLES 1
int2 main(int2 argc, char* argv[])
{
   #ifdef DEBUG
      #undef  MAIN_SAMPLES
      #define MAIN_SAMPLES 3   // 3

   //void utilitytest0(); utilitytest0();
   //void listtest0();    listtest0();
   //void systemtest0();  systemtest0();
   //return 0;
   #endif



   int retval;
   for( int i = 0; i < MAIN_SAMPLES; i++ )
   {
      System::init();
      #ifdef DIAGNOSTICS
       //System::showsched(cout, true) << "\n";
       //System::showtypes(cout) << "\n";
       //System::showivt(cout, 100) << "\n";
       //show_allocations() << "\n";
      #endif

      extern int userMain(int argc, char* argv[]);
      retval = userMain(argc, argv);

      LOCK;
         while( System::getactivecnt() > 2 );   // main nit je u ovom trenutku kao i idle nit
         System::exit();
      UNLOCK;

      #ifdef DIAGNOSTICS
         show_allocations() << "\n";
      #endif
   }

   return retval;
}



//// set_terminate example
//#include <exception.h>      // std::set_terminate
//#include <stdlib.h>         // std::abort
//
//void myterminate () {
//  cout << "terminate handler called\n";
//  abort();  // forces abnormal termination
//}
//
//int main (void) {
//  set_terminate (myterminate);
//  terminate();
//  return 0;
//}
