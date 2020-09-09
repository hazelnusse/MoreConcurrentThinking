#include "actors.hpp"
#include <map>

using account_id = unsigned long long;
using currency_value = unsigned long long;

struct get_balance {
  account_id account;
  messaging::sender response_target;
};

struct account_balance {
  account_id account;
  currency_value balance;
};

struct transfer_money {
  account_id from_account;
  account_id to_account;
  currency_value amount;
  messaging::sender response_target;
};

struct transfer_success {
  account_id from_account;
  account_id to_account;
  currency_value amount;
};

struct transfer_failed {
  account_id from_account;
  account_id to_account;
  currency_value amount;
};

struct deposit_money {
  account_id account;
  currency_value amount;
  messaging::sender response_target;
};

struct deposit_success {
  account_id account;
  currency_value amount;
};

struct withdraw_money {
  account_id account;
  currency_value amount;
  messaging::sender response_target;
};

struct withdrawal_success {
  account_id account;
  currency_value amount;
};

struct withdrawal_failure {
  account_id account;
  currency_value amount;
};

class bank : public messaging::actor {
  std::map<account_id, currency_value> balances;

public:
  void run() override {
    while(true) {
      incoming.wait()
        .handle<get_balance>([&](get_balance const &query) {
          query.response_target.send(account_balance{
            query.account, balances[query.account]});
        })
        .handle<transfer_money>(
          [&](transfer_money const &transfer) {
            if(balances[transfer.from_account] <
              transfer.amount) {
              transfer.response_target.send(
                transfer_failed{transfer.to_account,
                  transfer.from_account, transfer.amount});
            } else {
              balances[transfer.to_account] +=
                transfer.amount;
              balances[transfer.from_account] -=
                transfer.amount;
              transfer.response_target.send(
                transfer_success{transfer.to_account,
                  transfer.from_account, transfer.amount});
            }
          })
        .handle<deposit_money>(
          [&](deposit_money const &deposit) {
            balances[deposit.account] += deposit.amount;
            deposit.response_target.send(deposit_success{
              deposit.account, deposit.amount});
          })
        .handle<withdraw_money>(
          [&](withdraw_money const &withdrawal) {
            if(balances[withdrawal.account] <
              withdrawal.amount) {
              withdrawal.response_target.send(
                withdrawal_failure{
                  withdrawal.account, withdrawal.amount});
            } else {
              balances[withdrawal.account] -=
                withdrawal.amount;
              withdrawal.response_target.send(
                withdrawal_success{
                  withdrawal.account, withdrawal.amount});
            }
          });
    }
  }
};
