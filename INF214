#include "alang.hpp"
using namespace std;

int main() {
  A<int> x = 0; // must use A<int>; otherwise ATOMIC has no effect

  for (int i : range(0, 100000)) 
    CO([&]{ ATOMIC([&]{x = x + 1;}); }, 
       [&]{ ATOMIC([&]{x = x + 1;}); },
       [&]{ ATOMIC([&]{x = x + 1;}); }, 
       [&]{ ATOMIC([&]{x = x + 1;}); });

  cout << x << endl;
}
