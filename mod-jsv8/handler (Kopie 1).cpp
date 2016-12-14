
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <libgen.h>

#include <list>
#include <map>
#include <string>


#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"

#include "apr.h"
#include "apr_atomic.h"
#include "apr_strings.h"

#include "ap_config.h"

#include "include/v8.h"
#include "include/libplatform/libplatform.h"


#define MODNAME "JSengine"


void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);


#if 0


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value file(args[i]);
    if (*file == NULL) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    v8::Local<v8::String> source;
    if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
  }
}


#endif


using namespace v8;

#define MAXPOSTSIZE -1
#define BLOCKSIZE 256



class JSengine {

	public:

		Platform* platform;
		Isolate::CreateParams create_params;
		Isolate* isolate;
		Global<Script> globalScript;

		char modpidnam[100];

		request_rec *r;

		std::string basedir;
		std::list<std::string> allheaders ;
		std::map<std::string, std::string> headers ;

		int status;

	private:


	public:

		request_rec * getRequest( ){
			return r;
		}

		void setRequest( request_rec *_request ){
			r = _request;
			setHeaders( );
		}

		std::string getBaseDir(){
			return basedir;
		}

		void setBaseDir( ){
			char *dir = apr_pstrdup( r->pool, r->filename );
			basedir.assign( (const char*) dirname( dir ) );
			basedir += "/";
		}

		void setHeaders( ){

			headers.clear();

			const apr_array_header_t *fields;
			int i;
			apr_table_entry_t *e = 0;

			fields = apr_table_elts(r->headers_in);
			e = (apr_table_entry_t *) fields->elts;

			for(i = 0; i < fields->nelts; i++) {

				std::map<std::string, std::string>::iterator it = headers.find( e[i].key );
				if( it != headers.end() )
					headers[ e[i].key ] = it->second + "," + std::string( e[i].val );
				else
					headers[ e[i].key ] = e[i].val;

				allheaders.push_back( std::string( e[i].key ) + " : " + std::string( e[i].val ) );
			}
		}

		char* getStrError( apr_status_t fs ){

			const apr_size_t MAXL = 1024;

			char* buf = (char*)apr_palloc( r->pool, MAXL );
			return apr_strerror( fs, buf, MAXL );
		}

		char* getfilecontent( const char* fnam ){

			status = HTTP_NOT_FOUND;

			apr_file_t* fd;
			apr_status_t fs = apr_file_open( &fd , fnam, APR_FOPEN_READ, APR_OS_DEFAULT, r->pool );
			if( fs ){
				ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "file open error %s %s", getStrError( fs ), fnam );
				return NULL;
			}

