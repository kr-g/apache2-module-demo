
var global = {};
var gcnt =0;
var req;

function printc( str ){
	if( typeof str === "object" )
		str = JSON.stringify( str );	
	print( "<!--"+str+"-->");
}

global.templates = {};

function loadtemplate( name, returnobject ){

	var mtime = ftime( name );
	//log( "ftime for " + name + " = " + mtime );

	var fnct;
	var toload = true;

	if( name in global.templates ){
		var mt = global.templates[name].mtime;
		toload = !( mt === mtime );
	}

	if( toload ){
	//	log( "loading " + name );
		fcnt = read(name);
		global.templates[name] = { 'name': name,
						'mtime': mtime,
						'source': fcnt,
						'template': Handlebars.compile(fcnt)
					};
	}

	fcnt = global.templates[name].template;

	return !returnobject ? fcnt : global.templates[name]; 
}

global.modules = {};

function loadmodule( name, mtime ){
	//log( "ftime for " + name + " = " + ftime( name ) );
	return loadmodulecache( name, mtime, true, false );
}

function loadmodulecache( name, mtime, globalrunonce, isjsm ){

	var fnct;
	var toload = true;
	var modname = "global_mod_js___" + name.replace( /[\.\/\\]/g, "_" );
	//log( "modname= "+modname );

	if( name in global.modules ){
		var mt = global.modules[name].mtime;
		toload = !( mt === mtime );
	}

	if( toload ){
		log( "loading " + name );
		fcnt = read(name);
		global.modules[name] = { 'name': name,
						'modname': modname,
						'mtime': mtime,
						'source': fcnt,
						'runonce': 0
					};
		if( isjsm ){
			fcnt = "function "+modname+" (){ "+ fcnt + ";}\n";
			var geval = eval;
			geval( fcnt );
			//geval( "global.modules[name].module = fcnt;" );
			fcnt = modname+"();";
			log( "compiling " + modname );
		}

	}
	else{
	//	log( "get from cache " + name );
		if( isjsm ) {
			//log( "use precompiled " + modname );
			fcnt = modname+"();";
		}
		else {
			if( globalrunonce && global.modules[name].runonce > 0 ) {
				return;
			}

			log( "compiling " + modname );
			fcnt = global.modules[name].source;
		}
	}

	global.modules[name].runonce = 1;

	var geval = eval;
	return geval(  fcnt  );
}

function splitquery( q ){
	var o = {};
	o.query = {};
	o.queryaraw = q.split("&");
	o.queryaraw.forEach( function(val){
			val = unescape( val );
			var p = val.indexOf("=");
			if( p === -1 ){
			if( val.length > 0 )
			o.query[val] = "";
			}else{
			var k = val.substr( 0, p );
			var v = val.substr( p+1 );	
			o.query[k] = v;
			}
			});
	return o;
}


global.session = {
	'id': "modjsid",
	'on': false,
	'ttl': 60*30
};


function route( filename, filetime ){

	req = {};

	req.apfilename = apfilename();
	req.unparseduriraw = unparseduri();
	req.unparseduri = unescape( req.unparseduriraw );
	req.method = method();

	req.query = {};
	req.queryraw = "";

	var p = req.unparseduri.indexOf("?");
	if( p > 0 ){
		req.queryraw = req.unparseduri.substr( p );
		req.query = splitquery( req.queryraw.substr( 1 ) );
		req.query.raw = req.queryraw;
	}

	var m = loadmodulecache( filename, filetime, false, true );

	//log( "return from req" );

	if( global.session.on ){
		var setttl = req.session.isnew;
		if( req.session.isnew ){
			req.session.id = uuid();
			//log( "new session " + req.session.id );
			setcookie( global.session.id, req.session.id );
			//log( "set cookie " + req.session.id );
		}
		var s = JSON.stringify( req.session.vars );
		_setkey( req.session.id, s );
		_expirekey( req.session.id, global.session.ttl );
		global.session.on = false;
	}

	//log( "req done" );

	return req;
}

function session(){
	global.session.on = true;
	req.session = { 
		'isnew': false, 
		'vars': {} 
	};

	var c = getcookie( global.session.id );
	if( c === undefined ){
		req.session.isnew = true;
		return;
	}

	var s = _getkey( c );
	if( s === undefined ){
		// time out
		req.session.isnew = true;
		return;
	}

	req.session.id = c;
	req.session.vars = JSON.parse( s );
}

function sessionend(){
	_delkey( req.session.id );
	clearcookie( global.session.id );
	global.session.on = false;
	delete req.session;
}


function postdataparser( data, type ){
	if( type.indexOf( "application/x-www-form-urlencoded" ) >= 0 ){
		return splitquery( data ).query;
	}

	if( type.indexOf( "multipart/form-data" ) >= 0 ){
		var BND = "boundary=";
		var p = type.indexOf( BND );
		if( p == -1 ) return;
		p = p + BND.length + 1;
		var pattern = type.substr( p );

		var arr = data.replace(/\r/g, "\n").split( "\n" );
		var o = {};
		var okey = "";
		arr.forEach( function( e ){

				if( e.indexOf( pattern ) >= 0 ){
				okey="";
				return;
				}
				if( e.indexOf( "Content-Disposition" ) >= 0 && e.indexOf("name=") >= 0 ){
				var m = e.match(/name=[\"\']+(.+)[\"\']+/);
				var k = m[1];
				okey = k;
				return;
				}
				if( okey.length == 0 ){
				return;
				}
				var c = "";
				if( okey in o ) c = o[okey];
				o[okey] = c+e;
				});
		return o;

	}

}

loadmodule( "handlebars-v4.0.5.js" );

function gethtml( source, context ){
	return Handlebars.compile(source)(context);
}

function gethtmlfromtempl( source, context ){
	return source(context);
}


