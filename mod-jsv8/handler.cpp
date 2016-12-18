
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

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
void Status(const v8::FunctionCallbackInfo<v8::Value>& args);
void ContentType(const v8::FunctionCallbackInfo<v8::Value>& args);
void PostData(const v8::FunctionCallbackInfo<v8::Value>& args);
void UnparsedUri(const v8::FunctionCallbackInfo<v8::Value>& args);
void Method(const v8::FunctionCallbackInfo<v8::Value>& args) ;
void Header(const v8::FunctionCallbackInfo<v8::Value>& args);
void ApFilename(const v8::FunctionCallbackInfo<v8::Value>& args);
void Log(const v8::FunctionCallbackInfo<v8::Value>& args);


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


#define BLOCKSIZE 256



class JSengine;
static JSengine* theApp;

class JSengine {

	public:

		Platform* platform;
		Isolate::CreateParams create_params;
		Isolate* isolate;

		Global<Context> globalContext;
		Global<Script> globalScript;
		Global<Function> func;

		char modpidnam[100];

		request_rec *r;

		std::string basedir;
		std::list<std::string> allheaders ;
		std::map<std::string, std::string> headers ;

		int status;
		std::string contentType ;

		std::string jsm_ext = ".jsm";

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

		void setBaseDir( request_rec *r ){
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

				std::string key = e[i].key;
				for( int ii = 0 ; ii < key.size() ; ++ii )	key[ii] = tolower(key[ii]);

				std::map<std::string, std::string>::iterator it = headers.find( key );
				if( it != headers.end() )
					headers[ key ] = it->second + "," + std::string( e[i].val );
				else
					headers[ key ] = e[i].val;

				allheaders.push_back( std::string( key ) + " : " + std::string( e[i].val ) );
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

			return getfilecontent( fnam );
		}


		int get_post_data( char **retbuf, apr_size_t maxsize = -1, apr_size_t blocksize = BLOCKSIZE ){

			*retbuf = NULL;

			if( maxsize != -1 && maxsize < r->clength ){
				// raise error
				return HTTP_REQUEST_ENTITY_TOO_LARGE;
			}

			apr_size_t bytesRead = 0;
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
					*retbuf = apr_pstrcat( r->pool, *retbuf ? *retbuf : "", buffer, NULL );

					bytesRead += len;

					if( maxsize != -1 && bytesRead > maxsize ){
						ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, r , "post data too long" );
						return HTTP_REQUEST_ENTITY_TOO_LARGE;
					}

				}

			}

			return OK;
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

			Isolate::Scope isolate_scope(isolate);
			HandleScope handle_scope(isolate);

			globalContext.Reset( isolate, CreateGlobalContext( isolate ) );

		}

	private:
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

			// Bind the global 'status' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "status", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Status));

			// Bind the global 'contenttype' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "contenttype", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, ContentType));

			// Bind the global 'ctype' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "ctype", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, ContentType));

			// Bind the global 'postdata' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "postdata", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, PostData));

			// Bind the global 'unparseduri' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "unparseduri", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, UnparsedUri));

			// Bind the global 'method' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "method", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Method));

			// Bind the global 'header' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "header", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Header));

			// Bind the global 'apfilename' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "apfilename", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, ApFilename));

			// Bind the global 'log' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Log));

#if 0
			// Bind the global 'load' function to the C++ Load callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Load));
#endif

			return v8::Context::New(isolate, NULL, global);
		}

	public:

#define LOCKSCOPE( ISO ) \
		Locker v8Locker(ISO); \
		Isolate::Scope isolate_scope(ISO); \
		HandleScope handle_scope(ISO)

