
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>


#include <hiredis.h>


class modjsredis {

	public:

		std::string server;
		int port;

	private:

		redisContext *c = NULL;
		redisReply *r = NULL;

	public:

		modjsredis( ){
			server = "localhost";
			port = 6379;
		}

		~modjsredis( ){
			disconnect();
		}

		int connect(){

			struct timeval timeout = { 1, 500000 }; // 1.5 seconds
			c = redisConnectWithTimeout( server.c_str(), port, timeout );
			if (c == NULL || c->err) {
				if (c) {
					//printf("Connection error: %s\n", c->errstr);
					disconnect();
				} else {
					//printf("Connection error: can't allocate redis context\n");
				}
				return 1;
			}

			return 0;
		}

		int reconnect(){
			if( c )
				return redisReconnect( c );
			return -1;
		}

#define FREE \
		if( r ){	freeReplyObject(r); r=NULL;	}

		void disconnect(){
			if( c ){
				redisFree(c);
				c = NULL;
			}
			FREE;
		}

		std::string GET = "GET ";
		std::string DEL = "DEL ";
		std::string EXPIRE = "EXPIRE ";

		int get( const char* key, std::string & val ){
			val.clear();
			std::string cmd = GET + key ;
			r = (redisReply*) redisCommand(c, cmd.c_str() );

			if( r && r->str ){
				val.assign( r->str );
				FREE;
				return 0;
			}

			FREE;
			return -1;
		}

		void set( const char* key, const char* val ){
			std::string cmd = "SET %b %b" ;
			r = (redisReply*) redisCommand(c, cmd.c_str(), key, strlen( key ), val, strlen( val ) );

			FREE;
		}

		void del( const char* key ){
			std::string cmd = DEL + key;
			r = (redisReply*) redisCommand(c, cmd.c_str() );

			FREE;
		}

		void expire( const char* key, int seconds ){
			std::string cmd = EXPIRE + key + " " + std::to_string( seconds );
			r = (redisReply*) redisCommand(c, cmd.c_str() );

			FREE;
		}

};


