#include "BleStream.hpp"

BleStream::BleStream(BleParser& _parser)
    : txQueue(5), parser(_parser), rxQueue(10) {
    ble_nus_init(rxQueue.handle);
}

void BleStream::task() {
    nus_pkt_t cmd;
    for (;;) {
        if (rxQueue.pop(cmd, 10)) {
            this->parser.parsePkt(&cmd, txQueue);
        }

        if (txQueue.pop(cmd, 0)) {
            ble_nus_send(cmd.data, cmd.len);
        }

    }
    remove();
}
