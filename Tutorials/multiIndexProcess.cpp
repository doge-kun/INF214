#include "alang.hpp"
using namespace std;

int main() {
  vector<int> numbers = {10, 2, 22, 7};

  {  // note this extra scope
    processes ps;

    for (int i=0; i<numbers.size(); ++i) {
      for (int j=i+1; j<numbers.size(); ++j) {
        ps += [&, i, j] {
          if (numbers[i] > numbers[j]) {
            int tmp = numbers[i];
            numbers[i] = numbers[j];
            numbers[j] = tmp;
          }
        };
      }
    }
  }
  for (int i=0; i<numbers.size(); ++i) { cout << numbers[i] << " "; }
}
