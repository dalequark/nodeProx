var util = require('util')
var net = require('net')
var events = require('events')

var WAPROX_GET_START = "WAPROX-GET http://waprox-";
var WAPROX_GET_END = " HTTP/1.1\r\n\r\n";
var WAPROX_PUT_START = "WAPROX-PUT http://waprox-"  
var WAPROX_PUT_END = " HTTP/1.1\r\nContent-Length: %d\r\n\r\n" //chunk content goes next

exports = CacheRequest;
/* Make a special client with hashcache get and set methods */
function CacheRequest(metadata, cb)
{
	var self = this;
	this.client = net.connect(this.metadata, function()
	{
		if(cb)	cb();
	});

	this.client.on('data', function(data)
	{
		self.emit('data', data);
	});
	this.client.on('end', function()
	{
		self.emit('end');
	});
	this.client.on('error', function(e){
		self.emit('error', e);
	});


}


util.inherits(CacheRequest, events.EventEmitter);


CacheRequest.prototype.get = function(hashVal, cb){
	var getRequest = WAPROX_GET_START + hashVal + WAPROX_GET_END;
	this.client.write(getRequest);
	if(cb)	cb();
}

CacheRequest.prototype.put = function(hashVal, content, cb){
	var putRequest = WAPROX_PUT_START + hashVal + WAPROX_PUT_END;
	this.client.write(putRequest);
	if(cb)	cb();
}

CacheRequest.prototype.end = function(cb) {
	this.client.end();
	if(cb)	cb();
};

var req = new CacheRequest({host: "fermat.cs.princeton.edu", port: 33333}, function(){ console.log("In the callback")});
req.on('data', function(data)
{
	console.log("Got data from client:");
	console.log(data.toString());
});

req.on('end', function()
{
	console.log("HC disconnected");
});

req.on('error', function(e){
 	console.log("Heeeerz an error! " + e.code);
 	req.end();
 });

