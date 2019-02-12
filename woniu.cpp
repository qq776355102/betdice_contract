#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>
#include <map>
#include <cstdlib>
#include "library.hpp"

#include <iostream>

using eosio::key256;
using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;
using eosio::symbol_type;

#define DEBUG
#define EOS_SYMBOL S(4, EOS)
#define  REFERRALER  N(pppppppp1235) //默认推荐人账户
#define REVEALER N(3zhiwoniu121)  //开奖部署合约账户
typedef std::map<int, bool> MAP_RESULT;
typedef std::map<int, asset> BET_RESULT;

// 下注类型
enum enum_offer_type { 

    single_1 = 100,
    single_2 = 200,
    single_3 = 300,
	single_4 = 120,
    single_5 = 130,
    single_6 = 210,
	single_7 = 230,
    single_8 = 310,
    single_9 = 320,
	
	
    double_1 = 120,
    double_2 = 130,
    double_3 = 210,
	double_4 = 230,
	double_5 = 310,
	double_6 = 320,

};


class sicbo : public eosio::contract {
  public:
  
	const uint64_t HOUSEEDGE_REF_times10000 = 150;
	const uint64_t REFERRER_REWARD_times10000 = 50;
    sicbo(account_name self):eosio::contract(self),
       offers(_self, _self),
       games(_self, _self),
	   _global(_self, _self)
    {}

	
	
	struct st_result{
			uint64_t 		game_id;
			account_name	player; // 下注人
			account_name	referral; // 推荐人
			asset			bet_amt;  // 下注金额
			uint32_t        offer_type; // 押注点数
			checksum256		house_seed; // 服务器种子
			checksum256		house_hash; // 服务器哈希
			uint8_t 		result;
			asset			payout;
	};
	
	
	// 创建游戏
    //@abi action
	void creategame(const checksum256& commitment) {    
		//eosio_assert(id >= 201809140001, "id is too small");
		//eosio_assert(id <= 301809140001, "id is too big");

		// 需要合约账号的权限
		require_auth( _self );

		// 判断最后一个游戏 是否 在进行中
		if (games.begin() != games.end()) {
			auto game_ritr = games.rbegin();
			logger_info("game_ritr->id:", game_ritr->id);
			eosio_assert(game_ritr->game_state != 0, "some game is going");
			//eosio_assert(game_ritr->id < id, "has game_id bigger than id");
		}

		// 创建一个新游戏对象
		games.emplace(_self, [&](auto& new_game){
			// 这里要注意类型，要有一个数字是uint_64的类型才行
			new_game.id  = next_id();
			new_game.create_time = eosio::time_point_sec(now());
			new_game.update_time = eosio::time_point_sec(now());
			new_game.ttl = 80;
			new_game.commitment  = commitment;
			logger_info("new_game.id:", new_game.id);
		});
	}
    //@abi action
    void offerbet(const asset bet, const account_name player, const uint64_t gameid, const std::string& betlist);
    
