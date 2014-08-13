function obj(name){
	this.name = name;
}

obj.prototype.sayHello = function(){
	console.log(this.name);
}

var dale = new obj("Dale");
dale.sayHello();