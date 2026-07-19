void f(bool c){
	if(c){ g(); } else{ h(); }
	do{ g(); } while(c);
}
