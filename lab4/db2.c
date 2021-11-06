#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <string.h>

typedef struct {
	int32_t key;
	char info[16];
	double value;
} RECORD;

//  MODE:  
//  1 - read
//  2 - write
//  3 - delete
// -1 - error

int mode = -1;
char* mode_name;
int32_t key = -1;
char info[16] = {'\0'};
double value;

int isset_mode = 0, isset_k = 0, isset_i = 0, isset_v = 0, arg_err = 0;

void parse_args(int argc, char** argv);
void check_args(); 

int main(int argc, char** argv) {
	parse_args(argc, argv);
	check_args();
}

void parse_args(int argc, char** argv) {
	int option;
	char* end;

	while((option = getopt(argc, argv, "rwdk:i::v::")) != -1) {
		switch (option) {
			case 'r':
				mode = isset_mode == 0 ? 1 : -1;
				isset_mode = 1;
				mode_name = "read";
				break;
			case 'w':
				mode = isset_mode == 0 ? 2 : -1;
				isset_mode = 1;
				mode_name = "write";
				break;
			case 'd':
				mode = isset_mode == 0 ? 3 : -1;
				isset_mode = 1;
				mode_name = "delete";
				break;
			case 'k':
				isset_k = 1;
				key = (int32_t)(strtol(optarg, &end, 10));
				break;
			case 'i':
				isset_i = 1;
				strcpy(info, optarg);
			   	break;
			case 'v':
				isset_v = 1;
				value = strtod(optarg, NULL);
				break;
			default:
				arg_err = 1;
				break;
		}
	}

	printf("IS_SET:\nk: %d\ni: %d\nv: %d\n", isset_k, isset_i, isset_v);
	printf("VALUES:\nmode: %d\nkey: %d\ninfo: %s\nvalue: %lf\nargerr: %d\n", mode, key, info, value, arg_err);
}

int check_args() {
	if (arg_err) {
		fprintf(stderr, "Unvalid argument\n");
		return 1;
	} 
	
	if (mode == -1) {
		fprintf(stderr, "Mode error\n");
		return 2;
	}	

	if (mode == 1 || mode == 3) {
		if (isset_k == 0) 
			fprintf(stderr, "You must provide key in %s mode.\n", mode_name);
		if (isset_v || isset_i)
			fprintf(stderr, "Arguments other than key are forbidden in %s mode.\n", mode_name);
			
	}
	

}


