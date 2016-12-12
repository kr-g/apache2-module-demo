
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"

#include "apr.h"
#include "apr_atomic.h"

#include "ap_config.h"

#include "include/v8.h"
#include "include/libplatform/libplatform.h"


#define MODNAME "JSengine"


using namespace v8;


class JSengine {

	public:
		
		Platform* platform;
		Isolate::CreateParams create_params;
		Isolate* isolate;
		Global<Script> globalScript;

		char modpidnam[100];

	public:

		JSengine(){

			sprintf( modpidnam, "%s---%u", MODNAME, getpid() );
			
			//V8::InitializeICUDefaultLocation(buffer);
			//V8::InitializeExternalStartupData(buffer);
			platform = platform::CreateDefaultPlatform();
			V8::InitializePlatform(platform);
			V8::Initialize();
			create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
			
			isolate = v8::Isolate::New(create_params);

		}

		int compile( request_rec *r ){

			Locker v8Locker(isolate);

			Isolate::Scope isolate_scope(isolate);

			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			// Create a new context.
			Local<Context> context = Context::New(isolate);

			// Enter the context for compiling and running the hello world script.
			Context::Scope context_scope(context);

			// Create a string containing the JavaScript source code.
			Local<String> source =
				String::NewFromUtf8(isolate, "'Hello' + ', World! - demo integration run at ' + new Date().toString()",
				                    NewStringType::kNormal).ToLocalChecked();

			// Compile the source code.			
			globalScript.Reset( isolate, Script::Compile(context, source).ToLocalChecked() );

			return OK;
		}

		int run( request_rec *r ){

			Locker v8Locker(isolate);

			Isolate::Scope isolate_scope(isolate);

			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			// Create a new context.
			Local<Context> context = Context::New(isolate);

			// Enter the context for compiling and running the hello world script.
			Context::Scope context_scope(context);

			// Run the script to get the result.
			Local<Value> result = globalScript.Get(isolate)->Run(context).ToLocalChecked();

			// Convert the result to an UTF8 string and print it.
			String::Utf8Value utf8(result);
			ap_rprintf(r, "%s\n", *utf8);		

			//while ( v8::platform::PumpMessageLoop(platform, isolate ))	continue;

			return OK;
		}

		int runjs( request_rec *r, const char* js ){

			Locker v8Locker(isolate);

			Isolate::Scope isolate_scope(isolate);

			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			// Create a new context.
			Local<Context> context = Context::New(isolate);

			// Enter the context for compiling and running the hello world script.
			Context::Scope context_scope(context);

			// Create a string containing the JavaScript source code.
			Local<String> source =
				String::NewFromUtf8(isolate, js, NewStringType::kNormal).ToLocalChecked();

			// Compile the source code.			
			Local<Script> script = Script::Compile(context, source).ToLocalChecked() ;

			// Run the script to get the result.
			Local<Value> result = script->Run(context).ToLocalChecked();

			// Convert the result to an UTF8 string and print it.
			String::Utf8Value utf8(result);
			ap_rprintf(r, "%s\n", *utf8);		

			//while ( v8::platform::PumpMessageLoop(platform, isolate ))	continue;

			return OK;
		}

		~JSengine(){

			isolate->Dispose();

			v8::V8::Dispose();
			v8::V8::ShutdownPlatform();
			delete platform;
			delete create_params.array_buffer_allocator;

		}

};



static JSengine* theApp;
static apr_uint32_t calls = 0;



int process_req( request_rec *r ){

	try{

		time_t current_time = time(NULL);
		char* c_time_string = ctime(&current_time);

		if(c_time_string)
			ap_rprintf(r,"Current time is %s", c_time_string);

		ap_rprintf(r,"pid %u cpp calls %u\n", getpid(), calls );
		ap_rprintf(r,"pwd=%s\n",get_current_dir_name());

		if( 0 == apr_atomic_inc32( & calls ) ){

			ap_rprintf(r,"\n*******\n*******\nstartup engine\n*******\n*******\n");

			theApp = new JSengine() ;

			ap_rprintf( r, "using %s\n" , theApp->modpidnam );

			theApp->compile( r );
		}
	
		theApp->run( r );

		theApp->runjs( r, "'run script at ' + new Date().toString()" );

	} catch( ... ){
		ap_rprintf(r,"crashed\n");

	}

	return OK;
}


extern "C" {


int thehandlerimpl(request_rec *r){

	ap_rputs("mod handler cpp was called :o)\n", r);

	process_req( r );

	return OK;
}



};


