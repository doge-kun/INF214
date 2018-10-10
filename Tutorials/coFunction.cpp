#include "alang.hpp"
using namespace std;

int main() {
  int x = 0;

  for (int i : range(0, 1000000)) 
    CO([&]{ x = x + 1; }, [&]{ x = x + 1; });

  cout << x << endl;
}
