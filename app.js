var config = require("./config.json");
var argv = process.argv;

var isRWaprox = false;
if(argv.length < 3 || (argv[2] != "rwaprox" && argv[2] != "swaprox")){
	console.log("Usage: " + argv[0] + " " + argv[1] + " rwaprox/swaprox")
	return;
}

if(argv[2] == "rwaprox")
{
	isRWaprox = true;
}

if(isRWaprox)
{
	console.log("Hello, I'm r Waprox!");
	var rWaprox = require("./rwaprox.js");
	rWaprox.rWaproxMain();
}
else
{
	console.log("Hello, I'm s Waprox!");
	var sWaprox = require("./swaprox.js");
	sWaprox.sWaproxMain();
}