function f1(cb)
{
	var dog = "Emily";
	cb();
}

function f2(){
	console.log(dog);
}

f1(f2);
