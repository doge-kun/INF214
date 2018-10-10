#include "alang.hpp"
using namespace std;

int main() {
	processes ps;
	for (int i : range(0, 10)) ps += [&, i] { cout << i; };
}