	// 开奖接口   
    //@abi action
	void reveal(const uint64_t gameid, const checksum256& source) {
		//eosio_assert(gameid > 0, "gameid is not valid");
		logger_info("gameid:", gameid)
		// 需要合约账号的权限
		//require_auth( _self );
		// 判断gameid是否存在
		auto game = games.find(gameid);
		eosio_assert(gameid == game->id, "gameid is not valid");
		// 判断游戏是否在进行中
		eosio_assert(0 == game->game_state, "game_state is not valid");

		//logger_info("assert_sha256 start")
		// 判断签名是否一致
	//	char *c_src = (char *) &source;
	//	assert_sha256(c_src, sizeof(source), &game.commitment);
	//	logger_info("assert_sha256 success")

		// ++++++++++  开奖逻辑 ++++++++++++++++
		// 生成随机数
		//int num1 = (int)(source.hash[0] + source.hash[1] + source.hash[2] + source.hash[3] + source.hash[4] + source.hash[5])%10000+1;
		//int num2 = (int)(source.hash[6] + source.hash[7] + source.hash[8] + source.hash[9] + source.hash[10] + source.hash[11])%10000+1;
		//int num3 = (int)(source.hash[12] + source.hash[13] + source.hash[14] + source.hash[15] + source.hash[16] + source.hash[17])%10000+1;
		//logger_info("1th: ", num1, ", 2th: ", num2, ", 3th: ", num3)

		uint64_t num1 = ( source.hash[0]+  source.hash[1] + source.hash[2] +  source.hash[3] +  source.hash[4] +  source.hash[5]+   source.hash[6] + source.hash[7] +  source.hash[8] + source.hash[9]);
		uint64_t num2 = (source.hash[10]+ source.hash[11]+ source.hash[12] + source.hash[13] + source.hash[14] + source.hash[15] + source.hash[16] + source.hash[17]+ source.hash[18]+ source.hash[19]);
		uint64_t num3 = (source.hash[20]+ source.hash[21]+ source.hash[22] + source.hash[23] + source.hash[24] + source.hash[25] + source.hash[26] + source.hash[27]+ source.hash[28]+ source.hash[29]);

		
		// 修改游戏表
		games.modify(game, 0, [&](auto& game2) {
			game2.num1 = num1;
			game2.num2 = num2;
			game2.num3 = num3;
			game2.source = source;
			game2.game_state = 1;
			game2.update_time = eosio::time_point_sec(now());
		});

		// 计算中奖结果
		MAP_RESULT result = this->_getWinsMap(num1, num2, num3);
		
		

	

		// 修改下注表
		if(offers.begin()!= offers.end()){
			auto idx = offers.template get_index<N(gameid)>();
			logger_info("find gameid:", gameid)
			auto offer_iter = idx.find(gameid);

			// 遍历所有的数据，修改赌注结果
			int i = 0;
			while (offer_iter != idx.end()) {
				if (offer_iter->result != 0) {
					offer_iter++;
					continue;
				} 
				uint64_t ref_reward = 0;
				uint64_t payout = 0;
			//	if (offer_iter->referral != REFERRALER){
				//	ref_reward = offer_iter->bet.amount * REFERRER_REWARD_times10000 / 10000;
				//}
	
				if(result[offer_iter->offer_type]){
					payout = offer_iter->bet_odds * offer_iter->bet.amount;
				}
				
				if (payout > 0){
					action(
						permission_level{_self, N(active)},
						N(eosio.token), 
						N(transfer),
						std::make_tuple(
							_self, 
							offer_iter->player, 
							asset(payout, symbol_type(EOS_SYMBOL)), 
							std::string("Bet id: ") + std::to_string(offer_iter->id) + std::string(" -- Winner! Play: dice.io, roll type: " + std::to_string(offer_iter->offer_type))
						)
					).send();
				}
				
				// 修改赌注结果
				offers.modify(*offer_iter, 0, [&](auto& offer){
					if (result[offer_iter->offer_type]) {
						// 胜利
						offer.result = 1;
					} else {
						// 失败
						offer.result = 2;
					}
					offer.update_time = eosio::time_point_sec(now());
				});
				
				st_result result{.game_id = offer_iter->id,
								 .player = offer_iter->player,
								// .referral = offer_iter->referral,
								 .bet_amt = offer_iter->bet,
								 .offer_type = offer_iter->offer_type,
								 .house_seed = source,
								 .house_hash = game->commitment,
								 .result = offer_iter->result,
								 .payout = asset(payout, symbol_type(EOS_SYMBOL))
					
				};
				action(
					permission_level{_self, N(active)},
					_self,
					N(betreceipt),
					result
				).send();
				
				offer_iter++;
			}
		}


		// 设置开奖完毕
		games.modify(game, 0, [&](auto& game2) {
			game2.game_state = 2;
			game2.update_time = eosio::time_point_sec(now());
		});
	}
	
