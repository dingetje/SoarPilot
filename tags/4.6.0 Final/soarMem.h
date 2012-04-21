#ifndef SOARMEM_H
#define SOARMEM_H

#define SMTH __attribute__ ((section ("smth")))

/*****************************************************************************
 * protos
 *****************************************************************************/
Boolean AllocMem(void	**vPtrP, UInt32 size) SMTH;
Boolean ResizeMem(void	**vPtrP, UInt32 size) SMTH;
void FreeMem(void	**vPtrP) SMTH;

#endif
