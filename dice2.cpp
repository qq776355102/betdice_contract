#include <utility>
#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <boost/algorithm/string.hpp>

using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;
using eosio::unpack_action_data;
using eosio::symbol_type;
using eosio::transaction;
using eosio::time_point_sec;


#define REVEALER N(fuckdice1234)
#define REFUNDER N(zhaojixing12)
#define EOS_SYMBOL S(4, EOS)
// #define BET_SYMBOL S(4, BET)

class EOSBetDice : public eosio::contract {
	public:
		const uint32_t TWO_MINUTES = 2 * 60;
		const uint64_t MINBET = 1000; 
		const uint64_t HOUSEEDGE_times10000 = 200;
		const uint64_t HOUSEEDGE_REF_times10000 = 150;
		const uint64_t REFERRER_REWARD_times10000 = 50;

		const uint64_t BETID_ID = 1;
		const uint64_t TOTALAMTBET_ID = 2;
		const uint64_t TOTALAMTWON_ID = 3;
		const uint64_t LIABILITIES_ID = 4;

		EOSBetDice(account_name self):eosio::contract(self),
			// 初始化列表
			activebets(_self, _self),
			globalvars(_self, _self),
			randkeys(_self, _self),
			_global(_self, _self)
		{}

		struct st_result{
			uint64_t 		bet_id;
			account_name	bettor; // 下注人
			account_name	referral; // 推荐人
			asset			bet_amt;  // 下注金额
			uint8_t			roll_under; // 押注点数
			checksum256		user_seed; // 用户种子
			std::string		house_seed; // 服务器种子
			checksum256		house_hash; // 服务器哈希
			uint8_t 		result;
			asset			payout;
		};

		// @abi action
		void initcontract(public_key randomness_key){

			require_auth(REVEALER);

			auto globalvars_itr = globalvars.begin();
			eosio_assert(globalvars_itr == globalvars.end(), "Contract is init");

			globalvars.emplace(_self, [&](auto& g){
				g.id = BETID_ID;
				g.val = 0;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = TOTALAMTBET_ID;
				g.val = 0;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = TOTALAMTWON_ID;
				g.val = 0;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = LIABILITIES_ID;
				g.val = 0;
			});

			randkeys.emplace(_self, [&](auto & k){
				k.id = 1;
				k.key = randomness_key;
			});
		}

		// @abi action
		void newrandkey(public_key randomness_key){

			require_auth(REVEALER);

			auto rand_itr = randkeys.begin();
			randkeys.modify(rand_itr, _self, [&](auto& k){
				k.key = randomness_key;
			});
		}

		// @abi action
		void suspendbet(const uint64_t bet_id){

			require_auth(REVEALER);

			auto activebets_itr = activebets.find(bet_id);
			eosio_assert(activebets_itr != activebets.end(), "No bet exists");

			std::string bettor_str = name_to_string(activebets_itr->bettor);

			decrement_liabilities(activebets_itr->bet_amt);

			action(
				permission_level{_self, N(active)},
				N(eosio.token), 
				N(transfer),
				std::make_tuple(
					_self, 
					activebets_itr->bettor, 
					asset(activebets_itr->bet_amt, symbol_type(S(4, EOS))), 
					bettor_str
				)
			).send();

			activebets.erase(activebets_itr);
		}

		// @abi action
		void transfer(const account_name from, const account_name to, const asset& quantity, const std::string& memo) {

			eosio::print("Code is debug\n");

			if (from == _self || to != REVEALER) {
				return;
			}

			st_transfer transfer_data{.from = from,
									  .to = to,
									  .quantity = quantity,
									  .memo = memo};


			eosio_assert( transfer_data.quantity.is_valid(), "Invalid asset");

			const uint64_t your_bet_amount = (uint64_t)transfer_data.quantity.amount;
			eosio_assert(MINBET <= your_bet_amount, "Must bet greater than min");

			increment_liabilities_bet_id(your_bet_amount);

			uint8_t roll_under;
    		checksum256 house_seed_hash;
    		checksum256 user_seed_hash;
    		account_name referral = N(starrystarry);
			uint64_t house_edge = HOUSEEDGE_times10000;

    		parse_memo(memo, &roll_under, &house_seed_hash, &user_seed_hash, &referral);
			eosio_assert( roll_under >= 2 && roll_under <= 96, "Roll must be >= 2, <= 96.");

			const uint64_t your_win_amount = (your_bet_amount * get_payout_mult_times10000(roll_under, house_edge) / 10000) - your_bet_amount;
			eosio_assert(your_win_amount <= get_max_win(), "Bet less than max");

			activebets.emplace(_self, [&](auto& bet){
				bet.id = next_id();
				bet.bettor = transfer_data.from;
				bet.referral = referral;
				bet.bet_amt = your_bet_amount;
				bet.roll_under = roll_under;
				bet.user_seed = user_seed_hash;
				bet.house_seed_hash = house_seed_hash;
				bet.bet_time = time_point_sec(now());
			});
		}

