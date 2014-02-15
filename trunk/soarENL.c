#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarMem.h"
#include "soarENL.h"

SndStreamRef ENLstream = NULL;
UInt16 *ENLbuffer = NULL;

Boolean ENLCapableDevice()
{
	UInt32 version;
	Err err;
 	Boolean	retval = true;
 	
	err = FtrGet(sysFileCSoundMgr, sndFtrIDVersion, &version);

	if ( err ) {
		return(false);
		retval = false;
	}

	return(retval);
}

Err ENLcallback(void *userData, SndStreamRef ENLstream, void *ENLbuffer, UInt32 frameCount)
{
//	HostTraceOutputTL(appErrorClass, "ENLcallback");
	return(0);
}

Boolean initENL(Boolean reset)
{
//	Err streamOK = NULL;
	
	// no sound stream feature set
	if (!device.ENLCapable) return(false);

/*	if (reset) {
		HostTraceOutputTL(appErrorClass, "Close ENL Stream");
		// close sound stream
		if (ENLstream) {
			SndStreamStop(ENLstream);
			SndStreamDelete(ENLstream);
			FreeMem((void *)&ENLbuffer);
		}
	} else {
		HostTraceOutputTL(appErrorClass, "Open ENL Stream");
		// initialise sound stream
		streamOK = SndStreamCreate(&ENLstream,
				sndInput, 22050, sndInt16, sndMono,
				&ENLcallback, &ENLbuffer, ENLbuffersize,
				false);
		AllocMem((void *)&ENLbuffer, ENLbuffersize);
	}
	if (streamOK && ENLbuffer) {
		HostTraceOutputTL(appErrorClass, "ENL Stream OK");
		SndStreamStart(ENLstream);
	}
*/
	return(true);
}

Int16 getENL()
{
	// no sound stream feature set
	if (!device.ENLCapable || !ENLstream) return(0);

	// get sound level

	return (0);
}
