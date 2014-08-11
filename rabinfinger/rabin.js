fs = require("fs")

rabin = require("./build/Release/rabinfinger");

min_mask = rabin.getMRCMask(32);
max_mask = rabin.getMRCMask(128*1024-1);

//src = "Here is a very happy sentence, loooooook at how cooooooool it isssssss";

src = fs.readFileSync(process.argv[2]).toString();

function printFingers(src)
{

results = rabin.getRabinChunkSize(src, src.length, min_mask, max_mask);

console.log("Results length is " + results.length, " string length is " + src.length);
for(i = 0; i < results.length; i++){
  	if(i %2 == 0)	console.log("Finger " + results[i]);
	else{
	 	if(i == 1)
		{
			console.log("From boundaries 0 to " + results[i]+1);
			console.log(src.substring(0, results[i]+1));	
		}
		else
		{
			console.log("i = " + i + ":From boundaries " + (results[i-2]+1) + " to " +  results[i]+1);
	  		console.log(src.substring(results[i-2]+1, results[i]+1));
		}
	}
}
	return results[results.length - 1];
}
var offset = 0;

while(src)
{
	offset = printFingers(src);
	if(offset < src.length)
		src = src.substring(offset+1, src.length);
	else
		src = false;
	console.log("New src has length " + src.length);
}


