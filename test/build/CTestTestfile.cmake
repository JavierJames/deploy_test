# CMake generated Testfile for 
# Source directory: /usr/src/myapp/test
# Build directory: /usr/src/myapp/test/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(TestNMEAPkt "/usr/src/myapp/test/build/nmea")
add_test(TestSysInfo "/usr/src/myapp/test/build/sysinfo")
add_test(TestBleAdv "/usr/src/myapp/test/build/bleadv")
add_test(TestGroupInfo "/usr/src/myapp/test/build/groupinfo")
add_test(TestUTADataset "/usr/src/myapp/test/build/utadataset")
subdirs("googletest-build")
