#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

typedef struct {
  char type;
  short channel;
  int size;
  unsigned char *frame;
} packet_struct /*! test */;

/*! Breaks an application layer packet into something that makes sense for AMQP
 */
void break_packet(unsigned char *packet, packet_struct *dest);

unsigned char *AMQP_method_frame(short channel, int size, short packet_class,
                                 short method, unsigned char *payload);

unsigned char *AMQP_header_frame(short channel, int size, long body_size,
                                 short packet_class, short weight,
                                 short property_flags, char *properties,
                                 long size_of_properties);

unsigned char *AMQP_body_frame(short channel, int size, unsigned char *payload);

unsigned char *AMQP_frame(char type, short channel, int size,
                          unsigned char *payload);

char send_packet(int connfd, unsigned char *packet, long size);

int add_string_entry(unsigned char *dest, char field_size,
                     unsigned char *field_contents, int extra_info, int index);

unsigned char *get_arguments(unsigned char *src);

char verify_length(unsigned char *packet, int length);
