#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Packet.h"
#include "utils.h"


unsigned char *AMQP_frame(char type, short channel, int size,
                          unsigned char *payload) {
  int total_size = size + 4 + 4;
  unsigned char *packet = malloc((total_size) * sizeof(char));

  packet[0] = type;
  short_in_char(packet, channel, 1, total_size);
  int_in_char(packet, size, 3, total_size);

  memcpy(packet + (3 + 4) * sizeof(char), payload, size * sizeof(char));
  packet[total_size - 1] = 0xce;

  return packet;
}

// Adiciona também o tamanho do payload no começo
unsigned char *AMQP_method_frame(short channel, int size, short class,
                                 short method, unsigned char *payload) {

  unsigned char *full_payload = malloc(size + 4);
  short_in_char(full_payload, class, 0, size + 4);
  short_in_char(full_payload, method, 2, size + 4);
  if (payload != NULL)
    memcpy(full_payload + 4, payload, size + 4);

  unsigned char *packet = AMQP_frame(0x01, channel, size + 4, full_payload);
  free(full_payload);
  // *status = 10; // TODO: remove
  return packet;
}

// channel, length, remaining_length,
//            class_id, weight, property_flags,
//            delivery_mode
unsigned char *AMQP_header_frame(short channel, int size, long body_size,
                                 short class, short weight,
                                 short property_flags, char *properties,
                                 long size_of_properties) {

  int total_size = 8 + 2 + 2 + 2 + size_of_properties;
  unsigned char *payload = malloc(total_size);
  short_in_char(payload, class, 0, total_size);
  short_in_char(payload, weight, 2, total_size);
  long_in_char(payload, body_size, 4, total_size);
  short_in_char(payload, property_flags, 12, total_size);
  memcpy(payload + 14, properties, size_of_properties);
  unsigned char *packet = AMQP_frame(0x02, channel, size, payload);
  free(payload);
  return packet;
};

unsigned char *AMQP_body_frame(short channel, int size,
                               unsigned char *payload) {
  return AMQP_frame(0x03, channel, size, payload);
};

char send_packet(int connfd, unsigned char *packet, long size) {
  int written_size = write(connfd, packet, size);
  if (written_size == size) {
    free(packet);
    return 0;
  }
  free(packet);
  return 1;
}

int add_string_entry(unsigned char *dest, char field_size,
                     unsigned char *field_contents, int extra_info, int index) {
  int full_size = field_size + extra_info;
  dest[index] = field_size;
  memcpy(dest + index + 1, field_contents, full_size);
  return full_size;
}

unsigned char *get_arguments(unsigned char *src) {
  // arguments start on position 11;
  int size = char_in_int(src, 3) - 4;
  int arguments_size;
  for (arguments_size = 0;
       arguments_size < size && src[11 + arguments_size] != 0xce;
       arguments_size++)
    ;

  char *arguments = malloc(arguments_size * sizeof(char));
  memcpy(arguments, src + 11, arguments_size);
  return arguments;
}

char verify_length(unsigned char *packet, int length) {
  int index = 7;
  for (index = 7; index < length + 7; index++) {
    if ((unsigned char)packet[index] == 0xce) {
      return 2; // too short
    }
  }

  if ((unsigned char)packet[index] != 0xce) {
    printf("Octet at %d: %2x\n", index, (unsigned char)packet[index]);
    return 1;
  }
  return 0;
}


void free_packet_struct(packet_struct* packet){
  switch(packet->type){
	  case METHOD:
		  free(packet->method_payload->arguments_byte_array);
		  free(packet->method_payload);
		  break;
	  case HEADER:
		  free(packet->header_payload->property_list);
		  free(packet->header_payload);
		  break;
	  case BODY:
		  free(packet->body_payload->body);
		  free(packet->body_payload);
	  default:
		  break;
  }	  
  free(packet);
};

packet_struct* break_packet(unsigned char *packet){

	printf("BREAK_PACKET\n");
  packet_struct* result = calloc(1, sizeof(packet_struct));
  frame_type type = packet[0];
  short channel = char_in_short (packet, 1);
  int size = char_in_int(packet, 3);
  if (packet[7 + size] != 0xCE){
	  result->type = NONE;
	  return result;
  }
  result->type = type;
  result->channel = channel;
  result->size = size;
  switch(type){
	case METHOD:
		method_struct* method_payload = calloc(1, sizeof(method_struct));

		method_payload->class_id = packet[8] + packet[9] << 4;
		method_payload->method_id = char_in_short(packet, 10);

		method_payload->arguments_byte_array = calloc(size - 4, sizeof(char));
		memcpy(method_payload->arguments_byte_array, packet + 8 + size + 4, size - 4);

		method_payload->arguments_length = size - 4;

		result->method_payload = method_payload;
		break;
	case HEADER:
		break;
	case BODY:
		break;
	case HEARTBEAT:
		break;
	case NONE:
		break;
	default:
		break;
  }
  printf("assigning\n");
  return result;
};

