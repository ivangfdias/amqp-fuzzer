#include "Grammar_Interface.h"

void overwrite_rule_with_chaos(Grammar grammar, char *rule,
                               grammar_entry_t *ordered_rule) {

  char **rule_array;
  char *chaos_rule;
  char *order_rule;

  char *chaos_prefix = "fuzzer-chaos-";
  char *order_prefix = "fuzzer-order-";
  int new_rule_size = strlen(chaos_prefix) + sizeof(rule) + 1;

  switch (RULE_CHAOS) {
  case 0:
    grammar_insert(grammar, rule, ordered_rule);
    break;
  case 4:
    // Intentionally left blank
    break;
  case 2:
    rule_array = calloc(2, sizeof(char *));
    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    rule_array[0] = order_rule;
    rule_array[1] = chaos_rule;
    grammar_insert(grammar, chaos_rule, grammar_lookup(grammar, rule));
    grammar_insert(grammar, order_rule, ordered_rule);
    grammar_insert(grammar, rule, new_str_array_grammar_entry(rule_array, 2));
    break;
  case 1:
  case 3:
    rule_array = calloc(4, sizeof(char *));

    chaos_rule = calloc(new_rule_size, sizeof(char));
    order_rule = calloc(new_rule_size, sizeof(char));
    strcpy(chaos_rule, chaos_prefix);
    strcat(chaos_rule, rule);
    strcpy(order_rule, order_prefix);
    strcat(order_rule, rule);

    int i;
    for (i = 0; i < 4 - RULE_CHAOS; i++){
        rule_array[i] = order_rule;
        rule_array[4 - i - 1] = chaos_rule;
    }
    grammar_insert(grammar, chaos_rule, grammar_lookup(grammar, rule));
    grammar_insert(grammar, order_rule, ordered_rule);
    grammar_insert(grammar, rule, new_str_array_grammar_entry(rule_array, 2));
    break;
  }
}
