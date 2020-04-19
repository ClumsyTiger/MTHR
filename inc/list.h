// _________________________________________________________________________________________________________________________________________
// LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST
// _________________________________________________________________________________________________________________________________________
// LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST
// _________________________________________________________________________________________________________________________________________
// LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST...LIST

#ifndef _LIST_H_
#define _LIST_H_

#define UTIL_PRINTING_TOOLS
#include "utility.h"


// ova lista je thread-safe
template< class T >
class List
{
// declarations
   #ifdef DEBUG
      friend void listtest0();
   #endif


   struct Node
   {
   // declarations
   private:
      friend class List<T>;
      #ifdef DEBUG
         friend void listtest0();
      #endif

   // internal state
   private:
      Node *prev, *next;
      T* data;

   // methods
   private:
      Node(T* _data) : data(_data), prev(nullptr), next(nullptr)
      {
         if( !data ) DELETE_THIS_AND_RETURN;
      }
   };


// internal state
private:
   Node *head, *tail;
   uns4 len;
   bool owner;

   int2 (*compare)(T* t1, T* t2);       // returns GREATER, EQUAL, LESS, INCOMP
   bool (*match)  (T* t, void* args);   // returns true, false
   bool (*mp)     (T* t, void* args);   // returns true, false if the element has been mapped to
   Node *hit;                           // should be mutable

   void (*prnt) (ostream& os, const T* t);    // prints  data
   T*   (*cpy)  (T* t);                       // copies  data
   void (*del)  (T* t);                       // deletes data


//methods
private:
   // povezuje dva elementa (levi-desni)
   static void link(Node* l, Node* r)
   {
      LOCK;
         if( l ) l->next = r;
         if( r ) r->prev = l;
      UNLOCK;
   }
   // povezuje tri elementa (levi-centralni i centralni-desni)
   static void link(Node* l, Node* c, Node* r)
   {
      LOCK;
         link(l, c);
         link(c, r);
      UNLOCK;
   }


public:
   // konstruktor, ako je lista owner svojih elemenata ima pravo i da ih obrise; smatra se da lista uvek poseduje svoj komparator!
   List(bool _owner = false);
   // destruktor, jedino on brise komparator (clear() samo brise elemente)
   ~List();



   // ubacuje element na pocetak liste
   bool pushf(T* data);
   // ubacuje element na kraj liste
   bool pushb(T* data);
   // ubacuje element na odgovarajucu poziciju u rastuce sortiranoj listi (head <= tail) - stable sort
   bool pushs(T* data);



   // izbacuje element sa pocetka liste i vraca ga
   T* popf();
   // izbacuje element sa kraja liste i vraca ga
   T* popb();
   // izbacuje element koji je pronadjen sa find i vraca ga
   T* pop();



   // uklanja prvi element iz liste i brise ga ako je owner
   bool remf();
   // uklanja poslednji element iz liste i brise ga ako je owner
   bool remb();
   // uklanja element iz liste koji je pronadjen sa find i brise ga ako je owner
   bool rem();
   // brise celu listu, brise i elemente ako je owner
   bool clear();



   // !!! inherentno thread-UNSAFE - mora da se lock-uje izvan poziva
   // !!! ako se uradi context switch pre return data i doda nesto u listu vrati se neispravna vrednost!

   // vraca prvi element iz liste i ne brise ga u listi
   T* peekf() const;
   // vraca poslednji element iz liste i ne brise ga u listi
   T* peekb() const;
   // pronalazi prvo pojavljivanje elementa u listi, krece se od glave liste
   T* find(void* args) const;

   // mapira funkciju match na sve elemente liste, vraca broj elemenata za koje je match vratio true
   uns4 map(void* args, bool remove = false);
   // swap-uje pozicije dva elementa u listi, samo ako se match-uju oba u ovoj listi
   bool swap(void* args1, void* args2);



   // !!! inherentno thread-UNSAFE - mora da se lock-uje izvan poziva
   // !!! ako se uradi context switch pre return data i doda nesto u listu vrati se neispravna vrednost!

   // vraca velicinu niza/da li je lista vlasnik svojih podataka (da li ih ona brise ili neko izvan)
   uns4 size()    const;   // vracanje 4B nije atomicno!!! (2x 2B)
   bool isowner() const;
   bool isempty() const;

   // postavlja ownership
   void setowner(bool _owner);

   // postavlja komparator liste, primenjuje se sa svakim novim ubacivanjem elementa u listu (ne na celu listu kada je dodat)
   void setcompare( int2 (*_compare)(T* t1, T* t2)   );
   // postavlja match-er liste, primenjuje se kada se radi find, i nalazi max jedan element
   void setmatch  ( bool (*_match)(T* t, void* args) );
   // postavlja map liste, primenjuje se na sve elemente liste, i vraca se broj uspesnih primena
   void setmap    ( bool (*_map) (T* t, void* args)  );

