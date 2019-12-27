#include <stdint.h>
struct init {
   uint32_t type; 
   uint32_t hashreq_num;
   
} __attribute__((packed));
struct ack {
   uint32_t type; 
   uint32_t hashresp_len;//= 40 * hashrequest number
   
} __attribute__((packed));
struct hash_request {
   uint32_t type; 
   uint32_t data_len;
   
} __attribute__((packed));
struct hash_response {
   uint32_t type; 
   uint32_t index;
   uint8_t  data[32];
   
} __attribute__((packed));
