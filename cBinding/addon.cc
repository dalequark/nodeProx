#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <openssl/sha.h>

using namespace v8;

Handle<Value> Add(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
	    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
	        return scope.Close(Undefined());
	}

	v8::String::AsciiValue string(args[0]);
	 char* str = (char *) malloc(string.length() + 1);
	 strcpy(str, *string);
	 unsigned char hash[SHA_DIGEST_LENGTH];
	 SHA1((const unsigned char*) str, sizeof(str), hash);

/*
    Handle<Array> array = Array::New(3);
    if(array.IsEmpty())
    {
    	printf("Empty array??");
    	return scope.Close(Undefined());
    }

    unsigned long long someValue = 2345;
    array->Set(0, someValue);
/*
      if (args.Length() < 1) {
	    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
	        return scope.Close(Undefined());
		  }

        if (!args[0]->IsObject()) {
	      ThrowException(Exception::TypeError(String::New("Wrong arguments")));
	          return scope.Close(Undefined());
		    }


	  v8::String::AsciiValue string(args[0]);
	  char* str = (char *) malloc(string.length() + 1);
	  strcpy(str, *string);
	  printf("A test %s\n", str);
	  Local<String> tstring = args[0]->ToString();

	  Handle<Object> testObj = Object::New();
	  testObj->Set(String::New("name"), tstring);
	  unsigned long aLongNumber = 123124;
	  testObj->Set(String::New("value"), Number::New(aLongNumber));


	  Handle<Object> opt = Handle<Object>::Cast(args[0]);
	  Local<String> testField = String::New("testField");
	  	v8::String::AsciiValue objString(opt->Get(testField));
	  	char* printString = (char*) malloc(objString.length() + 1);
	  	strcpy(printString, *objString);
	  	printf("I found a field with value %s\n", printString);
	  	free(printString);
*/
	  printf("Hash was %s", hash);
	  Local<String> returnHash = String::New((const char*) hash);
	  free(str);
	  return scope.Close(returnHash);
}

Handle<Value> parseNum(const Arguments& args) {
	HandleScope scope;
	printf("%d\n", args[0]->Int32Value());
	return scope.Close(Undefined());

}

void Init(Handle<Object> exports) {
    exports->Set(String::NewSymbol("add"),
	      FunctionTemplate::New(Add)->GetFunction());
    exports->Set(String::NewSymbol("parsenum"), FunctionTemplate::New(parseNum)->GetFunction());
}

NODE_MODULE(addon, Init)
