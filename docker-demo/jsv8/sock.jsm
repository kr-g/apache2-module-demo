
var req = "GET /utc/now.json HTTP/1.1\r\n"+
	"Host: www.timeapi.org\r\n"+
	"Connection: close\r\n"+
	"Cache-Control: max-age=0\r\n"+
	"Upgrade-Insecure-Requests: 1\r\n"+
	"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/53.0.2785.143 Chrome/53.0.2785.143 Safari/537.36\r\n"+
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"+
	"Accept-Encoding: gzip, deflate, sdch\r\n"+
	"Accept-Language: en-US,en;q=0.8"+
	"\r\n\r\n";

print( "<pre><code>"+req+"</pre></code>" );

var rc = socketsend( "www.timeapi.org", req, 80, 1*15 );

print( "<pre><code>"+rc+"</pre></code>" );

try{
	var m = rc.match( /(.*)\r\n\r\n(.*)/ );
	var body = m[2];
	
	var o = JSON.parse( body );
	print( "<pre><code>"+o.dateString+"</pre></code>" );
}catch(e){
	print( e );
}


