#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "parser.h"

// common line parsing routines

// init the parser
int parse_state = 0;
char parse_id[1032];    int parse_id_length = 0;
char parse_value[1032]; int parse_value_length = 0;

void parser_reset(void) {
	parse_state = 0;
	parse_id_length = 0; parse_id[0] = 0;
	parse_value_length = 0; parse_value[0] = 0;
}

void split_string(char * s, char delim, int size, char * vector[], int limit) {
	int ws = 1;
	int i;
	int r = 0;
	for(i=0; i<size; i++) {
		char ch = s[i];
		// have we flipped state?
		int ns = (ch == ' ') ? 1 : 0;
		if(ws != ns) {
			ws = ns;
			if(ns==0) {
				// start of a new entry
				// printf("   split %i at %i\n",r,i);
				vector[r++] = &s[i];
			} else {
				// start of whitespace. reset character to 0 to make strings nice
				s[i] = 0;
			}
		}
		if(r==limit) return;
	}
	vector[r] = 0;
}


long parse_value_long(char * s) {
	if(s == NULL) return 0;
	// do basic parse
	long n = atol(s);
	// done
	return n;
}

float parse_value_float(char * s) {
	if(s == NULL) return 0;
	// do basic parse
	float f = atof(s);
	// done
	return f;
}

void parse_block(char * s, int length) {
	// printf("parse_block %d %s\n",length,s);
	int i;
	for(i = 0; i<length; i++) {
		// get the next character
		char ch = s[i];
		// printf("  state %d char %d %d \n",parse_state,i,ch);
		// main parser state switch
		switch(parse_state) {
			case 0: // expecting id or =
				switch(ch) {
					case '\n': case '\r':
						// line break before =. reset parser.
						parser_reset();
						break;
					case '=':
						// id terminator
						parse_id[parse_id_length] = '\0'; // zero terminate for easy string handling
						parse_state = 1;
						break;
					default:
						// id character
						parse_id[parse_id_length++] = ch;
				}
				break;
			case 1: // expecting value characters or eol
				switch(ch) {
					case '\n': case '\r':
						// line break. all done.
						parse_value[parse_value_length] = '\0'; // zero terminate for easy string handling
						parser_commit(parse_id, parse_id_length, parse_value, parse_value_length);
						// reset the parser for the next line
						parser_reset();
						break;
					default:
						// id character
						parse_value[parse_value_length++] = ch;
				}
				break;
		}
		// if any of our buffers have overrun, then reset
		if( (parse_id_length == 1024) || (parse_value_length == 1024) ) {
			parser_reset();
		}
	}
}
