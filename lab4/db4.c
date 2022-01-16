#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define LINE_LENGTH 40
size_t LINE_SIZE = sizeof(char) * LINE_LENGTH;
char* format = "%-7d  %-16s  %-10.6lf";

typedef struct {
	int32_t key;
	char info[16];
	double value;
} RECORD;

RECORD data = { -1, {'\0'}, -1.0};
#define R_SIZE sizeof(RECORD)

//  MODE:  
//  1 - read
//  2 - write
//  3 - delete
// -1 - error

int fd;

int mode = -1;
char* mode_name;
int32_t key = -1;
char info[16] = {'\0'};
double value;

int isset_mode = 0, isset_k = 0, isset_i = 0, isset_v = 0;

int parse_args(int argc, char** argv);
int check_args();
int db_read();
int db_write();
int db_delete();
off_t find_record(int key);


int main(int argc, char** argv) {
	int res;

	// Parse arguments and assign them to global variables	
	res = parse_args(argc, argv);
	if (res != 0) {
		return res;
	}

	// Validate arguments' values
	res = check_args();
	if (res != 0) {
		return res;
	}

	// Open database in correct mode
	const char* path = argv[argc-1];

	if (mode == 1) {
		fd = open(path, O_RDONLY);
		res = db_read(fd);
		if (res != 0) {
			return 1;
		}
	} else if (mode == 2) {
		fd = open(path, O_RDWR | O_APPEND | O_CREAT, S_IRWXU);
		res = db_write(fd);
	} else if (mode == 3) {
		fd = open(path, O_RDWR);
		res = db_delete(fd);
		if (res != 0) {
			return 1;
		}
	}

	// Close file.	
	res = close(fd);
	
	if (res == -1) {
		fprintf(stderr, "Error occured during closing a file.\n");
		return 1;
	}

	return 0;
}

int db_read() {
	off_t start = lseek(fd, find_record(key), SEEK_SET);
	char line[LINE_LENGTH] = {'\0'};
	if (start == -1) {
		fprintf(stderr, "Item with key = %d was not found.\n", key);
		return 1;
	}
	ssize_t len = read(fd, (void*)line, LINE_SIZE);
	if (len < LINE_SIZE) {
		fprintf(stderr, "Error when reading line key = %d\n", key);
	}
	printf("%s\n", line);
	return 0;
}


int db_write() {
	/*
	Write new record into database.
	If there is empty line (record deleted before), write into that 
	free space instead of appending at the end of file.
	*/
	RECORD data;
	data.key = key;
	strcpy(data.info, info);
	data.value = value;
	//char line[LINE_LENGTH]; // prepare line which will be written into db
	//snprintf(line, LINE_LENGTH, format, data.key, data.info, data.value);
	//line[LINE_LENGTH-2] = '\n';
	ssize_t len;
	off_t empty_line = find_record(-1); // search for an empty line in db
	printf("%ld\n", empty_line);
	if (empty_line != -1) {
		// write new record into free space found
		lseek(fd, empty_line, SEEK_SET);
	}
	len = write(fd, &data, sizeof(RECORD));
	return len == sizeof(RECORD) ? 0 : 1;
}

int db_delete() {
	/*
	Delete row with a key provided by option argument.
	Deleting is implemented as setting key to -1.
	*/
	off_t offset = find_record(key);
	if (offset == -1) {
		fprintf(stderr, "Item with key = %d was not found.\n", key);
		return 1;
	}
	lseek(fd, offset, SEEK_SET);
	RECORD empty = {-1, {'\0'}, 0};
	char line[LINE_LENGTH];
	snprintf(line, LINE_LENGTH, format, empty.key, empty.info, empty.value);
	line[LINE_LENGTH-2] = '\n';
	ssize_t len = write(fd, line, LINE_SIZE);
	if (len != LINE_SIZE) {
		fprintf(stderr, "Error during writing empty record/\n");
		return 1;
	}
	return 0;
}

off_t find_record(int key) {
	/*
	Find a record by given key, return its offset in the file.
	*/
	char buffer[sizeof(RECORD)];
	off_t end = lseek(fd, 0, SEEK_END);
	off_t start = lseek(fd, 0, SEEK_SET);
	printf("S: %lf, E: %lf\n", start, end);
	ssize_t len;
	while (start != end) {
		// read file line by line
		buffer[0] = '\0';
		len = read(fd, (void*)buffer, R_SIZE);
		// if (len  < LINE_SIZE) {
		// 	fprintf(stderr, "Error while reading line\n");
		// 	return -1;
		// }
		sscanf(buffer, "%d %s %lf", &(data.key), data.info, &(data.value));
		printf("KEY: %d\n", data.key);
		printf("INFO: %s\n", data.info);
		printf("VAL: %lf\n", data.value);

		if (key == data.key) {
			return start;	
		}
		start += R_SIZE;
	}
	return -1;
}	

int parse_args(int argc, char** argv) {
	/*
	Check which command line options were provided.
	Assign their values into global variables.
	*/
	int option;
	char* end;

	while((option = getopt(argc, argv, "rwdk:i::v::")) != -1) {
		switch (option) {
			case 'r': // mode: read
				mode = isset_mode == 0 ? 1 : -1;
				isset_mode = 1;
				mode_name = "read";
				break;
			case 'w': // mode: write
				mode = isset_mode == 0 ? 2 : -1;
				isset_mode = 1;
				mode_name = "write";
				break;
			case 'd': // mode: delete
				mode = isset_mode == 0 ? 3 : -1;
				isset_mode = 1;
				mode_name = "delete";
				break;
			case 'k': // db column: key
				isset_k = 1;
				key = (int32_t)(strtol(optarg, &end, 10));
				break;
			case 'i': // db column: info
				isset_i = 1;
				strncpy(info, optarg, 16);
			   	break;
			case 'v': // db column: value
				isset_v = 1;
				value = strtod(optarg, NULL);
				break;
			default:
				fprintf(stderr, "Unvalid argument found\n");
				return 1;
		}
	}

	return 0;
}

int check_args() {
	/*
	Check if given options are available.
	*/
	int result = 0;
	
	if (mode == -1) {
		fprintf(stderr, "Mode error\n");
		result = 2;
	}	

	if (mode == 1 || mode == 3) {
		if (isset_k == 0) {
			fprintf(stderr, "You must provide key in %s mode.\n", mode_name);
			result = 3;
		}
		if (isset_v || isset_i) {
			fprintf(stderr, "Arguments other than key are forbidden in %s mode.\n", mode_name);
			result = 4;
		}
	}
	
	if (mode == 2) {
		if (isset_k == 0 || isset_i == 0) {
			fprintf(stderr, "You must provide key and info in %s mode.\n", mode_name);
			result = 5;
		}
	}

	return result;
}


