// Viet Hoang Nguyen

#include "alang.hpp"
#include <queue>

class Bank : monitor {
	cond cv;
	int balance;
	std::queue<int> transactions;
	unsigned int transactionID;
	const bool debug;
public:
	Bank::Bank(bool debug) : debug(debug) {
		balance = 0;
	}

	int deposit(int amount) {
		SYNC;
		balance += amount;
		signal(cv);
		return balance;
	}

	int withdraw(int amount) {
		SYNC;
		while (balance < amount) wait(cv);
		balance -= amount;
		return balance;
	}
	int getBalance() {
		SYNC;
		return balance;
	}
};

int main(void) {
	const int C = 100;
	{
		Bank bank;
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.deposit(1); };
		}
		return bank.getBalance() == C;
	}

	{
		Bank bank;
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
		}
		return bank.getBalance() == 0;
	}

	{
		Bank bank;
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
			ps += [&bank] { bank.deposit(3); };
			ps += [&bank] { bank.withdraw(1); };
		}
		return bank.getBalance() == C;
	}
}