//******************************************************************************
// File:
//
//   amhandler.c
//
// Purpose:
//
//   maintain a registry of active message handlers
//
// Modification History
//
//  2009/06/06 - created John Mellor-Crummey
//  2014/04/18 - adapted for OCR Laksono Adhianto
//
//******************************************************************************

#include <ocr-config.h>
#ifdef ENABLE_COMM_PLATFORM_GASNET

//******************************************************************************
// system include files
//******************************************************************************

#include <assert.h>
#include <stdlib.h>



//******************************************************************************
// local include files
//******************************************************************************

#include "amhandler.h"



//******************************************************************************
// local type declarations
//******************************************************************************

typedef struct amhandler_registry_list_entry_s {
  gasnet_handlerentry_t entry;
  struct amhandler_registry_list_entry_s *next;
} amhandler_list_entry_t;



//******************************************************************************
// macros
//******************************************************************************




//******************************************************************************
// local data
//******************************************************************************

static gasnet_handlerentry_t *_amhandler_table = 0;
static int _amhandler_count = 0;
static gasnet_handler_t amhandler_index = AMHANDLER_INDEX_BASE;
static amhandler_list_entry_t *amhandler_registry_list = 0;



//******************************************************************************
// private operations
//******************************************************************************


static amhandler_list_entry_t *
new_amhandler_list_entry(amhandler_fn_t handler,
                         amhandler_list_entry_t *next)
{
  amhandler_list_entry_t *le =
    (amhandler_list_entry_t *) malloc(sizeof(amhandler_list_entry_t));
  gasnet_handler_t this_handler_index;

  assert(le);

  this_handler_index = amhandler_index++;
  _amhandler_count++;

  le->entry.index = this_handler_index;
  le->entry.fnptr = handler;

  le->next = next;

  return le;
}


static void
amhandler_table_fill(gasnet_handlerentry_t table[],
                         amhandler_list_entry_t *head)
{
  int slot = 0;
  amhandler_list_entry_t *le = head;
  while (le) {
    table[slot++] = le->entry;
    le = le->next;
  }
}


static void
amhandler_table_create()
{
  int table_bytes = _amhandler_count * sizeof(gasnet_handlerentry_t);
  _amhandler_table = (gasnet_handlerentry_t *) malloc(table_bytes);
  amhandler_table_fill(_amhandler_table, amhandler_registry_list);
}



//******************************************************************************
// interface operations
//******************************************************************************

gasnet_handlerentry_t *
amhandler_table()
{
  if (!_amhandler_table) {
    amhandler_table_create();
  }
  return _amhandler_table;
}


int
amhandler_count()
{
  return _amhandler_count;
}


// register a an active message handler with the CAF runtime system
// return a unique index that will be used to identify the handler
gasnet_handler_t
amhandler_register(amhandler_fn_t handler)
{
  assert(_amhandler_table == 0);
  amhandler_registry_list =
    new_amhandler_list_entry(handler, amhandler_registry_list);

  return amhandler_registry_list->entry.index;
}



#endif // ENABLE_COMM_PLATFORM_GASNET
