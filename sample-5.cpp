#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include "peg.hpp"

using namespace peg;
using namespace std;

struct A {
  string v;

  void action_0(alpha l, const A &a, alpha r) {
    v += l;
    v += '(';
    v += a.v;
    v += ')';
    v += r;
  }

  void action_1(alpha a) {
    v += a;
  }
};

struct B {
  A v;
  void action(const A& a, eoi) {
    v = a;
  }
};

void test(const string& s)
{
  cout << s << " ---> ";
  parser<B> parser;
  B v;

  if (!parser.parse(v, s.begin(), s.end())) {
    cout << "parse error" << endl;
  } else {
    cout << "parse ok: " << v.v.v << endl;
  }
  
}

int main()
{
  test("a");
  test("aa");
  test("aaa");
  test("aaaa");
  test("aaaaa");
  test("aaaaaa");
  test("aaaaaaa");
  test("aaaaaaaa");
  return 0;
}

