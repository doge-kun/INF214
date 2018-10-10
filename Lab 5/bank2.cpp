// Viet Hoang Nguyen

#include "bank.hpp"
#include <chrono>
#include <thread>

class Bank : monitor {
    cond cv;
    int balance;
    std::queue<int> transactions;
    unsigned int transaction_id;
    const bool debug;

public:
    Bank(bool debug) : debug(debug) {
        balance = 0;
        transaction_id = 0;
    }

    int deposit(int amount) {
        SYNC;
        balance += amount;
        signal(cv);
        return balance;
    }

    int withdraw(int amount) {
        // Get an id
        int id;
        {
            SYNC;
            id = transaction_id++;
            if (debug) alang::logl("Request for ", amount, " got id ", id);
            transactions.push(id);
        }

        // Try until ok
        while (true) {
            SYNC;

            if (debug) alang::logl(id, " trying to withdraw.");

            // Check if ok to withdraw
            bool balance_too_low = balance < amount;
            bool not_my_turn = transactions.front() != id;

            if (balance_too_low || not_my_turn) {
                if (debug) alang::logl(id, balance_too_low
                                           ? " failed, but not enough money. Waiting."
                                           : " failed, but not first in queue. Waiting.");
                wait(cv);
                continue;
            }

            if (debug) alang::logl(id, " succeeded!");
            transactions.pop();
            signal(cv);
            balance -= amount;
            return balance;
        }
    }

    int getBalance() {
        SYNC;
        return balance;
    }
};

namespace Test {
  bool test_fifo();
  bool test_deposit(int N);
  bool test_withdraw(int N);
  bool test_deposit_withdraw(int N);
}

int main(void) {
  const int N = 10000;

  Test::test_fifo();
  Test::test_deposit(N);
  Test::test_withdraw(N);
  Test::test_deposit_withdraw(N);

  alang::logl("Tests passed");
}

namespace Test {
  bool test_fifo() {
    Bank bank(true);
    bank.deposit(200);

    processes ps;

    // Start two processes, delay between both
    ps += [&bank] { bank.withdraw(300); }; alang::sleep_ms(500);
    ps += [&bank] { bank.withdraw(100); }; alang::sleep_ms(500);

    // Nobody should have taken yet
    assert(bank.get_balance() == 200);

    alang::logl("Depositing 100");
    bank.deposit(100); // Deposit
    alang::sleep_ms(100); // Sleep to give a thread time to withdraw
    assert(bank.get_balance() == 0); // First should have taken

    alang::logl("Depositing 100");
    bank.deposit(100); // Deposit again
    alang::sleep_ms(100); // Sleep to give a thread time to withdraw
    assert(bank.get_balance() == 0); // Second should have taken
    return true;
  }

  bool test_deposit(int N) {
    Bank bank;
    processes ps;
    for (int i : range(0, N)) {
      ps += [&bank]{ bank.deposit(1); };
    }
    return bank.get_balance() == N;
  }

  bool test_withdraw(int N) {
    Bank bank;
    bank.deposit(N);
    processes ps;
    for (int i : range(0, N)) {
      ps += [&bank] { bank.withdraw(1); };
    }
    return bank.get_balance() == 0;
  }

  bool test_deposit_withdraw(int N) {
    Bank bank;
    processes ps;
    for (int i : range(0, N)) {
      ps += [&bank] { bank.withdraw(1); };
      ps += [&bank] { bank.deposit(3); };
      ps += [&bank] { bank.withdraw(1); };
    }
    return bank.get_balance() == N;
  }
}
