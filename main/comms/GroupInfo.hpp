#pragma once

#include <array>
#include <cstdint>

typedef std::array<char, 12> mac_ascii_t;
typedef std::array<uint8_t, 6> mac_t;
typedef std::array<char, 6> spk_id_ascii_t;
typedef std::array<uint8_t, 3> spk_id_t;
typedef std::array<mac_t, 12> foll_list_t;

typedef struct {
    mac_t mac;
    spk_id_t id;
} GroupMemberInfo;

typedef std::array<GroupMemberInfo, 12> GroupMemberList;

/**
 * Class to parse the group information received from the PIC to forward it to
 * the app.
 */
class GroupInfo {
   public:
    GroupInfo();

    /**
     * Add a new group member to the list.
     *
     * @param mac MAC in ASCII format.
     * @param id speaker ID in ASCII format.
     */
    bool newMember(mac_ascii_t& mac, spk_id_ascii_t& addr);

    /**
     * Clear the list of members and all related info.
     */
    void clear();

    /**
     * Request the number of members that are currently in the list.
     *
     * @return number of group members.
     */
    int getNumMembers() { return this->membersNum; };

    /**
     * Get the list of members of a group.
     *
     * @return list of members.
     */
    GroupMemberList& getMembers() { return this->groupMembers; };

    /**
     * Get the ID of the group.
     *
     * @return ID of the group.
     */
    int getId() { return 0xFFFFFF; };

    /**
     * Update the expected number of members. Since the the members are
     * added 1 by 1, this value is used to know when the group is complete.
     *
     * @param expected number of members.
     */
    void setExpectedNumMembers(int num) { this->membersExpecNum = num; };

    /**
     * Check if there is any members missing in the list (i.e. if the number of
     * members is the expected one).
     *
     * @return true when complete, false otherwise.
     */
    bool isComplete();

   private:
    int membersNum;
    int membersExpecNum;
    GroupMemberList groupMembers;
};
