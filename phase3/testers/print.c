/* Function to call print parameterized output to a terminal or printer device */

#include <uriscv/liburiscv.h>

#include "h/tconst.h"
#include "h/types.h"

void print(int service_code, char *str) {
	int len;
	for (len = 0; str[len] != '\0'; len++);
	sst_print_t print_payload = {
		.length = len,
		.string = str,
	};
	ssi_payload_t sst_payload = {
		.service_code = service_code,
		.arg = &print_payload,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
	SYSCALL(RECEIVEMSG, 0, 0, 0);
}

