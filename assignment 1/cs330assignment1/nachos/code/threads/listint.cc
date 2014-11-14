
#include "copyright.h"
#include "listint.h"



ListIntElement::ListIntElement(int val, int sortKey)
{
     value = val;
     key = sortKey;
     next = NULL;   // assume we'll put it at the end of the list 
}



ListInt::ListInt()
{ 
    ListIntElement *element = new ListIntElement(5, 0);
    first = element; 
    last = element;
}


bool
ListInt::IsEmpty() 
{ 
    if (first == NULL)
        return TRUE;
    else
        return FALSE; 
}

void
ListInt::Find_Item(int * return_pointr,int key)
{
    ListIntElement *ptr;
    
    ptr = first;
    while(ptr != NULL && key < ptr->key)
    {
        ptr = ptr->next;
    }
    if(ptr != NULL && ptr->key == key)
    {
        *return_pointr = ptr->value;
        return;
    }
    return_pointr = NULL;
}
void
ListInt::SortedInsert(int item, int sortKey)
{
    ListIntElement *element = new ListIntElement(item, sortKey);
    ListIntElement *ptr;       // keep track
    /*first = element;*/
    printf("in sorted insert\n");
   // printf("printing first key %d\n",first->key);
    if (IsEmpty()) {    // if list is empty, put
        first = element;
        last = element;
    } else if (sortKey < first->key) {  
        // item goes on front of list
    element->next = first;
    first = element;
    } else {        // look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey < ptr->next->key) {
        element->next = ptr->next;
            ptr->next = element;
        return;
        }
    }
    last->next = element;       // item goes at end of list
    last = element;
    }
}