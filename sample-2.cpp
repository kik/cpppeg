#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include "peg.hpp"

namespace fusion = boost::fusion;
using namespace peg;
using namespace std;

typedef fusion::vector<digit, rep<alpha>, digit, alpha, digit, eoi> data;

void test(const string& s)
{
  cout << s << " ---> ";
  parser<data> parser;
  data v;

  if (!parser.parse(v, s.begin(), s.end())) {
    cout << "parse error" << endl;
  } else {
    using namespace fusion;
    cout << at_c<0>(v) << "(" << at_c<1>(v) << ")" << at_c<2>(v)
      << "(" << at_c<3>(v) << ")" << at_c<4>(v) << endl;
  }
  
}

int main()
{
  test("1hoge2x3");
  test("12x3");
  test("123");
  test("1hoge3");
  return 0;
}

