#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int parse_mode(int argc, char** argv) {
	int mode;
	int option;
	int num_of_modes = 0;
	while (option = getopt(argc, argv, "rwd") == -1) {
		switch (option) {
			case "r":
				mode = 1;
				break;
			case "w":
				mode = 2;
				break;
			case "d":
				mode = 3;
				break;
		}
		num_of_modes++;
	}	

	if (num_of_modes != 1) {
		return -1;
	} else {
		return mode;
	}
}

void db_read(int argc, char** argv) {
	int option;
	while (option = getopt(argc, argc)	
}

void db_write(int argc, char** argv) {
	

}

void db_delete(int argc, char** argv) {

}


int main(int argc, char** argv) {
	int mode = parse_mode(argc, argv);
	switch (mode) {
		case 1:
			db_read(argc, argv);
		case 2:
			db_write(argc, argv);
		case 3:
			db_delete(argc, argv);
		case -1:
			fprintf(stderr, "Wrong mode error\n");
	}
}
