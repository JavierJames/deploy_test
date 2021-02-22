#include "BluemanMock.hpp"
#include "PicmanMock.hpp"
// #include "system/SysInfo.hpp"

#include "globals.hpp"

// SysInfo& getSysInfo() {
//     static Mutex mtxSysInfo;
//     static SysInfo sys(&mtxSysInfo);
//     return sys;
// }

PicmanInterface& getPicman() {
    static PicmanMock pic;
    return pic;
}

BluemanInterface& getBlueman() {
    static BluemanMock blue;
    return blue;
}
