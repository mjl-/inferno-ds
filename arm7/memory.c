// Changes only the gba rom bus ownership
static void sysSetCartOwner(int arm9) {
  REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_ROM) | (arm9 ? 0 :  ARM7_OWNS_ROM);
}
// Changes only the nds card bus ownership
static  void sysSetCardOwner(int arm9) {
  REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_CARD) | (arm9 ? 0 : ARM7_OWNS_CARD);
}

// Changes all bus ownerships
static  void sysSetBusOwners(int arm9rom, int arm9card) {
  uint16 pattern = REG_EXMEMCNT & ~(ARM7_OWNS_CARD|ARM7_OWNS_ROM);
  pattern = pattern | (arm9card ?  0: ARM7_OWNS_CARD ) |
                      (arm9rom ?  0: ARM7_OWNS_ROM );
  REG_EXMEMCNT = pattern;
}
