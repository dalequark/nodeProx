var net = require('net')
var http = require('http')
var url = require('url')
var crypto = require('crypto')
var rabin = require("./rabinfinger//build/Release/rabinfinger")
var parseUrl = require("./urlParser.js");
var config = require("./config.json").sWaprox
var DEBUG = config.debug;
var HOST_REGEX = /Host: /;
var HOST_OFFSET = "Host: ".length;
var CHUNK_SIZE = 32;
var min_mask = rabin.getMRCMask(128);
var max_mask = rabin.getMRCMask(128*1024-1);


exports.sWaproxMain = sWaproxMain;

var date = new Date();
var cache = {};

/* sWaprox main listenes for connections from rWaprox's.  It expects to 
receive an http request which will be forwarded to the outside world,
then forwards the requests, and exchanges chunks with the rWarpox until
data is transfered.
*/


function sWaproxMain(){

	// Listen to connections from rWaprox's.
	var rwaproxServer = net.createServer(function(rwaprox){

	 	if(config.debug)	console.log("Received connection from " + rwaprox.remoteAddress + ":" + rwaprox.remotePort);

	 	rwaprox.on('data', function(data){

			// Parse http headers
	 		headers = parseUrl(data.toString());
	 		req = net.connect(headers.port ? headers.port : 80, headers.host,
	 		function(){
	 			rwaprox.write('HTTP/1.1 200 Connection Established\r\n' +
                    'Proxy-agent: Node-Proxy\r\n' +
                    '\r\n');
	 			req.write(data);

	 			req.on('data', function(data)
	 			{
	 				var processedData = new rabinData(data);
	 				var responseMessage = [];
	 				var thisRWaprox = null;
	 				if(!(thisRWaprox = cache[rwaprox.remoteAddress]))
	 				{
	 					if(DEBUG)	console.log("Registering new rwaprox with ID " + rwaprox.remoteAddress);
	 					thisRWaprox = new rwaproxCache(rwaprox.remoteAddress);
	 					cache[rwaprox.remoteAddress] = thisRWaprox;
	 					for(var i = 0; i < processedData.hashes.length; i++)
	 					{
	 						thisRWaprox.addHash(processedData.hashes[i]);
	 					}
	 					responseMessage.push(data);
	 				}
	 				else{
	 					for(var i = 0; i < processedData.hashes.length; i++)
	 					{
	 						// If the rWaprox has the data, just send the hash
	 						// Otherwise, send the data
	 						var thisHash = processedData.hashes[i];
	 						if(thisRWaprox.hashHash(thisHash))
	 						{
	 							// A hashvalue is indicated by beginning with
	 							// "H," (as opposed to data);
	 							responseMessage.push("H," + thisHash);
	 						}
	 						else
	 						{
	 							responseMessage.push(processedData.getChunk(i));
	 						}
	 					}
	 				}

	 				rwaprox.write(responseMessage.join());


	 			});
	 		});

	 		req.on('error', function(err){
	 			console.log("Problem with forwarding request: " + err.message);
	 		});
		});

		rwaprox.on('error', function(err){
			console.log("Error with rwaprox connection " + err);
			rwaprox.close();
		});
			
	 		//var proxy_request = http.request({host:, method:, headers:})
	 	
}).listen(config.rWaprox_port, function(){console.log("Listening on " + this.address().port)});

}

function rwaproxCache(id, hashVal)
{
	this.id = id;
	this.timestamp = date.getTime();
	this.hashes = [];
	if(hashVal)		this.addHash(hashVal);
}

rwaproxCache.prototype.addHash = function(hashVal){
	this.hashes.push(hashVal);
}

rwaproxCache.prototype.hashHash = function(hashVal)
{
	if(this.hashes.indexOf(hashVal) != -1)
	{
		return true;
	}
	return false;
}

function rabinData(src)
{
	this.data = src.toString();
	this.offsets = rabin.getRabinChunks(src, src.length, min_mask, max_mask);
	this.numChunks = this.offsets.length;
	this.hashes = null;
	this.initHashes();

}

rabinData.prototype.initHashes = function(){
	var hashArray = [];
	var lastBoundary = 0;
	for(var i = 0;i < this.offsets.length; i++)
	{
		var shasum = crypto.createHash('sha1');
		shasum.update(this.data.substring(lastBoundary, this.offsets[i]));
		hashArray.push(shasum.digest('hex'));
		lastBoundary = this.offsets[i];
	}
	this.hashes = hashArray;
}

rabinData.prototype.getChunk = function(chunkNum){
	if(chunkNum == 0)
	{
		return this.data.substring(0, this.offsets[0]);
	}
	else
	{
		return this.data.substring(this.offsets[chunkNum-1], this.offsets[chunkNum]);
	}
}


