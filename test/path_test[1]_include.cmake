if(EXISTS "/home/ademirao/workspace/restfs/test/path_test[1]_tests.cmake")
  include("/home/ademirao/workspace/restfs/test/path_test[1]_tests.cmake")
else()
  add_test(path_test_NOT_BUILT path_test_NOT_BUILT)
endif()
