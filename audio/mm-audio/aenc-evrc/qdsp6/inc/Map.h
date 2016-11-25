/*--------------------------------------------------------------------------
Copyright (c) 2010, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
#ifndef _MAP_H_
#define _MAP_H_

#include <stdio.h>
using namespace std;

template <typename T,typename T2>
class Map
{
    struct node
    {
        T    data;
        T2   data2;
        node* prev;
        node* next;
        node(T t, T2 t2,node* p, node* n) :
             data(t), data2(t2), prev(p), next(n) {}
    };
    node* head;
    node* tail;
    node* tmp;
    unsigned size_of_list;
    static Map<T,T2> *m_self;
public:
    Map() : head( NULL ), tail ( NULL ),tmp(head),size_of_list(0) {}
    bool empty() const { return ( !head || !tail ); }
    operator bool() const { return !empty(); }
    void insert(T,T2);
    void show();
    int  size();
    T2 find(T); // Return VALUE
    T find_ele(T);// Check if the KEY is present or not
    T2 begin(); //give the first ele
    bool erase(T);
    bool eraseall();
    bool isempty();
    ~Map()
    {
        while(head)
        {
            node* temp(head);
            head=head->next;
            size_of_list--;
            delete temp;
        }
    }
};

template <typename T,typename T2>
T2 Map<T,T2>::find(T d1)
{
    tmp = head;
    while(tmp)
    {
        if(tmp->data == d1)
        {
            return tmp->data2;
        }
        tmp = tmp->next;
    }
    return 0;
}

template <typename T,typename T2>
T Map<T,T2>::find_ele(T d1)
{
    tmp = head;
    while(tmp)
    {
        if(tmp->data == d1)
        {
            return tmp->data;
        }
        tmp = tmp->next;
    }
    return 0;
}

template <typename T,typename T2>
T2 Map<T,T2>::begin()
{
    tmp = head;
    if(tmp)
    {
        return (tmp->data2);
    }
    return 0;
}

template <typename T,typename T2>
void Map<T,T2>::show()
{
    tmp = head;
    while(tmp)
    {
        printf("%d-->%d\n",tmp->data,tmp->data2);
        tmp = tmp->next;
    }
}

template <typename T,typename T2>
int Map<T,T2>::size()
{
    int count =0;
    tmp = head;
    while(tmp)
    {
        tmp = tmp->next;
        count++;
    }
    return count;
}

template <typename T,typename T2>
void Map<T,T2>::insert(T data, T2 data2)
{
    tail = new node(data, data2,tail, NULL);
    if( tail->prev )
        tail->prev->next = tail;

    if( empty() )
    {
        head = tail;
        tmp=head;
    }
    tmp = head;
    size_of_list++;
}

template <typename T,typename T2>
bool Map<T,T2>::erase(T d)
{
    bool found = false;
    tmp = head;
    node* prevnode = tmp;
    node *tempnode;

    while(tmp)
    {
        if((head == tail) && (head->data == d))
        {
           found = true;
           tempnode = head;
           head = tail = NULL;
           delete tempnode;
           break;
        }
        if((tmp ==head) && (tmp->data ==d))
        {
            found = true;
            tempnode = tmp;
            tmp = tmp->next;
            tmp->prev = NULL;
            head = tmp;
            tempnode->next = NULL;
            delete tempnode;
            break;
        }
        if((tmp == tail) && (tmp->data ==d))
        {
            found = true;
            tempnode = tmp;
            prevnode->next = NULL;
            tmp->prev = NULL;
            tail = prevnode;
            delete tempnode;
            break;
        }
        if(tmp->data == d)
        {
            found = true;
            prevnode->next = tmp->next;
            tmp->next->prev = prevnode->next;
            tempnode = tmp;
            //tmp = tmp->next;
            delete tempnode;
            break;
        }
        prevnode = tmp;
        tmp = tmp->next;
    }
    if(found)size_of_list--;
    return found;
}

template <typename T,typename T2>
bool Map<T,T2>::eraseall()
{
    // Be careful while using this method
    // it not only removes the node but FREES(not delete) the allocated
    // memory.
    node *tempnode;
    tmp = head;
    while(head)
    {
       tempnode = head;
       head = head->next;
       tempnode->next = NULL;
       if(tempnode->data)
           free(tempnode->data);
       if(tempnode->data2)
           free(tempnode->data2);
           delete tempnode;
    }
    tail = head = NULL;
    return true;
}


template <typename T,typename T2>
bool Map<T,T2>::isempty()
{
    if(!size_of_list) return true;
    else return false;
}

#endif // _MAP_H_