	// @abi action
	void betreceipt(st_result& result) {
		require_auth(_self);
		// require_recipient( bettor );
	}
    //@abi action
    void balance(const uint64_t gameid, const account_name player){};
    //@abi action
    void cleargame(){
		while(games.begin() != games.end()){
			games.erase(games.begin());
		}
	};
    //清掉下注玩家
	//@abi action
    void clearoffer(){

		while(offers.begin() != offers.end()){
			offers.erase(offers.begin());
		}
		
	};
    //@abi action
    void closegame(const uint64_t gameid){};
	//@abi action
    void transfer(account_name from, account_name to, const asset& quantity, const std::string& memo){
		
		const std::size_t breaks = memo.find("-");


		if(breaks != std::string::npos){
		
			auto global =  _global.begin();
			uint64_t gameid = global->nextgameid;
			auto game =  games.find(gameid);

		//	eosio_assert(eosio::time_point_sec(now()) < (game->create_time + game->ttl) ,"The betting has been closed");
			eosio_assert(0 == game->game_state, "game_state is not valid");

        	//memo.erase(std::remove_if(memo.begin(), memo.end(), [](unsigned char x) { return std::isspace(x); }), memo.end());

        	//size_t sep_count = std::count(memo.begin(), memo.end(), '-');
        	//eosio_assert(sep_count == 3, "invalid memo");
			
			uint64_t game_id = _global.begin()->nextgameid;	
			
			

			std::string roll_1 = "10000";

			
			account_name referrer = REFERRALER;
			
			
			
			//const std::size_t first_break = memo.find("-");
			//roll_1 = memo.substr(0, first_break);
						
		//	const std::string after_first_break = memo.substr(first_break + 1);
		//	const std::size_t second_break = after_first_break.find("-");
			
		//	if(second_break != std::string::npos){
		//		referrer = eosio::string_to_name(after_first_break.c_str());
			//}
			
			
		
			
			
			
			

			
			if(!roll_1.empty() && stoi(roll_1)>0){
				offers.emplace(_self, [&](auto& new_offer){
					new_offer.id =  offers.available_primary_key();
					new_offer.player = from;
					new_offer.referral = REFERRALER;
					new_offer.bet = asset(stoi(roll_1), symbol_type(S(4, EOS)));
					new_offer.gameid = game_id;
					new_offer.offer_type = 100;
					new_offer.bet_odds = 290;
					new_offer.create_time = eosio::time_point_sec(now());
					new_offer.update_time = eosio::time_point_sec(now());
				});	
			};

			


        	//container = memo.substr(++pos);
        	// eosio_assert(!container.empty(), "no referrer");
		} 	//*referrer = eosio::string_to_name(container.c_str());
		
	};//注意没有action

  private:
    // 赔率类型
    std::map<int,uint32_t> wins_map = { 


        { 0, 290 },
        { 1, 290 },
        { 2, 290 },
	    { 3, 588 },
        { 4, 588 },
        { 5, 588 },
	    { 6, 588 },
        { 7, 588 },
        { 8, 588 },

		
    };
	
	// 下注类型
    std::map<int,uint32_t> roting_map = { 


        { 0, 100 },
        { 1, 200 },
        { 2, 300 },
	    { 3, 120 },
        { 4, 130 },
        { 5, 210 },
	    { 6, 230 },
        { 7, 310 },
        { 8, 320 },

    };
      
    //@abi table game i64 赌局表
    struct game {
        uint64_t id;  // 赌局id，如:201809130001
        uint32_t ttl = 120; // 倒计时时间，默认2分钟
        uint8_t game_state = 0; // 状态：0:进行中, 1:开奖中, 2:已结束
		checksum256 source;// 开奖种子
        checksum256 commitment; // 随机数加密串
        uint64_t num1 = 0; // 第1个骰子数字
        uint64_t num2 = 0; // 第2个骰子数字
        uint64_t num3 = 0; // 第3个骰子数字
        eosio::time_point_sec create_time; // 创建时间
        eosio::time_point_sec update_time; // 更新时间
        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE( game, (id)(ttl)(game_state)(source)(commitment)(num1)(num2)(num3)(create_time)(update_time) )
    };
    
    //@abi table offer i64 赌注明细表
    struct offer {
        uint64_t          id; // 下注id
        account_name      player; // 用户名
		account_name	  referral;
        asset             bet; // 赌注
        uint64_t          gameid; // 赌局id，如:201809130001
        uint32_t          offer_type; // 下注类型：1:大, 2:小
        uint32_t          bet_odds ; // 下注赔率，默认：x2
        uint8_t result = 0; // 赌注结果：0:未开奖, 1:胜利, 2:失败
        uint8_t tranffer_state = 0; // 转账状态：0:未转账, 1:转账中, 2:转账成功, 3:转账失败
        eosio::time_point_sec create_time; // 创建时间
        eosio::time_point_sec update_time; // 更新时间
        
        uint64_t primary_key() const { return id; }
        uint64_t by_gameid() const { return gameid; } // 可以通过赌局id查询数据
        account_name by_account_name() const { return player; } // 可以通过用户名查询数据
        EOSLIB_SERIALIZE( offer, (id)(player)(referral)(bet)(gameid)(offer_type)(bet_odds)(result)(tranffer_state)(create_time)(update_time) )
    };
	
	// @abi table global i64
	struct st_global {
		uint64_t id = 0;
		uint64_t nextgameid = 0;

		uint64_t primary_key() const { return id; }
	};

	typedef eosio::multi_index<N(global), st_global> tb_global;

    // 创建一个多索引容器的游戏列表
    typedef eosio::multi_index< N(game), game> game_index;

