// Viet Hoang Nguyen

#include "alang.hpp"
#include <chrono>
#include <thread>
#include <queue>

class Bank : monitor {
    cond cv;
    int balance = 0;
    std::queue<std::thread::id> transactionIDs;
    //unsigned int transactionID = 0;
    //const bool debug = true;

public:
    int deposit(int amount) {
        SYNC;
        balance += amount;
        signal(cv);
        return balance;
    }

    int withdraw(int amount) {
        //The following commented out code gets stuck if the withdraw requests are not executed in the correct order
        //I have thus just commented it out and tried another solution. It works otherwise.
        /*int id;
        {
            SYNC;
            id = transactionID++;
            if (debug) {
                alang::logl("Request to withdraw $", amount, " has been assigned the queue #", id);
            }
            transactions.push(id);
        }

        while (true) {
            SYNC;

            if (debug) {
                alang::logl("Queuer #", id, " is trying to withdraw.");
            }

            bool balance_too_low = balance < amount;
            bool not_my_turn = transactions.front() != id;

            if (balance_too_low || not_my_turn) {
                if (debug) {
                    alang::logl("Queuer #", id, balance_too_low
                                                ? " failed, not enough money in bank. Waiting.\n"
                                                : " failed, not first in queue. Waiting.\n");
                }
                wait(cv);
                continue;
            }

            if (debug) {
                alang::logl("Queuer #", id, " succeeded!\n");
            }
            transactions.pop();
            signal(cv);
            balance -= amount;
            return balance;
        }*/
        SYNC;

        std::thread::id id = std::this_thread::get_id();
        transactionIDs.push(id);

        while (id != transactionIDs.front() || balance < amount) {
            wait(cv);
        };

        balance -= amount;
        transactionIDs.pop();
        signal(cv);

        return balance;
    }

    int getBalance() {
        SYNC;
        return balance;
    }
};

int main(void) {
  const int C = 10;
  Bank bank;
  {
      bank.deposit(200);
      alang::logl("Depositing $200. \nBalance is: ", bank.getBalance(), "\n");

      processes ps;

      ps += [&bank] { bank.withdraw(300); }; alang::sleep_ms(500);
      ps += [&bank] { bank.withdraw(100); }; alang::sleep_ms(500);

      assert(bank.getBalance() == 200);
      alang::logl("Balance should sill be $200. \nGetting balance: ", bank.getBalance(), "\n");

      alang::logl("Depositing $100.\n");
      bank.deposit(100);
      alang::sleep_ms(100);
      assert(bank.getBalance() == 0);

      alang::logl("Depositing $100.\n");
      bank.deposit(100);
      alang::sleep_ms(100);
      assert(bank.getBalance() == 0);

      alang::logl("-------Depositing and then withdrawing.-------\n");
      for (int i : range(0, C)) {
          ps += [&bank] { bank.deposit(1); };
      }
      for (int i : range(0, C)) {
          ps += [&bank] { bank.withdraw(1); };
      }

      alang::logl("-------Deposits and withdraws at the same time.-------\n");
      for (int i : range(0, C)) {
          ps += [&bank] { bank.withdraw(1); };
          ps += [&bank] { bank.withdraw(1); };
          ps += [&bank] { bank.deposit(3); };
      }
  }

    alang::logl("Balance is ", bank.getBalance());
    if(bank.getBalance() == C) {
        alang::logl("Pass!");
    } else {
        alang::logl("Fail!");
    }
}