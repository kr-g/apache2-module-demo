/* 
**  mod_asdbjs.c -- Apache sample asdbjs module
**  [Autogenerated via ``apxs -n asdbjs -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory 
**  by running:
**
**    $ apxs -c -i mod_asdbjs.c
**
**  Then activate it in Apache's apache2.conf file for instance
**  for the URL /asdbjs in as follows:
**
**    #   apache2.conf
**    LoadModule asdbjs_module modules/mod_asdbjs.so
**    <Location /asdbjs>
**    SetHandler asdbjs
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /asdbjs and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/asdbjs 
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**  
**    The sample page from mod_asdbjs.c
*/ 

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"

#include <stdio.h>
#include <stdlib.h>



static char* runjs( request_rec *r, const char* jsfile ){
	FILE *pp;

	char cmd[FILENAME_MAX];
	cmd[0] = 0;

	strcat ( cmd, "spidermonkeyjs" );
	strcat ( cmd, " " );
	strcat ( cmd, jsfile );

	if( r->args ){
		//strcat ( cmd, " -- " );
		strcat ( cmd, " '" );
		strcat ( cmd, r->args );
		strcat ( cmd, "'" );
	}

	// open a pipe to markdown 
  	pp = popen(cmd, "r");

	// returning later an internal buffer is bad ...
	
	// this also not thread safe !!!

	// also output is limited in total to 60000 chars

	static char buf[60000];
	buf[0]=0;

	if (pp != NULL) {

		strcat( buf, "opened pipe <br/>" );
		strcat( buf, "args " );	
		if( r->args )
			strcat( buf, r->args );	
		strcat( buf, "<br/>" );	
	
		char line[8192];
		char *l = line;

		for(;;){
			l = fgets( l, sizeof line, pp );
			if( !l ) break;
			strncat ( buf, l, sizeof line-1 );
		}
			
		pclose(pp);
  	}	
	else {
		strcat( buf, "calling js failed" );
	}

	return buf;
}



/* The sample content handler */
static int asdbjs_handler(request_rec *r)
{
    if (strcmp(r->handler, "asdbjs")) {
        return DECLINED;
    }
    r->content_type = "text/html";      


	char fnam [FILENAME_MAX];
	fnam[0]=0;
	strcat( fnam , r->filename );
	int len = strlen( fnam );

	// hack for apache file extension
	char* fpos = strstr( fnam, ".html" );
	if( fpos && &(fnam[len -5]) == fpos ){
		*fpos = 0;
	}
	strcat( fnam, ".js" );

	ap_rprintf( r, "running %s\n", fnam );    

	char *html = runjs( r, fnam );
	ap_rprintf( r, "<br/>%s", html );    


    return OK;
}

static void asdbjs_register_hooks(apr_pool_t *p)
{
    ap_hook_handler(asdbjs_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA asdbjs_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    NULL,                  /* table of config file commands       */
    asdbjs_register_hooks  /* register hooks                      */
};
