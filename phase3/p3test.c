#include "../headers/const.h"
#include "../headers/types.h"
#include <uriscv/arch.h>
#include <uriscv/liburiscv.h>

void initUprocPageTable(pcb_PTR);



void test3() {
  // tster function
}

void initUprocPageTable(pcb_PTR p){
  // fill the support page table with the correct values
  // assuming the asid is already set

  for (int i = 0; i < MAXPAGES; i++) {
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryHI = (KUSEG + i << VPNSHIFT) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
  }

}
