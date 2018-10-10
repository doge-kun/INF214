#include "alang.hpp"
using namespace std;

int main() {
  int sum = 0;
  {  // note this extra scope
    processes ps;

    for (int i=0; i<1000; ++i) ps += [&,i]{ sum = sum + i; };
  } // ps goes out of scope here. Execution waits until all processes finished

  cout << "Sum is = " << sum << endl; 
}

