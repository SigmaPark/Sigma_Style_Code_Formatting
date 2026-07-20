struct S{
	int v;
};

auto Make(S *q)->S *{
	auto  
		r
		= new struct S{ 1 }
		* q
	;

	return r;
}
