/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_ARMCI_INTERNALS_H
#define HAVE_ARMCI_INTERNALS_H

#include <armci.h>

extern ARMCI_Group ARMCI_GROUP_WORLD;
extern ARMCI_Group ARMCI_GROUP_DEFAULT;


void ARMCII_Error(const char *file, const int line, const char *func, const char *msg, int code);

int  ARMCII_Translate_absolute_to_group(MPI_Comm group_comm, int world_rank);

void ARMCII_Strided_to_iov(armci_giov_t *iov,
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels);


/* Shared to private buffer management routines */
void ARMCII_Buf_put_prepare(void **orig_bufs, void ***new_bufs_ptr, int count, int size);
void ARMCII_Buf_put_finish(void **orig_bufs, void **new_bufs, int count, int size);
void ARMCII_Buf_acc_prepare(void **orig_bufs, void ***new_bufs_ptr, int count, int size,
                            int datatype, void *scale);
void ARMCII_Buf_acc_finish(void **orig_bufs, void **new_bufs, int count, int size);
void ARMCII_Buf_get_prepare(void **orig_bufs, void ***new_bufs_ptr, int count, int size);
void ARMCII_Buf_get_finish(void **orig_bufs, void **new_bufs, int count, int size);

#endif /* HAVE_ARMCI_INTERNALS_H */
