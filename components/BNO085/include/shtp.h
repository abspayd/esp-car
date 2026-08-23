#ifndef BNO085_SHTP_H
#define BNO085_SHTP_H

#include <stdint.h>

typedef struct {
    uint16_t length;
    uint8_t channel;
    uint8_t seq_num;
    uint8_t continuation;
} shtp_header_t;

/**
 * @brief Encode data into an SHTP packet
 *
 * @param (uint8_t *) dst : the encoded data
 * @param (size_t) dst_capacity : the capacity (in bytes) of the result buffer
 * @param (size_t *) dst_length : the length (in bytes) returned after encoding the data
 * @param (uint8_t *) payload : the input data
 * @param (size_t) payload_length : the output data length (in bytes)
 * @param (uint8_t) channel : shtp channel number
 * @param (uint8_t) sequence : shtp sequence number
 * @param (bool) continuation : true if this is a continuation of another packet, false otherwise
 */
uint8_t SHTP_Encode(uint8_t *dst, size_t dst_capacity, size_t *dst_length, uint8_t *payload, size_t payload_length,
                    uint8_t channel, uint8_t sequence, bool continuation);

/**
 * @brief Parse an SHTP header from a raw SHTP packet
 * @param (uint8_t*) buf : the data to encode
 * @param (size_t) length : the length (in bytes) of buf
 * @param (shtp_header_t *) : the parsed header
 * @return (uint8_t) status : 0 on success
 */
extern uint8_t SHTP_ParseHeader(uint8_t *buf, size_t length, shtp_header_t *header);

#endif
