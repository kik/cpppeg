#include <iostream>
#include <string>
#include <numeric>
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
  void action(t<ch<c> >) {}
};

template<class T, class U>
struct op_or : public boost::function<int(int,int)>
{
  typedef boost::function<int(int,int)> Base;
  op_or& operator=(const Base& x) {
    *static_cast<Base *>(this) = x;
    return *this;
  }
  void action_0(const T& t) {
    *this = t;
  }
  void action_1(const U& u) {
    *this = u;
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

typedef t<ch<'('> > open_paren;
typedef t<ch<')'> > close_paren;

template<class T, class Op>
struct left_assoc_expr
{
  typedef typename boost::result_of<Op(T,T)>::type result_type;
  result_type v;
  
  struct f {
    result_type operator()(const result_type& e, const fusion::vector<Op, T>& x) {
      return fusion::at_c<0>(x)(e, fusion::at_c<1>(x).v);
    }
  };
  void action(const T& e, const rep<fusion::vector<Op, T> >& elist) {
    v = std::accumulate(elist.begin(), elist.end(), e.v, f());
  }
};

struct primary_expr;

typedef left_assoc_expr<primary_expr, op_or<op_multiplies, op_divides> > multi_expr;
typedef left_assoc_expr<multi_expr, op_or<op_plus, op_minus> > expr;

struct primary_expr
{
  int v;
  void action_0(int_lit lit) {
    v = lit.v;
  }

  void action_1(open_paren, const expr& e, close_paren) {
    v = e.v;
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