   // postavlja print, copy i delete funkcije koje rade nad data-om elementa liste
   void setprint  ( void (*_print)(ostream& os, const T* t) );
   void setcopy   ( T*   (*_copy)(T* t)                     );
   void setdelete ( void (*_del)(T* t)                      );


   // vraca kopiju liste, ako je deep_copy true onda se radi kopiranje i elemenata, a ako nije onda se samo pokazivaci na elemente kopiraju
   List<T>* copy(bool _owner, bool deep_copy) const;

   // ispisuje listu na navedeni ostream
   ostream& print(ostream& os = cout, bool drawframe = true, char* sep = " ") const;
};






// konstruktor, ako je lista owner svojih elemenata ima pravo i da ih obrise; smatra se da lista uvek poseduje svoj komparator!
template< class T >
inline List<T>::List(bool _owner)
{
   head  = nullptr;
   tail  = nullptr;
   //curr  = nullptr;

   len   = 0;
   owner = _owner;

   compare = nullptr;
   match   = nullptr;
   mp      = nullptr;
   hit     = nullptr;

   cpy  = nullptr;
   prnt = nullptr;
   del  = nullptr;
}
// destruktor, jedino on brise komparator (clear() samo brise elemente)
template< class T >
inline List<T>::~List()
{
   LOCK;
      if( owner && !del ) owner = false;
      clear();
   UNLOCK;
}



// ubacuje element na pocetak liste
template< class T >
inline bool List<T>::pushf(T* data)
{
   LOCK;
      if( !data )
      {
         UNLOCK;   // unlock spoljasnjeg lock-a
         return false;
      }

      Node* el = new Node(data);

      if( !el )
      {
         if( owner && del ) del(data);

         UNLOCK;   // unlock spoljasnjeg lock-a
         return false;
      }

      link(el, head);
      head = el;

      if( !tail )
         tail = el;

      len++;

   UNLOCK;
   return true;
}

// ubacuje element na kraj liste
template< class T >
inline bool List<T>::pushb(T* data)
{
   LOCK;
      if( !data )
      {
         UNLOCK; // unlock spoljasnjeg lock-a
         return false;
      }

      Node* el = new Node(data);

      if( !el )
      {
         if( owner && del ) del(data);

         UNLOCK;   // unlock spoljasnjeg lock-a
         return false;
      }

      link(tail, el);
      tail = el;

      if( !head )
         head = el;

      len++;

   UNLOCK;
   return true;
}

// ubacuje element na odgovarajucu poziciju u rastuce sortiranoj listi (head <= tail) - stable sort
template< class T >
bool List<T>::pushs(T* data)
{
   LOCK;
      if( !data )
      {
         UNLOCK;   // unlock spoljasnjeg lock-a
         return false;
      }


      if( !head || !compare )
      {
       //cout << "B" << endl;
         pushb(data);
      }
      else if( compare(data, head->data) == LESS    )   // data < head->data
      {
       //cout << "F" << endl;
         pushf(data);
      }
      else if( compare(tail->data, data) != GREATER )   // tail->data <= data ili nije uporedivo
      {
       //cout << "B" << endl;
         pushb(data);
      }
      else
      {
       //cout << "I" "\n";
         Node* el = new Node(data);

         if( !el )
         {
            if( owner && del ) del(data);

            UNLOCK;   // unlock spoljasnjeg lock-a
            return false;
         }

         for( Node* curr = head;  curr;  curr = curr->next )
         {
            if( compare(data, curr->data) == LESS )   // data < curr->data
            {
               link(curr->prev, el, curr);
               len++;
               break;
            }
         }
      }

   UNLOCK;
   return true;
}



// izbacuje element sa pocetka liste i vraca ga
template< class T >
inline T* List<T>::popf()
{
   LOCK;
      if( !head )
      {
         UNLOCK;   // unlock spoljasnjeg lock-a
         return nullptr;
      }

      Node* node = head;
      T* data    = head->data;

      link(nullptr, head = head->next);
      if( !head ) tail = nullptr;

      if( hit == node ) hit = nullptr;
      delete node;

      len--;

   UNLOCK;
   return data;
}

