class ListIntElement {
   public:
     ListIntElement(int val, int sortKey);   // initialize a list element

     ListIntElement *next;     // next element on list, 
                // NULL if this is the last
     
     int key;               // priority, for a sorted list
     int value;            // pointer to item on the list
};

//List of ListIntElements
class ListInt {
  public:
    ListInt();         // initialize the list
    ~ListInt();

    void Find_Item(int * return_pointr,int key);    //made by kundan(not pre-made)    
    bool IsEmpty();
    // Routines to put/get items on/off list in order (sorted by key)
    void SortedInsert(int item, int sortKey); // Put item into list

  private:
    ListIntElement *first;     // Head of the list, NULL if list is empty
    ListIntElement *last;      // Last element of list
};