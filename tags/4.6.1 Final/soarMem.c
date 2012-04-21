#include <PalmOS.h>
#include "soarMem.h"


/*****************************************************************************
 * AllocMem - use FreeMem when you're finished
 *****************************************************************************/
Boolean AllocMem(void	**memPtrP, UInt32	size)
{
	MemHandle	mHand;

	// nothing to work with
	if (!memPtrP)
		return false;

	// empty the ptr first
	*memPtrP = NULL;

	// nothing to do on no size
	if (size < 1)
		return false;

	if ((mHand = MemHandleNew(size)) == NULL ||
			(*memPtrP = MemHandleLock(mHand)) == NULL) {
		// alloc failed, release memory if it is there
		if (mHand)
			MemHandleFree(mHand);
		return false;
	}
	return true;
}



/*****************************************************************************
 * ResizeMem - use FreeMem when you're finished
 *****************************************************************************/
Boolean ResizeMem(void	**memPtrP, UInt32	size)
{
	MemHandle	mHand;
	Err			err;

	// nothing to work with
	if (! memPtrP || size < 1)
		return false;

	// recover the handle
	if ((mHand = MemPtrRecoverHandle(*memPtrP)) == NULL)
		return false;

	// unlock, resize, relock
	MemHandleUnlock(mHand);
	*memPtrP = NULL;
	if ((err = MemHandleResize(mHand, size)) != 0 ||
			(*memPtrP = MemHandleLock(mHand)) == NULL) {
		FreeMem((void *)&memPtrP);
		return false;
	}
	return true;
}



/*****************************************************************************
 * FreeMem - free memory from AllocMem
 *****************************************************************************/
void FreeMem(void	**memPtrP)
{
	if (*memPtrP)
		MemPtrFree(*memPtrP);
	*memPtrP = NULL;
}



