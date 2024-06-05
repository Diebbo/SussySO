#ifndef STD_LIB_H

// general headers
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/msg.h"
#include "../../phase1/headers/pcb.h"

// uriscv headers
#include <uriscv/aout.h>
#include <uriscv/arch.h>
#include <uriscv/bios.h>
#include <uriscv/const.h>
#include <uriscv/cpu.h>
#include <uriscv/csr.h> //ucode.h <--- da errore 'impossibile aprire il codice errore "uriscv/ucode.h" del file origine',
#include <uriscv/liburiscv.h>
#include <uriscv/regdef.h>
#include <uriscv/types.h>

// internal global variables
extern pcb_PTR swap_mutex;
extern swap_t swap_pool[POOLSIZE];

// internal phase2 global variables & functions
extern pcb_PTR ssi_pcb;

// support level next free asid
extern int next_asid;

int getASID(void) {
  unsigned asid = next_asid;
  if (asid > 8) {
    /*function cannot be called more than 8 time*/
    next_asid = 1;
    return NOPROC;
  }
  next_asid++;
  return asid;
}

void initUprocPageTable(pcb_PTR p) {
  // fill the support page table with the correct values
  // nucleus proces has asid 0
  if (p->p_supportStruct->sup_asid == NOPROC)
    p->p_supportStruct->sup_asid = getASID();

  for (int i = 0; i < MAXPAGES; i++) {
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryHI =
        ((KUSEG + i) << VPNSHIFT) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
  }
  p->p_supportStruct->sup_privatePgTbl[31].pte_entryHI =
      (0xbffff << VPNSHIFT) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
}

support_t *getSupportData() {
  support_t *support_data;
  ssi_payload_t getsup_payload = {
      .service_code = GETSUPPORTPTR,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&getsup_payload), 0);
  return support_data;
}

#endif // !STD_LIB_H
