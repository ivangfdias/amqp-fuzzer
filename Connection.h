#include "../grammar/packet-generator.h"
#include "Packet.h"
#include "utils.h"

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *sent_packet,
                                         int *sent_packet_size);
