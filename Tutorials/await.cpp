#include "alang.hpp"
using namespace std;

int main() {
  A<int> x = 0;
  CO([&]{ ATO AWAIT (x == 100); x = -100; MIC; },
     [&]{ ATO while (x < 100) x = x + 1; MIC; });
  alang::logl(x);
}
