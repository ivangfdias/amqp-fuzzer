#include "Connection.h"

static Grammar connection_grammar = NULL;
typedef enum {
  START = 10,
  START_OK = 11,
  SECURE = 20,
  SECURE_OK = 21,
  TUNE = 30,
  TUNE_OK = 31,
  OPEN = 40,
  OPEN_OK = 41,
  CLOSE = 50,
  CLOSE_OK = 51
} connection_method;

unsigned char *shortstring_generator(int *length, Grammar environment) {
  unsigned char shortstring_length = (rand() % RULE_REPETITION_INFTY) % 256;

  unsigned char *result = calloc(shortstring_length + 1, sizeof(char));

  result[0] = shortstring_length;
  int length_dummy = 0;
  for (int i = 1; i < shortstring_length + 1; i++) {
    result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
  }
  *length = shortstring_length + 1;
  return result;
}

unsigned char *longstring_generator(int *length, Grammar environment) {
  unsigned int string_length = (rand() % RULE_REPETITION_INFTY);
  unsigned char *result = calloc(string_length + 4, sizeof(char));

  int_in_char(result, string_length, 0, string_length + 4);
  int length_dummy = 0;
  for (int i = 4; i < string_length + 4; i++) {
    result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
  }
  *length = string_length + 4;
  return result;
}
void overwrite_rule_set_length(char *rule_to_decode, char *length_rule,
                               int length_size, Grammar grammar) {

  int decoded_rule_length = 0;
  unsigned char *decoded_rule =
      decode_rule(rule_to_decode, &decoded_rule_length, grammar);

  grammar_insert(grammar, rule_to_decode,
                 new_grammar_entry_t(BYTE_ARRAY, (char *)decoded_rule, NULL,
                                     decoded_rule_length, NULL));

  char *decoded_length_literal = calloc(length_size, sizeof(char));
  int_in_char((unsigned char *)decoded_length_literal, decoded_rule_length, 0,
              length_size);

  grammar_insert(grammar, length_rule,
                 new_grammar_entry_t(BYTE_ARRAY, decoded_length_literal, NULL,
                                     length_size, NULL));
}

unsigned char *connection_start_ok(int *sent_packet_size) {
  Grammar mutable_grammar = copy_grammar(connection_grammar);

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(mutable_grammar, "method-id",
                 new_string_grammar_entry("m11-method-id"));

  grammar_insert(mutable_grammar, "method-properties",
                 new_string_grammar_entry("m11-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  overwrite_rule_set_length("client-properties-payload",
                            "client-properties-length", 4, mutable_grammar);

  /* PLAIN AUTHENTICATION METHOD */
  grammar_insert(mutable_grammar, "authcid",
                 new_string_grammar_entry("\"username\""));

  grammar_insert(mutable_grammar, "passwd",
                 new_string_grammar_entry("\"password\""));

  overwrite_rule_set_length("message", "message-length", 4, mutable_grammar);

  /* PREPARING METHOD PACKET */

  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar);
  unsigned char *result =
      decode_rule("method", sent_packet_size, mutable_grammar);

  return result;
}