// izbacuje element sa kraja liste i vraca ga
template< class T >
inline T* List<T>::popb()
{
   LOCK;
      if( !tail )
      {
         UNLOCK;   // unlock spoljasnjeg lock-a
         return nullptr;
      }

      Node* node = tail;
      T* data    = tail->data;

      link(tail = tail->prev, nullptr);
      if( !tail ) head = nullptr;

      if( hit == node ) hit = nullptr;
      delete node;

      len--;

   UNLOCK;
   return data;
}

// izbacuje element koji je pronadjen sa find i vraca ga
template< class T >
inline T* List<T>::pop()
{
   LOCK;
      if( !hit )
      {
         UNLOCK;   // unlock spoljasnjeg lock-a
         return nullptr;
      }

      T* data = hit->data;

      link(hit->prev, hit->next);
      if( !hit->prev ) head = hit->next;
      if( !hit->next ) tail = hit->prev;

      delete hit; hit = nullptr;

      len--;

   UNLOCK;
   return data;
}



// uklanja prvi element iz liste i brise ga ako je owner
template< class T >
inline bool List<T>::remf()
{
   LOCK;
      if( owner && !del )   // ako je owner i ne moze da obrise element to je greska
      {
         UNLOCK;
         return false;
      }


      T* data = popf();
      if( data && owner ) del(data);

   UNLOCK;
   return true;
}

// uklanja poslednji element iz liste i brise ga ako je owner
template< class T >
inline bool List<T>::remb()
{
   LOCK;
      if( owner && !del )   // ako je owner i ne moze da obrise element to je greska
      {
         UNLOCK;
         return false;
      }


      T* data = popb();
      if( data && owner ) del(data);

   UNLOCK;
   return true;
}

// uklanja element iz liste koji je pronadjen sa find i brise ga ako je owner
template< class T >
inline bool List<T>::rem()
{
   LOCK;
      if( owner && !del )   // ako je owner i ne moze da obrise element to je greska
      {
         UNLOCK;
         return false;
      }


      T* data = pop();
      if( data && owner ) del(data);

   UNLOCK;
   return true;
}

// brise celu listu, brise i elemente ako je owner
template< class T >
bool List<T>::clear()
{
   LOCK;
      if( owner && !del )   // ako je owner i ne moze da obrise element to je greska
      {
         UNLOCK;
         return false;
      }

      for( Node *temp, *curr = head;  curr;  )
      {
         temp = curr; curr = curr->next;

         if( owner ) del(temp->data);
         delete temp;
      }

      head = tail = nullptr;
      hit = nullptr;
      len = 0;

   UNLOCK;
   return true;
}



// !!! inherentno thread-UNSAFE - mora da se lock-uje izvan poziva
// !!! ako se uradi context switch pre return data i doda nesto u listu vrati se neispravna vrednost!

// vraca prvi element iz liste i ne brise ga u listi
template< class T >   inline T* List<T>::peekf() const { LOCK; T* data = head ? head->data : nullptr; UNLOCK; return data; }

// vraca poslednji element iz liste i ne brise ga u listi
template< class T >   inline T* List<T>::peekb() const { LOCK; T* data = tail ? tail->data : nullptr; UNLOCK; return data; }

// pronalazi prvo pojavljivanje elementa u listi, krece se od glave liste
template< class T >
T* List<T>::find(void* args) const
{
   LOCK;
   if( !match )
   {
      UNLOCK;   // unlock spoljasnjeg lock-a
      return nullptr;
   }

   T* data = nullptr;
   for( Node* curr = head;  curr;  curr = curr->next )
   {
      if( match(curr->data, args) )
      {
         (Node*)hit  = curr;   // hack in order to keep const-ness
         data = curr->data;
         break;
      }
   }

   UNLOCK;
   return data;
}

// mapira funkciju match na sve elemente liste, vraca broj elemenata za koje je match vratio true
template< class T >
uns4 List<T>::map(void* args, bool remove = false)
{
   LOCK;
      if( !mp || ( remove && !del ) )   // ako ne moze da se match-uje ili moze ali ne moze da se ukloni element, a trazeno je da se ukloni
      {
         UNLOCK;
         return 0;
      }

      uns4 res = 0;
      Node* oldhit = hit;

      for( Node *curr = head, *temp;  curr;   )
      {
         temp = curr; curr = curr->next;

         if( mp(temp->data, args) )
         {
            res++;
            if( remove )
            {
               if( oldhit == temp ) oldhit = nullptr;
               hit = temp; rem();
            }
         }

      }

      hit = oldhit;

   UNLOCK;
   return res;
}

