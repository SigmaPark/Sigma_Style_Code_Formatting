# Code Formatting for C++ and CUDA

## Line Length Limit
한 줄의 길이는 탭('\t')과 공백(' ')을 포함하여 100자를 넘지 않아야 한다. 탭은 4자, 공백은 1자로 계산하며 개행('\\n')은 카운트하지 않는다.  

## Indentation using Tabs
들여쓰기는 반드시 공백이 아닌 탭으로 사용한다.  

특별한 경우가 아니면 "줄의 시작" 지점은 들여쓰기가 시작되는 부분을 의미한다. 예를 들어 들임 2단계가 적용된 줄 "\\t\\tAbc..." 에서 그 줄의 시작 문자는 "\\t"가 아닌 "A"이다.  

## Exclusions
다음의 경우는 위에서 언급한 두 규칙(줄길이제한, 탭으로 들여쓰기)만 적용하며 이 후 언급하는 모든 formatting 규칙은 적용하지 않는다.
- 주석
- 문자열 리터럴
- static_assert 및 attribute에 포함된 문자열 인자(컴파일타임 진단 메시지)
- 전처리기 문법(#pragma, #define, #error 등)과 같은 줄에 있는 경우( 백슬래시 "\\" 로 연장된 다음 줄도 포함)  

## Brackets - `( )`, `{ }`, `[ ]`, `< >`
### Exclusions
다음 중 하나라도 해당한다면 `(`, `)`, `{`, `}`, `[`, `]`, `<`, `>` 등의 문자가 있다 하더라도 괄호 표현(brackets expression)으로 간주하지 않으며 후술할 괄호 표현과 관련된 어떠한 규칙에도 적용되지 않는다.

- 매크로 함수 안에 있는 경우
- 괄호가 아닌 비교나 비트 연산 같은 연산자로 해석되는 경우
- 원시 문자열 리터럴에 사용되는 괄호 ( `R"( ... )"`,  `LR"( ... )"` )
- 여는 괄호와 닫는 괄호가 1:1로 대응되지 않는 경우
- 열고 닫는 괄호 문자 쌍 사이에 공백이나 개행 없이 연속해서 쓰인 경우(`()`, `{}`, `[]`, `<>`)

### Single Line
괄호 표현의 여는 괄호 뒤에 개행이 아닌 다른 문자나 공백이 온다면 같은 줄 어딘가에 반드시 그에 대응하는 닫는 괄호가 존재해야 한다.
```C++
Foo(item1, item2, item3); 

std::vector<int> vec;

double d[3] = {0.0, 1.0, 2.0};
```
여는 괄호 바로 다음에 개행이 온다면 이후의 다른 줄 어딘가에 존재하는 닫는 괄호 문자 까지 범위에 대해 multiline 규칙을 적용한다.

#### Nested Brackets in Single line
 같은 줄에 걸쳐 괄호 표현식 안에 또 다른 괄호 표현식이 중첩되어 나타난 경우 여는 괄호 직후와 닫는 괄호 직전에 각각 n개의 공백을 추가하며 n은 다음과 같이 결정된다.
- 여닫는 괄호 안에 같은 종류의 괄호 표현이 없다면 그 괄호의 중첩 단계는 0이다. 
- 여닫는 괄호 안에 있는 같은 종류의 괄호 표현식 중 중첩 단계가 가장 높은 것이 k 라면 그 괄호는 중첩 단계가 k+1 이다.
- 소괄호 `(...)`, 사각괄호 `[...]`, 꺾쇠괄호 `<...>`의 경우 n = (중첩 단계), 중괄호 `{...}`의 경우 n = (중첩 단계) + 1 을 적용한다. 
- 단, n > 3 인 경우는 이 규칙을 적용하지 않으며 여닫는 괄호를 다른 줄에 배치 후 multiline 규칙을 적용해야 한다.

### Multiline
열고 닫는 괄호가 서로 다른 줄에 있는 괄호 표현식에 대해 다음 규칙을 적용한다.
- 열고 닫는 괄호가 있는 두 줄의 들임 깊이(indentation depth)는 같아야 하고, 그 사이에 있는 줄들의 들임 깊이는 여닫는 괄호가 있는 줄의 그것보다 한 단계 깊게 시작한다.
- 여는 괄호는 해당 줄의 가장 마지막, 닫는 괄호는 해당 줄의 처음에 위치한다. 
- 닫는 괄호가 있는 줄에는 콤마 `,`와 세미콜론 `;` 이외의 문자는 쓸 수 없다.
```C++
Bar_blah_blah_blah(
	item1_blah, item2_blah,
	item3_blah_blah_blah, 
	item4_blah_blah_blah_blah
);


struct Object1{
	Object1();
	void calc();
};


auto 
	lambda
	= [
		item1, &item2, item3 = expression_blah_blah,
		&item4 = lvalue
	]
	(int a, int b)->double{
		double res = 0;
		...
		
		return res;
	}
;


template<
	class T, class U,
	class 
	= std::enable_if_t<
		Expression_Blah_Blah
	>
>
void foo(T t, U u);
```

## Virtual Brackets

가상 괄호(virtual brackets)는 실제 코드에는 보이지 않고 자리도 차지하지 않지만, 위에서 언급한 괄호 규칙이 동일하게 적용되는 괄호다(단, 일반 괄호들과 달리 글자 수에 포함하지 않는다). 가상 괄호는 특정 구문에서 특정 키워드 다음에만 발생할 수 있고 그 키워드들을 가상 괄호 키워드(virtual bracket keyword)라고 한다. 가상 괄호가 발생할 수 있는 구문과 그 그 키워드는 다음과 같다.(구문 / "키워드")

- 반환문 / "return"
- 예외 반환문 / "throw"
- 타입 별칭 선언문, 이름공간 using 선언문 / "using"
- 변수 선언문 / "[변수의 타입명(타입 한정사까지 포함)]"

그리고 위 구문의 종료를 알리는 세미콜론을 가상 괄호 종료 세미콜론이라 하고 이 세미콜론 바로 직전에 닫는 가상 괄호가 배치된다.

단, 가상 괄호의 여는 괄호와 닫는 괄호는 반드시 다른 줄에 배치되어야만 한다. 즉, 가상 괄호 키워드와 가상 괄호 종료 세미콜론이 같은 줄에 있는 경우 가상 괄호를 적용하지 않는다.  

개념 설명 및 시각화를 위해 가상 괄호를 `『 』`로 표기하면 다음과 같다.  

### 변수 선언문

```C++
	/*
		double const『
			b = 10*a + 0.23,
			c = b/4,
			*p = &b,
			*const cp = p
		』;
	*/
	double const
		b = 10*a + 0.23,
		c = b/4,
		*p = &b,
		*const cp = p
	;

	/*
		auto const『
			swap_f
			= [](int &a, int &b) noexcept{
				int temp = a;

				a = b;
				b = temp;
			}
		』;
	*/
	auto const
		swap_f
		= [](int &a, int &b) noexcept{
			int temp = a;

			a = b;
			b = temp;
		}
	;
```

### using 을 이용한 구문
```C++
	/*
		using『
			complicated_t
			= std::tuple<
				std::vector<int>, std::list<double>,
				std::map<
					foo_enum_t,
					std::pair<std::string, port_t const *>
				>
			>
		』;
	*/
	using
		complicated_t
		= std::tuple<
			std::vector<int>, std::list<double>,
			std::map<
				foo_enum_t,
				std::pair<std::string, port_t const *>
			>
		>
	;

	/*
		using『
			std::cout, std::endl, std::cerr,
			std::wcout, std::wcerr
		』;
	*/
	using
		std::cout, std::endl, std::cerr,
		std::wcout, std::wcerr
	;
```

### return과 throw 구문
```C++
	if(condition){
		/*
			throw『
				std::runtime_error(
					"a runtime error occurs."
				)
			』;
		*/
		throw
			std::runtime_error(
				"a runtime error occurs."
			)
		;
	}

	/*
		return『
			std::tuple<int, double, Foo *>{
				3, 0.14, &foo
			}
		』;	
	*/
	return
		std::tuple<int, double, Foo *>{
			3, 0.14, &foo
		}
	;
```

## Line Breaks

### Expressions

- 단항연산자와 피연산자는 반드시 같은 줄에 공백 없이 붙여 쓴다.

- 이항연산자 @와 피연산자 F와 B로 이루어진 표현식 "F @ B" 에서 개행이 가능한 위치는 피연산자 F 바로 다음 뿐이다. 개행을 한 경우 그 다음 줄은 반드시 연산자 @로 시작해야 하며 @와 피연산자 B 사이에는 반드시 한 칸 이상의 공백을 넣어야 한다. 

```C++
std::cout
<< "sentence " << Word1 << Number
<< var + 2;

x
= (
	-b
	+ std::sqrt( std::pow(b, 2) - 4*a*c )
)
/ (2*a);
```

- 삼항연산자 `? :` 의 경우 `?` 와 `:` 를 각각 별개의 이항연산자로 취급하여 위의 이항연산자 개행 규칙을 그대로 적용한다. 따라서 `?` 앞에서만 개행, `:` 앞에서만 개행, 또는 양쪽 모두에서 개행하는 형태가 모두 가능하다.

```C++
a ? b
: c;

a
? b
: c;

a
? b : c;
```

- 콤마 `,` 문자를 기준으로 개행을 할 때에는 콤마의 성격에 따라 개행의 위치가 달라진다. 복수의 변수 선언이나 복수의 매개변수를 구분할 때 사용하는 구분자로 콤마를 사용할 때에는 콤마 직후에 개행을 한다. 반면 콤마가 이항연산자로 사용되는 경우는 앞의 이항연산자의 규칙을 동일하게 적용하여 콤마 직전에 개행하고 다음 줄의 맨 앞에 콤마가 위치하게 된다.

```C++
func(
	a, b,
	d, e
);

int const volatile
	var1 = expression1,
	var2 = var1 + expression2,
	*var_p = &var1
;

std::lock_guard<std::mutex>{mtx}
, protected_func(
	param1,
	param2,
	param3
);
```

### Declarations

- 선언문 전체(타입부터 종료 세미콜론까지)가 두 줄 이상을 차지한다면 위에서 언급한 가상 괄호 규칙을 먼저 적용한다. 그리고 그 가상 괄호 내 스코프 안에서는 기본적으로 앞서 언급한 표현식(expressions) 규칙을 적용한다.   

- 타입 선언 후 여러개의 변수들을 선언할 때 구분자로 사용하는 콤마 `,`는 이항연산자가 아니므로 위에서 언급한대로 해당 줄의 맨 마지막에 놓이도록 콤마의 바로 다음에 개행이 이루어진다.

- 변수를 선언할 때 초기화자로 사용하는 `=` 는 연산자는 아니지만 이항연산자와 동일한 개행 규칙을 적용한다. 단, 초기화자 `=` 가 포함된 `var = expression` 형태의 선언문에서 개행을 해야 한다면 반드시 초기화자 `=` 앞의 `var` 직후에 개행을 우선적으로 해야만 한다. 즉, 초기화자를 포함한 선언문의 개행시 초기화자가 가장 높은 개행의 우선순위를 가진다.

```C++
double const
	var0 = x1 + x2, // OK, No line break
	var1
	= a + func1(b, c) * func2(d), // OK
	var2
	= y1 + sqrt(y2 + y3)
	- func3(y4, 2) // OK
;

int const
	violated = x1 - x2
	+ x3 // Violated. initializer "=" should have top priority for line breaking.
;
```

### Member Initializer List 

- 생성자에서 멤버변수를 초기화하는 리스트에 사용하는 콜론 `:` 과 멤버 초기화 구문간에 들어가는 콤마 `,` 는 모두 이항연산자가 아니지만 이항연산자와 동일한 개행 규칙을 적용한다.

- 생성자 전체(생성자부터 멤버초기화, 그리고 구현부까지)가 개행 없이 한 줄로 표현할 수 있을 정도로 짧을 경우는 한 줄에 쓸 수 있다. 개행을 해야한다면 초기화 리스트의 마지막 멤버 초기화 후 개행을 하고 생성자의 구현부를 시작하는 중괄호의 시작은 다음 줄의 맨 처음에 위치시킨다. 즉 멤버 초기화 리스트를 동반한 생성자 구현부에서 개행시 마지막 멤버 초기화 구문 직후 위치가 가장 높은 우선순위를 가진다.

```C++
Bar::Bar(int x, int y) : _mem_x(x), _mem_y(y){} // OK, No line break


Foo::Foo(int x, int y)
: _mem1(x), _mem2(y)
, _mem3(_init3())
, _mem_var(
	[x]{
		auto res = x+1;

		...

		return res;
	}
	()
)
, _final_mem_var(0) // line break after final member initialization
{
	...
}

/*
	Violated. Line break after final member initialization should be considered as top priority.
*/
Wrong::Wrong(int x, int y) : _a(x)
, _b(y){} 
```

