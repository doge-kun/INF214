// Viet Hoang Nguyen

#include "alang.hpp"

class Bank : monitor {
	cond cv;
	int balance = 0;

public:
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
	const int C = 234;
	/*{
		Bank bank;
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.deposit(1); };
		}
		std::cout << "Balance 1 is " << bank.getBalance() << std::endl;
		if(bank.getBalance() == C) {
			alang::logl("Pass 1");
		}
	}

	{
		Bank bank;
		bank.deposit(C);
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
		}
		std::cout << "Balance 2 is " << bank.getBalance() << std::endl;
		if(bank.getBalance() == C) {
			alang::logl("Pass 2");
		}

	}

	{
		Bank bank;
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
			ps += [&bank] { bank.deposit(1); };
		}
		std::cout << "Balance 3 is " << bank.getBalance() << std::endl;
		if(bank.getBalance() == 0) {
			alang::logl("Pass 3");
		}

	}*/
	Bank bank;
	{
		processes ps;
		for (int i : range(0, C)) {
			ps += [&bank] { bank.deposit(1); };
		}
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
		}
		for (int i : range(0, C)) {
			ps += [&bank] { bank.withdraw(1); };
			ps += [&bank] { bank.withdraw(1); };
			ps += [&bank] { bank.deposit(3); };
		}

	}
	//alang::logl("Balance is " + bank.getBalance());
	std::cout << "Balance is " << bank.getBalance() << std::endl;
	if(bank.getBalance() == C) {
		alang::logl("Pass!");
	} else {
		alang::logl("Fail!");
	}
}