// swap-uje pozicije dva elementa u listi, samo ako se match-uju oba u ovoj listi
template< class T >
bool List<T>::swap(void* args1, void* args2)
{
   LOCK;
      Node *n1 = nullptr, *n2 = nullptr;
      for( Node* curr = head;  curr;   )
      {
         if( !n1 && match(curr->data, args1) )   // pronadjena prva pojava prvog elementa
         {
            n1 = curr;
            if( n1 && n2 ) break;
         }
         else if( !n2 && match(curr->data, args2) )   // pronadjena prva pojava drugog elementa
         {
            n2 = curr;
            if( n1 && n2 ) break;
         }

         curr = curr->next;
      }


      if( !n1 || !n2 )
      {
         UNLOCK;
         return false;
      }

      T*  temp = n1->data;
      n1->data = n2->data;
      n2->data =     temp;

   UNLOCK;
   return true;
}



// !!! inherentno thread-UNSAFE - mora da se lock-uje izvan poziva
// !!! ako se uradi context switch pre return data i doda nesto u listu vrati se neispravna vrednost!

// vraca velicinu niza/da li je lista vlasnik svojih podataka (da li ih ona brise ili neko izvan)
template< class T >   inline uns4 List<T>::size()    const { LOCK; uns4 length  = len;        UNLOCK; return length;  }   // vracanje 4B nije atomicno!!! (2x 2B)
template< class T >   inline bool List<T>::isowner() const { LOCK; bool isowner = owner;      UNLOCK; return isowner; }
template< class T >   inline bool List<T>::isempty() const { LOCK; bool empty   = (len == 0); UNLOCK; return empty;   }

// postavlja ownership
template< class T >   inline void List<T>::setowner(bool _owner) { LOCK; owner = _owner; UNLOCK; }

// postavlja komparator liste, primenjuje se sa svakim novim ubacivanjem elementa u listu (ne na celu listu kada je dodat)
template< class T >   inline void List<T>::setcompare( int2 (*_compare)(T* t1, T* t2)   ) { LOCK; compare = _compare;  UNLOCK; }
// postavlja match-er liste, primenjuje se kada se radi find, i nalazi max jedan element
template< class T >   inline void List<T>::setmatch  ( bool (*_match)(T* t, void* args) ) { LOCK; match   = _match;    UNLOCK; }
// postavlja map liste, primenjuje se na sve elemente liste, i vraca se broj uspesnih primena
template< class T >   inline void List<T>::setmap    ( bool (*_map)(T* t, void* args)   ) { LOCK; mp      = _map;      UNLOCK; }

// postavlja delete i print funkcije
template< class T >   inline void List<T>::setprint ( void (*_print)(ostream& os, const T* t) ) { LOCK; prnt = _print;  UNLOCK; }
template< class T >   inline void List<T>::setcopy  ( T*   (*_copy)(T* t)                     ) { LOCK; cpy  = _copy;   UNLOCK; }
template< class T >   inline void List<T>::setdelete( void (*_del)(T* t)                      ) { LOCK; del  = _del;    UNLOCK; }


// vraca kopiju liste, ako je deep_copy true onda se radi kopiranje i elemenata, a ako nije onda se samo pokazivaci na elemente kopiraju
template< class T >
List<T>* List<T>::copy(bool owner, bool deep_copy) const
{
   LOCK;
      if( deep_copy && !cpy )
      {
         UNLOCK;
         return nullptr;
      }


      List<T>* list = new List<T>(owner);

      if( !list )
      {
         UNLOCK;
         return nullptr;
      }


      list->setcompare(compare);
      list->setmatch(match);
      list->setmap(mp);

      list->setprint(prnt);
      list->setcopy(cpy);
      list->setdelete(del);


      for( Node* curr = head; curr; curr = curr->next )
      {
         if( !list->pushb( (deep_copy) ? cpy(curr->data) : curr->data ) )
         {
            delete list; list = nullptr;
            UNLOCK;
            return nullptr;
         }
      }

   UNLOCK;
   return list;
}


// ispisuje listu na navedeni ostream
template< class T >
ostream& List<T>::print(ostream& os, bool drawframe, char* _sep) const
{
   LOCK;
      char* sep = _sep;
      if( !sep  ) { sep = " ";     _sep = " "; }
      if( !prnt ) { sep = " ... "; _sep = " "; }

      if( drawframe )
         os << "List<" << setw(2) << size() << ">: [H] -->" << _sep;

      for( Node* node = head;  node;  node = node->next )
      {
         if( node != head )
            os << sep;

         if( prnt ) prnt(os, node->data);
         else       os << UTIL_PRETTYHEX << FP_SEG(node->data) << "<<4 + " << UTIL_PRETTYHEX << FP_OFF(node->data) << UTIL_NORMPRINT;
      }

      if( drawframe ) os << _sep << "--> [T]";

   UNLOCK;
   return os;
}



#endif//_LIST_H_