#define LOCKSCOPECONTEXT( ISO, GCTX )  \
		LOCKSCOPE( ISO ) ; \
		/*Local<Context> context = CreateGlobalContext(isolate);*/ \
		v8::Local<v8::Context> context = v8::Local<v8::Context>::New(ISO, GCTX); \
		Context::Scope context_scope(context)


		int compile( const char* js ){

			//LOCKSCOPECONTEXT( isolate, globalContext );

			globalScript.Reset( isolate, _compile( js ));

			return OK;
		}

		Local<Script> _compile( const char* js ){

			EscapableHandleScope handle_scope(isolate);

			// Create a string containing the JavaScript source code.
			Local<String> source =
				String::NewFromUtf8(isolate, js,
						NewStringType::kNormal).ToLocalChecked();

			// Compile the source code.
			Local<Script> script = Script::Compile(isolate->GetCurrentContext(), source).ToLocalChecked() ;

			return handle_scope.Escape(script);;
		}

		char* run( ){

			LOCKSCOPECONTEXT( isolate, globalContext );

			return _run( globalScript.Get(isolate) ) ;
		}

		void setmain(){

			//LOCKSCOPECONTEXT( isolate, globalContext );

			// The script compiled and ran correctly.  Now we fetch out the
			// Process function from the global object.
			Local<String> process_name =
				String::NewFromUtf8( isolate, "route", NewStringType::kNormal)
				.ToLocalChecked();
			Local<Value> process_val;
			// If there is no route function, or if it is not a function,
			// dont set it
			if ( isolate->GetCurrentContext()->Global()->Get( isolate->GetCurrentContext(), process_name).ToLocal(&process_val) &&
					process_val->IsFunction()) {

				// It is a function; cast it to a Function
				Local<Function> process_fun = Local<Function>::Cast(process_val);

				// Store the function in a Global handle, since we also want
				// that to remain after this call returns
				func.Reset( isolate, process_fun);

			}
		}

		char* _run( Local<Script> script ){

			Local<Value> result = script->Run(isolate->GetCurrentContext()).ToLocalChecked();

			// Convert the result to an UTF8 string and print it.
			String::Utf8Value utf8(result);

			char* buf = (char*)apr_palloc( r->pool, utf8.length() );
			buf[0]=0;
			strcat( buf, *utf8 );

			return buf;
		}

		char* runjs( const char* js ){

			//LOCKSCOPECONTEXT( isolate, globalContext );

			Local<Script> script = _compile( js );
			char* buf = _run( script );

			return buf;
		}

		void call(){

			//LOCKSCOPECONTEXT( isolate, globalContext );

			// Invoke the process function, giving the global object as 'this'
			// and one argument, the request.
			const int argc = 0;
			Local<Value> argv[] = {};
			v8::Local<v8::Function> process =
				v8::Local<v8::Function>::New(isolate, func);
			Local<Value> result;
			if (!process->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), argc, argv).ToLocal(&result)) {
				//String::Utf8Value error(try_catch.Exception());
				//Log(*error);
				//return false;
			}
		}


		~JSengine(){

			isolate->Dispose();

			v8::V8::Dispose();
			v8::V8::ShutdownPlatform();
			delete platform;
			delete create_params.array_buffer_allocator;

		}

};



static apr_uint32_t calls = 0;


bool endswith( std::string s, std::string t ){

	std::string::size_type len = t.size();

	if( s.size() < t.size() )
		return false;

	return s.compare( s.size()-t.size(), t.size(), t ) == 0;
}



