#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include "peg.hpp"

namespace fusion = boost::fusion;
using namespace peg;
using namespace std;

template<class T>
struct t : public T
{
  void action(const T& t, const rep<peg::ws>&) {
    *static_cast<T*>(this) = t;
  }
};

template<char c, class T>
struct op_t : public T
{
  void action(t<typename ch<c>::type>) {}
};

template<class T, class U>
struct op_or : public boost::function<int(int,int)>
{
  typedef boost::function<int(int,int)> Base;
  op_or& operator=(const Base& x) {
    *static_cast<Base *>(this) = x;
    return *this;
  }
  void action(const T& t, sor orr, const U& u) {
    if (orr.left) {
      *this = t;
    } else {
      *this = u;
    }
  }
};

typedef op_t<'+', std::plus<int> > op_plus;
typedef op_t<'-', std::minus<int> > op_minus;
typedef op_t<'*', std::multiplies<int> > op_multiplies;
typedef op_t<'/', std::divides<int> > op_divides;

struct int_lit_s
{
  int v;
  void action(const replus<digit>& s) {
    v = boost::lexical_cast<int>(s);
  }
};

typedef t<int_lit_s> int_lit;

typedef t<ch<'('>::type> open_paren;
typedef t<ch<')'>::type> close_paren;

template<class T, class Op>
struct left_assoc_expr
{
  int v;
  void action(const T& e, const rep<fusion::vector<Op, T> >& elist) {
    v = e.v;
    typedef typename rep<fusion::vector<Op, T> >::const_iterator It;
    for (It i = elist.begin(); i != elist.end(); ++i) {
      v = fusion::at_c<0>(*i)(v, fusion::at_c<1>(*i).v);
    }
  }
};

struct primary_expr;

typedef left_assoc_expr<primary_expr, op_or<op_multiplies, op_divides> > multi_expr;
typedef left_assoc_expr<multi_expr, op_or<op_plus, op_minus> > expr;

struct primary_expr
{
  int v;
  void action(int_lit lit, sor orr,
              open_paren, const expr& e, close_paren) {
    v = orr.left ? lit.v : e.v;
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
  test("10 + 13");
  test("10 + 20 * 3");
  test("(2 + 4) * (5 - 2)");
  test("2 * 4 / 8");
  return 0;
}

