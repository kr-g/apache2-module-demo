
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
#include "http_core.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "util_cookies.h"
#include "ap_config.h"

#include "apr.h"
#include "apr_atomic.h"
#include "apr_strings.h"
#include "apr_uuid.h"
#include "apr_network_io.h"

#include "include/v8.h"
#include "include/libplatform/libplatform.h"


#include "mod_redis.h"


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
void FTime(const v8::FunctionCallbackInfo<v8::Value>& args);
void SetCookie(const v8::FunctionCallbackInfo<v8::Value>& args);
void GetCookie(const v8::FunctionCallbackInfo<v8::Value>& args);
void ClearCookie(const v8::FunctionCallbackInfo<v8::Value>& args);
void SetHeader(const v8::FunctionCallbackInfo<v8::Value>& args);
void Uuid(const v8::FunctionCallbackInfo<v8::Value>& args);
void SocketSend(const v8::FunctionCallbackInfo<v8::Value>& args);
void GetRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args);
void SetRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args);
void DelRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args);
void ExpireRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args);


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

		modjsredis *redis = NULL;

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

		void setBaseDir( const char* dir ){
			//char *dir = apr_pstrdup( r->pool, r->filename );
			basedir.assign( dir );
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

		void connectsessionserver( ){

			if( !redis ){
				redis = new modjsredis();
				int rc = redis->connect();
				ap_log_error( __FILE__, __LINE__, 0, 0, 0, r->server, "*** connect to redis %i ***", rc );
			}

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

			// Bind the global 'ftime' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "ftime", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, FTime));

			// Bind the global 'getcookie' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "getcookie", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, GetCookie));

			// Bind the global 'setcookie' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "setcookie", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, SetCookie));

			// Bind the global 'clearcookie' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "clearcookie", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, ClearCookie));

			// Bind the global 'setheader' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "setheader", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, SetHeader));

			// Bind the global 'uuid' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "uuid", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, Uuid));

			// Bind the global 'socketsend' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "socketsend", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, SocketSend));

			// Bind the global '_getkey' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "_getkey", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, GetRedisKey));

			// Bind the global '_setkey' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "_setkey", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, SetRedisKey));

			// Bind the global '_delkey' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "_delkey", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, DelRedisKey));

			// Bind the global '_expirekey' function to the C++ Read callback.
			global->Set(v8::String::NewFromUtf8(
						isolate, "_expirekey", v8::NewStringType::kNormal).ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, ExpireRedisKey));

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

		void call( const char* fnam, apr_time_t ftime ){

			//LOCKSCOPECONTEXT( isolate, globalContext );

			// strip away basedir
			int l = basedir.length();
			const char *fn = &fnam[l];

			Local<String> file_name =
				  String::NewFromUtf8(isolate, fn, NewStringType::kNormal)
					  .ToLocalChecked();
			Local<Integer> file_time =
				  Integer::New(isolate, ftime);

			// Invoke the process function, giving the global object as 'this'
			// and one argument, the request.
			const int argc = 2;
			Local<Value> argv[] = { file_name, file_time };
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

			if( redis ){
				delete redis;
				redis = NULL;
			}

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
		// dont use ap_document_root
		if( 0 == apr_atomic_inc32( & calls ) ){

			theApp = new JSengine( ) ;
			// todo
			// get the basedir from a server config
			// not from the request itself
			theApp->setBaseDir( ap_document_root(r) );

			LOCKSCOPECONTEXT( theApp->isolate, theApp->globalContext );
			theApp->setRequest( r );

			theApp->connectsessionserver();

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

		// get the file time
		apr_finfo_t finfo2;
		apr_status_t stat = apr_stat ( &finfo2, srfnam.c_str(), APR_FINFO_MTIME, r->pool );

		// call precompiled route method
		theApp->call( srfnam.c_str(), stat ? 0 : finfo2.mtime );

#if 0
		char* js = theApp->getjsfilecontent( srfnam.c_str() );
		if( js ){
			char* buf = theApp->runjs( js );
		}
#endif

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

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.
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
// function is called.
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
		String::NewFromUtf8(args.GetIsolate(), content, NewStringType::kNormal).ToLocalChecked();

	//if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
	if( !content ){
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	args.GetReturnValue().Set(source);
}

// The callback that is invoked by v8 whenever the JavaScript 'status'
// function is called.
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
// function is called.
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
// function is called.
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
			String::NewFromUtf8(args.GetIsolate(), content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}


}



// The callback that is invoked by v8 whenever the JavaScript 'unparseduri'
// function is called.
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
			String::NewFromUtf8(args.GetIsolate(), content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'method'
// function is called.
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
			String::NewFromUtf8(args.GetIsolate(), content, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'header'
// function is called.
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
			String::NewFromUtf8(args.GetIsolate(), it->second.c_str(), NewStringType::kNormal).ToLocalChecked();
		//ap_rprintf( theApp->getRequest(), "header->%s\n", it->second.c_str() );

		args.GetReturnValue().Set(data);
		return;
	}

}


// The callback that is invoked by v8 whenever the JavaScript 'apfilename'
// function is called.
void ApFilename(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	Local<String> data =
		String::NewFromUtf8(args.GetIsolate(), theApp->r->filename, NewStringType::kNormal).ToLocalChecked();
	//ap_rprintf( theApp->getRequest(), "filename->%s\n", theApp->r->filename );

	args.GetReturnValue().Set(data);

}


// The callback that is invoked by v8 whenever the JavaScript 'log'
// function is called.
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


// The callback that is invoked by v8 whenever the JavaScript 'apfilename'
// function is called.
void FTime(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value fnam(args[0]);

	std::string fn = theApp->getBaseDir().c_str();
	fn.append( *fnam );

	// get the file time
	apr_finfo_t finfo;
	apr_status_t stat = apr_stat ( &finfo, fn.c_str(), APR_FINFO_MTIME, theApp->r->pool );
	apr_time_t ftime = finfo.mtime;

	Local<Integer> file_time = Integer::New( args.GetIsolate(), ftime);

	args.GetReturnValue().Set(file_time);
}



// The callback that is invoked by v8 whenever the JavaScript 'setcookie'
// function is called.
void SetCookie(const v8::FunctionCallbackInfo<v8::Value>& args) {

	int argc = args.Length();

	if ( !( argc == 2 || argc == 3 ) ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value name(args[0]);
	v8::String::Utf8Value value(args[1]);

	int expr = 0;

	if ( argc == 3 && !args[2]->IsInt32() ) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad value type",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	if ( argc == 3 ){
		expr = args[2]->Int32Value();
	}

	std::string cookieargs;
	//cookieargs.append( "secure;" );
	cookieargs.append( "HttpOnly;Version=1;" );

	apr_status_t st = ap_cookie_write( theApp->r, *name, *value, cookieargs.c_str(), expr, theApp->r->headers_out, NULL );
}


// The callback that is invoked by v8 whenever the JavaScript 'clearcookie'
// function is called.
void ClearCookie(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value name(args[0]);

	const char *val;
	apr_status_t st = ap_cookie_read( theApp->r, *name, &val, 1 );

}


// The callback that is invoked by v8 whenever the JavaScript 'getcookie'
// function is called.
void GetCookie(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value name(args[0]);

	const char *val;
	apr_status_t st = ap_cookie_read( theApp->r, *name, &val, 0 );

	if( val != NULL ){

		Local<String> data =
			String::NewFromUtf8(args.GetIsolate(), val, NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);

	}

}


// The callback that is invoked by v8 whenever the JavaScript 'setheader'
// function is called.
void SetHeader(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 2) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value name(args[0]);
	v8::String::Utf8Value value(args[1]);

	apr_table_add( theApp->r->headers_out, *name, *value );

}


// The callback that is invoked by v8 whenever the JavaScript 'uuid'
// function is called.
void Uuid(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 0) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	apr_uuid_t uuid;
	apr_uuid_get( &uuid );
	char buf[APR_UUID_FORMATTED_LENGTH + 1];
	apr_uuid_format( buf, &uuid );

	Local<String> data =
		String::NewFromUtf8(args.GetIsolate(), buf, NewStringType::kNormal).ToLocalChecked();

	args.GetReturnValue().Set(data);

}


