var net = require('net')
var http = require('http')
var config = require("./config.json").sWaprox


exports.sWaproxMain = sWaproxMain;

function sWaproxMain(){

	var rwaproxServer = net.createServer(function(rwaprox){
	 	if(config.debug)	console.log("Received connection");
	 	rwaprox.on('data', function(data){
	 		console.log("Data was: " + data);
	 		rwaprox.write("SWAPROX RESPONSE");
	 		rwaprox.end();

	 		//var proxy_request = http.request({host:, method:, headers:})
	 	});
}).listen(config.rWaprox_port, function(){console.log("Listening on " + this.address().port)});

}
