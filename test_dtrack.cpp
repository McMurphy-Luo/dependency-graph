#include "BaseDefine.h"
#include <iostream>
#include "catch.hpp"
#include "dtrack.h"

std::string Calculator(const int& input, const std::string& test_string) {
  std::string result;
  result = test_string;
  result += input;
  return result;
}

int main(int argc, char* argv[]) {
  std::cout << "Test" << std::endl;
  dtrack::Node<int> test_node(5);
  std::function<std::string(int, std::string)> fuck = Calculator;
  dtrack::Value<std::string, int, std::string> test_value("Fuck", &Calculator);
  std::string test_1 = "123";
  std::string const& test = test_1;
  test_value.Bind<0>(test_node);
  test_node.SetValue(10);
  assert(test_value.GetValue() == "100");
  return 0;
}