    // // 创建一个多索引容器的赌注列表
    typedef eosio::multi_index< N(offer), offer,
        indexed_by< N(gameid), const_mem_fun<offer, uint64_t, &offer::by_gameid > >,
        indexed_by< N(player), const_mem_fun<offer, account_name,  &offer::by_account_name> >
    > offer_index;
    // typedef eosio::multi_index< N(offer), offer> offer_index;


    game_index games;
    offer_index offers;
	tb_global 	_global;

    // 通过结果计算所有下注类型是否胜利
    MAP_RESULT _getWinsMap(const uint64_t num1, const uint64_t num2, const uint64_t num3){
		MAP_RESULT map;
		//uint64_t sum = num1 + num2 + num3;

		//if (num1 == num2 && num1 == num3) {
		//	map[num1 * 100] = true;
		//}

		// 设置单骰
	//	map[num1 * 100] = true;
	//	map[num2 * 100] = true;
		//map[num3 * 100] = true;
		
		if(num1 != num2 && num1 !=num3 && num2 !=num3 ){
			if(num1 > num3 && num1 < num2 ){
				map[200] = true;
				map[210] = true;
			}else if(num1 > num2 && num1 < num3 ){
				map[300] = true;
				map[310] = true;
			}else if(num2 > num1 && num2 <num3){
				map[300] = true;
				map[320] = true;
			}else if(num2 < num1 && num2 > num3){
				map[100] = true;
				map[120] = true;
			}else if(num3 > num1 && num3 < num2){
				map[200] = true;
				map[230] = true;
			}else{
				map[100] = true;
				map[130] = true;
			}
		}

		// 设置双骰子
		//uint64_t double_num = 0;
		//uint64_t another_num = 0;
		//if (num1 == num2) {
		//	double_num = num1;
		//	another_num = num3;
		//} else if (num2 == num3) {
		//	double_num = num2;
			//another_num = num1;
		//} else if (num1 == num3) {
		//	double_num = num1;
		//	another_num = num2;
		//} 

		//if (double_num > 0) {
		//	map[double_num * 100 + another_num * 10] = true;
		//	map[double_num * 100] = true;
		//}

		return map;
	};
    // bool _changeState(auto offer_iter, const int tranffer_state);

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
	// 分割与验证多注
	//BET_RESULT _split_extra(const std::string data, uint64_t* gameidptr=nullptr)
	//{
		//std::string data = strlist;

	//	eosio_assert(data.length(), "offerbet is null");



		//if (gameidptr)
		//{
			//*gameidptr = 0;

			//if (data.find("|") != std::string::npos)
			//{
			//	auto gameidstr = SPLITONE(data);
			//	eosio_assert(isdecnum(gameidstr), "gameid isn't decimal num");

			//	*gameidptr = atoll(gameidstr.c_str());
			//}
		//}

	//	BET_RESULT map;

		//while(data.find("|") != std::string::npos)
	//	{
		//	auto eos  = SPLITONE(data);
		//	auto play = SPLITONE(data);

		//	eosio_assert(play.length(), "betlist format error");
		//	eosio_assert(isdecnum(eos), "eos isn't decimal num");
		//	eosio_assert(isdecnum(play), "play isn't decimal num");

		//	int key = atoi(play.c_str());

		//	require_offertype(key);

		//	if (map.find(key) != map.end()) {
		//		map[key] += str2eos(eos);
		//	} else {
		//		map[key]  = str2eos(eos);
		//	}
		//}
		


		//return map;
	//};	//0: offerbet, 1: transfer
	asset str2eos(const std::string eosnum)
	{
		asset result;

		result.amount = atoi(eosnum.c_str());
		result.symbol = S(4, EOS);

		return result;
	};
	
	std::string SPLITONE(std::string data) {
			data = data.substr(0, (data).find("|")); 
			return data.erase(0, (data).find("|")+1);
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
	};
	void require_offertype(int sicboplay)
	{
		switch(sicboplay)
		{
			
			//case three_n:
			//	break;
			default:
				eosio_assert(0, "offertype error");
		}

		return;
	};
};

//注意没有offerbet
#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply(uint64_t receiver, uint64_t code, uint64_t action) { \
       if (action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if ((code==N(eosio.token) && action==N(transfer)) || (code==self && (action==N(creategame) || action==N(reveal) || action==N(balance) || action==N(cleargame) || action==N(clearoffer) || action==N(closegame))) ) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(sicbo, (creategame)(reveal)(balance)(cleargame)(clearoffer)(betreceipt)(closegame)(transfer))
