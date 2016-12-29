
session();

print("use query parameter to fill the session vars</br>");
print("e.g. <a href='page?a=5&b=6'>page with vars</a> to refresh click then here <a href='page'>page</a></br>");

print("</br><pre><code>");
print( "last vars=" + JSON.stringify( req.session.vars, null, "\t" ) );
print("</code></pre></br>");


req.session.vars = req.query;


setheader( "x-modjs-v", "jojo---" );

//print( "args=" + scriptArgs + "<br/>" );
print("another page<br/>");

print( "<br/>link to index <a href='index'>index</a><br/>" );
print( "<br/>link to v8 <a href='v8'>v8 page</a><br/>" );
print( "<br/>link to logout <a href='logout'>logout page</a><br/>" );

print("</br><pre><code>");
print( "request=" + JSON.stringify( req, null, "\t" ) );
print("</code></pre></br>");

print("cookie = " + getcookie( "modjs" ) );

print("<br/>");

if( method().toLowerCase() === "post" ){
	var type = header("content-type");
	var data = postdata();

	print("</br>contenttype="+type);

	print("</br>");
	print("postdataparser=" + JSON.stringify( postdataparser( data, type ), null, '\t' ) );
}

