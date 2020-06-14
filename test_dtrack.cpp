#include "BaseDefine.h"
#include <iostream>
#include "catch.hpp"
#include "dtrack.h"

int Calculator(const int& input) {
  return input + 1;
}

TEST_CASE("Test one node and one value bind") {
  dtrack::DNode<int> test_node(5);
  dtrack::DValue<int, int> test_value(0, &Calculator);
  CHECK(test_value.Value() == 0);
  test_value.Bind<0>(test_node);
  CHECK(test_value.Value() == 6);
  test_node.SetValue(10);
  CHECK(test_value.Value() == 11);
}

int main(int argc, char* argv[]) {
  printf("Running main() from %s\n", __FILE__);
  int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  flag |= _CRTDBG_LEAK_CHECK_DF;
  flag |= _CRTDBG_ALLOC_MEM_DF;
  _CrtSetDbgFlag(flag);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
  _CrtSetBreakAlloc(-1);
  int result = Catch::Session().run(argc, argv);
  std::getchar();
  return result;
}
