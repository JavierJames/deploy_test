#pragma once

// #include <cstddef>
// #include <cstdint>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CRC-16-CCITT obtained with the table given in this website
 * http://automationwiki.com/index.php/CRC-16-CCITT
 */
uint16_t CRCCCITT(uint8_t* data, uint16_t length, uint16_t seed);

uint32_t byteCRCCCITT(uint8_t data, uint32_t seed);

#ifdef __cplusplus
}
#endif
