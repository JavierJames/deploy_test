# CMake generated Testfile for 
# Source directory: /home/javier/repo/deploy_env_test/deploy_test_esp/test
# Build directory: /home/javier/repo/deploy_env_test/deploy_test_esp/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(TestNMEAPkt "/home/javier/repo/deploy_env_test/deploy_test_esp/test/nmea")
add_test(TestSysInfo "/home/javier/repo/deploy_env_test/deploy_test_esp/test/sysinfo")
add_test(TestBleAdv "/home/javier/repo/deploy_env_test/deploy_test_esp/test/bleadv")
add_test(TestGroupInfo "/home/javier/repo/deploy_env_test/deploy_test_esp/test/groupinfo")
add_test(TestUTADataset "/home/javier/repo/deploy_env_test/deploy_test_esp/test/utadataset")
subdirs(googletest-build)
