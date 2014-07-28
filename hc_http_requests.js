var PORT = 33333;
//var PORT = 80;
var HOST = "localhost";
var WAPROX_GET_START = "WAPROX-GET http://waprox-";
var WAPROX_GET_END = " HTTP/1.1\r\n\r\n";
var WAPROX_PUT_START = "WAPROX-PUT http://waprox-"  
var WAPROX_PUT_END = " HTTP/1.1\r\nContent-Length: %d\r\n\r\n" //chunk content goes next

var net = require('net');


 var client = net.connect({port: PORT, host:HOST}, function(){
 	var putMessage = WAPROX_PUT_START + "123" + WAPROX_PUT_END + "HERE IS SOME FUN CONTENT LA LA LA\n\n";
 	var getMessage = WAPROX_GET_START + "123" + WAPROX_GET_END
	console.log("Connected to HC.");
 	console.log(process.argv[2]);
 	
	if(process.argv[2] == "get"){
		message = getMessage;
	}
	else if(process.argv[2] == "put"){
		message = putMessage;
	}
 	else
   	{
		console.log("Arg did not match: " + process.argv[2]);
		return;
	}
	console.log("sending message " + message);
 	client.write(message);
 });

 client.on('data', function(data){
 	console.log("Got data from client");
 	console.log(data.toString());
 	client.end();
 });

 client.on('end', function(){
 	console.log("HC disconnected");
 });

 client.on('error', function(e){
 	console.log("Got error " + e.code);
 	client.end();
 });
