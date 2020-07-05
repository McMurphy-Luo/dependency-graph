#include "BaseDefine.h"
#include <iostream>
#include "catch.hpp"
#include "dtrack.h"

using std::string;
using std::pair;
using std::make_pair;

int Calculator(const int& input) {
  return input + 1;
}

TEST_CASE("Test one value and one tracker bind") {
  dtrack::DTrack global;
  dtrack::DValue<int> test_node(global, 5);
  dtrack::DTracker<int, int> test_value(global, 2, &Calculator);
  CHECK(test_value.Value() == 2);
  test_value.Watch<0>(test_node);
  test_value.Update();
  CHECK(test_value.Value() == 6);
  test_node.SetValue(10);
  CHECK(test_value.Value() == 11);
}

pair<bool, int> StringToIntConvertor(string param) {
  return make_pair(false, 0);
}

pair<bool, int> IntSum() {
  return make_pair(false, 0);
}

TEST_CASE("Test several value and several trackers bind") {
  dtrack::DTrack global;
  dtrack::DValue<string> test_value_string_1(global);
  dtrack::DValue<int> test_value_int(global);
  dtrack::DValue<string> test_value_string_2(global);
  dtrack::DTracker<pair<bool, int>, string, int, string> tracker_1(
    global,
    [] (string, int, string) -> pair<bool, int> {
      return make_pair(false, 0);
    }
  );
  tracker_1.Apply();
  CHECK(tracker_1.Value().second == 0);
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
  _CrtDumpMemoryLeaks();
  std::getchar();
  return result;
}
