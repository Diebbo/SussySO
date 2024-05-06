#include "../headers/const.h"
#include "../headers/types.h"
#include <memory>
#include <uriscv/arch.h>
#include <uriscv/liburiscv.h>

unsigned next_asid = 1;
void initUprocPageTable(pcb_PTR);
int getASID(void);

void test3() {
  // tster function
}

int getASID(void) {
  unsigned asid = next_asid;
  if (asid > 7) {
    // errore la funzione non può essere chiamata più di 8 volte
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
        (KUSEG + i << VPNSHIFT) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
  }
}
