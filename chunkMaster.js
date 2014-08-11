var async = require('async')
var hcClient = require("cacheRequest")


/* Should sWaprox store chunks? How should this work? */
function chunkMaster(content, chunkLevels)
{
	var mrcTree = [];
	for(var i = 0; i < chunkLevels; i++)
	{
		
	}

}

function chunkClient()
{

}


function rabin(content, chunkSize)
{
	// first array entry is number of chunks, the rest are offsets into content
	var length = content.length()/chunkSize;
	int[] a = int[length+1];
	a[0] = length;
	for(var i = 1; i <= length; i++)
	{
		a[i] = length*i -1;
	}

	return i;
}
