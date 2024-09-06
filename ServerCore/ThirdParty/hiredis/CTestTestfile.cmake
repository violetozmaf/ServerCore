# CMake generated Testfile for 
# Source directory: C:/Users/naddic/Downloads/hiredis-master/hiredis-master
# Build directory: C:/Users/naddic/Downloads/hiredis-master/hiredis-cmake
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(hiredis-test "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/test.sh")
  set_tests_properties(hiredis-test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;235;ADD_TEST;C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(hiredis-test "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/test.sh")
  set_tests_properties(hiredis-test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;235;ADD_TEST;C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(hiredis-test "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/test.sh")
  set_tests_properties(hiredis-test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;235;ADD_TEST;C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(hiredis-test "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/test.sh")
  set_tests_properties(hiredis-test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;235;ADD_TEST;C:/Users/naddic/Downloads/hiredis-master/hiredis-master/CMakeLists.txt;0;")
else()
  add_test(hiredis-test NOT_AVAILABLE)
endif()
