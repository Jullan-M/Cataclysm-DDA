# CMake generated Testfile for 
# Source directory: C:/Users/jmihk/source/repos/Cataclysm-DDA/tests
# Build directory: C:/Users/jmihk/source/repos/Cataclysm-DDA/out/build/x64-Debug/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test-tiles "sh" "-c" "C:/Users/jmihk/source/repos/Cataclysm-DDA/out/build/x64-Debug/tests/cata_test-tiles.exe -r cata --rng-seed `shuf -i 0-1000000000 -n 1`")
set_tests_properties(test-tiles PROPERTIES  WORKING_DIRECTORY "C:/Users/jmihk/source/repos/Cataclysm-DDA" _BACKTRACE_TRIPLES "C:/Users/jmihk/source/repos/Cataclysm-DDA/tests/CMakeLists.txt;8;add_test;C:/Users/jmihk/source/repos/Cataclysm-DDA/tests/CMakeLists.txt;0;")
