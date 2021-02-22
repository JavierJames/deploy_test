#include "PicStream.hpp"

static const char LTAG[] = "PicStream";
static HAL& hal = getHAL();

PicStream::PicStream(PicParser& _parser)
    : txqueue(20), parser(_parser), rxBuff{}, rxBuffLastPos(0) {
    LOGD(LTAG, "Queue size %i", txqueue.available());
}

void PicStream::task() {
    /**
     * Object used to dequeue the packets to be transmitted through UART.
     * It can also be used internally to encode and add new packets to the
     * queue.
     */
    SerialPkt pkt;

    for (;;) {
        int res = hal.uartPIC->read(this->rxBuff + this->rxBuffLastPos, 1);
        if (res < 0) {
            LOGE(LTAG, "Failed to read");
        } else if (res > 1) {
            LOGE(LTAG, "Returned more items than expected");
        } else if (res == 1) {
            if (this->rxBuff[this->rxBuffLastPos] == '$') {
                this->rxBuff[0] = '$';
                this->rxBuffLastPos = 1;
            } else if (this->rxBuff[this->rxBuffLastPos++] == '\0') {
                if (pktRx.parseArray(this->rxBuff, this->rxBuffLastPos))
                    this->parser.parsePkt(pktRx, &this->txqueue);
                this->rxBuffLastPos = 0;
            }
        }

        // Send only one packet if there's enough space left
        if (txqueue.pop(pkt, 0)) {
            hal.uartPIC->write(pkt.raw, pkt.rawLen);
        }
    }
    remove();
}
