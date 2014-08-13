var config = require('./config.json').rWaprox;
var http = require('http');
var net = require('net');
var DEBUG = config.debug;
exports.rWaproxMain = rWaproxMain;

function rWaproxMain(){


var proxyServer = net.createServer(function(endClient)
{
	endClient.on("data", function(data){
		if(DEBUG)	console.log("Got a connection.");
		var swaprox = net.connect({host: config.sWaprox_address, port:config.sWaprox_port});
		swaprox.on('connect', function(){
			if(DEBUG)
			{
				console.log("Writing ");
				console.log(data.toString());
			}
			swaprox.write(data, 'binary');
			endClient.on('data', function(chunk){
				console.log("Writing client data to sWaprox");
				if(DEBUG) console.log("Writing " + chunk.toString());
				swaprox.write(chunk, 'binary');
			});

			endClient.on('end', function(){
				swaprox.end();
			});


		});

		swaprox.on('error', function(e){ console.log("Got an error: " + e)});

		swaprox.on('data', function(chunk){
			if(DEBUG)	console.log("Data from proxy....");
			console.log(chunk.toString());
			swaprox.on('end', function(){
				endClient.end();
			});

		});


	});

	endClient.on('error', function(err)
	{
		console.log("An error occured with client: " + err);
		endClient.close();
	});
});

proxyServer.on('error', function(e){
	console.log("There was an error with rWaprox: " + e.code);
	return;
});

proxyServer.listen(config.http_proxy_port, function(){
	if(DEBUG)	console.log("Server is listening on " + proxyServer.address().port);
});



}
