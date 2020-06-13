#include "BaseDefine.h"
#include <iostream>
#include "catch.hpp"
#include "Node.h"

int Calculator(const int& input) {
  return input + 1;
}

int main(int argc, char* argv[]) {
  std::cout << "Test" << std::endl;
  dependency::Node<int> test_node(5);
  dependency::Value<int, int> test_value(6, &Calculator);
  test_value.Bind<0>(test_node);
  test_node.SetValue(10);
  assert(test_value.GetValue() == 11);
  return 0;
}
