#ifndef SDP_H
#define SDP_H

struct sdp_pdu
{
    uint8_t pdu_id;
    uint16_t transaction_id; //big endian
    uint8_t data[];
} __attribute__((packed));

void sdp_recv_data(uint8_t * buf, int32_t len);
int32_t sdp_get_data(uint8_t * buf);

#endif /* SDP_H */
