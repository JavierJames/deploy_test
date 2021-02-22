# CMake generated Testfile for 
# Source directory: /home/javier/repo/esp32-firmware/test
# Build directory: /home/javier/repo/esp32-firmware/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(TestNMEAPkt "/home/javier/repo/esp32-firmware/test/nmea")
add_test(TestSysInfo "/home/javier/repo/esp32-firmware/test/sysinfo")
add_test(TestBleAdv "/home/javier/repo/esp32-firmware/test/bleadv")
add_test(TestGroupInfo "/home/javier/repo/esp32-firmware/test/groupinfo")
add_test(TestUTADataset "/home/javier/repo/esp32-firmware/test/utadataset")
subdirs(googletest-build)