		// @abi action
		void reveal(const uint64_t bet_id, const std::string& house_seed) {
			
			require_auth(REVEALER);

			auto activebets_itr = activebets.find( bet_id );
			eosio_assert(activebets_itr != activebets.end(), "Bet doesn't exist");

			assert_sha256(house_seed.c_str(), strlen(house_seed.c_str()), (const checksum256*)&activebets_itr->house_seed_hash);
			
			const uint8_t random_roll = compute_random_roll(activebets_itr->user_seed, house_seed, std::to_string(bet_id));

			uint64_t edge = HOUSEEDGE_REF_times10000;
			uint64_t ref_reward = 0;
			uint64_t payout = 0;
			if (activebets_itr->referral != REVEALER){
				edge = HOUSEEDGE_REF_times10000;
				ref_reward = activebets_itr->bet_amt * REFERRER_REWARD_times10000 / 10000;
			}

			if(random_roll < activebets_itr->roll_under){
				payout = (activebets_itr->bet_amt * get_payout_mult_times10000(activebets_itr->roll_under, edge)) / 10000;
			}

			increment_game_stats(activebets_itr->bet_amt, payout);
			decrement_liabilities(activebets_itr->bet_amt);

			if (payout > 0){
				action(
					permission_level{_self, N(active)},
					N(eosio.token), 
					N(transfer),
					std::make_tuple(
						_self, 
						activebets_itr->bettor, 
						asset(payout, symbol_type(EOS_SYMBOL)), 
						std::string("Bet id: ") + std::to_string(bet_id) + std::string(" -- Winner! Play: playdice.io, roll number: " + std::to_string(random_roll) )
					)
				).send();
			}

			st_result result{.bet_id = bet_id,
							 .bettor = activebets_itr->bettor,
							 .referral = activebets_itr->referral,
							 .bet_amt = asset(activebets_itr->bet_amt, EOS_SYMBOL),
							 .roll_under = activebets_itr->roll_under,
							 .user_seed = activebets_itr->user_seed,
							 .house_seed = house_seed,
							 .house_hash = activebets_itr->house_seed_hash,
							 .result = random_roll,
							 .payout = asset(payout, EOS_SYMBOL)				
			};


			action(
				permission_level{_self, N(active)},
				_self,
				N(betreceipt),
				result
			).send();

			activebets.erase(activebets_itr);
		}

		// @abi action
		void clearbet(const uint64_t bet_id) {
			require_auth(REVEALER);
			auto activebets_itr = activebets.find( bet_id );
			eosio_assert(activebets_itr != activebets.end(), "Bet doesn't exist");
			activebets.erase(activebets_itr);
		}

		// @abi action
		void betreceipt(st_result& result) {
			require_auth(REVEALER);
 			// require_recipient( bettor );
		}
	
	private:
		// @abi table activebets i64
		struct bet {
			uint64_t 		id;
			account_name	bettor; // 下注人
			account_name	referral; // 推荐人
			uint64_t		bet_amt;  // 下注金额
			uint8_t			roll_under; // 押注点数
			checksum256		user_seed; // 用户种子哈希
			checksum256	    house_seed_hash; // 服务器种子哈希
			time_point_sec 	bet_time; // 时间戳
			
			uint64_t 		primary_key() const { return id; }
			
			EOSLIB_SERIALIZE( bet, (id)(bettor)(referral)(bet_amt)(roll_under)(user_seed)(house_seed_hash)(bet_time) )
		};

		typedef eosio::multi_index< N(activebets), bet> bets_index;

		// @abi table globalvars i64
		struct globalvar{
			uint64_t		id;  // 全局bet id
			uint64_t		val;

			uint64_t		primary_key() const { return id; }

			EOSLIB_SERIALIZE(globalvar, (id)(val));
		};

		typedef eosio::multi_index< N(globalvars), globalvar> globalvars_index;

		

		// @abi table randkeys i64
		struct randkey {
			uint64_t 		id;
			public_key		key;

			uint64_t		primary_key() const { return id; }
		};

		typedef eosio::multi_index< N(randkeys), randkey > randkeys_index;

		// @abi table global i64
		struct st_global {
			uint64_t id = 0;
    		uint64_t nextgameid = 0;

