var re = /:\d+\\?/;
module.exports = function parseHeader(httpHeader)
{
	responseObj = {};
	header = httpHeader.toString().toLowerCase().split("\n")[0];
	words = header.split(/\s+/);
	res = words[1].search(re);
	var host, port;
	if(res != -1)
	{
		host = words[1].substring(0, res);
		port = words[1].substring(res).match(/\d+/)[0];
	}
	else
	{
		host = words[1];
	}
	host = host.split("/")[2];
	responseObj["host"] = host;
	responseObj["port"] = port;
	return responseObj;
}

