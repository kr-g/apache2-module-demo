

static int printitem(void *rec, const char *key, const char *value)
{
	request_rec *r = rec;
	ap_rprintf(r, "%s: %s\n", ap_escape_html(r->pool, key), ap_escape_html(r->pool, value));
	/* Zero would stop iterating; any other return value continues */
	return 1;
}

static void printheader(request_rec *r){
	ap_rprintf( r, "protocol %s\n", r->protocol );
	ap_rprintf( r, "uri %s\n", r->unparsed_uri );
	ap_rprintf( r, "path_info %s\n", r->path_info );
	ap_rprintf( r, "method %s\n", r->method );
	ap_rprintf( r, "useragent_ip %s\n", r->useragent_ip );
	ap_rprintf( r, "filename %s\n", r->filename );
   	ap_rprintf( r, "args %s\n", r->args );

	apr_array_header_t* header = apr_table_elts( r->headers_in ) ;
	ap_rprintf( r, "*headers_in\n");
	apr_table_do(printitem, r, r->headers_in, NULL);
}

static char getfilecontent(request_rec *r, char* fnam){
	apr_file_t* fd;

	apr_status_t rv = apr_file_open( &fd , fnam, APR_FOPEN_READ, APR_OS_DEFAULT, r->pool );
	apr_finfo_t finfo;
	rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, fd);

	char* buf = ap_malloc( finfo.size+1 );
	buf[finfo.size]=0;

	apr_size_t rb ;

	rv = apr_file_read_full( fd, buf, finfo.size, &rb );

	apr_file_close(fd);

	if( verbose ){
		ap_rprintf( r, "size %u\n", finfo.size );
		ap_rprintf( r, "content %s\n", buf );
	}

	return buf;
}



