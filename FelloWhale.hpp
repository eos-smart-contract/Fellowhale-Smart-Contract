#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>


// The time after which a new round begins when no whale is claimed
// 5 days (in seconds)
#define WHALE_LIFE_TIME_LIMIT 60 * 60 * 24 * 5
#define CLAIM_MULTIPLIER 1.21
#define COMMISSION_PERCENTAGE_POINTS 0.03



class fellowhale : public eosio::contract
{
  public:
    fellowhale(account_name self)
        : contract(self),
          felloclaims(self, self) {}

    struct claim
    {
        claim(){};
        claim(account_name name) : name(name) {};
        claim(account_name name, std::string whaleMessage) : name(name), whaleMessage(whaleMessage) {};

        account_name name;
        std::string whaleMessage;

        EOSLIB_SERIALIZE(claim, (name)(whaleMessage))
    };
    


    // @abi table felloclaims i64
    struct claim_record
    {
        claim_record(){};
        claim_record(   uint64_t whaleIndex, 
                        time claimTime, 
                        claim claim, 
                        uint16_t roundNumber, 
                        uint16_t roundWhaleNumber)
            :   whaleIndex(whaleIndex), 
                claimTime(claimTime), 
                claim(claim), 
                roundNumber(roundNumber), 
                roundWhaleNumber(roundWhaleNumber){};

        uint64_t    whaleIndex;
        time        claimTime;
        claim       claim;
        uint16_t    roundNumber;
        uint16_t    roundWhaleNumber;

        uint64_t primary_key() const { return whaleIndex; }
        
        EOSLIB_SERIALIZE(claim_record, (whaleIndex)(claimTime)(claim)(roundNumber)(roundWhaleNumber))
    };



    // @abi action init
    struct init
    {
        init() {};
        
        account_name name;

        EOSLIB_SERIALIZE(init, (name))
    };

    

    // @abi action end
    struct end
    {
        end() {};

        account_name name;

        EOSLIB_SERIALIZE(end, (name))
    };

    

    typedef eosio::multi_index<N(felloclaims), claim_record> felloclaims_db;

    void onTransfer(const eosio::currency::transfer& transfer);
    void end ();
    void init(account_name name);
    void apply( account_name contract, account_name act );

    private:
        felloclaims_db felloclaims;
};

