#include "Grammar_Interface.h"
#include "Packet.h"
#include "utils.h"

unsigned char setup_connection_grammar();

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *sent_packet,
                                         int *sent_packet_size,
                                         char *response_expected);
unsigned char *connection_start_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected);
unsigned char *connection_tune_ok(int *next_state, int *sent_packet_size,
                                  char *response_expected,
                                  method_struct *method_data);
unsigned char *connection_open(int *next_state, int *sent_packet_size,
                               char *response_expected);
unsigned char *connection_close(int *next_state, int *sent_packet_size,
                                char *response_expected);
unsigned char *connection_close_ok(int *next_state, int *sent_packet_size,
                                   char *response_expected);
