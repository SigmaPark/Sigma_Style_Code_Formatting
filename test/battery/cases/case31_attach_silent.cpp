void f(){
	auto v = Foo{ 1 };
	use(v);
	[](int x){ use(x); }(5);
}
