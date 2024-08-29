/* Check if TOD is returned correctly */

#include <uriscv/liburiscv.h>

#include "h/tconst.h"
#include "h/print.h"
#include "h/types.h"

void main() {
	print(WRITETERMINAL, "TOD Test starts\n");
	print(WRITETERMINAL, "TOD Test concluded\n");
	ssi_payload_t tod_payload = {
		.service_code = GET_TOD,
		.arg = 0,
	};
	unsigned int time1;
	SYSCALL(SENDMSG, PARENT, (unsigned int)&tod_payload, 0);
	SYSCALL(RECEIVEMSG, PARENT, (unsigned int)&time1, 0);
	unsigned int time2;
	SYSCALL(SENDMSG, PARENT, (unsigned int)&tod_payload, 0);
	SYSCALL(RECEIVEMSG, PARENT, (unsigned int)&time2, 0);
	if (time2 > time1) {
		print(WRITETERMINAL, "TOD Test Concluded Successfully\n");
	} else {
		print(WRITETERMINAL, "ERROR: TOD not correct\n");
	}
	/* Terminate normally */
	ssi_payload_t terminate_payload = {
		.service_code = TERMINATE,
		.arg = 0,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)&terminate_payload, 0);
	SYSCALL(RECEIVEMSG, 0, 0, 0);
}

