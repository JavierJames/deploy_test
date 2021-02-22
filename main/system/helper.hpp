#pragma once

#include <cstddef>
#include <cstdint>
#include <stdint.h>

/**
 * CRC-16-CCITT obtained with the table given in this website
 * http://automationwiki.com/index.php/CRC-16-CCITT
 */
 // uint16_t CRCCCITT(uint8_t* data,
 //                   size_t length,
 //                   uint16_t seed);

inline uint8_t nibbles2byte(uint8_t msn, uint8_t lsn) {
    return (msn << 4) +
           lsn;  // join upper Nibble and lower Nibble to make a byte
}

uint8_t ascii2nibble(char c);

/**
 * Converts 2 chars {most significant, least significant} to a byte.
 */
uint8_t ascii2byte(char msn, char lsn);
