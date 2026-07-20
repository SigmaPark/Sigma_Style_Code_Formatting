void f(){
	[](int x){ use(x); }
	(5);
}
// EXPECT-SUSPECT 3:9.3
