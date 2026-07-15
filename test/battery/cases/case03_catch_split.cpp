void f(){
	try{
		g();
	}
	catch(...){
		h();
	}
}
// EXPECT 5:9.3
