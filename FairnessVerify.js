const Eos = require('eosjs');
const eosecc = require('eosjs-ecc');
const config = require('./config');
// https://github.com/cryptocoinjs/bs58
const bs58 = require('bs58');


scripts = {
	verifyFairness : (sig, seed) => {

		// params sig & hash are strings 
		const recoverKey = eosecc.recoverHash(sig, Buffer.from(seed, 'hex'));
		console.log("Keys are matching: ", "EOS6ESzwoxB1F3VLWVfy1VUyLzBjjFGB9nB2YUX5F9PkvhPwQUTgg" == recoverKey);

		tempSig = sig.substring(7);
		bytes = bs58.decode(tempSig);
		bytes_str = bytes.toString('hex');
		bytes_str = bytes_str.substring(0, bytes_str.length - 8);
		bytes_str = '00' + bytes_str;

		hash = eosecc.sha256(Buffer.from(bytes_str, 'hex'));

		// now get random number of this hash
		// rand = parseInt(hash.substring(0,2)) + parseInt(hash.substring(2,4)) + parseInt(hash.substring(4,6)) + parseInt(hash.substring(6,8)) + parseInt(hash.substring(8,10)) + parseInt(hash.substring(10,12)) + parseInt(hash.substring(12,14)) + parseInt(hash.substring(14,16));

		var rand = 0;
		for (var j = 0; j <= 14; j += 2){
			rand += parseInt(hash.substring(j, j + 2), 16);
		}

		rand = (rand % 100) + 1

		console.log("Your random number is: ", rand);

	}
}
