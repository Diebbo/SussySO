/*	Test of a CPU intensive recusive job */

#include <uriscv/liburiscv.h>

#include "h/tconst.h"
#include "h/print.h"
#include "h/types.h"

int fib (int i) {
	if ((i == 1) || (i ==2)) {
		return (1);
	}
	return(fib(i-1)+fib(i-2));
}


void main() {
	int i;
	print(WRITETERMINAL, "Recursive Fibonacci (8) Test starts\n");
	i = fib(8);
	print(WRITETERMINAL, "Recursion Concluded\n");
	if (i == 21) {
		print(WRITETERMINAL, "Recursion Concluded Successfully\n");
	} else {
		print(WRITETERMINAL, "ERROR: Recursion problems\n");
	}
	/* Terminate normally */
	ssi_payload_t terminate_payload = {
		.service_code = TERMINATE,
		.arg = 0,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)&terminate_payload, 0);
	SYSCALL(RECEIVEMSG, 0, 0, 0);
}