#define SLOG( L ) \
		args.GetIsolate()->ThrowException( \
				v8::String::NewFromUtf8(args.GetIsolate(), L, \
					v8::NewStringType::kNormal).ToLocalChecked()); \
		ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , L )

// The callback that is invoked by v8 whenever the JavaScript 'socketsend'
// function is called.
void SocketSend(const v8::FunctionCallbackInfo<v8::Value>& args) {

	int argc = args.Length();

	if ( argc != 4) {
		SLOG( "Bad parameters" );
		return;
	}

	v8::String::Utf8Value server(args[0]);
	v8::String::Utf8Value data(args[1]);
	int port = 80;
	int timeout = 15;

	if ( !args[2]->IsInt32() && !args[3]->IsInt32() ) {
		SLOG( "Bad value type" );
		return;
	}

	port = args[2]->Int32Value();
	timeout = args[3]->Int32Value();

	apr_sockaddr_t *sa;
	apr_socket_t *sock;
	apr_status_t st;

#if 0
	std::string l ;
	l.append( "conneting to " );
	l.append( *server );
	l.append( ":" );
	l.append( std::to_string(port) );
	l.append( " to=" );
	l.append( std::to_string(timeout) );
	ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , l.c_str() );
#endif

	st = apr_sockaddr_info_get( &sa, *server, APR_INET, port, 0, theApp->r->pool );
    if (st != APR_SUCCESS) {
		SLOG( "server adress failed" );
		return ;
    }

	st = apr_socket_create( &sock, APR_INET, SOCK_STREAM, APR_PROTO_TCP, theApp->r->pool );
	if (st != APR_SUCCESS) {
		SLOG( "create socket failed" );
		return ;
    }

    apr_socket_opt_set( sock, APR_SO_NONBLOCK, 1 );
    apr_socket_timeout_set( sock, APR_USEC_PER_SEC*timeout );

	st = apr_socket_connect( sock, sa );
    if (st != APR_SUCCESS) {
		SLOG( "connect socket failed" );
		return ;
    }

   	apr_socket_opt_set( sock, APR_SO_NONBLOCK, 1 );
    apr_socket_timeout_set( sock, APR_USEC_PER_SEC*timeout );

	apr_size_t len = data.length();

	st = apr_socket_send( sock, *data, &len );
    if (st != APR_SUCCESS) {
		SLOG( "send on socket failed" );
		return ;
    }
    if ( len != data.length() ) {
		SLOG( "send buffer on socket failed" );
		return ;
    }

	//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , "data send" );

	std::string resp;
	apr_status_t rv;

