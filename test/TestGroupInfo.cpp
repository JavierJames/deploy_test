#include <stdio.h>
#include <string.h>
#include "GroupInfo.hpp"
#include "gtest/gtest.h"

#include "HAL_TDD.hpp"

namespace {

mac_ascii_t mac_new = {'a', 'b', '0', '1', '2', '3',
                       'c', 'd', '2', '3', 'c', 'd'};
mac_t mac_exp = {0xab, 0x01, 0x23, 0xcd, 0x23, 0xcd};
spk_id_ascii_t addr_new = {'1', 'b', '2', '1', 'c', 'd'};
spk_id_t addr_exp = {0x1B, 0x21, 0xCD};

class TestGroupInfo : public ::testing::Test {
   protected:
    GroupInfo* group;
    mac_t mac;
    GroupMemberList list;

    virtual void SetUp() { group = new GroupInfo(); }

    virtual void TearDown() { delete group; }
};

ACTION_P(PassArg0ThruPtr, value) {
    strcpy(static_cast<char*>(arg0), value);
}

TEST_F(TestGroupInfo, GroupEmpty_when_Init) {
    EXPECT_EQ(0, group->getNumMembers());
    EXPECT_EQ(0xFFFFFF, group->getId());
    EXPECT_FALSE(group->isComplete());
}

TEST_F(TestGroupInfo, UpdateInfo_when_SingleNewFollowerAdded) {
    group->newMember(mac_new, addr_new);

    EXPECT_EQ(1, group->getNumMembers());
    list = group->getMembers();
    EXPECT_EQ(mac_exp, list.at(0).mac);
    EXPECT_EQ(addr_exp, list.at(0).id);
}

TEST_F(TestGroupInfo, CleanUpInfo_when_NotEmptyAndRequested) {
    group->newMember(mac_new, addr_new);
    group->clear();

    EXPECT_EQ(0, group->getNumMembers());
}

TEST_F(TestGroupInfo, UpdateInfo_when_MultipleNewFollowersAdded) {
    mac_ascii_t mac2_new = {'0', '1', '0', '1', '2', '3',
                            'c', 'd', '2', '3', 'c', 'd'};
    mac_t mac2_exp = {0x01, 0x01, 0x23, 0xcd, 0x23, 0xcd};
    spk_id_ascii_t addr2_new = {'c', '1', 'c', '2', '0', '0'};
    spk_id_t addr2_exp = {0xC1, 0xC2, 0x00};

    group->newMember(mac_new, addr_new);
    group->newMember(mac2_new, addr2_new);

    list = group->getMembers();
    EXPECT_EQ(2, group->getNumMembers());

    EXPECT_EQ(mac_exp, list.at(0).mac);
    EXPECT_EQ(addr_exp, list.at(0).id);
    EXPECT_EQ(mac2_exp, list.at(1).mac);
    EXPECT_EQ(addr2_exp, list.at(1).id);
}

TEST_F(TestGroupInfo, ReturnError_when_MaxFollowers) {
    for (int i = 0; i < list.size(); i++)
        EXPECT_TRUE(group->newMember(mac_new, addr_new));

    EXPECT_FALSE(group->newMember(mac_new, addr_new));

    ASSERT_EQ(list.size(), group->getNumMembers());
    EXPECT_FALSE(group->isComplete());
}

TEST_F(TestGroupInfo, NewItemNotSaved_when_FollowersListFull) {
    mac_ascii_t mac_last_new = {'D', 'D', 'A', 'C', '2', '3',
                                'c', 'd', '2', '3', 'c', 'd'};
    mac_t mac_last_exp = {0xdd, 0xac, 0x23, 0xcd, 0x23, 0xcd};
    spk_id_ascii_t addr_last_new = {'c', '1', 'c', '2', '0', '0'};
    spk_id_t addr_last_exp = {0xC1, 0xC2, 0x00};

    for (int i = 0; i < list.size() - 1; i++)
        EXPECT_TRUE(group->newMember(mac_new, addr_new));
    EXPECT_TRUE(group->newMember(mac_last_new, addr_last_new));
    EXPECT_FALSE(group->newMember(mac_new, addr_new));

    list = group->getMembers();
    EXPECT_EQ(mac_last_exp, list.back().mac);
    EXPECT_EQ(addr_last_exp, list.back().id);
}

TEST_F(TestGroupInfo, GroupComplete_when_AllFollowersMacsSaved) {
    group->setExpectedNumMembers(1);

    group->newMember(mac_new, addr_new);

    EXPECT_TRUE(group->isComplete());
}

}  // namespace
