
session();

function inside(){
	return "jojo";
}

// test

var e = loadmodule( "include.js" ) ;

var ctx = { 'bootstrap-css' : '/bootstrap-3.3.7-dist/css/bootstrap.min.css',
	'bootstrap-js' : '/bootstrap-3.3.7-dist/js/bootstrap.min.js',
	'jquery-js' : 'https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js'
};

var cnt = getcontent();


cnt.push("hello world"+"</br>"+new Date());
cnt.push("</br>eval result="+eval(15+3));

log( "logmessage from v8.jsm " + new Date().toString() );

cnt.push( "</br>json = " + JSON.stringify( { a: 1 }, null, '\t' ) );
cnt.push( "</br>json wo stringify = " +  { a: 1 }  );
cnt.push("</br>");

cnt.push( "call function = " + getmsg() +"</br>");

cnt.push( "<br/>link to index <a href='index'>index page</a><br/>" );
cnt.push( "<br/>link to page <a href='page'>page</a><br/>" ); 

if( method().toLowerCase() === "post" ){
	var type = header("content-type");
	var data = postdata();

	cnt.push("</br>contenttype="+type);

	cnt.push("</br>");
	cnt.push("postdataparser=" + JSON.stringify( postdataparser( data, type ), null, '\t' ) );
}


cnt.push("</br><pre><code>");
cnt.push( "request=" + JSON.stringify( req, null, "\t" ) );
cnt.push("</code></pre></br>");

cnt.push("</br>");

// using additional build in template

var source   = "<hr><p>using handlebars for templating</p><p>{{name}}</p><p>{{body}}</p><hr>";
var context = {name: "the v8 javascript demo", body: "This is my first demo!"};
// call global function from main.jsm
cnt.push( gethtml( source, context ) );

setcookie( "modjs", "v8", 60*60 );

// set the main content inside the context ctx
ctx['main-content'] = cnt.get();
var templ = loadtemplate( "v8-2.templ" );
print( gethtmlfromtempl( templ, ctx ) );


// return status 201 - created
status( 201 );
contenttype( "text/html" );


