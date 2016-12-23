
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>


#include <hiredis.h>


class redis {

	public:

		std::string server;
		int port;

	private:

		redisContext *c = NULL;
		redisReply *r = NULL;

	public:

		redis( ){
			server = "localhost";
			port = 6379;
		}

		~redis( ){
			disconnect();
		}

		int connect(){

			struct timeval timeout = { 1, 500000 }; // 1.5 seconds
			c = redisConnectWithTimeout(hostname, port, timeout);
			if (c == NULL || c->err) {
				if (c) {
				    //printf("Connection error: %s\n", c->errstr);
				    disconnect();
				} else {
				    //printf("Connection error: can't allocate redis context\n");
				}
				return 1;
			}

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

		std::string get( const char* key ){
			std::string val;
			std::string cmd = "GET " + key;
			r = redisCommand(c, cmd );

			FREE;

			if( r->str ){
				val.assign( r->str );
				return val;
			}

			return NULL;
		}

		void set( const char* key, const char* val ){
			std::string cmd = "SET %b %b" ;
			r = redisCommand(c, cmd, key, strlen( k ), val, strlen( v ) );

			FREE;
		}

		void del( const char* key ){
			std::string cmd = "DEL " + key;
			r = redisCommand(c, cmd );

			FREE;
		}

		void expire( const char* key, int seconds ){
			std::string cmd = "EXPIRE " + key + " " + std::to_string( seconds );
			r = redisCommand(c, cmd );

			FREE;
		}

#undef FREE

};




