/**
* \file soarENL.c
* \brief SoarPilot Engine Noise Level [feature not implemented]
*
* The Engine Noise Level (ENL) system is where the noise level at
* the recorder is recorded with each GNSS fix in the form of three numbers
* from 000 to 999. This is the IGC-preferred system for Motor Gliders with
* engines that produce significant noise in the cockpit, because it does
* not require wiring external to the recorder or any other actions by the pilot,
* and is self-checking because an ENL value is recorded with each fix, 
* even during quiet flight.
*/

#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarMem.h"
#include "soarENL.h"
#include "soarMath.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarIO.h"

#define HERTZ	22050
// do not set BUFSIZE below 4096
#define BUFSIZE 4096
// sound samples are signed 16-bit numbers
#define STYPE   UInt8

SndStreamRef ENLstream = NULL;
STYPE *ENLbuffer = NULL;
UInt16 ENLLevel = 0;

/**
* \brief check if device is capable to record sounds
* \return true if device has sound recoding capability, else false
*/
Boolean ENLCapableDevice()
{
	UInt32 version;
	Err err;
	UInt32 companyID;
	UInt32 deviceID;

	// check for sound manager feature
	err = FtrGet(sysFileCSoundMgr, sndFtrIDVersion, &version);
	if (err == errNone) {
		return true;
	}

	// check for Tungsten T
	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &companyID);
	FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &deviceID);
	if ((companyID == 'Palm') && (deviceID == 'Frg1')) {
		return true;
	}
 	
	FrmCustomAlert(WarningAlert, "Device not ENL capable!"," "," ");
	return false;
}

Int16 bufpos=0;
Int16 buffers=0;
Int16 savbuf=0;

struct dataPass {
  Int16 *buffers;
  Int16 *bufpos;
  STYPE *sndbuf;
  Boolean busy;
  Boolean copy;
};

struct dataPass p;

/**
* \brief callback for Engine Noise Level sound stream
* \param userDataP Caller-defined data, as provided in the callbackArg parameter to SndStreamCreate().
* \param stream Token that represents the stream to which this buffer belongs.
* \param bufferP The data buffer.
* \param bufsize size of the buffer
* \return Currently, the return value is ignored.
*/
Err myCallback(void *userDataP, SndStreamRef stream, void *bufferP, UInt32 bufsize) {
	struct dataPass *pp = userDataP;
	if (!pp->busy)
	{
		STYPE *buf = (STYPE*) bufferP;
		pp->copy = true;
//		HostTraceOutputTL(appErrorClass, "ENLcallback bufsize=%lu, buffers=%hd, buffer pos=%hd", bufsize, *pp->buffers, *pp->bufpos);
//		HostTraceOutputB(appErrorClass, buf, 16);
		while(bufsize && !pp->busy) {
			// wrap around?
			if(*pp->bufpos == BUFSIZE) {
				*pp->bufpos = 0;
				(*pp->buffers)++; // buffer full for processing
			}	
			// copy samples
			pp->sndbuf[*pp->bufpos] = *buf;
			(*pp->bufpos)++;
			buf++;
			bufsize--;
		}
		pp->copy = false;
	}
	return errNone;
}

