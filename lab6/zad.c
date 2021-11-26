#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

typedef unsigned char byte;
int fd;

struct __attribute__((packed)) rrecord {
	short id;
	byte len;
	char text[0];
};

typedef struct rrecord RECORD; 

int main(int argc, char** argv) {
	if (argc == 1) {
		fprintf(stderr, "No arguments provided\n");
		return 1;
	} else if (argc == 2) {
		fd = open(argv[1], O_RDONLY);
		off_t current = 0;
		off_t EOF = 
		
	} else {
		fd = open(argv[1], O_RDWR | O_APPEND | O_CREAT);
		int max_len = 0;
		int text_size;
		for (int i = 1; i < argc; i++) {
			text_size = sizeof(argv[i]);
			if (text_size > max_len) {
				max_len = text_size;
			}
		}

		RECORD* t = (RECORD*)(malloc(sizeof(sizeof(short) + sizeof(byte) + max_len)));
		
		//TODO: search for the highest id already in file
		for (int i = 2; i < argc; i++) {
			t->id = i;
			t->len = sizeof(argv[i]);
			strcpy(t->text,argv[i]);
			
			write(fd, t, sizeof(t));	
		}
	}
	return 0;
}