#define BUFLEN 64

	char buf[BUFLEN+1];
	len = 1;

	while( 1 ) {
		buf[BUFLEN] = 0;
		len = sizeof(buf)-1;

		rv = apr_socket_recv( sock, buf, &len );

		if( len > 0 ){
			buf[len] = 0;
			resp.append( buf );
			//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , buf );
		}
		else {
			//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , ("nothing recv " + std::to_string(len)).c_str() );
		}

		if (rv == APR_EOF || rv == APR_TIMEUP || len == 0 ) {
			break;
		}

	}

	//apr_socket_close( sock );

	Local<String> response =
		String::NewFromUtf8(args.GetIsolate(), resp.c_str(), NewStringType::kNormal).ToLocalChecked();

	args.GetReturnValue().Set(response);

}


// The callback that is invoked by v8 whenever the JavaScript '_getkey'
// function is called.
void GetRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value key(args[0]);
	std::string val;

	int rc = theApp->redis->get( *key, val );

	if( rc == 0 ){

		Local<String> data =
			String::NewFromUtf8(args.GetIsolate(), val.c_str(), NewStringType::kNormal).ToLocalChecked();

		args.GetReturnValue().Set(data);

	}

}


// The callback that is invoked by v8 whenever the JavaScript '_setkey'
// function is called.
void SetRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 2) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value key(args[0]);
	v8::String::Utf8Value val(args[1]);

	//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , "key %s, val %s", *key, *val );

	theApp->redis->set( *key, *val );

}


// The callback that is invoked by v8 whenever the JavaScript '_setkey'
// function is called.
void DelRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value key(args[0]);

	//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , "del key %s", *key );

	theApp->redis->del( *key );

}


// The callback that is invoked by v8 whenever the JavaScript '_expirekey'
// function is called.
void ExpireRedisKey(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() != 2) {
		args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
					v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	v8::String::Utf8Value key(args[0]);

	if ( !args[1]->IsInt32()  ) {
		SLOG( "Bad value type" );
		return;
	}

	int ttl = args[1]->Int32Value();

	//ap_log_rerror( __FILE__,__LINE__,0, 0, NULL, theApp->r , "expire key %s, %i", *key, ttl );

	theApp->redis->expire( *key, ttl );

}


extern "C" {


	int thehandlerimpl(request_rec *r){

		//ap_rputs("mod handler cpp was called :o)\n", r);

		return process_req( r );;
	}



};


