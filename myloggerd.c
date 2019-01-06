/* myloggerd.c
 * Source file for thread-lab
 * Creates a server to log messages sent from various connection_array
 * in real time.
 *
 * Student: Murtaza Meerza
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <pthread.h>
#include "message-lib.h"

#define BUFF_SIZE 1024

// forward declarations
int usage(char name[]);
void * recv_log_msgs(void * arg); // a function to be executed by each thread
int log_fd; // log file descriptor 
char buffer[BUFF_SIZE]; // buffer to store messages


void * recv_log_msgs(void * arg) {
	
	int connection = ((int *)arg);
	int bytesread = read_msg(connection, buffer, BUFF_SIZE);

	while (bytesread > 0) {
		write(log_fd, &buffer, bytesread);
		bytesread = read_msg((connection), buffer, BUFF_SIZE);
		if (bytesread == -1) {
			perror("Read error");
		}
	}
	close_connection(connection);
	return NULL;
}

int usage(char name[]) {
	printf("Usage:\n");
	printf("\t%s <log-file-name> <UDS path>\n", name);
	return 1;
}

int main(int argc, char * argv[])
{
	int listener;
	int connection_array[8];
	int thread_count = 0;

	if (argc != 3)
		return usage(argv[0]);

	// open the log file for appending
	log_fd = open(argv[1], O_RDWR | O_APPEND | O_CREAT, 0666);
	if (log_fd == -1) {
		perror("Could not open the requested file");
		close(log_fd);
		return -1;
	}

	// build the connection array
	listener = permit_connections(argv[2]);
	if (listener == -1) {
		perror("Failed to build array connections ");
		close(log_fd);
		return -1;
	}

	pthread_t msgthr;

	while (connection_array[thread_count] >= 0) {
		connection_array[thread_count] = accept_next_connection(listener);
		if (connection_array[thread_count] > 0) {
			pthread_create(&msgthr, NULL, recv_log_msgs, (void *)connection_array[thread_count]);
			thread_count = (thread_count + 1) % 8;
		}

	}
	// close listener
	close_listener(listener);
	// close file descriptor
	close(log_fd);
	return 0;
}