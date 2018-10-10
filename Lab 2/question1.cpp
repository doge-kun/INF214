#include "alang.hpp"

using namespace std;

int x = 3;

int main() {
	CO([&]{ AWAIT (x >= 3); x = x - 3;}, [&]{ AWAIT (x >= 2); x = x - 2; }, [&]{ AWAIT (x == 1); x = x + 5; });
	alang::logl(x);
}
