#include "alang.hpp"
#include <iostream>
#include <cassert>
using namespace std;

template <typename E> class queue;    // forward declaration 
template <typename E> class node_iterator;  // forward declaration

template <typename E>
class node {
  E data;
  A<node<E>*> next;
  node(E d, node<E>* n) : data(d), next(n) {}
public:

  friend class queue<E>;         // allow queue<E> to access private members
  friend class node_iterator<E>; // allow node_iterator<E> to access private members
};

template <typename E>
class node_iterator {
  node<E>* it;
public:
  node_iterator(node<E>* n) : it(n) {}
  node_iterator& operator++() { assert(it != nullptr); it = it->next; return *this; }
  bool done() const { return it == nullptr; }
  E operator*() const { assert(it != nullptr); return it->data; }
};

template <typename E> 
class queue {
  A<node<E>*> head;
  A<node<E>*> rear;
  A<int> _size;
public:
  queue() : head(nullptr), rear(nullptr), _size(0) {}

  bool empty() const { return _size == 0; }
  // alternatively we could test for head or rear being equal to nullptr

  void enqueue(E d) {
    node<E>* newNode = new node<E>(d, nullptr);
    ATO{
           if (rear == nullptr) { // enqueueing the first element
               head = newNode;
           } else {
               rear.read()->next = newNode;
           }
           rear = newNode;
           _size = _size + 1;
       }MIC;
  }

  E dequeue() {
    node<E>* oldHead;

    ATO{
           if (empty()) throw "empty queue";
           oldHead = head;
           head = head.read()->next;
           if (head.read() == nullptr) rear = nullptr; // removed the last element
           _size = _size - 1;
       }MIC;

    E e = oldHead->data;
    delete oldHead;

    return e;
  }

  int size() const {
    return _size.read();
  }

  node_iterator<E> iterator() {
    return head.read();
  }
};


int main() {
    queue<int> l;
    int threads = 1000;
    int incs = 100;
    A<int> queued = 0;

    {
        processes ps;

        for (int i=0; i<= threads; i++) {
            ps += [&, i] {
                for (int j=0; j <= incs; j++) {
                    l.enqueue(i);
                    ATO queued = queued+1; MIC;
                    if (j%2 == 0) {
                        l.dequeue();
                        ATO queued = queued - 1; MIC;
                    }
                }
            };
        }
    }

    int size = l.size();
    int actualSize = 0;
    for (auto it = l.iterator(); !it.done(); ++it) actualSize++;

    int nSuccessful = 0;
    while(true) {
        try {
            l.dequeue();
            nSuccessful++;
        } catch (...) {
            break;
        }
    }

    std::cout
            << "Queued: " << queued << std::endl
            << "Size: " << size << std::endl
            << "Iterated: " << actualSize << std::endl
            << "Dequeued: " <<nSuccessful << std::endl;
}