			apr_finfo_t finfo;
			fs = apr_file_info_get( &finfo, APR_FINFO_SIZE, fd );
			if( fs ){
				ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "file stat error %s %s", getStrError( fs ), fnam );
				return NULL;
			}

			char* buf = (char*)apr_palloc( r->pool, finfo.size+1 );
			buf[finfo.size]=0;

			apr_size_t rb ;

			fs = apr_file_read_full( fd, buf, finfo.size, &rb );
			if( fs ){
				ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "file read error %s %s", getStrError( fs ), fnam );
				return NULL;
			}

			apr_file_close(fd);

			if( rb != finfo.size ){
				ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "file read buffer error %s %s",  getStrError( fs ), fnam  );
				return NULL;
			}

			status = OK;

			return buf;
		}


		char* getjsfilecontent( const char* fnam ){
			char* buf = (char*)apr_palloc( r->pool, strlen(fnam)+3 );
			*buf=0; strcat( buf, fnam );

			// just a hack
			char* pos = strstr( buf, ".html" );
			if( pos ){
				*pos=0;
			}

			pos = strstr( buf, ".js" );
			if( !pos ){
				strcat( buf, ".js" );
			}
			return getfilecontent( buf );
		}


		char* get_post_data( apr_size_t maxsize = -1, apr_size_t blocksize = BLOCKSIZE ){

			if( maxsize != -1 && maxsize < r->clength ){
				// raise error
			}

			apr_size_t bytesRead = 0;
			char* retbuf = "";
			char* buffer = (char*)apr_pcalloc( r->pool, blocksize );
			*buffer=0;

		 	int ret = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR);

			if( OK == ret && ap_should_client_block( r ) )
			{
				int len;

				while( ( len = ap_get_client_block( r, buffer , blocksize )) > 0 )
				{
					// write data...
					buffer[len] = 0;
					retbuf = apr_pstrcat( r->pool, retbuf, buffer, NULL );

					bytesRead += len;

					if( maxsize != -1 && bytesRead > maxsize ){
							ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "post data too long" );
							return NULL;
					}

				}

			}

			return retbuf;
		}

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

		Local<Context> CreateGlobalContext(Isolate* isolate) {
		  // Create a template for the global object.
		  Local<ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
		  // Bind the global 'print' function to the C++ Print callback.
		  global->Set(
			  v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
				  .ToLocalChecked(),
			  v8::FunctionTemplate::New(isolate, Print));

		  // Bind the global 'read' function to the C++ Read callback.
		  global->Set(v8::String::NewFromUtf8(
				          isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
				      v8::FunctionTemplate::New(isolate, Read));

#if 0
		  // Bind the global 'load' function to the C++ Load callback.
		  global->Set(v8::String::NewFromUtf8(
				          isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
				      v8::FunctionTemplate::New(isolate, Load));
#endif

		  return v8::Context::New(isolate, NULL, global);
		}

		int compile( const char* js ){

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
				String::NewFromUtf8(isolate, js,
				                    NewStringType::kNormal).ToLocalChecked();

			// Compile the source code.
			globalScript.Reset( isolate, Script::Compile(context, source).ToLocalChecked() );

			return OK;
		}

		char* run( ){

			Locker v8Locker(isolate);

			Isolate::Scope isolate_scope(isolate);

			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			// Create a new context.
			Local<Context> context = CreateGlobalContext(isolate);//Context::New(isolate);

			// Enter the context for compiling and running the hello world script.
			Context::Scope context_scope(context);

			// Run the script to get the result.
			Local<Value> result = globalScript.Get(isolate)->Run(context).ToLocalChecked();

			// Convert the result to an UTF8 string and print it.
			String::Utf8Value utf8(result);

			char* buf = (char*)apr_palloc( r->pool, utf8.length() );
			buf[0]=0;
			strcat( buf, *utf8 );

			//while ( v8::platform::PumpMessageLoop(platform, isolate ))	continue;

			return buf;
		}

		char* runjs( const char* js ){

			Locker v8Locker(isolate);

			Isolate::Scope isolate_scope(isolate);

			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			// Create a new context.
			Local<Context> context = CreateGlobalContext(isolate);//Context::New(isolate);

			// Enter the context for compiling and running the hello world script.
			Context::Scope context_scope(context);

			// Create a string containing the JavaScript source code.
			Local<String> source =
				String::NewFromUtf8(isolate, js, NewStringType::kNormal).ToLocalChecked();

			TryCatch try_catch(isolate);

			// Compile the source code.
			Local<Script> script = Script::Compile(context, source).ToLocalChecked() ;

			if( try_catch.HasCaught() ){
				String::Utf8Value error( try_catch.Exception() );
				//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "compile error v8: %s", (char*) *error );
				ap_rprintf( r, "run error v8: " );
				return NULL;
			}

			// Run the script to get the result.
			Local<Value> result = script->Run(context).ToLocalChecked();

			if( try_catch.HasCaught() ){
				String::Utf8Value error(try_catch.Exception());
				//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "run error v8: %s", (char*) *error );
				ap_rprintf( r, "run error v8: " );
				return NULL;
			}

			// Convert the result to an UTF8 string and print it.
			String::Utf8Value utf8(result);
			//ap_rprintf(r, "%s\n", *utf8);

			char* buf = (char*)apr_palloc( r->pool, utf8.length() );
			buf[0]=0;
			strcat( buf, *utf8 );

			//while ( v8::platform::PumpMessageLoop(platform, isolate ))	continue;

			return buf;
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

		if( 0 == apr_atomic_inc32( & calls ) ){

			//ap_rprintf(r,"\n*******\n*******\nstartup engine\n*******\n*******\n");

			theApp = new JSengine( ) ;

			//char* mainjs = getjsfilecontent( r, main );

			theApp->setRequest( r );
			theApp->setBaseDir( );
			//ap_rprintf( theApp->getRequest(), "dir %s\n", theApp->getBaseDir().c_str() );

			theApp->compile( "print('Hello' + ', World! - demo integration run at ' + new Date().toString());" );
		}

		//ap_rprintf( r, "result1 \n%s\n",theApp->run( ) );

		//ap_rprintf( r, "path_info %s\n", r->path_info );
		//ap_rprintf( r, "filename %s\n", r->filename );

		theApp->setRequest( r );

		//ap_rprintf( r, "post data\n%s\n", theApp->get_post_data( ) );

		char* js = theApp->getjsfilecontent( r->filename );
		if( js ){
			char* buf = theApp->runjs( js );
		}


	} catch( ... ){
		ap_rprintf(r,"crashed\n");

	}

	return OK;
}


const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      ap_rprintf( theApp->getRequest(), " " );
    }
    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
	if( cstr )
    	ap_rprintf( theApp->getRequest(), "%s", cstr );
  }
  //printf("\n");
  //fflush(stdout);
}

// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  v8::String::Utf8Value file(args[0]);
  if (*file == NULL) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }

	char *content = theApp->getfilecontent( (theApp->getBaseDir() + *file).c_str() );

	Local<String> source =
		String::NewFromUtf8(theApp->isolate, content, NewStringType::kNormal).ToLocalChecked();

  //if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
	if( !content ){
    	args.GetIsolate()->ThrowException(
		    v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
		                            v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  args.GetReturnValue().Set(source);
}



extern "C" {


int thehandlerimpl(request_rec *r){

	//ap_rputs("mod handler cpp was called :o)\n", r);

	process_req( r );

	return OK;
}



};


