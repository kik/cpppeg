#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include "peg.hpp"

namespace f = boost::fusion;
using namespace peg;
using namespace std;

typedef ch<'a'> a_t;
typedef ch<'b'> b_t;
typedef ch<'c'> c_t;

struct A;
struct B;

struct S {
  void action(if_<f::vector<A, unless<b_t> > >, replus<a_t>, const B&, unless<c_t>, eoi) {
  }
};

struct A {
  void action(a_t, opt<A>, b_t) {
  }
};

struct B {
  void action(b_t, opt<B>, c_t) {
  }
};

void test(const string& s)
{
  cout << s << " ---> ";
  parser<S> parser;
  S v;

  if (!parser.parse(v, s.begin(), s.end())) {
    cout << "parse error" << endl;
  } else {
    cout << "parse ok " << endl;
  }
  
}

int main()
{
  test("");
  test("abc");
  test("aabbcc");
  test("aaabbbccc");
  test("aaabbccc");
  test("aabbbccc");
  test("aaabbbcc");
  return 0;
}

