#include "alang.hpp"
#include "thread"

using std::cout;
using std::endl;

int main() {
  // declare a variable that contains processes
  processes ps;

  // add processes the container; they will start running immediately
  ps += [&] { cout << "Hello, "; };
  ps += [&] { cout << "World!" << endl; };
};
