
long parse_value_long(char * s);
float parse_value_float(char * s);

void parser_commit(char * parse_id, int parse_id_length, char * parse_value, int parse_value_length);
void parse_block(char * s, int length);
