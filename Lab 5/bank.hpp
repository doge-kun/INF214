
#ifndef BANK_HPP
#define BANK_HPP

#include "alang.hpp"
#include <queue>

class Bank : monitor {
private:
  cond cv;
  int balance;
  std::queue<int> transactions;
  unsigned int transaction_id;
  const bool debug;
public:
  Bank(bool debug = false);
  int deposit(int amount);
  int withdraw(int amount);
  int get_balance();
};

#endif /* ifndef BANK_HPP  */
