#include <ocr-types.h>

#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct _func_entry {
    u64 offset;
    char *func_name;
} func_entry;

#define MAXFNS (2048)
#define BASE_ADDRESS (0x0)
#define LIBOCR_SO "./libocr.so"

// The below holds all functions from the .so
func_entry blob_func_lists[MAXFNS];
int blob_func_list_size = 0;

// The below holds all functions in the app
func_entry app_func_lists[MAXFNS];
int app_func_list_size = 0;

static inline u64 mark_addr_magic(u64 addr, void* base)
{
  return (addr-(u64)base) + BASE_ADDRESS;
}

/* Below code adapted from libelf tutorial example */
int extract_funcs (const char *str, func_entry *func_list)
{
  Elf *e;
  Elf_Kind ek;
  Elf_Scn *scn;
  Elf_Data *edata;
  int fd, i, symbol_count;
  GElf_Sym sym;
  GElf_Shdr shdr;
  int func_count = 0;

  if(elf_version(EV_CURRENT) == EV_NONE) {
    printf("Error initializing ELF: %s\n", elf_errmsg(-1));
    return -1;
  }

  if ((fd = open(str, O_RDONLY, 0)) < 0) {
    printf("Unable to open %s\n", str);
    return -1;
  }

  if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
    printf("elf_begin failed: %s\n", elf_errmsg(-1));
  }

  ek = elf_kind(e);
  if(ek != ELF_K_ELF) {
    printf("not an ELF object");
  } else {
    scn = NULL;
    edata = NULL;
    while((scn = elf_nextscn(e, scn)) != NULL)
      {   
	gelf_getshdr(scn, &shdr);
	if(shdr.sh_type == SHT_SYMTAB)
	  {   
	    edata = elf_getdata(scn, edata);
	    symbol_count = shdr.sh_size / shdr.sh_entsize;
	    for(i = 0; i < symbol_count; i++)
	      {   
		gelf_getsym(edata, i, &sym);
		if(sym.st_size == 0) continue;
		if(ELF32_ST_TYPE(sym.st_info) != STT_FUNC) continue;

		func_list[func_count].offset = sym.st_value;
		func_list[func_count++].func_name = strdup(elf_strptr(e, shdr.sh_link, sym.st_name));

		if(func_count >= MAXFNS) {
		  printf("Func limit (%d) reached, please increase MAXFNS & rebuild\n", MAXFNS);
		  raise(SIGABRT);
		}
		//                    printf("%08x %08x\t%s\n", sym.st_value, sym.st_size, elf_strptr(e, shdr.sh_link, sym.st_name));
	      }
	  }
      }
  }

  elf_end(e);
  close(fd);
  return func_count;
}

void free_func_names(func_entry *func_lists, int func_list_size)
{
  int i;
  for(i = 0; i < func_list_size; i++)
    free(func_lists[i].func_name);
}

char* find_function(u64 address, func_entry *func_lists, int func_list_size)
{
  int i;

  for(i = 0; i<func_list_size; i++)
    if(func_lists[i].offset == address)
      return func_lists[i].func_name;

  return NULL;
}

u64 find_function_address (char *fname, func_entry *func_lists, int func_list_size)
{
  int i;

  for(i = 0; i<func_list_size; i++)
    if(!strcmp(func_lists[i].func_name, fname))
      return func_lists[i].offset;

  printf("Warning: %s not found in app, continuing...\n", fname);
  return 0;
}

void fix_funcPtrs (void *to, void *from, func_entry *bloblist, int blobcount, func_entry *applist, int appcount)
{
  u64 *src = (u64 *)from;
  u64 *dst = (u64 *)to;
  char *funcname = NULL;
  u64 address = 0;

  funcname = find_function(*src, bloblist, blobcount);
  if(funcname) {
    address = find_function_address(funcname, applist, appcount);
    if(address) {
      *dst = address;
      //printf("%x        %s\n", address, funcname);
    } else {
      printf("Function %s needed by OCR not built into the app!\n", funcname);
      *dst = 0xdeadc0de;
    }
  }
}

void dump_fctPtrs (char *mem, void *fctPtrs, int count, char *base)
{
  int i;
//  char *funcname = NULL;
  u64 *src = fctPtrs;
  u64 *dst = (u64 *)mem;

  *dst = mark_addr_magic((u64)(mem+sizeof(void *)), base);

  dst++;
  for(i = 0; i < count; i++)
    fix_funcPtrs (&dst[i], src++, blob_func_lists, blob_func_list_size, app_func_lists, app_func_list_size);
}

void elf_main(int argc, char *argv[])
{
  blob_func_list_size = extract_funcs(LIBOCR_SO, blob_func_lists);
  app_func_list_size = extract_funcs(argv[2], app_func_lists);

  //ptr = mybringUpRuntime(argv[1]);
  //dumpAllStructs(argv[3], ptr);
  //myfreeUpRuntime();

  free_func_names(blob_func_lists, blob_func_list_size);
  free_func_names(app_func_lists, app_func_list_size);
}