unsigned char *connection_tune_ok(int *sent_packet_size,
                                  method_struct *method_data) {

  /* GETTING INFORMATION FROM METHOD DATA */
  int channel_max = 0;
  int frame_max = 0;

  int received_channel_max =
      char_in_short((unsigned char *)method_data->arguments_byte_array, 0);
  int received_frame_max =
      char_in_int((unsigned char *)method_data->arguments_byte_array, 2);

  channel_max = rand() % (received_channel_max + 1); // enable overflow
  frame_max = rand() % (received_frame_max + 1);     // enable overflow

  Grammar mutable_grammar = copy_grammar(connection_grammar);

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(mutable_grammar, "method-id",
                 new_string_grammar_entry("m31-method-id"));

  grammar_insert(mutable_grammar, "method-properties",
                 new_string_grammar_entry("m31-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  unsigned char *frame_max_char = calloc(4, sizeof(char));
  int_in_char(frame_max_char, frame_max, 0, 4);
  grammar_insert(mutable_grammar, "frame-max",
                 new_byte_array_grammar_entry((char *)frame_max_char, 4));
  unsigned char *channel_max_char = calloc(2, sizeof(char));
  short_in_char(channel_max_char, channel_max, 0, 2);
  grammar_insert(mutable_grammar, "channel-max",
                 new_byte_array_grammar_entry((char *)channel_max_char, 2));

  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar);
  unsigned char *result =
      decode_rule("method", sent_packet_size, mutable_grammar);

  return result;
}

unsigned char *connection_open(int *sent_packet_size) {

  Grammar mutable_grammar = copy_grammar(connection_grammar);

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(mutable_grammar, "method-id",
                 new_string_grammar_entry("m40-method-id"));

  grammar_insert(mutable_grammar, "method-properties",
                 new_string_grammar_entry("m40-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  grammar_insert(mutable_grammar, "virtual-host",
                 new_string_grammar_entry("\%x01.2F"));
  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar);
  unsigned char *result =
      decode_rule("method", sent_packet_size, mutable_grammar);

  return result;
}

unsigned char *connection_close(int *sent_packet_size) {
  fuzz_debug_printf("Connection close!!\n");

  Grammar mutable_grammar = copy_grammar(connection_grammar);

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(mutable_grammar, "method-id",
                 new_string_grammar_entry("m50-method-id"));

  grammar_insert(mutable_grammar, "method-properties",
                 new_string_grammar_entry("m50-method-properties"));

  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar);
  unsigned char *result =
      decode_rule("method", sent_packet_size, mutable_grammar);

  return result;
}

unsigned char *connection_close_ok(int *sent_packet_size) {

  Grammar mutable_grammar = copy_grammar(connection_grammar);

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(mutable_grammar, "method-id",
                 new_string_grammar_entry("m51-method-id"));

  grammar_insert(mutable_grammar, "method-properties",
                 new_string_grammar_entry("m51-method-properties"));
  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            mutable_grammar);
  unsigned char *result =
      decode_rule("method", sent_packet_size, mutable_grammar);

  return result;
}

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *next_state, int *sent_packet_size,
                                         char *response_expected) {

  if (method_data == NULL && *next_state == 5) {
    *next_state = 7;
    *response_expected = 1;
    printf("Channel 0: Sending CLOSE\n");
    return connection_close(sent_packet_size);
  } else {

    if (method_data->class_id != CONNECTION) {
      fuzz_debug_printf(
          "Connection Packet Decider received non-Connection payload!\n");
      fuzz_debug_printf("Received class_id = %u, expected 10\n",
                        method_data->class_id);
      *next_state = 7;
      return NULL;
    }
    if (connection_grammar == NULL) {
      /* SETTING UP CONNECTION GRAMMAR */
      connection_grammar = generate_grammar("./grammars/grammar-connection");

      grammar_insert(connection_grammar, "short-string",
                     new_function_grammar_entry(shortstring_generator));

      grammar_insert(connection_grammar, "long-string",
                     new_function_grammar_entry(longstring_generator));
    }

    printf("Channel 0: Received method_id = %u\n", method_data->method_id);
    switch (method_data->method_id) {
    case START:
      *next_state = 1;
      *response_expected = 1;
      printf("Channel 0: Sending START_OK\n");
      return connection_start_ok(sent_packet_size);
      break;
    case TUNE:
      *next_state = 3;

      *response_expected = 1;

      int tune_ok_length = 0;
      unsigned char *tune_ok = connection_tune_ok(&tune_ok_length, method_data);
      int open_length = 0;
      unsigned char *open = connection_open(&open_length);

      *sent_packet_size = tune_ok_length + open_length;
      printf("Channel 0: Sending TUNE_OK + OPEN\n");
      return packet_append(tune_ok, open);
    case SECURE:
    case OPEN_OK:
      fuzz_debug_printf("Connected!!!\n");
      *next_state = 5;
      *response_expected = 0;
      return NULL;
    case CLOSE:
      *next_state = 7;
      *response_expected = 1;
      printf("Channel 0: Sending CLOSE_OK\n");
      return connection_close_ok(sent_packet_size);
    case CLOSE_OK:
      *next_state = 8;
      *response_expected = 1;
      return NULL;
    default:
      printf("[ERROR] Not Implemented\n");
      *next_state = 0;
      return NULL;
      break;
    }
  }
}
