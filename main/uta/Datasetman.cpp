#include "Datasetman.hpp"
#include "comms/NMEAPkt.hpp"
#include "comms/pic/Picman.hpp"

#include "globals.hpp"
static PicmanInterface& pic = getPicman();

static const char LTAG[] = "Datasetman";

void Datasetman::start(void) {
    // create task
    this->create("dataset man", 2048, 10);
}

void Datasetman::task() {
    Kien::ds_uta_t ds;

    while (1) {
        // If there is data in the rx uta dataset then process it
        if (Kien::ds_get_rx_queue().pop(ds, 20)) {
            LOGI(LTAG, "Processing new beacon...");
            LOGD(LTAG, " - spk TX ID: 0x%02X%02X%02X", ds.spk_tx_id.at(0),
                 ds.spk_tx_id.at(1), ds.spk_tx_id.at(2));
            LOGD(LTAG, " - room ID: 0x%02x", ds.room_id);
            LOGD(LTAG, " - tof: 0x%04x", ds.tof);
            LOGD(LTAG, " - rssi: %d", ds.rssi);

            // Format the data
            char data[25];  // minimum size to serialize dataset
            Kien::ds_serialize(ds, data, sizeof(data));

            // Send the data to the PIC
            pic.sendUTADatasetSample(data);
        } 

        this->delay(200);
    }
    this->remove();
}
