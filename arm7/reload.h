typedef void __attribute__ ((long_call)) (*COPYFUNC)(const char*);
typedef u32  __attribute__ ((long_call)) (*QUERYFUNC)(u32);
 
typedef struct {
  u32         VERSION;    contains version information about loader
  QUERYFUNC   QUERY;      used to retreive extended information from loader
  COPYFUNC    ARM7FUNC;   a pointer to the ARM7 load function
  COPYFUNC    ARM9FUNC;   a pointer to the ARM9 load function
  const char *PATH;       THIS VALUE IS SET FOR YOU, DONT TOUCH IT
  u32         RESERVED;   reserved for future expansion
} LOADER_DATA;
 
#define LOADNDS ((volatile LOADER_DATA*)(0x02800000-48))
#define BOOT_NDS ((const char*)-1)
 
 
 
/*
dump this near the begining of your ARM7's main function
 
LOADNDS->PATH = 0;
 
 
inside of your ARM7's main loop or VBlank IRQ or what-ever, add this code:
 
if (LOADNDS->PATH != 0) {
  LOADNDS->ARM7FUNC(LOADNDS->PATH);
}
 
 

 
in your ARM9 code, to return to the menu, call this function:
 
WAIT_CR &= ~0x8080;
LOADNDS->ARM9FUNC(BOOT_NDS);
 
*/
 
 
