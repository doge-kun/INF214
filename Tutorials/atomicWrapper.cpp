#include "alang.hpp"
using namespace std;

int main() {
  A<int> x = 0; // must use A<int>; otherwise ATOMIC has no effect

  for (int i : range(0, 100000))
    CO([&]{ ATO x = x + 1; MIC; },
       [&]{ ATO x = x + 1; MIC; },
       [&]{ ATO x = x + 1; MIC; },
       [&]{ ATO x = x + 1; MIC; });

  cout << x << endl;
}