			uint64_t primary_key() const { return id; }
		};

		typedef eosio::multi_index<N(global), st_global> tb_global;

		// taken from eosio.token.hpp
		struct account {
	    	asset    balance;

	    	uint64_t primary_key() const { return balance.symbol.name(); }
	    };

		typedef eosio::multi_index<N(accounts), account> accounts;

		// taken from eosio.token.hpp
		struct st_transfer {
            account_name  from;
            account_name  to;
            asset         quantity;
            std::string   memo;
        };

        struct st_seeds{
         	checksum256 	seed1;
         	checksum256		seed2;
        };

		bets_index			activebets;
		globalvars_index	globalvars;
		randkeys_index		randkeys;
		tb_global 			_global;


		std::string name_to_string(uint64_t acct_int) const {
	     	static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

	      	std::string str(13,'.');

	      	uint64_t tmp = acct_int;

	      	for( uint32_t i = 0; i <= 12; ++i ) {
	         	char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
	         	str[12-i] = c;
	         	tmp >>= (i == 0 ? 4 : 5);
	      	}

	      	boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
	      	return str;
	   }

		void increment_liabilities_bet_id(const uint64_t bet_amt){

			auto globalvars_itr = globalvars.find(BETID_ID);
			
			globalvars.modify(globalvars_itr, _self, [&](auto& g){
				g.val++;
			});

			globalvars_itr = globalvars.find(LIABILITIES_ID);

			globalvars.modify(globalvars_itr, _self, [&](auto& g){
				g.val += bet_amt;
			});
		}

		void increment_game_stats(const uint64_t bet_amt, const uint64_t won_amt){

			auto globalvars_itr = globalvars.find(TOTALAMTBET_ID);

			globalvars.modify(globalvars_itr, _self, [&](auto& g){
				g.val += bet_amt;
			});

			if (won_amt > 0){
				globalvars_itr = globalvars.find(TOTALAMTWON_ID);

				globalvars.modify(globalvars_itr, _self, [&](auto& g){
					g.val += won_amt;
				});
			}
		}

		void decrement_liabilities(const uint64_t bet_amt){
			auto globalvars_itr = globalvars.find(LIABILITIES_ID);

			globalvars.modify(globalvars_itr, _self, [&](auto& g){
				g.val -= bet_amt;
			});
		}

