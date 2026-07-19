void f(){
	try{
		g();
	} catch(...){
		h();
	}
}
// EXPECT 4:4.3
