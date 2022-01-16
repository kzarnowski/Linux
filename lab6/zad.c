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
		printf("FD: %d\n", fd);
		off_t current = 0;
		off_t end = lseek(fd, 0, SEEK_END);
		printf("end: %ld\n", end);
		char temp[3];
		short id;
		byte len;
		//short* id_temp = (short*)(malloc(sizeof(short)));
		//byte* len_temp = (byte*)(malloc(sizeof(byte)));
		while (current != end) {
			int readres = read(fd, temp, 3);
			printf("SRAKA%d\n", readres);
			if (readres) printf("SRAKA%s\n", temp);
			sscanf(temp, "%hn %hhn", &id, &len);
			printf("id: %hn; len: %hhn\n", id, len);
			//char* text = (char*)(malloc(sizeof(len_temp)));
			//read(fd, (void*)text, *len_temp);
			//printf("%s\n", text);
			//free(text);

			current += sizeof(short) + sizeof(byte) + len;
			printf("current:%ld\n", current);

			if (current > 2*end) break;
		}

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

		RECORD* t = (RECORD*)(malloc(sizeof(short) + sizeof(byte) + max_len));
		
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
