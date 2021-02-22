#include "GroupInfo.hpp"

#include "HAL.hpp"

#include "system/helper.hpp"

static const char LTAG[] = "GroupInfo";

GroupInfo::GroupInfo() : membersNum(0), membersExpecNum(0) {}

bool GroupInfo::newMember(mac_ascii_t& mac, spk_id_ascii_t& addr) {
    if (this->membersNum == this->groupMembers.size()) {
        LOGE(LTAG, "No space left for new followers");
        return false;
    } else {
        // Convert and save MAC address
        for (uint8_t i = 0; i < sizeof(mac_t); i++) {
            this->groupMembers.at(this->membersNum).mac[i] =
                ascii2byte(mac[2 * i], mac[2 * i + 1]);
        }
        // Convert and save KN address (member ID)
        for (uint8_t i = 0; i < sizeof(spk_id_t); i++) {
            this->groupMembers.at(this->membersNum).id[i] =
                ascii2byte(addr[2 * i], addr[2 * i + 1]);
        }
        this->membersNum++;
        return true;
    }
}

void GroupInfo::clear() {
    this->membersNum = 0;
}

bool GroupInfo::isComplete() {
    if (this->membersExpecNum == 0) {
        return false;
    } else if (this->membersExpecNum == this->membersNum) {
        LOGI(LTAG, "GroupInfo complete");
        return true;
    } else {
        return false;
    }
}
