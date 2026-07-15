class C{
public:
	auto size() const->int{ return _n; }
	auto has() const->bool{ return _n != 0; }
private:
	int _n = 0;
};
