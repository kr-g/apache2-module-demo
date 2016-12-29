
function getmsg(){

	return "jojo-func";

}

function getcontent(){
	return {
			c : "",
			push: function( v ){ this.c+=v; return this; },
			get: function(){ return this.c; },
			clear: function(){ c = ""; }
		};
}

	

