#ifndef _USERMAIN_H_
#define _USERMAIN_H_

#include "utility.h"


// wrapper uslovne promena konteksta
static void dispatch()
{
   void switch_context();
   switch_context();
}

const StackSize defaultStackSize = 4096;
const Time defaultTimeSlice = 2;   // 2*55ms
typedef int2 ID;
typedef int1 IVTNo;



#endif//_USERMAIN_H_
