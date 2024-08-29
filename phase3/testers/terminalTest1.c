/* Does nothing but outputs to the terminal and terminates */

#include <uriscv/liburiscv.h>

#include "h/tconst.h"
#include "h/print.h"
#include "h/types.h"

void main() {
	print(WRITETERMINAL, "printTest is ok\n");
	print(WRITETERMINAL, "Test Number 1 is ok\n");
	/* Terminate normally */
	ssi_payload_t terminate_payload = {
		.service_code = TERMINATE,
		.arg = 0,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)&terminate_payload, 0);
	SYSCALL(RECEIVEMSG, 0, 0, 0);
}

