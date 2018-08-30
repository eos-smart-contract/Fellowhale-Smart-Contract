#include "./FelloWhale.hpp"

#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <cmath>

#undef CORE_SYMBOL
#define CORE_SYMBOL S(4,EOS)

using namespace eosio;
using namespace std;



inline uint64_t whaleOrderToClaimPrice(uint16_t whaleOrder)
{
    return pow(CLAIM_MULTIPLIER, whaleOrder) * 1E4;
}



void fellowhale::onTransfer(const currency::transfer &transfer)
{
    if (transfer.to != _self)
    {
        return;
    }

    eosio_assert(transfer.from != _self, "This contract cannot become a Fellowhale himself");
    eosio_assert(transfer.quantity.symbol == CORE_SYMBOL, "Use EOS tokens only, please");

    auto itr = felloclaims.end();
    --itr; // itr points to last element
    eosio_assert(itr != felloclaims.end(), "No previous claim exists");
    
    claim_record latestClaimRecord = *itr;
    
    uint64_t claimPrice = whaleOrderToClaimPrice(latestClaimRecord.roundWhaleNumber);

    eosio_assert(transfer.quantity.amount == claimPrice, "Wrong claim price"); // TODO: Add what price should be
    eosio_assert(transfer.memo.length() <= 100, "Message is too long (should be less than 100 characters)");

    claim newClaim(transfer.from, transfer.memo);

    felloclaims.emplace(_self, [&](claim_record &claimRecord) {
        claimRecord.whaleIndex = latestClaimRecord.whaleIndex + 1;
        claimRecord.claimTime = now();  
        claimRecord.claim = newClaim;
        claimRecord.roundNumber = latestClaimRecord.roundNumber;
        claimRecord.roundWhaleNumber = latestClaimRecord.roundWhaleNumber + 1;
    });

    // First whale is always a deployed contract itself
    // Cannot send transfer from itself to itself
    if (latestClaimRecord.claim.name != _self)
    {
        asset amount = asset{(int64_t)((CLAIM_MULTIPLIER - COMMISSION_PERCENTAGE_POINTS) / CLAIM_MULTIPLIER * claimPrice), transfer.quantity.symbol};
        action{
            permission_level{_self, N(active)},
            N(eosio.token),
            N(transfer),
            currency::transfer{
                .from = _self, 
                .to = latestClaimRecord.claim.name, 
                .quantity = amount, 
                .memo = "Oh no! You were swallowed by the new Whale -- (Always yours, Fellowhale)"
                }
            }
            .send();
    }
}



void fellowhale::end()
{
    // anyone can end the round
    auto itr = felloclaims.end();
    --itr; // itr now points to last element
    eosio_assert(itr != felloclaims.end(), "no previous claim exists");

    time lastClaimTime = itr->claimTime;

    eosio_assert(now() > lastClaimTime + WHALE_LIFE_TIME_LIMIT, "Max Fellowhale time not reached yet");

    felloclaims.emplace(_self, [&](claim_record &claimRecord) {        
        claimRecord.whaleIndex = itr->whaleIndex + 1;
        claimRecord.claimTime = now();
        claimRecord.claim = claim(_self);
        claimRecord.claim.whaleMessage = "Starting New Round!";
        claimRecord.roundNumber = itr->roundNumber + 1;
        claimRecord.roundWhaleNumber = 1;
    });
}



void fellowhale::init(account_name name)
{
    print("Fellowhale - Version 1.0");

    require_auth(_self);    // Only initialized by the author

    // make sure table felloclaims is empty
    eosio_assert(felloclaims.begin() == felloclaims.end(), "already initialized");
    
    felloclaims.emplace(_self, [&](claim_record &claimRecord) {
        claimRecord.whaleIndex = (uint64_t) 1;
        claimRecord.claimTime = now();
        claimRecord.claim = claim(_self);
        claimRecord.claim.whaleMessage = "Starting New Round!";
        claimRecord.roundNumber = (uint16_t) 1;
        claimRecord.roundWhaleNumber = (uint16_t) 1;
    });
}



void fellowhale::apply(account_name contract, account_name act)
{
    if (contract == N(eosio.token) && act == N(transfer))
    {
        onTransfer(unpack_action_data<currency::transfer>());
        return;
    }

    if (contract != _self)
    {
        return;
    }

    // needed for EOSIO_API macro
    auto &thiscontract = *this;
    switch (act)
    {
        EOSIO_API(fellowhale, (init)(end))
    };
}



extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        fellowhale whale(receiver);
        whale.apply(code, action);
        eosio_exit(0);
    }
}
