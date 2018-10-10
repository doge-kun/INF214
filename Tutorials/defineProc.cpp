#include "alang.hpp"

using namespace std; // bring in all names from the standard library (cout for example)

int x = 0; // shared variable

int main() {
  processes ps; // ps will hold all processes

  ps += [&] { // define a task and start executing it
    while (x == 0); 
    cout << "x is no longer 0\n"; 
  }; 
  ps += [&] {  // define another task and start executing it
    sleep(2s);   // wait a bit
    x = x + 1;
  };

  // ps goes out of scope here, so execution waits until both tasks have finished
}
