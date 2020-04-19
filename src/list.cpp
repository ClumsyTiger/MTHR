#include "list.h"


#ifdef DEBUG
   struct int2x2
   {
      int val;
      int idx;

      int2x2(int _val, int _idx) : val(_val), idx(_idx)
      {}

      friend ostream& operator <<(ostream& os, const int2x2& i)
      {
         LOCK;
         os << i.val << "[" << i.idx << "]";
         UNLOCK;

         return os;
      }
   };



   int2 compare_int2x2(int2x2* i1, int2x2* i2)
   {
      if( !i1 || !i2 )         return INCOMP;

      if( i1->val <  i2->val ) return LESS;
      if( i1->val == i2->val ) return EQUAL;
                               return GREATER;
   }
   bool match_int2x2_val(int2x2* i, void* args)
   {
      if( !i || !args )            return false;
      if( i->val == *(int2*)args ) return true;

      return false;
   }
   bool match_int2x2_idx(int2x2* i, void* args)
   {
      if( !i || !args )            return false;
      if( i->idx == *(int2*)args ) return true;

      return false;
   }
   bool map_int2x2_inc_idx(int2x2* i, void*)
   {
      if( !i  ) return false;
      if( i->idx != 0 ) { i->idx++; return true; }

      return false;
   }


   void print_int2x2(ostream& os, const int2x2* i)
   {
      if( i ) os << *i;
   }

   int2x2* copy_int2x2(int2x2* i)
   {
      return ( i ) ? new int2x2(i->val, i->idx) : nullptr;
   }

   void delete_int2x2(int2x2* i)
   {
      if( i ) { delete i; cout << "A"; }
   }



   // testiranje liste
   void listtest0()
   {
      LOCK;
         cout << "==================================================list=test======"  "\n";
         show_allocations();

         List<int2x2> *L, *L2;

         L= new List<int2x2>(true);
         L->setcompare(compare_int2x2);
         L->setmatch(match_int2x2_val);

         L->setcopy(copy_int2x2);
         L->setprint(print_int2x2);
         L->setdelete(delete_int2x2);

         {
            for( int4 i = 0; i <= 6; i++ )
            {
               L->pushf(new int2x2(i,   0));
               L->pushb(new int2x2(6-i, 0));
               L->pushf(nullptr);
               L->pushb(nullptr);
            }
         }

         cout << "L Pop front/back" "\n";
         while( L->size() )
         {
            L->print(cout, true)  << "\n";
            cout << "------------------";
            L->print(cout, false) << "\n";

            int2x2* a = L->popf();
            if( a ) cout << a->val << "\n";
            int2x2* b = L->popb();
            if( b ) cout << b->val << "\n";

            delete a;
            delete b;
         }
         L->print(cout, true)  << "\n";
         L->print(cout, false) << "\n";
         cout << "\n" << endl;



         cout << "L Push sorted" "\n";
         L->pushs(new int2x2(4, 0));   // F
         L->print() << "\n";
         L->pushs(new int2x2(6, 0));   // B
         L->print() << "\n";
         L->pushs(new int2x2(2, 0));   // F
         L->print() << "\n";
         L->pushs(new int2x2(8, 0));   // B
         L->print() << "\n";
         L->pushs(new int2x2(0, 0));   // F
         L->print() << "\n" << endl;


         L->pushs(new int2x2(1, 0));   // I
         L->print() << "\n";
         L->pushs(new int2x2(3, 0));   // I
         L->print() << "\n";
         L->pushs(new int2x2(5, 0));   // I
         L->print() << "\n";
         L->pushs(new int2x2(7, 0));   // I
         L->print() << "\n";
         L->pushs(new int2x2(9, 0));   // B
         L->print() << "\n" << endl;

         L->pushs(new int2x2(10, 0));
         L->print() << "\n";
         L->pushs(new int2x2(11, 0));
         L->print() << "\n";
         L->pushs(new int2x2(2, 1));
         L->print() << "\n" << endl;

         L->pushs(new int2x2(13, 0));
         L->print() << "\n";
         L->pushs(new int2x2(15, 0));
         L->print() << "\n";
         L->pushs(new int2x2(2, 2));
         L->print() << "\n" << endl;

         L->pushs(new int2x2(0, 1));
         L->print() << "\n";
         L->pushs(new int2x2(6, 1));
         L->print() << "\n";
         L->pushs(new int2x2(1, 1));
         L->print() << "\n" << endl;

         L->pushs(new int2x2(0, 2));
         L->print() << "\n";
         L->pushs(new int2x2(9, 1));
         L->print() << "\n";
         L->pushs(new int2x2(15, 1));
         L->print() << "\n";
         cout << "\n" << endl;




         cout << "L print without print()\n";
         L->setprint(nullptr);
         L->print(cout, true)  << "\n";
         cout << "------------------";
         L->print(cout, false) << "\n";
         L->setprint(print_int2x2);
         cout << "\n" << endl;


         cout << "L Find\n";
         {
            int2x2* hit;
            for( int i = 0; i < 20; i++ )
            {
               if( ( hit = L->find((void*)&i) ) != nullptr )
               {
                  cout << *hit << "\n";
                  L->print() << "\n";

                  hit = L->pop();
                  L->pushs(hit);
                  L->print() << "\n";
               }

            }
         }
         cout << "\n" << endl;



         cout << "L Swap\n";
         {
            int a = 2, b = 1;
            L->setmatch(match_int2x2_idx);
            L->swap((void*)&a, (void*)&b);
            L->print() << "\n";
         }
         cout << endl;


         cout << "L Deep copy\n";
         L2 = L->copy(true, true);
         L2->setmap(map_int2x2_inc_idx);
         L2->print() << "\n";
         cout << endl;


         cout << "L Delete\n";
         delete L; L = nullptr; cout << "\n";
         cout << "\n" << endl;




         cout << "L2 Map\n";
         L2->map(nullptr, false);
         L2->print() << "\n";
         L2->map(nullptr, true);
         cout << "\n"; L2->print() << "\n";
         cout << endl;


         cout << "L2 Clear, delete\n";
         L2->clear();
         cout << "\n"; L2->print() << "\n";
         delete L2; L2 = nullptr;
         cout << endl;

         show_allocations();
         cout << "-----------------------------------------------------------------" << endl;
      UNLOCK;
   }
#endif
