#include "alang.hpp"
using namespace std;

int main() {
  semaphore sem;

  CO([&]{ cout << "Waiting... "; sem.P(); cout << "Got through." << endl; },
     [&]{ sleep(1s); cout << "Releasing... "; sem.V(); });
}
