#include <stdio.h>
#include <string.h>
#include "SysInfo.hpp"
#include "gtest/gtest.h"

#include "HAL_TDD.hpp"

using ::testing::ElementsAreArray;
using ::testing::Return;

namespace {

class TestSysInfo : public ::testing::Test {
   protected:
    TestSysInfo()
        : nameLen_Expected(10),  // letters in name_Expected
          name_Expected("Im_tha_PIC"),
          ver_Expected{{'a', 'b', 'c'}},
          id_Expected{{0xA1, 0xB3, 0x75}},
          mode(SpkMode::LEADER){};

    // Values to set (write)
    size_t nameLen_Expected;
    const char name_Expected[PIC_FW_NAME_MAX_LEN];
    const std::array<char, FW_VER_LEN> ver_Expected;
    const std::array<uint8_t, 3> id_Expected;

    SpkMode mode_Expected;

    // Placeholders to get (read) values
    char name[PIC_FW_NAME_MAX_LEN];
    size_t nameLen;
    std::array<char, FW_VER_LEN> ver;
    std::array<uint8_t, 3> id;
    SpkMode mode;

    SysInfo* sys;
    Mutex mtx;

    virtual void SetUp() {
        sys = new SysInfo(&mtx);
        memset(name, 0, sizeof(name));
        ver = {'\0', '\0', '\0'};
        nameLen = 0;
    }
    virtual void TearDown() { delete sys; }
};

TEST_F(TestSysInfo, SavePicName_when_ResourceNotBusy) {
    // Save the name (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setPicFwName(name_Expected, nameLen_Expected));

    // Read back the name (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    nameLen = sys->getPicFwName(name);
    ASSERT_EQ(nameLen, nameLen_Expected);
    EXPECT_THAT(
        std::vector<char>(name, name + nameLen),
        ElementsAreArray({'I', 'm', '_', 't', 'h', 'a', '_', 'P', 'I', 'C'}));
}

TEST_F(TestSysInfo, SavePicName_when_TwoAttempts_and_ResourceNotBusy) {
    // Save the name (resource not being used)
    const char nameWrong_Expected[PIC_FW_NAME_MAX_LEN] = "Wrong_name";
    size_t nameWrongLen_Expected = sizeof(nameWrong_Expected);
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setPicFwName(nameWrong_Expected, nameWrongLen_Expected));

    // Save the name (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setPicFwName(name_Expected, nameLen_Expected));

    // Read back the name (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    nameLen = sys->getPicFwName(name);
    ASSERT_EQ(nameLen, nameLen_Expected);
    EXPECT_THAT(
        std::vector<char>(name, name + nameLen),
        ElementsAreArray({'I', 'm', '_', 't', 'h', 'a', '_', 'P', 'I', 'C'}));
}

TEST_F(TestSysInfo, DoNotSavePicName_when_ResourceBusy) {
    // Do not save the name (resource being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->setPicFwName(name_Expected, nameLen_Expected));

    // Read back empty name (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    nameLen = sys->getPicFwName(name);
    EXPECT_EQ(nameLen, 0);  // Empty!
}

TEST_F(TestSysInfo, DoNotReadPicName_when_ResourceBusy) {
    // Read back empty name (resource not being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    nameLen = sys->getPicFwName(name);
    EXPECT_EQ(nameLen, 0);  // Empty!
}

TEST_F(TestSysInfo, SavePicVersion_when_ResourceNotBusy) {
    // Save the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setPicFwVersion(ver_Expected));

    // Read back the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getPicFwVersion(ver));
    EXPECT_EQ(ver, ver_Expected);
}

TEST_F(TestSysInfo, DoNotSavePicVersion_when_ResourceBusy) {
    // Do not save the version (resource being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->setPicFwVersion(ver_Expected));

    // Read back empty version (resource not being used)
    const std::array<char, FW_VER_LEN> verDefault{PIC_FW_VERSION_ERROR};
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getPicFwVersion(ver));
    EXPECT_EQ(ver, verDefault);
}

TEST_F(TestSysInfo, DoNotReadPicVersion_when_ResourceBusy) {
    // Read back empty version (resource not being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->getPicFwVersion(ver));
}

TEST_F(TestSysInfo, SaveSpeakerMode_when_ResourceNotBusy) {
    // Save the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setCurrentSpkMode(mode_Expected));

    // Read back the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getCurrentSpkMode(&mode));
    EXPECT_EQ(mode, mode_Expected);
}

TEST_F(TestSysInfo, DoNotSaveSpeakerMode_when_ResourceBusy) {
    // Do not save the version (resource being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->setCurrentSpkMode(mode_Expected));

    // Read back empty version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getCurrentSpkMode(&mode));
    EXPECT_EQ(mode, SpkMode::UNKNOWN);  // Empty!
}

TEST_F(TestSysInfo, DoNotReadSpeakerMode_when_ResourceBusy) {
    // Read back empty version (resource not being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->getCurrentSpkMode(&mode));
    EXPECT_EQ(mode, SpkMode::UNKNOWN);  // Empty!
}

TEST_F(TestSysInfo, SaveSpkId_when_ResourceNotBusy) {
    // Save the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->setSpkId(id_Expected));

    // Read back the version (resource not being used)
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getSpkId(id));
    EXPECT_EQ(id, id_Expected);
}

TEST_F(TestSysInfo, DoNotSaveSpkId_when_ResourceBusy) {
    // Do not save the version (resource being used)
    EXPECT_CALL(mtx, lock()).WillOnce(Return(false));
    ASSERT_FALSE(sys->setSpkId(id_Expected));

    // Read back empty version (resource not being used)
    const std::array<uint8_t, 3> idDefault{SPK_ID_DEFAULT};
    EXPECT_CALL(mtx, unlock()).WillOnce(Return(true));
    EXPECT_CALL(mtx, lock()).WillOnce(Return(true));
    ASSERT_TRUE(sys->getSpkId(id));
    EXPECT_EQ(id, idDefault);
}

}  // namespace
