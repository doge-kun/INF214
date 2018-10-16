// Viet Hoang Nguyen

#include "alang.hpp"

int gcd(int n, int m) {
  A<int> x = n;
  A<int> y = m;
  while (x != y) {
    processes ps;
    ps += [&] { ATO if(x > y) x = x - y; MIC; };
    ps += [&] { ATO if(x < y) y = y - x; MIC; };
  }
  return x;
}

int main() {
  logl(gcd(2, 3));
  logl(gcd(25235, 2345));
  logl(gcd(310, 150));
}

