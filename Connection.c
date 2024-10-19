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
      decode_rule(rule_to_decode, &decoded_rule_length, connection_grammar);

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

  /* SETTING UP CONNECTION GRAMMAR */

  grammar_insert(connection_grammar, "short-string",
                 new_function_grammar_entry(shortstring_generator));

  grammar_insert(connection_grammar, "long-string",
                 new_function_grammar_entry(longstring_generator));

  /* SETTING UP METHOD MESSAGE */
  grammar_insert(connection_grammar, "method-id",
                 new_string_grammar_entry("m11-method-id"));

  grammar_insert(connection_grammar, "method-properties",
                 new_string_grammar_entry("m11-method-properties"));

  /* GOING THROUGH METHOD SPECIFIC STUFF */
  overwrite_rule_set_length("client-properties-payload",
                            "client-properties-length", 4, connection_grammar);

  /* PLAIN AUTHENTICATION METHOD */
  grammar_insert(connection_grammar, "authcid",
                 new_string_grammar_entry("\"username\""));

  grammar_insert(connection_grammar, "passwd",
                 new_string_grammar_entry("\"password\""));

  overwrite_rule_set_length("message", "message-length", 4, connection_grammar);

  /* PREPARING METHOD PACKET */
  overwrite_rule_set_length("method-payload", "payload-size", 4,
                            connection_grammar);

  return decode_rule("method", sent_packet_size, connection_grammar);
}

unsigned char *connection_packet_decider(method_struct *method_data,
                                         int *next_state,
                                         int *sent_packet_size) {
  if (method_data->class_id != CONNECTION) {
    printf("Connection Packet Decider received non-Connection payload!\n");
    printf("Received class_id = %u, expected 10\n", method_data->class_id);
    *next_state = 0;
    return NULL;
  }
  if (connection_grammar == NULL)
    connection_grammar = generate_grammar("../grammar/grammar-connection");

  switch (method_data->method_id) {
  case START:
    *next_state = 1;
    return connection_start_ok(sent_packet_size);
    break;
  case TUNE:
  case SECURE:
  case OPEN:
  case CLOSE:
  case CLOSE_OK:
  default:
    printf("Not Implemented\n");
    *next_state = 0;
    return NULL;
    break;
  }
}
