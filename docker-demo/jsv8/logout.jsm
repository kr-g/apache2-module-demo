
session();

print("</br><pre><code>");
print( "last vars=" + JSON.stringify( req.session.vars, null, "\t" ) );
print("</code></pre></br>");

print( "<br/>link to index <a href='index'>index</a><br/>" );
print( "<br/>link to page <a href='page'>page</a><br/>" );
print( "<br/>link to v8 <a href='v8'>v8 page</a><br/>" );

sessionend();

print("</br><pre><code>");
print( "after session closed\nrequest=" + JSON.stringify( req, null, "\t" ) );
print("</code></pre></br>");

