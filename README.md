# betdice_contract
#EOSBETDI骰子游戏，合约代码
#author:区块链青年qq_776355102

合约授权
cleos -u http://api-kylin.eoshenzhen.io:8890 set account permission baidudice222 active '{"threshold" : 1, "keys" : [{"key":"EOS5ihbMJSJ7GC3zxbJaKU1h571UEUxn81FLoJdPX7YaH5T2hVrdu","weight":1}], "accounts" : [{"permission":{"actor":"baidudice222","permission":"eosio.code"},"weight":1}]}' owner -p baidudice222@owner



获取账户信息：

cleos -u http://api-kylin.eoshenzhen.io:8890 get account baidudice222

创建账户
cleos -u http://api-kylin.eoshenzhen.io:8890 system newaccount -x 1000 --stake-net "0.1 EOS" --stake-cpu "0.1 EOS" --buy-ram-kbytes 8 baidudice222 baidudice333 EOS5ihbMJSJ7GC3zxbJaKU1h571UEUxn81FLoJdPX7YaH5T2hVrdu EOS5ihbMJSJ7GC3zxbJaKU1h571UEUxn81FLoJdPX7YaH5T2hVrdu

购买RAM
cleos -u http://api-kylin.eoshenzhen.io:8890 system buyram baidudice333 baidudice333 "30.0000 EOS"

抵押net和cpu资源
cleos -u http://api-kylin.eoshenzhen.io:8890 system delegatebw baidudice333 baidudice333  "10.0000 EOS" "10.0000 EOS"

编译源码
eosio-cpp -o EOSBetDice.wasm EOSBetDice.cpp --abigen
