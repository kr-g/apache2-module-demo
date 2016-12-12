// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/libplatform/libplatform.h"
#include "include/v8.h"

#include <pthread.h>



using namespace v8;

Platform* platfrm;
Isolate::CreateParams create_params;
Isolate* isolate;


int init(){

  // Initialize V8.
  //V8::InitializeICUDefaultLocation(argv[0]);
  //V8::InitializeExternalStartupData(argv[0]);
  platfrm = platform::CreateDefaultPlatform();
  V8::InitializePlatform(platfrm);
  V8::Initialize();

  // Create a new Isolate and make it the current one.
  
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

  isolate = Isolate::New(create_params);

	return 0;
}

int shtdwn(){
// Dispose the isolate and tear down V8.
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
  delete platfrm;
  delete create_params.array_buffer_allocator;

	return 0;
}

Eternal<Script> script ;

int compile(){

	//Locker v8Locker(isolate);

	Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);

    // Create a new context.
    Local<Context> context = Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    Local<String> source =
        String::NewFromUtf8(isolate, "'Hello' + ', World! - demo integration run'",
                            NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    script.Set( isolate, Script::Compile(context, source).ToLocalChecked() );

	return 0;
}


int run(){

	Locker v8Locker(isolate);

	Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);

    // Create a new context.
    Local<Context> context = Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);

#if 0
    // Create a string containing the JavaScript source code.
    Local<String> source =
        String::NewFromUtf8(isolate, "'Hello' + ', World! - demo integration run'",
                            NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    Local<Script> script = Script::Compile(context, source).ToLocalChecked();

#endif 

    // Run the script to get the result.
    Local<Value> result = script.Get(isolate)->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(result);
    printf("%s\n", *utf8);

	return 0;
}


void *thread_function( void *ptr ){
	unsigned t = (unsigned) ptr;
	printf( "%u " , t );
	run();
}


int main(int argc, char* argv[]) {


	printf( "mutli-threaded hello-world\n" );


	init();

	compile();

	const int M=10;

	pthread_t thread[M];

	for( int t = 0 ; t < M ; ++t ){
	
		int  iret = pthread_create( &thread[t], NULL, thread_function, (void*) t);
		if( iret ){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
			exit(1);
		}

	}

	for( int t = 0 ; t < M ; ++t ){
		pthread_join( thread[t], NULL);
	}

	for( int i = 0; i<5 ; ++i )
  {
   	run();
  }

  shtdwn();

  return 0;
}
