/* 
Coded by Mulvun
Use educationally/legally.;)
*/
#ifndef SAKITO_TOOLS_H

#define SAKITO_TOOLS_H

#define BUFLEN 8192

// Default allocation for conns.clients (to repeat repititive calls to realloc/reduce computations).
#define MEM_CHUNK 5

 // Function to read/store stdin until \n is detected.
size_t get_line(char* const buf) {
	char c;
	size_t cmd_len = 0;

	buf[cmd_len++] = '0';
	c = getchar();

	while (c != '\n' && cmd_len < BUFLEN) {
		buf[cmd_len++] = c;
		c = getchar();
	}

	return cmd_len;
}

// Function to compare two strings (combined logic of strcmp and strncmp).
int compare(const char* buf, const char* str) {
	for (int j = 0; str[j] != '\0'; j++) {
		if (str[j] != buf[j])
			return 0;
	}

	return 1;
}

// Function to copy int bytes to new memory block/location to abide strict aliasing.
static inline uint32_t ntohl_conv(char* const buf) {
	uint32_t new;
	memcpy(&new, buf, sizeof(new));

	// Return deserialized bytes.
	return ntohl(new);
}

#endif