		void parse_memo(std::string memo, uint8_t* roll_under, checksum256* house_seed_hash, checksum256* user_seed_hash, account_name* referrer) {
        	// remove space
        	memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { return std::isspace(x); }), memo.end());

        	size_t sep_count = std::count(memo.begin(), memo.end(), '-');
        	eosio_assert(sep_count == 3, "invalid memo");

        	size_t pos;
        	std::string container;
        	pos = sub2sep(memo, &container, '-', 0, true);
        	eosio_assert(!container.empty(), "no roll under");
        	*roll_under = stoi(container);
        	pos = sub2sep(memo, &container, '-', ++pos, true);
        	eosio_assert(!container.empty(), "no seed hash");
        	*house_seed_hash = hex_to_sha256(container);
        	pos = sub2sep(memo, &container, '-', ++pos, true);
        	eosio_assert(!container.empty(), "no user seed hash");
        	*user_seed_hash = hex_to_sha256(container);
        	container = memo.substr(++pos);
        	// eosio_assert(!container.empty(), "no referrer");
        	*referrer = eosio::string_to_name(container.c_str());
    	}

		size_t sub2sep(const std::string& input, std::string* output, const char& separator, const size_t& first_pos = 0, const bool& required = false) {
    		eosio_assert(first_pos != std::string::npos, "invalid first pos");
    		auto pos = input.find(separator, first_pos);
    		if (pos == std::string::npos) {
        		eosio_assert(!required, "parse memo error");
        		return std::string::npos;
    		}
    		*output = input.substr(first_pos, pos - first_pos);
    		return pos;
		}

		checksum256 hex_to_sha256(const std::string& hex_str) {
    		eosio_assert(hex_str.length() == 64, "invalid sha256");
    		checksum256 checksum;
    		from_hexs(hex_str, (char*)checksum.hash, sizeof(checksum.hash));
    		return checksum;
		}		

		void airdrop_tokens(const uint64_t bet_id, const uint64_t bet_amt, const account_name bettor){
			uint64_t drop_amt = (1 * bet_amt) / 30;

			const uint64_t bet_token_balance = get_token_balance( N(betdividends), symbol_type(EOS_SYMBOL) );

			if (bet_token_balance == 0){
				return;
			}
			else if (bet_token_balance < drop_amt){
				drop_amt = bet_token_balance;
			}
			action(
               permission_level{_self, N(active)},
               N(betdividends), 
               N(transfer),
               std::make_tuple(
                   _self, 
                   bettor, 
                   asset(drop_amt, symbol_type(EOS_SYMBOL)), 
                   std::string("Bet id: ") + std::to_string(bet_id) + std::string(" -- Enjoy airdrop! Play: playdice.io")
	            )
	       ).send();
		}

		uint64_t next_id() {
			auto global_itr = _global.begin();
			if (global_itr == _global.end()) {
				global_itr = _global.emplace(_self, [&](auto& g){
					g.nextgameid = 0;
				});
			}

			// Increment global bet counter
			_global.modify(global_itr, 0, [&](auto& g){
				g.nextgameid++;
			});

        	return global_itr->nextgameid;
    	}

		std::string to_hex(const char* d, uint32_t s) {
    		std::string r;
    		const char* to_hex = "0123456789abcdef";
    		uint8_t* c = (uint8_t*)d;
    		for (uint32_t i = 0; i < s; ++i)
        		(r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    		return r;
		}

		std::string sha256_to_hex(const checksum256& sha256) {
    		return to_hex((char*)sha256.hash, sizeof(sha256.hash));
		}

		uint8_t compute_random_roll(const checksum256& seed1, const std::string& seed2, const std::string& id) {
        	eosio::print("compute random roll\n");
        	uint8_t index = 0;
			checksum256 seed_hash;

			std::string seed_string = sha256_to_hex(seed1) + seed2 + id;
			sha256( seed_string.c_str(), seed_string.length(), &seed_hash );

			// printhex(&seed_hash, sizeof(seed_hash));

			std::string hash_string = sha256_to_hex(seed_hash);

			long value;
			// checksum256 lucky;
			do {				
				std::string pre_five = hash_string.substr(index, 5);
				char* str;
				value = strtol(pre_five.c_str(), &str, 16);
				index += 5;
			} while(value >= 1000000);

			// printf("%ld\n", value);
			
        	// hash = uint64_hash_string(sha256_to_hex(seed1) + sha256_to_hex(seed2) + id);
        	return value % 100 + 1;
    	}

		uint64_t uint64_hash_string(const std::string& hash) {
    		return std::hash<std::string>{}(hash);
		}

		uint64_t uint64_hash(const checksum256& hash) {
    		return uint64_hash_string(sha256_to_hex(hash));
		}

		uint8_t from_hex(char c) {
    		if (c >= '0' && c <= '9') return c - '0';
    		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    		// eosio_assert(false, "Invalid hex character");
    		return 0;
		}

		size_t from_hexs(const std::string& hex_str, char* out_data, size_t out_data_len) {
    		auto i = hex_str.begin();
    		uint8_t* out_pos = (uint8_t*)out_data;
    		uint8_t* out_end = out_pos + out_data_len;
    		while (i != hex_str.end() && out_end != out_pos) {
        	*out_pos = from_hex((char)(*i)) << 4;
        	++i;
        	if (i != hex_str.end()) {
            	*out_pos |= from_hex((char)(*i));
            	++i;
        	}
        		++out_pos;
    		}
    		return out_pos - (uint8_t*)out_data;
		}


		uint64_t get_token_balance(const account_name token_contract, const symbol_type& token_type) const {

			accounts from_accounts(token_contract, _self);

			const auto token_name = token_type.name();
			auto my_account_itr = from_accounts.find(token_name);
			if (my_account_itr == from_accounts.end()){
				return 0;
			}
			const asset my_balance = my_account_itr->balance;
			return (uint64_t)my_balance.amount;
		}

		uint64_t get_payout_mult_times10000(const uint8_t roll_under, const uint64_t house_edge_times_10000) const {

			return ((10000 - house_edge_times_10000) * 100) / (roll_under - 1);
		}

		uint64_t get_max_win() const {
			const uint64_t eos_balance = get_token_balance(N(eosio.token), symbol_type(EOS_SYMBOL));

			auto liabilities_struct = globalvars.get(LIABILITIES_ID);
			const uint64_t liabilities = liabilities_struct.val;

			return (eos_balance - liabilities) / 25;
		}
};

#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == N(eosio.token)) { \
      	 if( action == N(transfer)){ \
      	 	eosio_assert( code == N(eosio.token), "Must transfer EOS"); \
      	 } \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_ABI_EX( EOSBetDice, (initcontract)(newrandkey)(reveal)(transfer)(clearbet)(betreceipt)(suspendbet) )