/**
* \brief initialize or reset Engine Noise Level sound stream
* \param reset set to true to perform shutdown of ENL stream else initialize
* \return true when initialization successful, else false
*/
Boolean initENL(Boolean reset)
{
	Err streamOK = NULL;
	Boolean retval = false;

	p.busy = true;
	
	// no sound stream feature set
	if (!device.ENLCapable) {
//		HostTraceOutputTL(appErrorClass, "Device not ENL capable");
		FrmCustomAlert(WarningAlert, "Device not ENL capable!"," "," ");
		return(retval);
	}

	if (reset) {
//		HostTraceOutputTL(appErrorClass, "Close ENL Stream");
		// close sound stream
		if (ENLstream) {
			SndStreamStop(ENLstream);
			SndStreamDelete(ENLstream);
			FreeMem((void *)&ENLbuffer);
		}
		retval = true;
	} else {
		buffers= 0;
		bufpos = 0;
		savbuf = 0;
//		HostTraceOutputTL(appErrorClass, "Open ENL Stream");
		AllocMem((void *)&ENLbuffer, BUFSIZE);
		if (ENLbuffer) {
			// init pass through structure for sound callback routine
			p.buffers = &buffers;
			p.bufpos  = &bufpos;
			p.sndbuf  = ENLbuffer;
			p.copy	  = false;

//			HostTraceOutputTL(appErrorClass, "ENL Stream buffer created");
			// initialise sound stream
			streamOK = SndStreamCreate(
					&ENLstream,	// sound stream handle
					sndInput, 	// recording
					HERTZ, 		// sample rate
					sndUInt8,	// unsigned 8-bit integer data
					sndMono,	// mono sound
					&myCallback,	// A callback function that gets called when another buffer is needed. 
					&p,		// Caller-defined data that gets passed to callback.
					BUFSIZE,	// buffer size
					false);		// callback function is written in 68k code.
//			HostTraceOutputTL(appErrorClass, "ENL Stream created = %d", streamOK);
			if (streamOK == errNone) {
				p.busy = false;
				// stream created, now start it
				streamOK = SndStreamStart(ENLstream);
//				HostTraceOutputTL(appErrorClass, "ENL Stream started = %d", streamOK);
				retval = (streamOK == errNone);
				if (!retval) {
					FrmCustomAlert(WarningAlert, "ENL stream NOT started!"," "," ");
				}
			}
			else {
				char tempchar[60];
				StrCopy(tempchar,"ENL stream NOT created, error = ");
				StrCat(tempchar,DblToStr(streamOK,3));
				FrmCustomAlert(WarningAlert,tempchar," "," ");
			}
		} else {
			FrmCustomAlert(WarningAlert, "ENL buffer NOT allocated!"," "," ");
		}
	}
	return(retval);
}

/**
* \brief get Engine Noise Level value
* \return Engine Noise Level value
* \todo real life test of the algorithm
*/
Int16 getENL()
{
//	HostTraceOutputTL(appErrorClass, "getENL()");
	// no sound stream feature set?
	if (!device.ENLCapable || !ENLstream) {
//		static Boolean bOneTime = true;
//		if (bOneTime) {
//			FrmCustomAlert(WarningAlert, "getENL always 0!"," "," ");
//			bOneTime = false;
//		}
		return(0);
	}
	// buffer full (see sound callback routine)?
	// usually more sound buffers than GPS logger points
	// so we should have sufficient data
	if(savbuf != buffers && !p.copy) {
		Int32 k;
		STYPE *pBuffer = ENLbuffer;
		double total = 0.0;
		double mean = 0.0;
		double volume = 0.0;
		
		// signal sound callback to skip data copy
		p.busy = true;
		savbuf = buffers;
#ifdef ENLLOG
		if (device.VFSCapable) {
			XferInit("enl.log", IOOPENTRUNC, USEVFS);
		}
#endif
		// process buffer
		for(k=0; k<BUFSIZE; k++) {
			STYPE sample = *pBuffer;
			total += Fabs(sample - 128);
#ifdef ENLLOG
			if (device.VFSCapable) {
				outputlog(DblToStr(sample,3), true);
			}
#endif		
			pBuffer++;
		}
		mean = Fabs(total) / k;
		volume = (mean / 0.64);
//		HostTraceOutputTL(appErrorClass, "ENL mean = %s", DblToStr(mean, 3));
//		HostTraceOutputTL(appErrorClass, "ENL volume = %s", DblToStr(volume, 3));
#ifdef ENLLOG
		if (device.VFSCapable) {
			outputlog(DblToStr(mean,3), true);
			outputlog(DblToStr(volume,3), true);
			XferClose(USEVFS);
		}
#endif
		ENLLevel = (UInt32) volume;
		// ENL is between 000 and 999
		if (ENLLevel > 999) {
			ENLLevel = 999;
		}
//		HostTraceOutputTL(appErrorClass, "ENL = %lu",  ENLLevel);
		p.busy = false;
	}
	// get sound level
	return (ENLLevel);
}
