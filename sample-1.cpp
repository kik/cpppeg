#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include "peg.hpp"

using namespace peg;
using namespace std;

template<class T>
struct token : public T
{
  void action(const T& t, const rep<peg::ws>&) {
    *static_cast<T*>(this) = t;
  }
};

struct int_lit_s
{
  int v;
  void action(const replus<digit>& s) {
    v = boost::lexical_cast<int>(s);
  }
};

typedef token<int_lit_s> int_lit;

struct op_s
{
  boost::function<int(int,int)> v;
  
  void action(const replus<alpha>& s) {
    if (s == "add") {
      v = std::plus<int>();
    } else {
      v = std::minus<int>();
    }
  }
};

typedef token<op_s> op;
typedef token<ch<'('> > open_paren;
typedef token<ch<')'> > close_paren;

struct expr
{
  int v;
  void action_0(int_lit e) {
    v = e.v;
  }

  void action_1(open_paren, op o, const expr& e1, const expr& e2, close_paren) {
    v = o.v(e1.v, e2.v);
  }
};

void test(const string& s)
{
  cout << s << " ---> ";
  parser<expr> parser;
  expr e;

  if (!parser.parse(e, s.begin(), s.end())) {
    cout << "parse error" << endl;
  } else {
    cout << e.v << endl;
  }
  
}

int main()
{
  test("1");
  test("42");
  test("(add 10 23)");
  test("(add 100 (sub 20 5))");
  test("(add (add 10 20) (add 5 13))");
  test("(add)");
  return 0;
}

