/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <gmr.h>
#include <debug.h>

/** One-sided fetch-and-op.  Source and output buffer must be private.
  *
  * @param[in] mreg      Memory region
  * @param[in] src       Address of source data
  * @param[in] out       Address of output buffer (same process as the source)
  * @param[in] dst       Address of destination buffer
  * @param[in] type      MPI datatype of the source, output and destination elements
  * @param[in] op        MPI_Op to apply at the destination
  * @param[in] proc      Absolute process id of target process
  * @return              0 on success, non-zero on failure
  */
int gmr_fetch_and_op(gmr_t *mreg, void *src, void *out, void *dst,
		MPI_Datatype type, MPI_Op op, int proc) {

  int        grp_proc;
  gmr_size_t disp;

  grp_proc = ARMCII_Translate_absolute_to_group(&mreg->group, proc);
  ARMCII_Assert(grp_proc >= 0);
  ARMCII_Assert_msg(mreg->window != MPI_WIN_NULL, "A non-null mreg contains a null window.");

  /* built-in types only so no chance of seeing MPI_BOTTOM */
  disp = (gmr_size_t) ((uint8_t*)dst - (uint8_t*)mreg->slices[proc].base);

  // Perform checks
  ARMCII_Assert_msg(disp >= 0 && disp < mreg->slices[proc].size, "Invalid remote address");
  ARMCII_Assert_msg(disp <= mreg->slices[proc].size, "Transfer is out of range");

  MPI_Fetch_and_op(src, out, type, grp_proc, (MPI_Aint) disp, op, mreg->window);

  return 0;
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Rmw = PARMCI_Rmw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Rmw ARMCI_Rmw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Rmw as PARMCI_Rmw
#endif
/* -- end weak symbols block -- */

/** Perform atomic read-modify-write on the given integer or long location and
  * return the location's original value.
  *
  * \note ARMCI RMW operations are atomic with respect to other RMW operations,
  * but not with respect to other one-sided operations (get, put, acc, etc).
  *
  * @param[in]  op    Operation to be performed:
  *                     ARMCI_FETCH_AND_ADD (int)
  *                     ARMCI_FETCH_AND_ADD_LONG
  *                     ARMCI_SWAP (int)
  *                     ARMCI_SWAP_LONG
  * @param[out] ploc  Location to store the original value.
  * @param[in]  prem  Location on which to perform atomic operation.
  * @param[in]  value Value to add to remote location (ignored for swap).
  * @param[in]  proc  Process rank for the target buffer.
  */
int PARMCI_Rmw(int op, void *ploc, void *prem, int value, int proc) {
  int           is_long;
  gmr_t *mreg;

  mreg = gmr_lookup(prem, proc);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  if (op == ARMCI_SWAP_LONG || op == ARMCI_FETCH_AND_ADD_LONG)
    is_long = 1;
  else
    is_long = 0;

  if (op == ARMCI_SWAP || op == ARMCI_SWAP_LONG) {
    long swap_val_l;
    int  swap_val_i;

    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    PARMCI_Get(prem, is_long ? (void*) &swap_val_l : (void*) &swap_val_i, 
              is_long ? sizeof(long) : sizeof(int), proc);
    PARMCI_Put(ploc, prem, is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);

    if (is_long)
      *(long*) ploc = swap_val_l;
    else
      *(int*) ploc = swap_val_i;
  }

  else if (op == ARMCI_FETCH_AND_ADD || op == ARMCI_FETCH_AND_ADD_LONG) {
    long fetch_val_l, new_val_l;
    int  fetch_val_i, new_val_i;
    
    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    PARMCI_Get(prem, is_long ? (void*) &fetch_val_l : (void*) &fetch_val_i,
              is_long ? sizeof(long) : sizeof(int), proc);
    
    if (is_long)
      new_val_l = fetch_val_l + value;
    else
      new_val_i = fetch_val_i + value;

    PARMCI_Put(is_long ? (void*) &new_val_l : (void*) &new_val_i, prem, 
              is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);

    if (is_long)
      *(long*) ploc = fetch_val_l;
    else
      *(int*) ploc = fetch_val_i;
  }

  else {
    ARMCII_Error("invalid operation (%d)", op);
  }

  return 0;
}
