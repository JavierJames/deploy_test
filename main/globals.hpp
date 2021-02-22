#pragma once

// #ifndef SHOW_INFO
//     #define SHOW_INFO
// #endif

#include "blue/BluemanInterface.hpp"
#include "blue/BTC_BluemanInterface.hpp"
#include "comms/pic/PicmanInterface.hpp"
#include "system/SysInfo.hpp"

extern bool isOtaBtClassic;

PicmanInterface& getPicman();
BluemanInterface& getBlueman();
BTC_BluemanInterface& getBTCBlueman();
SysInfo& getSysInfo();
