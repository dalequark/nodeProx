fs = require("fs")
crypto = require("crypto")
rabin = require("./build/Release/rabinfinger");



src = fs.readFileSync(process.argv[2]).toString();

function printFingers(src)
{

results = rabin.getRabinChunks(src, src.length, min_mask, max_mask);

console.log("Results length is " + results.length, " string length is " + src.length);
for(i = 0; i < results.length; i++){
  	if(i %2 == 0)	console.log("Finger " + results[i]);
	else{
		shasum = crypto.createHash('sha1');
	 	if(i == 1)
		{
			console.log("From boundaries 0 to " + results[i]+1);
			shasum.update(src.substring(0, results[i]+1));
			console.log("Hash: " + shasum.digest('hex'));
			console.log(src.substring(0, results[i]+1));	
		}
		else
		{
			console.log("i = " + i + ":From boundaries " + (results[i-2]+1) + " to " +  results[i]+1);
	  		shasum.update(src.substring(results[i-2]+1, results[i]+1));
			console.log("Hash: " + shasum.digest('hex'));
	  		console.log(src.substring(results[i-2]+1, results[i]+1));
		}
	}
}
	return results[results.length - 1];
}
var offset = 0;
/*
while(src)
{
	offset = printFingers(src);
	if(offset < src.length)
		src = src.substring(offset+1, src.length);
	else
		src = false;
	console.log("New src has length " + src.length);
}*/

function hasNTrailingZeros(finger, numZeros)
{
	for(var i = 0; i < numZeros; i++)
	{
		if(finger & 1<<i)
			return false;
	}
	return true;
}

function getBoundaries(src, chunkSize)
{
	min_mask = rabin.getMRCMask(chunkSize);
	var results = rabin.getRabinChunkSize(src, src.length, min_mask, max_mask);
	// At this point we don't really care about 
}
