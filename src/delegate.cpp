#define UTIL_PRINTING_TOOLS
#include "utility.h"
#include "delegate.h"
#include "list.h"


Delegate::Delegate(List<SignalHandler>* _hlist)
{
   if( !_hlist ) DELETE_THIS_AND_RETURN;
   hlist = _hlist;
}

Delegate::Delegate()
{
   LOCK;
   hlist = new List<SignalHandler>(true);   // delegat jeste owner kontejnera koji sadrze prost tip - signalhandler pointer
   UNLOCK;

   if( !hlist ) DELETE_THIS_AND_RETURN;

   hlist->setmatch(match_signal_handler_container);
   hlist->setmap(map_call_signal_handler);

   hlist->setprint(print_signal_handler);
   hlist->setcopy(copy_signal_handler_container);
   //hlist->setdelete(delete_signal_handler_container);   ////////////////////////////////////////// proveriti zasto ne treba da se postavi delete
}

Delegate::~Delegate()
{
   LOCK;
   if( hlist )
   {
      delete hlist; hlist = nullptr;
   }
   UNLOCK;
}



bool Delegate::add(SignalHandler h)
{
   if( !h ) return false;   // nije unutar lock-a jer ne moze da se promeni prilikom promene konteksta i povratka

   LOCK;   // mora da se zakljuca zbog new-a
   bool added = ( hlist ) ? hlist->pushb( new SignalHandler(h) ) : false;
   UNLOCK;

   return added;
}

bool Delegate::swap(SignalHandler h1, SignalHandler h2)
{
   LOCK;   // zasto lock, kada je swap thread-safe? zato sto dok neko zove swap (i nije jos stavio sve stvari na stek) moze da mu nestane lista (neko drugi pozove clear)
   bool swapped = ( hlist ) ? hlist->swap((void*) &h1, (void*) &h2) : false;
   UNLOCK;

   return swapped;
}

uns4 Delegate::signal()
{
   LOCK;   // zabranjena promena konteksta, tako je trazeno u zadatku
   int4 mapped = ( hlist ) ? hlist->map(nullptr, false) : false;   // ne uklanja elemente iz liste ako uspe da ih map-uje, zato false
   UNLOCK;

   return mapped;
}
uns4 Delegate::size()
{
   LOCK;
   uns4 size = ( hlist ) ? hlist->size() : 0;
   UNLOCK;

   return size;
}

Delegate* Delegate::copy()
{
   LOCK;   // zasto lock, kada je copy thread-safe? zato sto dok neko zove copy (i nije jos stavio sve stvari na stek) moze da mu nestane lista (neko drugi pozove clear)
   Delegate* copy = ( hlist ) ? new Delegate( hlist->copy(true, true) ) : nullptr;  // nova lista jeste owner svojih elemenata, i ima kopije elemenata iz originalne liste
   UNLOCK;

   return copy;
}

void Delegate::clear()
{
   LOCK;
   if( hlist ) hlist->clear();
   UNLOCK;
}

ostream& Delegate::print(ostream& os) const
{
   LOCK;
      if( hlist ) hlist->print(os, false, " ... ");

   UNLOCK;
   return os;
}

ostream& operator <<(ostream& os, const Delegate& d)
{
   return d.print(os);
}





// signalhandler list tools
bool match_signal_handler_container(SignalHandler* h, void* args)   // args je u stvari kontejner signalhandler-a koga trazimo
{
   if( !h || !args )                   return false;
   if( *h == *((SignalHandler*)args) ) return true;   // ukoliko signalhandler == *args

   return false;
}

bool map_call_signal_handler(SignalHandler* h, void*)   // zove signal handler ako je ispravno prosledjen signal handler kontejner
{
   if( !h || !*h )
      return false;

   (*h)();   // call the signal handler
   return true;
}



void print_signal_handler(ostream& os, const SignalHandler* h)      // ispisuje signal handler na izlazu
{
   if( h ) os << UTIL_PRETTYHEX << FP_SEG(*h) << "<<4 + " << FP_OFF(*h) << UTIL_NORMPRINT;
}

SignalHandler* copy_signal_handler_container(SignalHandler* h)   // kopira kontejner signalhander-a (pokazivac na prosto polje tipa signalhandler)
{
   return ( h ) ? new SignalHandler(*h) : nullptr;
}

void delete_signal_handler_container(SignalHandler* h)   // brise kontejner signalhandler-a
{
   if( h ) delete h;
}
