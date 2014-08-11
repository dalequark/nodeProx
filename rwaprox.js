var config = require('./config.json').rWaprox;
var http = require('http');
var net = require('net');
var DEBUG = config.debug;
exports.rWaproxMain = rWaproxMain;

function rWaproxMain(){


var httpProxyServer = http.createServer(function(req, res)
{
	if(DEBUG)	console.log("Got a connection: ");
	var swaprox = net.connect({host: config.sWaprox_address, port:config.sWaprox_port});
	swaprox.on('connect', function(){
		swaprox.write(req.headers.host);
		req.on('data', function(chunk){
			console.log("Writing client data to sWaprox");
			swaprox.write(chunk, 'binary');
		});

	});

	swaprox.on('error', function(e){ console.log("Got an error: " + e)});

	swaprox.on('data', function(chunk){
		console.log("Data from proxy....");
		console.log(chunk.toString());
			//res.write(chunk, 'binary');
		swaprox.on('end', function(){
			res.end();
		});
		//res.writeHead(proxy_response.statusCode, proxy_response.headers);

	});


	req.on('end', function(){
		swaprox.end();
	});
});

httpProxyServer.on('error', function(e){
	console.log("There was an error with rWaprox: " + e.code);
	return;
});

httpProxyServer.listen(config.http_proxy_port, function(){
	if(DEBUG)	console.log("Server is listening on " + httpProxyServer.address().port);
});



}