int process_req( request_rec *r ){

	try{

		time_t current_time = time(NULL);
		char* c_time_string = ctime(&current_time);

		// this is a hack
		// todo
		// move this to handler startup
		//
		if( 0 == apr_atomic_inc32( & calls ) ){

			theApp = new JSengine( ) ;
			// todo
			// get the basedir from a server config
			// not from the request itself
			theApp->setBaseDir( r );

			LOCKSCOPECONTEXT( theApp->isolate, theApp->globalContext );
			theApp->setRequest( r );
			std::string bd = theApp->getBaseDir().c_str();
			char* mainjs = theApp->getjsfilecontent( (bd + "main.jsm").c_str() );
			theApp->compile( mainjs );
			theApp->run();
			theApp->setmain( );

#if APR_HAS_THREADS
			ap_log_error( __FILE__, __LINE__, 0, 0, 0, r->server, "*** not an error !!! ***" );
			ap_log_error( __FILE__, __LINE__, 0, 0, 0, r->server, "*** this apache2 supports multiple threads ***" );
			ap_log_error( __FILE__, __LINE__, 0, 0, 0, r->server, "*******" );
#endif
		}

		//ap_rprintf( r, "request %s %s size %i \n" , r->filename, c_time_string, r->finfo.size );

		std::string srfnam = r->filename;

		if( !endswith( srfnam, theApp->jsm_ext ) ){

			srfnam.append( theApp->jsm_ext );

			apr_finfo_t finfo;
			apr_status_t stat = apr_stat ( &finfo, srfnam.c_str(), APR_FINFO_SIZE, r->pool );

			if( stat ){
				return DECLINED;
			}

		}

		LOCKSCOPECONTEXT( theApp->isolate, theApp->globalContext );

		theApp->status = OK;
		theApp->contentType = "text/html";

		theApp->setRequest( r );

		// call precompiled route method
		theApp->call();

		char* js = theApp->getjsfilecontent( srfnam.c_str() );
		if( js ){

			char* buf = theApp->runjs( js );

		}

	} catch( ... ){
		ap_rprintf(r,"crashed\n");
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	ap_set_content_type( r, theApp->contentType.c_str() );

	return theApp->status;
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


void Status(const v8::FunctionCallbackInfo<v8::Value>& args) {

	//v8::HandleScope handle_scope(args.GetIsolate());

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	if ( !args[0]->IsInt32() ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad value type",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	theApp->r->status = args[0]->Int32Value();

}


// The callback that is invoked by v8 whenever the JavaScript 'ctype'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void ContentType(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value ctype(args[0]);

	theApp->contentType = *ctype;

}


// The callback that is invoked by v8 whenever the JavaScript 'postdata'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void PostData(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0 ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	char *content ;

	int rc = theApp->get_post_data( &content );

	if( content ){
		Local<String> data =
			String::NewFromUtf8(theApp->isolate, content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}


}



// The callback that is invoked by v8 whenever the JavaScript 'unparseduri'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void UnparsedUri(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0 ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	char *content = theApp->r->unparsed_uri;

	if( content ){
		Local<String> data =
			String::NewFromUtf8(theApp->isolate, content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'method'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Method(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0 ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	const char *content = theApp->r->method;

	if( content ){
		Local<String> data =
			String::NewFromUtf8(theApp->isolate, content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'header'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Header(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value header(args[0]);
	std::string key = *header;

	for( int ii = 0 ; ii < key.size() ; ++ii )	key[ii] = tolower(key[ii]);

	//ap_rprintf( theApp->getRequest(), "header->%s\n", key.c_str() );

	std::map<std::string,std::string>::iterator it = theApp->headers.find(key);

	if (it != theApp->headers.end() ){
		Local<String> data =
			String::NewFromUtf8(theApp->isolate, it->second.c_str(), NewStringType::kNormal).ToLocalChecked();
		//ap_rprintf( theApp->getRequest(), "header->%s\n", it->second.c_str() );

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'apfilename'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void ApFilename(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	Local<String> data =
		String::NewFromUtf8(theApp->isolate, theApp->r->filename, NewStringType::kNormal).ToLocalChecked();
	//ap_rprintf( theApp->getRequest(), "filename->%s\n", theApp->r->filename );

	args.GetReturnValue().Set(data);

}


// The callback that is invoked by v8 whenever the JavaScript 'log'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Log(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value logmsg(args[0]);

	ap_log_rerror( NULL, 0, 0, 0, NULL, theApp->r , "%s", *logmsg );

}





extern "C" {


	int thehandlerimpl(request_rec *r){

		//ap_rputs("mod handler cpp was called :o)\n", r);

		return process_req( r );;
	}



};


