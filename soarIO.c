#include <PalmOS.h>	// all the system toolbox headers
#include <SerialMgr.h>
#include <SerialMgrOld.h>
#include <MemoryMgr.h>
#include <SystemMgr.h>
#include <VFSMgr.h>
#include <DLServer.h>
#include <SmsLib.h>
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarMem.h"
#include "soarGar.h"
#include "soarMath.h"
#include "soarUMap.h"
#include "soarBT.h" // AGM: for prefered BT GPS connection feature

#ifdef TREOSUPPORT
	#include <HsExt.h>
	#include <HsPhone.h>
#endif

#define myCustomSerQueueSize 4*1024
#define VFSBufferSize 16383

UInt16 serial_ref = 0;
UInt16 serial_alt = 0;
UInt16 smslib_ref = sysInvalidRefNum;
UInt32 SerportID35 = 0x8000;
UInt32 SerportID40 = 0x8003;
UInt32 USBportID = 0x8004;
UInt32 IRportID = sysFileCVirtIrComm;
UInt32 BTportID = sysFileCVirtRfComm;
UInt32 CFportID = 'u550'; //Handera Compact Flash Serial Port
UInt32 EXportID = 0xFFFF; // connection "ExtGPS" port
FileRef vfsfileRef;

Err fileerror;
Char *customSerQP;
Char *buffer;
Char *customSerQP2;
Char *buffer2;
Char *vfsbuffer;
Char *docbuffer;
UInt16 buffermax;
Int32 waitTime = 0;
Int32 rxrecs = 0;
Char rxtype[15] = "Records";
Int16 currentFilePage = 0;
Int16 selectedFileIndex = -1;
Int16 Filenumperpage = 7;
Char transfer_filename[30];
Int8 io_file_type = NO_FILE_TYPE;
Int8 io_type = IO_NONE;
Int8 xfrdialog = XFRXFR;
Char *filelist = NULL;
Int16 numfilesfound = 0;

extern Boolean menuopen;
extern Boolean PrgCancel;

void ClearSerial()
{
//	HostTraceOutputTL(appErrorClass, "Clearing Serial Port");
	if (device.NewSerialPresent || device.CliePDA) {
		SrmReceiveFlush(serial_ref, 0);
		SrmSendFlush(serial_ref);
		SrmClearErr(serial_ref);
	} else {
		SerReceiveFlush(serial_ref, 0);
		SerSendFlush(serial_ref);
		SerClearErr(serial_ref);
	}
}

Boolean RxData(Int8 xfertype)
{
	if (xfertype == USEVFS) {
//		HostTraceOutputTL(appErrorClass, "RxDataVFS");
		return(RxDataVFS());
	} else if (xfertype == USEMEMO) {
//		HostTraceOutputTL(appErrorClass, "RxDataMemo");
		return(RxDataMemo());
	} else if (xfertype == USEDOC) {
//		HostTraceOutputTL(appErrorClass, "RxDataDOC");
		return(RxDataDOC());
	} else if (device.NewSerialPresent || device.CliePDA || xfertype == USECF) {
//		HostTraceOutputTL(appErrorClass, "RxDataNew");
		if (serial_ref) {
			return(RxDataNew());
		} else {
			return(false);
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "RxDataOld");
		if (serial_ref) {
			return(RxDataOld());
		} else {
			return(false);
		}
	}
}

Boolean RxDataOld(void)
{
	UInt32 numBytes = 0;			
	Err err;
 
	// Set waitTime to 1/5 of a second
	waitTime = SysTicksPerSecond() / 5;
	err = SerReceiveCheck(serial_ref, &numBytes);

	if (err) {
		SerClearErr(serial_ref);
		// clean out errors and check if there is still data left
		SerReceiveFlush(serial_ref, 0);
		return(false);
	}

//	HostTraceOutputTL(appErrorClass, "serial:numBytes |%lu|", numBytes);

	if (numBytes <= 0) {
		return(false);
	}

	if (numBytes > buffermax) {
		numBytes = buffermax;
	}

	numBytes=SerReceive(serial_ref, &buffer[0], numBytes, waitTime, &err);
	if (err) {
		SerClearErr(serial_ref);					
		SerReceiveFlush(serial_ref, 0);
		return(false);
	}

	if ((rxrecs % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, rxrecs, -1, rxtype);

//	HostTraceOutputTL(appErrorClass, "serial:numBytes2 |%lu|", numBytes);
	buffer[numBytes] = '\0';
//	HostTraceOutputTL(appErrorClass, "serial:buffer |%s|", buffer);
	// Using this for retransmission
	// Too slow for PalmOS 4.X and below
//	TxDataOld(buffer);

#ifdef NMEALOG
	if (device.VFSCapable) HandleVFSData(IOWRITE, buffer, &numBytes);
#endif

	data.parser.parser_func(&buffer[0], numBytes, false);

	return(true);
}

Boolean RxDataNew(void)
{
	Err err;
	UInt32 numBytesPending=0;
	UInt32 nb, numBytes = 0;

	// Set waitTime to 1/5 of a second
	waitTime = SysTicksPerSecond() / 5;
//	HostTraceOutputTL(appErrorClass, "RxDataNew: Inside RxDataNew");
	err = SrmReceiveCheck(serial_ref, &numBytesPending);
//	HostTraceOutputTL(appErrorClass, "RxDataNew: numBytesPending: |%lu|", numBytesPending);
	if (err == serErrLineErr) {
//		HostTraceOutputTL(appErrorClass, "RxDataNew: ShowSerialError1");
//		ShowSerialError();
		SrmClearErr(serial_ref);
		SrmReceiveFlush(serial_ref, 0);
		return(false);
	}

	if (numBytesPending <= 0) {
//		HostTraceOutputTL(appErrorClass, "RxDataNew: numBytesPending <= 0");
		return(false);
	}

	nb = (numBytesPending > buffermax) ? buffermax : numBytesPending;
//	HostTraceOutputTL(appErrorClass, "RxDataNew: nb: |%lu|", nb);
	if (nb > 0) {
		numBytes = SrmReceive(serial_ref, buffer, nb, waitTime, &err);
		if(numBytes > 0) {
			if ((rxrecs % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, rxrecs, -1, rxtype);

//			HostTraceOutputTL(appErrorClass, "RxDataNew:buffer before|%s|", buffer);
			buffer[numBytes] = '\0';
//			HostTraceOutputTL(appErrorClass, "RxDataNew:buffer after|%s|", buffer);
			// Using this for retransmission
			// Too slow for PalmOS 4.X and below
//			TxDataNew(buffer);
#ifdef NMEALOG
			if (device.VFSCapable) HandleVFSData(IOWRITE, buffer, &numBytes);
#endif
			data.parser.parser_func(&buffer[0], numBytes, false);
			nb -= numBytes;
		}
		if (err == serErrLineErr) {
//			HostTraceOutputTL(appErrorClass, "RxDataNew: ShowSerialError2");
//			ShowSerialError();
			SrmClearErr(serial_ref);
			SrmReceiveFlush(serial_ref, 0);	// will clear the error
			return(false);
		}
	}
	return(true);
}

Boolean RxDataMemo(void)
{
	UInt32 numBytesRead = buffermax;

	while(HandleMemoData(IOREAD, buffer, &numBytesRead)) {
		if ((rxrecs % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, rxrecs, -1, rxtype);
//		buffer[numBytesRead] = '\0';
//		HostTraceOutputTL(appErrorClass, "RxDataMemo: About to parse data");
		data.parser.parser_func(buffer, numBytesRead, false);
		MemSet(buffer, buffermax+1, 0);
	}
	return(false);
}

Boolean RxDataDOC(void)
{
	UInt32 numBytesRead = buffermax;

	while(HandleDOCData(IOREAD, docbuffer, &numBytesRead)) {
		if ((rxrecs % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, rxrecs, -1, rxtype);
//		HostTraceOutputTL(appErrorClass, "RxDataMemo: About to parse data");
//		HostTraceOutputTL(appErrorClass, "numbytesread %s", DblToStr(numBytesRead,0));
		data.parser.parser_func(docbuffer, numBytesRead, false);
		MemSet(docbuffer, buffermax+1, 0);
		numBytesRead = buffermax;
	}
	return(false);
}

Boolean RxDataVFS(void)
{
	UInt32 numBytesRead = buffermax;

	while(HandleVFSData(IOREAD, vfsbuffer, &numBytesRead)) { // && !PrgCancel) {
		if ((rxrecs % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, rxrecs, -1, rxtype);
//		HandleWaitDialogUpdate(CHECKCANCEL, 0, 0, NULL);
//		HostTraceOutputTL(appErrorClass, "RxDataVFS: About to parse data");
		data.parser.parser_func(vfsbuffer, numBytesRead, false);
//		HostTraceOutputTL(appErrorClass, "RxDataVFS: Parsed data");
	}
//	HostTraceOutputTL(appErrorClass, "RxDataVFS: Returning false");
	return(false);
}

void ShowSerialError(void)
{
	UInt16 res;
	UInt32 sts;

//	Char msg[132];

	res = SrmGetStatus (serial_ref, &sts, &res);
	SrmClearErr(serial_ref);
	SrmReceiveFlush(serial_ref, 0);	// will clear the error
/*
	StrCopy(msg, "Serial Error");
	if(res & serLineErrorParity) {
		StrCat(msg,"\nParity");
	}
	if(res & serLineErrorHWOverrun) {
		StrCat(msg,"\nHWOverrun");
	}
	if(res & serLineErrorFraming) {
		StrCat(msg,"\nFraming");
	}
	if(res & serLineErrorBreak) {
		StrCat(msg,"\nBreak");
	}
	if(res & serLineErrorHShake) {
		StrCat(msg,"\nHandshake");
	}
	if(res & serLineErrorSWOverrun) {
		StrCat(msg,"\nSW Overrun");
	}
	Report(msg);
*/
}

Boolean XferInit(Char *baudrate, UInt16 flowctrl, Int8 xfertype)
{
	Boolean retval;
	switch(xfertype) {
		case USEMEMO:
			// Overloading baudrate to be the file to open
			// Overloading flowctrol to show whether to truncate the file on opening
			return(XferInitMemo(baudrate, (UInt32)flowctrl));
			break;
		case USEVFS:
			// Overloading baudrate to be the file to open
			// Overloading flowctrol to show whether to truncate the file on opening
			return(XferInitVFS(baudrate, (UInt32)flowctrl));
			break;
		case USEDOC:
			// Overloading baudrate to be the file to open
			// Overloading flowctrol to show whether to truncate the file on opening
			return(XferInitDOC(baudrate, (UInt32)flowctrl));
			break;
		case USEIQUE:
			return(XferInitiQue());
			break;
		case USEIQUESER:
			retval = XferInitiQue();
			// If the iQue failed to initialize, don't go any further
			if (!retval) {
				return(retval);
			}
			if (device.NewSerialPresent || device.CliePDA) {
//				HostTraceOutputTL(appErrorClass, "XferInitNew");
				retval = XferInitNew(StrAToI(baudrate), flowctrl, xfertype);
			} else {
//				HostTraceOutputTL(appErrorClass, "XferInitOld");
				retval = XferInitOld(StrAToI(baudrate), flowctrl);
			}
			return(retval);
			break;
		case USECF:
			return(XferInitNew(StrAToI(baudrate), flowctrl, xfertype));
			break;
		default:
			if (device.NewSerialPresent || device.CliePDA) {
//				HostTraceOutputTL(appErrorClass, "XferInitNew");
				return(XferInitNew(StrAToI(baudrate), flowctrl, xfertype));
			} else {
//				HostTraceOutputTL(appErrorClass, "XferInitOld");
				return(XferInitOld(StrAToI(baudrate), flowctrl));
			}
			break;
	}
}

Boolean XferInitOld(UInt32 baudrate, UInt16 flowctrl)
{
	SerSettingsType serial_settings;
	Err err;

	if (SysLibFind("Serial Library", &serial_ref)) {
		ErrDisplay("Error trying to load the serial library");
		return(false);
	}

	if (SerOpen(serial_ref, 0, baudrate)) {
//		ErrDisplay("Could not open serial port");
		FrmCustomAlert(WarningAlert, "Could not open serial port"," "," ");
		serial_ref = 0;
		return(false); 
	}

	SerClearErr(serial_ref);	
	SerReceiveFlush(serial_ref, 0);

	// Allocate a dynamic memory chunk for our custom receive queue
	customSerQP = MemPtrNew(myCustomSerQueueSize);
//	buffermax = 16+max(baudrate/20, 128);
	if (baudrate/20 > 128) {
		buffermax = 16+(baudrate/20);
	} else {
		buffermax = 16+128;
	}
	buffer = MemPtrNew (buffermax+1);

	// Replace the default receive queue
	if (customSerQP)
		err = SerSetReceiveBuffer(serial_ref, customSerQP, myCustomSerQueueSize);

	if (flowctrl) {
		serial_settings.flags = serSettingsFlagStopBits1 | serSettingsFlagBitsPerChar8 | 
										serSettingsFlagRTSAutoM | serSettingsFlagCTSAutoM;
		serial_settings.ctsTimeout = SERIAL_TIMEOUT_DEFAULT;
	} else {
		serial_settings.flags = serSettingsFlagStopBits1 | serSettingsFlagBitsPerChar8 ;
		serial_settings.ctsTimeout=0;
	}
	serial_settings.baudRate=baudrate;
//	HostTraceOutputTL(appErrorClass, "XferInit: baudrate - |%lu|", baudrate);

	if (SerSetSettings(serial_ref,&serial_settings)) {
		ErrDisplay("Err when setting serial port values");
		return(false);
	}
	return(true);
}

Boolean XferInitNew (UInt32 baudrate, UInt16 flowctrl, Int8 xfertype)
{
	Err error = 0;
	UInt32 flgs = 0;
	DeviceInfoType dt;
	Char msg[128];
	Int16 n = 0;
	UInt16 sz;
	UInt32 value;
	Int32 ctsTimeout = SERIAL_TIMEOUT_DEFAULT;

	*msg = 0;

	error = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
	if (error != 0) {
		ErrDisplay("Error trying to load the New Serial Library");
		return(false);
	}

	if (OpenPortNew(baudrate, xfertype)) {
//		HostTraceOutputTL(appErrorClass, "XferInitNew: OpenPortNew returned true-error");
		return(false);
	}

	SrmClearErr(serial_ref);
	SrmReceiveFlush(serial_ref, 0);

	SrmGetDeviceInfo(serial_ref, &dt);
	if(baudrate > dt.serDevMaxBaudRate) {
		baudrate =  dt.serDevMaxBaudRate;
		n = StrPrintF(msg, "Rate capped at %ld baud", baudrate);
	}

	if(flowctrl == NOFLOW && baudrate > dt.serDevHandshakeBaud) {
//		flowctrl = RTS;
//		if(n) {
//			*(msg+n++) = '\n';
//		}
//		n += StrPrintF(msg,"\nForcing RTS for %ld baud", baudrate);
	}
	if(n) {
		Report(msg);
	}

//	HostTraceOutputTL(appErrorClass, "XferInitNew: Setting baudrate-|%lu|", baudrate);
	// Sets the baudrate
	sz = sizeof((Int32)baudrate);
	error = SrmControl(serial_ref, srmCtlSetBaudRate, &baudrate, &sz);

	if (flowctrl == RTS || flowctrl == BOTH) {
		// Turns on RTS/CST Flow control
		flgs |= (srmSettingsFlagCTSAutoM |
					srmSettingsFlagRTSAutoM |
					srmSettingsFlagFlowControlIn);
		// Supposedly needed to get Tungsten serial port into proper state
		flgs = flgs & ~srmSettingsFlagRTSInactive;
	} else {
		// Turns off all hardware flow control
		flgs &= ~(srmSettingsFlagCTSAutoM |
					 srmSettingsFlagRTSAutoM |
					 srmSettingsFlagFlowControlIn);
	}

	// Turns off Stop Bits Settings
//	flgs &= ~srmSettingsFlagStopBitsM;
	// Sets Stop Bits to 1
	flgs |= srmSettingsFlagStopBits1;

	// Turns off BitsPerChar Setting
//	flgs &= ~srmSettingsFlagBitsPerCharM;
	// Sets to 8 bits per character
	flgs |= srmSettingsFlagBitsPerChar8;
//	switch(prefs.bits) {
//		case 5:
//			flgs |= srmSettingsFlagBitsPerChar5;
//			break;
//		case 6:
//			flgs |= srmSettingsFlagBitsPerChar6;
//			break;
//		case 7:
//			flgs |= srmSettingsFlagBitsPerChar7;
//			break;
//		default:
//			flgs |= srmSettingsFlagBitsPerChar8;
//			break;
//	}

//	if (flowctrl == XON || flowctrl == FLOWBOTH) {
		// Turns Xon/Xoff Flow Control On
//		flgs |= srmSettingsFlagXonXoffM ;
//	} else {
		// Turns Xon/Xoff Flow Control Off
//		flgs &= ~srmSettingsFlagXonXoffM ;
//	}

	// Turns Parity Off
//	flgs &=  ~(srmSettingsFlagParityOnM | srmSettingsFlagParityEvenM);
//	if (prefs.parity == 1) {
		// Sets Odd Parity
//		flgs |= srmSettingsFlagParityOnM;
//	} else if (prefs.parity == 2) {
		// Sets Even Parity
//		flgs |= srmSettingsFlagParityOnM | srmSettingsFlagParityEvenM;
//	}

	sz = sizeof(flgs);
	if(error == 0) {
		error = SrmControl(serial_ref, srmCtlSetFlags, &flgs, &sz);
	}

	if (flowctrl == NOFLOW) {
		ctsTimeout = 0;
	}
	sz = sizeof(ctsTimeout);
	if(error == 0) {
		error = SrmControl(serial_ref, srmCtlSetCtsTimeout, &ctsTimeout, &sz);
	}

	if(xfertype == USEIR) {
//		error = SrmControl(serial_ref, srmCtlIrDAEnable,NULL,NULL);
//		error = SrmControl(serial_ref, srmCtlRxEnable,NULL,NULL);
	}

	if (error) {
		SrmClearErr(serial_ref);
//		ErrNonFatalDisplayIf(error, errSettingSerial);
		ErrDisplay(errSettingSerial);
		return(false);
	} else {
		customSerQP = MemPtrNew(myCustomSerQueueSize);
		if (baudrate/20 > 128) {
			buffermax = 16+(baudrate/20);
		} else {
			buffermax = 16+128;
		}
// For Testing
//		buffermax = 1024 * 6;

//		buffer = MemPtrNew(buffermax+1);
		buffer = MemPtrNew(2*buffermax+1);

		error = SrmSetReceiveBuffer (serial_ref, customSerQP, myCustomSerQueueSize);
		if (error) {
			ErrNonFatalDisplayIf ((error == serErrBadPort), "Port Doesn't Exist");
			ErrNonFatalDisplayIf ((error == memErrNotEnoughSpace), "Out Of Memory");
		}

	}
	SrmReceiveFlush(serial_ref, 0);	// will clear the error
//	HostTraceOutputTL(appErrorClass, "XferInitNew: Succeeded-about to return false");
	return(true);
}

Boolean XferInitAlt (UInt32 baudrate, UInt16 flowctrl, Int8 xfertype)
{
	Err error = 0;
	UInt32 flgs = 0;
	DeviceInfoType dt;
	Char msg[128];
	Int16 n = 0;
	UInt16 sz;
	UInt32 value;
	Int32 ctsTimeout = SERIAL_TIMEOUT_DEFAULT;

	*msg = 0;

	error = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
	if (error != 0) {
		ErrDisplay("Error trying to load the New Serial Library");
		return(false);
	}

//	if (OpenPortNew(baudrate, xfertype)) {
//		HostTraceOutputTL(appErrorClass, "XferInitNew: OpenPortNew returned true-error");
//		return(false);
//	}
	if (device.romVersion >= SYS_VER_40) {
		error = SrmOpen(SerportID40, baudrate, &serial_alt);
	} else {
		error = SrmOpen(SerportID35, baudrate, &serial_alt);
	}
	if (error) {
		return(false);
//		HostTraceOutputTL(appErrorClass, "XferInitNew: OpenPortNew returned true-error");
	}

	SrmClearErr(serial_alt);
	SrmReceiveFlush(serial_alt, 0);

	SrmGetDeviceInfo(serial_alt, &dt);
	if(baudrate > dt.serDevMaxBaudRate) {
		baudrate =  dt.serDevMaxBaudRate;
		n = StrPrintF(msg, "Rate capped at %ld baud", baudrate);
	}

	if(flowctrl == NOFLOW && baudrate > dt.serDevHandshakeBaud) {
//		flowctrl = RTS;
//		if(n) {
//			*(msg+n++) = '\n';
//		}
//		n += StrPrintF(msg,"\nForcing RTS for %ld baud", baudrate);
	}
	if(n) {
		Report(msg);
	}

//	HostTraceOutputTL(appErrorClass, "XferInitNew: Setting baudrate-|%lu|", baudrate);
	// Sets the baudrate
	sz = sizeof((Int32)baudrate);
	error = SrmControl(serial_alt, srmCtlSetBaudRate, &baudrate, &sz);

	if (flowctrl == RTS || flowctrl == BOTH) {
		// Turns on RTS/CST Flow control
		flgs |= (srmSettingsFlagCTSAutoM |
					srmSettingsFlagRTSAutoM |
					srmSettingsFlagFlowControlIn);
		// Supposedly needed to get Tungsten serial port into proper state
		flgs = flgs & ~srmSettingsFlagRTSInactive;
	} else {
		// Turns off all hardware flow control
		flgs &= ~(srmSettingsFlagCTSAutoM |
					 srmSettingsFlagRTSAutoM |
					 srmSettingsFlagFlowControlIn);
	}

	// Turns off Stop Bits Settings
//	flgs &= ~srmSettingsFlagStopBitsM;
	// Sets Stop Bits to 1
	flgs |= srmSettingsFlagStopBits1;

	// Turns off BitsPerChar Setting
//	flgs &= ~srmSettingsFlagBitsPerCharM;
	// Sets to 8 bits per character
	flgs |= srmSettingsFlagBitsPerChar8;
//	switch(prefs.bits) {
//		case 5:
//			flgs |= srmSettingsFlagBitsPerChar5;
//			break;
//		case 6:
//			flgs |= srmSettingsFlagBitsPerChar6;
//			break;
//		case 7:
//			flgs |= srmSettingsFlagBitsPerChar7;
//			break;
//		default:
//			flgs |= srmSettingsFlagBitsPerChar8;
//			break;
//	}

//	if (flowctrl == XON || flowctrl == FLOWBOTH) {
		// Turns Xon/Xoff Flow Control On
//		flgs |= srmSettingsFlagXonXoffM ;
//	} else {
		// Turns Xon/Xoff Flow Control Off
//		flgs &= ~srmSettingsFlagXonXoffM ;
//	}

	// Turns Parity Off
//	flgs &=  ~(srmSettingsFlagParityOnM | srmSettingsFlagParityEvenM);
//	if (prefs.parity == 1) {
		// Sets Odd Parity
//		flgs |= srmSettingsFlagParityOnM;
//	} else if (prefs.parity == 2) {
		// Sets Even Parity
//		flgs |= srmSettingsFlagParityOnM | srmSettingsFlagParityEvenM;
//	}

	sz = sizeof(flgs);
	if(error == 0) {
		error = SrmControl(serial_alt, srmCtlSetFlags, &flgs, &sz);
	}

	if (flowctrl == NOFLOW) {
		ctsTimeout = 0;
	}
	sz = sizeof(ctsTimeout);
	if(error == 0) {
		error = SrmControl(serial_alt, srmCtlSetCtsTimeout, &ctsTimeout, &sz);
	}

	if(xfertype == USEIR) {
//		error = SrmControl(serial_alt, srmCtlIrDAEnable,NULL,NULL);
//		error = SrmControl(serial_alt, srmCtlRxEnable,NULL,NULL);
	}

	if (error) {
		SrmClearErr(serial_alt);
//		ErrNonFatalDisplayIf(error, errSettingSerial);
		ErrDisplay(errSettingSerial);
		return(false);
	} else {
		customSerQP2 = MemPtrNew(myCustomSerQueueSize);
		if (baudrate/20 > 128) {
			buffermax = 16+(baudrate/20);
		} else {
			buffermax = 16+128;
		}
// For Testing
//		buffermax = 1024 * 6;

//		buffer = MemPtrNew(buffermax+1);
		buffer2 = MemPtrNew(2*buffermax+1);

		error = SrmSetReceiveBuffer (serial_alt, customSerQP2, myCustomSerQueueSize);
		if (error) {
			ErrNonFatalDisplayIf ((error == serErrBadPort), "Port Doesn't Exist");
			ErrNonFatalDisplayIf ((error == memErrNotEnoughSpace), "Out Of Memory");
		}

	}
	SrmReceiveFlush(serial_alt, 0);	// will clear the error
//	HostTraceOutputTL(appErrorClass, "XferInitNew: Succeeded-about to return false");
	return(true);
}

Boolean XferInitMemo(Char *fileName, UInt32 inittype)
{
	buffermax = 82;
	buffer = MemPtrNew (buffermax+1);
	// Initialize the memopad output
	return(HandleMemoData(IOINIT, fileName, &inittype));
}

Boolean XferInitDOC(Char *fileName, UInt32 inittype)
{
	buffermax = 82;
	docbuffer = MemPtrNew (buffermax+1);
	MemSet(docbuffer, buffermax+1, 0);
	// Initialize the DOC output
	return(HandleDOCData(IOINIT, fileName, &inittype));
}

Boolean XferInitVFS(Char *fileName, UInt32 inittype)
{
	buffermax = 82;
	vfsbuffer = MemPtrNew (buffermax+1);
	// Initialize the VFS output
	return(HandleVFSData(IOINIT, fileName, &inittype));
}

Boolean VFSFileCopy(Char *oldpath, Char *newpath)
{
	FileRef ofref, nfref;
	UInt32 numBytes = 0;
	Err err;
	static Char *filebuffer = NULL;
	UInt16 volRefNum = vfsInvalidVolRef;
	UInt32 volIterator = vfsIteratorStart;

	// Open the card
	while (volIterator != vfsIteratorStop) {
		err = VFSVolumeEnumerate(&volRefNum, &volIterator);
		if (err == errNone) {
			// Stopping on the first found
			// Should be OK for now
			break;
		}
	}
	if (volRefNum == vfsInvalidVolRef) return(false);

	// initialise buffer
	filebuffer = MemPtrNew(VFSBufferSize+1);
	if (!filebuffer) return(false);
	MemSet(filebuffer, VFSBufferSize+1, 0);

	// open files
	err = VFSFileOpen(volRefNum, oldpath, vfsModeRead,  &ofref);
	if (err != errNone) return(false);
	err = VFSFileOpen(volRefNum, newpath, (vfsModeWrite|vfsModeCreate|vfsModeTruncate), &nfref);
	if (err != errNone) return(false);

	// Copy the old file to the new one
	while (!VFSFileEOF(ofref)) {
		err = VFSFileRead(ofref, VFSBufferSize, filebuffer, &numBytes);
		if (numBytes >= 1) err = VFSFileWrite(nfref, numBytes, filebuffer, &numBytes);
	}
	if (err != errNone) return(false);
	
	// Close the files
	VFSFileClose(ofref);
	VFSFileClose(nfref);

	// free buffer memory
	if (filebuffer) {
		MemPtrFree(filebuffer);
		filebuffer = NULL;
	}

	return(true);
}

Boolean HandleVFSData(Int8 cmd, Char *data, UInt32 *numBytes)
{
	Err		err = errNone;
	static UInt16	volRefNum=vfsInvalidVolRef;
	UInt32	volIterator = vfsIteratorStart;
	Char	basedirName1[16];
	Char	basedirName2[16];
	Char	dirName[256];
	Char	fileName[256];
	static FileRef vfsdirRef;
	static FileRef vfsfileRef;
	static UInt32 filepos = 0;
	static Char *filebuffer = NULL;
	Int16 prev_numfilesfound = 0;
	FileInfoType FileInfo;
	Char nameP[FILENAMESIZE];
	Char *prev_filelist = NULL;
	Char extension[5];

	StrCopy (basedirName1, "/PALM");
	StrCopy (basedirName2, "/PALM/Programs");
	StrCopy (dirName, "/PALM/Programs/SoarPilot");

	if ((cmd == IODELETE) || (cmd == IOEXIST) || (cmd == IODIR) || (cmd == IOINIT)) {
		// common code to check for a card present and the /PALM/Programs/SoarPilot directory
		volRefNum = vfsInvalidVolRef;

		if (device.CardPresent) {
			while (volIterator != vfsIteratorStop) 
			{
				err = VFSVolumeEnumerate(&volRefNum, &volIterator);
				if (err == errNone) {
					// Stopping on the first found
					// Should be OK for now
					break;
				}
			}
			if (volRefNum == vfsInvalidVolRef) {
				return(false);
			}
	
			// try to open the /PALM directory to ensure that it is present
			err = VFSFileOpen(volRefNum, basedirName1, (vfsModeReadWrite), &vfsdirRef);
	
			if (err == vfsErrFileNotFound) {
				err = VFSDirCreate(volRefNum, basedirName1);
				if (err != errNone) {
					return(false);
				}
			} else if (err != errNone) {
				return(false);
			}
			VFSFileClose(vfsdirRef);

			// try to open the /PALM/Programs directory to ensure that it is present
			err = VFSFileOpen(volRefNum, basedirName2, (vfsModeReadWrite), &vfsdirRef);
	
			if (err == vfsErrFileNotFound) {
				err = VFSDirCreate(volRefNum, basedirName2);
				if (err != errNone) {
					return(false);
				}
			} else if (err != errNone) {
				return(false);
			}

			VFSFileClose(vfsdirRef);

			// try to open the /Palm/Programs/SoarPilot directory & get a valid dirref
			err = VFSFileOpen(volRefNum, dirName, (vfsModeReadWrite), &vfsdirRef);
	
			if (err == vfsErrFileNotFound) {
				err = VFSDirCreate(volRefNum, dirName);
				if (err != errNone) {
					return(false);
				}
			} else if (err != errNone) {
				return(false);
			}
			VFSFileClose(vfsdirRef);

		} else {
			FrmCustomAlert(WarningAlert, "No Memory Card In Slot"," "," ");
			return(false);
		}
	}

	switch(cmd) {
		case IODELETE:
			// delete a file
			StrCopy(fileName, dirName);
			StrCat(fileName, "/");
			// The filename is passed in data
			StrCat(fileName, data);
			err = VFSFileDelete(volRefNum, fileName);
			if (err == errNone) {
				return(true);
			} else {
				fileerror = err;
				return(false);
			}
			break;
		case IOEXIST:
			// test if a filename is valid and exists
			StrCopy(fileName, dirName);
			StrCat(fileName, "/");
			// The filename is passed in data
			StrCat(fileName, data);
			err = VFSFileCreate(volRefNum, fileName);
			if (err == errNone) {
				err = VFSFileDelete(volRefNum, fileName);
				return(true);
			} else {
				fileerror = err;
				return(false);
			}
			break;
		case IODIR:
			// list files in /Palm/Programs/SoarPilot directory  that have the required extension (data)
			numfilesfound = 0;
			prev_numfilesfound = 0;
			err = VFSFileOpen(volRefNum, dirName, vfsModeRead, &vfsdirRef);
			if(err == errNone) {
				FileInfo.nameP = nameP; // point to local buffer
				FileInfo.nameBufLen = FILENAMESIZE;
				volIterator = vfsIteratorStart;
				while (volIterator != vfsIteratorStop) {
					MemSet(nameP, FILENAMESIZE, 0);
					// Get the next file
					err = VFSDirEntryEnumerate(vfsdirRef, &volIterator, &FileInfo);
					if (err == errNone) {
//						HostTraceOutputTL(appErrorClass, "File =|%s|", FileInfo.nameP);
						// check file extension (ignore case)
						StrToLower(extension , Right(nameP, 4));
						if (StrCompare(extension, data) == 0) {
//							HostTraceOutputTL(appErrorClass, "Add File to List");
							// increment file count
							numfilesfound++;

							// add item to the list
							if (prev_numfilesfound > 0) {
								AllocMem((void *)&prev_filelist, prev_numfilesfound * FILENAMESIZE);
								MemMove(prev_filelist, filelist, prev_numfilesfound * FILENAMESIZE);
							}
							if (filelist) FreeMem((void *)&filelist);

							AllocMem((void *)&filelist, numfilesfound * FILENAMESIZE);
							if ((prev_numfilesfound > 0) && prev_filelist) MemMove(filelist, prev_filelist, prev_numfilesfound * FILENAMESIZE);

							// filename without extension
							StrCopy(&filelist[(numfilesfound-1)*FILENAMESIZE], Left(nameP, StrLen(nameP)-4));
		
							if ((prev_numfilesfound > 0) && prev_filelist) FreeMem((void *)&prev_filelist);
							prev_numfilesfound = numfilesfound;
						}

					} else {
						// handle error, possibly by breaking out of the loop
						break;
					}
				}
			} else {
				// handle directory open error here
				return(false);
			}
			VFSFileClose(vfsdirRef);
			quicksort(filelist, numfilesfound, FILENAMESIZE, 1);
			*numBytes = numfilesfound;
			return(true);
			break;
		case IOINIT:
			if (StrLen(data) < 5) return(false);
			StrCopy(fileName, dirName);
			StrCat(fileName, "/");
			// The filename is passed in data
			StrCat(fileName, data);

			// initialise buffer
			filepos = 0;
			filebuffer = MemPtrNew(VFSBufferSize+1);
			if (!filebuffer) return(false);
			MemSet(filebuffer, VFSBufferSize+1, 0);

			// Overloading numBytes to pass whether to clean the file upon opening
			if (*numBytes == IOOPENTRUNC) {
				// try to open the file and get a valid fileref for the file
				err = VFSFileOpen(volRefNum, fileName, (vfsModeReadWrite|vfsModeCreate|vfsModeTruncate), &vfsfileRef);
			} else {
				err = VFSFileOpen(volRefNum, fileName, (vfsModeRead), &vfsfileRef);
			}
	
			if (err != errNone) {
				return(false);
			}
			return(true);
			break;
		case IOWRITE:
			// new buffered write code from soarCol.c COLWriteString function
//			HostTraceOutputTL(appErrorClass, "file pos-|%lu|", filepos);
			if (*numBytes) {
				if ((filepos + *numBytes + 1) > VFSBufferSize) {
//					HostTraceOutputTL(appErrorClass, "WRITE Data1-|%lu|", filepos);
					err = VFSFileWrite(vfsfileRef, filepos, filebuffer, NULL);	// write the buffer to the end of the output file
					if (err == errNone) {
						MemSet(filebuffer, VFSBufferSize, 0);		// erase the whole buffer
						MemMove(filebuffer, data, *numBytes);		// copy the new string to the beginning of the empty buffer
						filepos = *numBytes;
					}
				} else {
					MemMove(&filebuffer[filepos], data, *numBytes);	// copy the new string to the end of the buffer
					filepos += *numBytes;
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "WRITE Data2-|%lu|", filepos);
				err = VFSFileWrite(vfsfileRef, filepos, filebuffer, NULL);	// write the buffer to the end of the output file
				if (err == errNone) {
					MemSet(filebuffer, VFSBufferSize, 0);		// erase the whole buffer
					filepos = 0;
				}
			}

			// removed simple write
			//err = VFSFileWrite(vfsfileRef, *numBytes, data, NULL);

			if (err != errNone) {
				return(false);
			}
			return(true);
			break;
		case IOREAD:
			if (VFSFileEOF(vfsfileRef)==vfsErrFileEOF) {
				return(false);
			} else {
				err = VFSFileRead(vfsfileRef, *numBytes, data, numBytes);
				data[*numBytes] = '\0';
//				HostTraceOutputTL(appErrorClass, "data read-|%s|", data);

				if (err == vfsErrFileEOF) {
//					HostTraceOutputTL(appErrorClass, "error-at EOF");

					if (numBytes == 0) {
						*numBytes = 2;
					}
					if (data[(*numBytes)-1] != '\n') {
//						HostTraceOutputTL(appErrorClass, "VFS Read: Adding newline");
						data[*numBytes] = '\n';
						(*numBytes)++;
					}
					data[*numBytes] = '\0';
					// Return true one more
//					HostTraceOutputTL(appErrorClass, "returning true once more");
					return(true);
				} else if (err != errNone) {
//					HostTraceOutputTL(appErrorClass, "error-returning false");
					return(false);
				}
				return(true);
			}
			break;
		case IOCLOSE:
			// empty buffer to file
//			HostTraceOutputTL(appErrorClass, "CLOSING file pos-|%lu|", filepos);
			if (filepos > 0) {
				err = VFSFileWrite(vfsfileRef, filepos, filebuffer, NULL);	// write the buffer to the end of the output file
			}

			// close the current file
			VFSFileClose(vfsfileRef);

			// free buffer memory
			if (filebuffer) {
				MemPtrFree(filebuffer);
				filebuffer = NULL;
				filepos = 0;
			}
			return(true);
			break;
		default:
			return(false);
			break;
	}
}


void XferClose(Int8 xfertype)
{
	switch(xfertype) {
		case USEMEMO:
			XferCloseMemo();
			break;
		case USEVFS:
			XferCloseVFS();
			break;
		case USEDOC:
			XferCloseDOC();
			break;
		case USEIQUE:
			XferCloseiQue();
			break;
		case USEIQUESER:
			XferCloseiQue();
			if (device.NewSerialPresent || device.CliePDA) {
//				HostTraceOutputTL(appErrorClass, "XferCloseNew");
				if (serial_ref) {
					XferCloseNew();
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "XferCloseOld");
				if (serial_ref) {
					XferCloseOld();
				}
			}
			break;
		case USECF:
//			HostTraceOutputTL(appErrorClass, "XferCloseNew");
			if (serial_ref) {
				XferCloseNew();
			}
			break;
		default:
//			if (device.romVersion >= SYS_VER_32 || device.CliePDA) {
			if (device.NewSerialPresent || device.CliePDA) {
//				HostTraceOutputTL(appErrorClass, "XferCloseNew");
				if (serial_ref) {
					XferCloseNew();
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "XferCloseOld");
				if (serial_ref) {
					XferCloseOld();
				}
			}
			break;
	}
	return;
}

void XferCloseOld(void)
{
	Err err;

	//Pass 0 for bufSize to restore the default queue
	err = SerSetReceiveBuffer (serial_ref, NULL, 0);
	if (customSerQP) {
		MemPtrFree (customSerQP);
		customSerQP = NULL;
	}
	if (buffer) {
		MemPtrFree(buffer);
		buffer = NULL;
	}
	SerClearErr(serial_ref);	
	SerReceiveFlush(serial_ref, 0);
	SerClose(serial_ref);
}

void XferCloseNew(void)
{
	SrmSetReceiveBuffer(serial_ref, NULL, 0);
	if (customSerQP) {
		MemPtrFree(customSerQP);
		customSerQP = NULL;
	}
	if (buffer) {
		MemPtrFree(buffer);
		buffer = NULL;
	}
	SrmClearErr(serial_ref);
	SrmReceiveFlush(serial_ref, 0);
	SrmClose(serial_ref);
}

void XferCloseAlt(void)
{
	if (serial_alt) {
		SrmSetReceiveBuffer(serial_alt, NULL, 0);
		if (customSerQP2) {
			MemPtrFree(customSerQP2);
			customSerQP2 = NULL;
		}
		if (buffer2) {
			MemPtrFree(buffer2);
			buffer2 = NULL;
		}
		SrmClearErr(serial_alt);
		SrmReceiveFlush(serial_alt, 0);
		SrmClose(serial_alt);
		serial_alt = 0;
	}
}

void XferCloseMemo(void)
{
	// Close and cleanup the memopad output
	HandleMemoData(IOCLOSE, "NULL", 0);
	if (buffer) {
		MemPtrFree(buffer);
		buffer = NULL;
	}
}

void XferCloseDOC(void)
{
	// Close and cleanup the DOC output
	HandleDOCData(IOCLOSE, "NULL", 0);
	if (docbuffer) {
		MemPtrFree(docbuffer);
		docbuffer = NULL;
	}
}

void XferCloseVFS(void)
{
	HandleVFSData(IOCLOSE, "NULL", 0);
	if (vfsbuffer) {
		MemPtrFree(vfsbuffer);
		vfsbuffer = NULL;
	}
}

Boolean TxData(Char *data, Int8 xfertype)
{
	Boolean retval=true;

	if (xfertype == USEMEMO) {
		retval = TxDataMemo(data);
	} else if (xfertype == USEVFS) {
		retval = TxDataVFS(data);
	} else if (xfertype == USEDOC) {
		retval = TxDataDOC(data);
	} else {
		if (device.NewSerialPresent || device.CliePDA || xfertype == USECF ||
			 xfertype == USEBT || xfertype == USEUSB) {
//			HostTraceOutputTL(appErrorClass, "TxDataNew");
			if (serial_ref) {
				retval = TxDataNew(data);
			}
		} else {
//			HostTraceOutputTL(appErrorClass, "TxDataOld");
			if (serial_ref) {
				retval = TxDataOld(data);
			}
		}
	}
	return(retval);
}

Boolean TxDataOld(Char *data)
{
	UInt32 numbytestx = 0;
	Err error;
	numbytestx = SerSend(serial_ref, data, StrLen(data), &error);
	ErrNonFatalDisplayIf( (error!=0), "Failed to send data to GPS");
	SerClearErr(serial_ref);	
	SerSendWait(serial_ref, -1);	
	if (error == 0) {
		return(true);
	} else {
		return(false);
	}
}

Boolean TxDataNew(Char *data)
{
	UInt32 numbytestx = 0;
	Err error;
//	HostTraceOutputTL(appErrorClass, "TxDataNew data-|%s|", data);
	numbytestx = SrmSend(serial_ref, data, StrLen(data), &error);
	ErrNonFatalDisplayIf( (error!=0), "Failed to send data to device");
	SrmClearErr(serial_ref);	
	if (!device.StyleTapPDA) {
		SrmSendWait(serial_ref);	
	}
	if (error == 0) {
		return(true);
	} else {
		return(false);
	}
}

Boolean TxDataAlt(Char *data)
{
	UInt32 numbytestx = 0;
	Err error;
//	HostTraceOutputTL(appErrorClass, "TxDataAlt data-|%s|", data);

	if (!serial_alt) return(false);
	
	numbytestx = SrmSend(serial_alt, data, StrLen(data), &error);
	ErrNonFatalDisplayIf( (error!=0), "Failed to send data to device");
	SrmClearErr(serial_alt);
	if (!device.StyleTapPDA) {
		SrmSendWait(serial_alt);
	}
	if (error == 0) {
		return(true);
	} else {
		return(false);
	}
}

Boolean TxDataMemo(Char *data)
{
	UInt32 numBytes = StrLen(data);

	return(HandleMemoData(IOWRITE, data, &numBytes));
}

Boolean TxDataDOC(Char *data)
{
	UInt32 numBytes = StrLen(data);

	return(HandleDOCData(IOWRITE, data, &numBytes));
}

Boolean TxDataVFS(Char *data)
{
	UInt32 numBytes = StrLen(data);

	return(HandleVFSData(IOWRITE, data, &numBytes));
}

Boolean OpenPortNew (UInt32 baudrate, Int8 xfertype)
{
	Err error;

	if (xfertype == USEIR) {
//		HostTraceOutputTL(appErrorClass, "Using IRportID");
		error = SrmOpen(IRportID, baudrate, &serial_ref);
	} else if (xfertype == USECF) {
//		HostTraceOutputTL(appErrorClass, "Using CFportID");
		error = SrmOpen(CFportID, baudrate, &serial_ref);
	} else if (xfertype == USEUSB) {
//		HostTraceOutputTL(appErrorClass, "Using USBportID");
		error = SrmOpen(USBportID, baudrate, &serial_ref);
	} else if (device.romVersion >= SYS_VER_40) {
		if (xfertype == USEBT) {
//			HostTraceOutputTL(appErrorClass, "Using BTportID");
//			original BT open code
//			error = SrmOpen(BTportID, baudrate, &serial_ref);
			{
				// AGM: new feature, dedicated BT GPS connection
				// BT address is taken from config file, no GUI
				// if not found use address 00:00:00:00:00:00
				// which will popup the BT search dialog, just like
				// in the old code. Selected device can be saved
				// in config file by using Transfer > Configuration
				SrmOpenConfigType config; 
				BtVdOpenParams btParams; 
				UInt8 NoConfig = 0; 
				config.function = 0;                              // must be zero 
				config.drvrDataP = (MemPtr)&btParams; 
				config.drvrDataSize = sizeof(BtVdOpenParams); 
				 
				btParams.role = btVdClient;                       // we are the client side 
				
				// BT address in config is 00:00:00:00:00?
				NoConfig = data.config.BTAddr[5] == 0 &&
							data.config.BTAddr[4] == 0 &&
							data.config.BTAddr[3] == 0 &&
							data.config.BTAddr[2] == 0 &&
							data.config.BTAddr[1] == 0 &&
							data.config.BTAddr[0] == 0;
							 
				if (NoConfig) {
					// let user select a BT device
					Err err = BT_FindDevice();
					if (!err) {
						extern UInt8 BTAddress[6];
						// copy selected address in config
						// swap address bytes because it is swapped again below
						// this is due to be able to use human readable BT address
						// in the config file.
						data.config.BTAddr[5] = BTAddress[0];
						data.config.BTAddr[4] = BTAddress[1];
						data.config.BTAddr[3] = BTAddress[2];
						data.config.BTAddr[2] = BTAddress[3];
						data.config.BTAddr[1] = BTAddress[4];
						data.config.BTAddr[0] = BTAddress[5];
					}
				}
				
				// Set preferred BT address from config
				btParams.u.client.remoteDevAddr.address[0] = data.config.BTAddr[5]; // remote device addr byte 1 
				btParams.u.client.remoteDevAddr.address[1] = data.config.BTAddr[4]; // remote device addr byte 2 
				btParams.u.client.remoteDevAddr.address[2] = data.config.BTAddr[3]; // remote device addr byte 3 
				btParams.u.client.remoteDevAddr.address[3] = data.config.BTAddr[2]; // remote device addr byte 4 
				btParams.u.client.remoteDevAddr.address[4] = data.config.BTAddr[1]; // remote device addr byte 5 
				btParams.u.client.remoteDevAddr.address[5] = data.config.BTAddr[0]; // remote device addr byte 6 
				btParams.u.client.method = btVdUseChannelId; 
				btParams.u.client.u.channelId = 0x53; //??

				// by using SrmExtOpen the BT lib is not needed and
				// BT connection acts like a regular RS232 serial link
				error = SrmExtOpen( 
						BTportID, 			// type of port == RFCOMM 
						&config,            // port configuration params 
						sizeof(config),     // size of port config params 
						&serial_ref         // receives the id of this virtual serial port instance 
				);
 			}
		} else if (xfertype == USEST) {
//			HostTraceOutputTL(appErrorClass, "Using STporID for StyleTap Support");
			error = SrmOpen(data.config.stcurdevcreator, baudrate, &serial_ref);
		} else if (xfertype == USEEXT) {
//			HostTraceOutputTL(appErrorClass, "Using EXportID");
			error = SrmOpen(EXportID, baudrate, &serial_ref);
		} else {
//			HostTraceOutputTL(appErrorClass, "Using SerportID40");
			error = SrmOpen(SerportID40, baudrate, &serial_ref);
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "Using SerportID35");
		error = SrmOpen(SerportID35, baudrate, &serial_ref);
	}

	if (error) {
		if (error==serErrAlreadyOpen) FrmCustomAlert(WarningAlert, "Serial Port in use"," "," ");
		if (error==memErrNotEnoughSpace) FrmCustomAlert(WarningAlert, "Out of memory"," "," ");
		if (error==serErrBadParam) FrmCustomAlert(WarningAlert, "Failed to open port"," "," ");
		if (error==serErrBadPort) FrmCustomAlert(WarningAlert, "This port doesn't exist"," "," ");

		serial_ref = 0;
//		HostTraceOutputTL(appErrorClass, "OpenPortNew returning true(error)");
		return(true);
	}
//	HostTraceOutputTL(appErrorClass, "OpenPortNew returning false(good)");
	return(false);
}

Boolean FindVolRefNum(UInt16 *volRefNum)
{
	Err err;
	UInt32 volIterator = vfsIteratorStart;

	*volRefNum=vfsInvalidVolRef;

	while (volIterator != vfsIteratorStop) 
	{
		err = VFSVolumeEnumerate(volRefNum, &volIterator);
		if (err == errNone) {
			// Stopping on the first found
			// Should be OK for now
			break;
		}
	}
	if (*volRefNum == vfsInvalidVolRef) {
		return(false);
	}
	return(true);
}

Err CardMountHandler(SysNotifyParamType *notifyParamsP)
{
	Err err=0;
	UInt16 volRefNum;
	VolumeInfoType volInfoP;
	
//	HostTraceOutputTL(appErrorClass, "Volume Mount Event");
	notifyParamsP->handled |= vfsHandledStartPrc;
	notifyParamsP->handled |= vfsHandledUIAppSwitch;
	device.CardPresent = FindVolRefNum(&volRefNum);;
	VFSVolumeInfo(volRefNum, &volInfoP);

	if (volInfoP.attributes & vfsVolumeAttrReadOnly) {
		device.CardRW = false;
		// popup warning for readonly SD card
		warning->type = Wcardwrite;
		FrmPopupForm(form_warning);
	} else {
		device.CardRW = true;
	}

	if (FrmGetActiveFormID() == form_set_port) {
		ctl_set_visible(form_set_port_usevfs, true);
		// Have to do this to ensure the right line of the button is displayed
		ctl_set_visible(form_set_port_usedoc, true);
	}

	return(err);
}	

Err CardUnMountHandler(SysNotifyParamType *notifyParamsP)
{
	Err err=0;

//	HostTraceOutputTL(appErrorClass, "Volume UnMount Event");
	device.CardPresent = false;
	device.CardRW = false;

	if (FrmGetActiveFormID() == form_set_port) {
		ctl_set_visible(form_set_port_usevfs, false);
		if (device.NewSerialPresent || device.CliePDA) {
			ctl_set_visible(form_set_port_useir, true);
		} else {
			ctl_set_visible(form_set_port_useir, false);
		}
		if (device.BTCapable) {
			ctl_set_visible(form_set_port_usebt, true);
		}
		// Have to do this to ensure the right line of the button is displayed
		ctl_set_visible(form_set_port_usedoc, true);
	}

	return(err);
}	

Boolean WaitFor(Char *prompt, Int8 xfertype)
{
	Char c;
	const Char *ptr = prompt;
	UInt32 st = TimGetTicks();
	
	if (xfertype == USESER || xfertype == USECF || xfertype == USEBT ||
		 xfertype == USEIR || xfertype == USEUSB || xfertype == USEST || xfertype == USEEXT) {
		while (*ptr) {
			if (!GetData(&c, 1, xfertype))
				return(false);
			if (*ptr == c) {
				ptr++;
			} else {
				ptr = prompt;
				if (TimGetTicks() > st+SysTicksPerSecond()*5) return(false);
			}
		}
		return(true);
	} else {
		return(false);
	}
}

Boolean WaitForC(Char prompt, Int8 xfertype)
{
	Char c;
	Char ptr = prompt;

	if (xfertype == USESER || xfertype == USECF || xfertype == USEBT || 
		 xfertype == USEIR || xfertype == USEUSB || xfertype == USEST || xfertype == USEEXT) {
		while (ptr != c) {
			if (!GetData(&c, 1, xfertype))
				return(false);
		}

		return(true);
	} else {
		return(false);
	}
}

Boolean GetDataEOL(Char *buf, Int8 xfertype)
{
	Char c;
	Char *ptr=buf;

	if (xfertype == USESER || xfertype == USECF || xfertype == USEBT || 
		 xfertype == USEIR || xfertype == USEUSB || xfertype == USEST || xfertype == USEEXT) {
		while (c!='\r') {
			if (!GetData(&c, 1, xfertype)) return(false);
			*ptr = c;
//			HostTraceOutputTL(appErrorClass, "c-|%c|", c);
//			HostTraceOutputTL(appErrorClass, "*ptr-|%c|", *ptr);
			ptr++;
		}
		ptr--;
		*ptr = '\0';

//		HostTraceOutputTL(appErrorClass, "GetDataEOL-About to return buf-|%s|", buf);
		return(true);
	} else {
		return(false);
	}
}

Boolean GetData(Char *buf, UInt32 datalen, Int8 xfertype)
{
	if (xfertype == USESER || xfertype == USECF || xfertype == USEBT || 
		 xfertype == USEIR || xfertype == USEUSB || xfertype == USEST || xfertype == USEEXT) {
		if (device.NewSerialPresent || device.CliePDA) {
//			HostTraceOutputTL(appErrorClass, "GetDataNew");
			return(GetDataNew(buf, datalen));
		} else {
//			HostTraceOutputTL(appErrorClass, "GetDataOld");
			return(GetDataOld(buf, datalen));
		}
	}
	return(false);
}

Boolean GetDataOld(Char *buf, UInt32 datalen)
{
	Err err;
	UInt32 numBytes = 0;
	UInt32 numBytesPending=0;

//	HostTraceOutputTL(appErrorClass, "GetDataTest: Inside GetDataTest");
//	HostTraceOutputTL(appErrorClass, "GetDataTest: datalen: |%lu|", datalen);
	// Set waitTime to 5 seconds
	waitTime = SysTicksPerSecond() * 5;

	err = SerReceiveWaitNew(serial_ref, datalen, waitTime, &numBytesPending);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "GetDataTest: ShowSerialError1");
		SerClearErr(serial_ref);
		SerReceiveFlush(serial_ref, 0);
		return(false);
	}

	numBytes = SerReceive(serial_ref, buf, datalen, waitTime, &err);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "GetDataNew: ShowSerialError2");
		SerClearErr(serial_ref);
		SerReceiveFlush(serial_ref, 0);	// will clear the error
		return(false);
	}
	if(numBytes > 0) {
//		HostTraceOutputTL(appErrorClass, "GetDataTest:buf before|%s|", buf);
		buf[numBytes] = '\0';
//		HostTraceOutputTL(appErrorClass, "GetDataTest:buf after|%s|", buf);
//		HostTraceOutputTL(appErrorClass, "GetDataTest:StrLen(buf):|%hu|", StrLen(buf));
		return(true);
	}
	return(false);
}

Boolean GetDataNew(Char *buf, UInt32 datalen)
{
	Err err;
	UInt32 numBytes = 0;
	UInt32 numBytesPending=0;

//	HostTraceOutputTL(appErrorClass, "GetDataTest: Inside GetDataTest");
//	HostTraceOutputTL(appErrorClass, "GetDataTest: datalen: |%lu|", datalen);
	// Set waitTime to 5 seconds
	waitTime = SysTicksPerSecond() * 5;

	err = SrmReceiveWaitNew(serial_ref, datalen, waitTime, &numBytesPending);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "GetDataTest: ShowSerialError1");
		SrmClearErr(serial_ref);
		SrmReceiveFlush(serial_ref, 0);
		return(false);
	}

	numBytes = SrmReceive(serial_ref, buf, datalen, waitTime, &err);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "GetDataNew: ShowSerialError2");
		SrmClearErr(serial_ref);
		SrmReceiveFlush(serial_ref, 0);	// will clear the error
		return(false);
	}
	if(numBytes > 0) {
//		HostTraceOutputTL(appErrorClass, "GetDataTest:buf before|%s|", buf);
		buf[numBytes] = '\0';
//		HostTraceOutputTL(appErrorClass, "GetDataTest:buf after|%s|", buf);
//		HostTraceOutputTL(appErrorClass, "GetDataTest:StrLen(buf):|%hu|", StrLen(buf));
		return(true);
	}
	return(false);
}

Err SerReceiveWaitNew(UInt16 serial_ref, UInt32 datalen, Int32 timeout, UInt32 *numBytesPending)
{
	Err err=errNone;
	UInt32 curticks;

	curticks = TimGetTicks();
	err = SerReceiveCheck(serial_ref, numBytesPending);
//	HostTraceOutputTL(appErrorClass, "SerReceiveWaitOld: *numBytesPending: |%lu|", *numBytesPending);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "SerReceiveWaitOld: ShowSerialError1");
	}

	while (*numBytesPending < datalen && err == errNone) {
		if ((curticks+(UInt32)timeout) <= TimGetTicks()) {
			err = serErrTimeOut;
		} else {
			err = SerReceiveCheck(serial_ref, numBytesPending);
//			HostTraceOutputTL(appErrorClass, "SerReceiveWaitOld: Inside *numBytesPending: |%lu|", *numBytesPending);
		}
		if (err) {
//			HostTraceOutputTL(appErrorClass, "SerReceiveWaitOld: ShowSerialError2");
		}
		Sleep(0.1);
	}
	return(err);
}

Err SrmReceiveWaitNew(UInt16 serial_ref, UInt32 datalen, Int32 timeout, UInt32 *numBytesPending)
{
	Err err=errNone;
	UInt32 curticks;

	curticks = TimGetTicks();
	err = SrmReceiveCheck(serial_ref, numBytesPending);
//	HostTraceOutputTL(appErrorClass, "SrmReceiveWaitNew: *numBytesPending: |%lu|", *numBytesPending);
	if (err) {
//		HostTraceOutputTL(appErrorClass, "SrmReceiveWaitNew: ShowSerialError1");
	}

	while (*numBytesPending < datalen && err == errNone) {
		if ((curticks+(UInt32)timeout) <= TimGetTicks()) {
			err = serErrTimeOut;
		} else {
			err = SrmReceiveCheck(serial_ref, numBytesPending);
//			HostTraceOutputTL(appErrorClass, "SrmReceiveWaitNew: Inside *numBytesPending: |%lu|", *numBytesPending);
		}
		if (err) {
//			HostTraceOutputTL(appErrorClass, "SrmReceiveWaitNew: ShowSerialError2");
		}
		Sleep(0.1);
	}
	return(err);
}

Boolean serial_out(UInt8 data)
{
	// send single byte to serial port
	Err error;
	UInt32 numbytes = 0;

	if (serial_ref) {
		if (device.NewSerialPresent || device.CliePDA) {
			numbytes = SrmSend(serial_ref, &data, 1, &error);
			ErrNonFatalDisplayIf( (error!=0), "Failed to send data");
			SrmClearErr(serial_ref);
			SrmSendWait(serial_ref);
		} else {
			numbytes = SerSend(serial_ref, &data, 1, &error);
			ErrNonFatalDisplayIf( (error!=0), "Failed to send data");
			SerClearErr(serial_ref);
			SerSendWait(serial_ref, -1);	
		}
	}

	if (!error) {
		return(true);
	} else {
		return(false);
	}
}

Boolean serial_in(UInt8 *data)
{
	// receive single byte from serial port
	Err error;
	UInt32 numBytes = 0;

	if (serial_ref) {
		if (device.NewSerialPresent || device.CliePDA) {
			error = SrmReceiveCheck(serial_ref, &numBytes);
			if ((error) || (numBytes == 0)) return(false);
			numBytes = SrmReceive(serial_ref, data, 1, 0, &error);
		} else {
			error = SerReceiveCheck(serial_ref, &numBytes);
			if ((error) || (numBytes == 0)) return(false);
			numBytes = SerReceive(serial_ref, data, 1, 0, &error);
		}
	}

	if ((numBytes > 0) && (!error)) {
		return(true);
	} else {
		return(false);
	}

}

// Check to see if USB Ports are available
Boolean USBEnabledDevice() 
{
	DeviceInfoType devInfo;
	Err	err;
	Boolean	retval=false;
	
	if (device.NewSerialV2Present) {
		err = SrmGetDeviceInfo('usbc', &devInfo);
		if (err == 0) {
			if (devInfo.serDevFtrInfo & serDevUSBCapable) {
				retval = true;
			}
		}
	}
	return(retval);
}

void SetDefaultFilename(Int8 filetype, Boolean withextension) {

	// standard file names
	switch (filetype) {
		case CONFIG_SCG:
			StrCopy(transfer_filename, "config");
			if (withextension) StrCat(transfer_filename, ".scg");
			break;
		case WAYPOINTS_DAT:
			StrCopy(transfer_filename, "waypoints");
			if (withextension) StrCat(transfer_filename, ".dat");
			break;
		case WAYPOINTS_CUP:
			StrCopy(transfer_filename, "waypoints");
			if (withextension) StrCat(transfer_filename, ".cup");
			break;
		case WAYPOINTS_WPL:
			StrCopy(transfer_filename, "waypoints");
			if (withextension) StrCat(transfer_filename, ".wpl");
			break;
		case WAYPOINTS_OZI:
			StrCopy(transfer_filename, "waypoints");
			if (withextension) StrCat(transfer_filename, ".wpt");
			break;
		case TASKS_SPT:
			StrCopy(transfer_filename, "tasks");
			if (withextension) StrCat(transfer_filename, ".spt");
			break;
		case POLARS_SPL:
			StrCopy(transfer_filename, "polars");
			if (withextension) StrCat(transfer_filename, ".spl");
			break;
		case SUADATA_TNP:
			StrCopy(transfer_filename, "suadata");
			if (withextension) StrCat(transfer_filename, ".sua");
			break;
		case SUADATA_OPENAIR:
			StrCopy(transfer_filename, "suadata");
			if (withextension) StrCat(transfer_filename, ".air");
			break;
		default:
			transfer_filename[0] = 0;
			break;
		}
	return;
}

Boolean form_list_files_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	static Char extension[5];
	static UInt32 numbytes;
	Boolean overwritefile;
	Char tempchar[10];
	Char tempchar2[30];

	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_files_list));

	switch (event->eType) {
		case frmOpenEvent:
//		case frmUpdateEvent:

			FrmDrawForm(pfrm);
			menuopen = false;
			selectedFileIndex = -1;
			currentFilePage = 0;
			transfer_filename[0] = 0;			

			if (io_type == IO_TRANSMIT) {
				// set buttons for transmitting files
				ctl_set_visible(form_list_files_savebtn, true);
				ctl_set_visible(form_list_files_defaultbtn, true);
				FrmSetFocus(pfrm, FrmGetObjectIndex(pfrm, form_list_files_filename));
			} else {
				// set buttons for receiving files
				ctl_set_visible(form_list_files_loadbtn, true);
				ctl_set_visible(form_list_files_deletebtn, true);
			}				

			// set window title and extension
			switch (io_file_type) {
				case CONFIG_SCG:
					StrCopy(tempchar2, "Select Config - ");
					StrCopy(extension, ".scg");
					break;
				case WAYPOINTS_DAT:
					StrCopy(tempchar2, "Select Waypoints - ");
					StrCopy(extension, ".dat");
					break;
				case WAYPOINTS_CUP:
					StrCopy(tempchar2, "Select Waypoints - ");
					StrCopy(extension, ".cup");
					break;
				case WAYPOINTS_WPL:
					StrCopy(tempchar2, "Select Waypoints - ");
					StrCopy(extension, ".wpl");
					break;
				case WAYPOINTS_OZI:
					StrCopy(tempchar2, "Select Waypoints - ");
					StrCopy(extension, ".wpt");
					break;
				case TASKS_SPT:
					StrCopy(tempchar2, "Select Tasks - ");
					StrCopy(extension, ".spt");
					break;
				case POLARS_SPL:
					StrCopy(tempchar2, "Select Polars - ");
					StrCopy(extension, ".spl");
					break;
				case SUADATA_TNP:
					StrCopy(tempchar2, "Select SUA - ");
					StrCopy(extension, ".sua");
					break; 		
				case SUADATA_OPENAIR:
					StrCopy(tempchar2, "Select SUA - ");
					StrCopy(extension, ".air");
					break;
				default:
					StrCopy(tempchar2, "Select File - ");
					StrCopy(extension, ".xxx");
					break; 		
			}
			SysStringByIndex(form_port_table, data.config.xfertype, tempchar, 7);
			StrCat(tempchar2, tempchar);
			frm_set_title(tempchar2);

			// get directoty for specfic file type
			numfilesfound = 0;
			switch (data.config.xfertype) {
				case USEVFS:
					HandleVFSData(IODIR, extension, &numbytes);
					refresh_files_list(0);
					break;
				case USEDOC:
					HandleDOCData(IODIR, extension, &numbytes);
					refresh_files_list(0);
					break;
				default:
					break;
			}
			// set file name
			field_set_value(form_list_files_filename, transfer_filename);

			handled=true;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_list_files_defaultbtn:
					// default filename
					SetDefaultFilename(io_file_type, false);
					field_set_value(form_list_files_filename, transfer_filename);
					selectedFileIndex = -1;
					refresh_files_list(0);
					break;
				case form_list_files_loadbtn:
					// return with filename
					StrCopy(transfer_filename, field_get_str(form_list_files_filename));
					if (StrLen(transfer_filename) > 0) FrmGotoForm(form_transfer);					
					break;
				case form_list_files_savebtn:
					// check if file exists
					StrCopy(transfer_filename, field_get_str(form_list_files_filename));
					StrCat(transfer_filename, extension);
					overwritefile = false;
					switch (data.config.xfertype) {
						case USEVFS:
							if (!HandleVFSData(IOEXIST, transfer_filename, &numbytes)) {
								switch (fileerror) {
									case vfsErrBadName:
										FrmCustomAlert(WarningAlert, "Bad Filename"," "," ");
										break;
									case vfsErrFileAlreadyExists:
										if (FrmCustomAlert(ConfirmAlertYN, "Overwrite Existing File?"," "," ") == YES) {
											overwritefile = true;
										}
										break;
									default:
										// allow main routine to trap error
										overwritefile = true;
										break;
									}
							} else {
								// no error 
								overwritefile = true;
							}
							break;
						case USEDOC:
							if (!HandleDOCData(IOEXIST, transfer_filename, &numbytes)) {
								if (FrmCustomAlert(ConfirmAlertYN, "Overwrite Existing File?"," "," ") == YES) {
									overwritefile = true;
								}
							} else {
								// no error 
								overwritefile = true;
							}
							break;
						default:
							break;
					}	
					if (overwritefile) {
						// return with filename
						StrCopy(transfer_filename, field_get_str(form_list_files_filename));
						if (StrLen(transfer_filename) > 0) FrmGotoForm(form_transfer);
					}
					break;
				case form_list_files_deletebtn:
					StrCopy(transfer_filename, field_get_str(form_list_files_filename));
					StrCat(transfer_filename, extension);
					if (StrLen(transfer_filename) > 4) {
						if (FrmCustomAlert(ConfirmAlert, "Delete", transfer_filename, "?") == OK) {
							// delete the file
							switch (data.config.xfertype) {
								case USEVFS:
									HandleVFSData(IODELETE, transfer_filename, &numbytes);
									break;
								case USEDOC:
									HandleDOCData(IODELETE, transfer_filename, &numbytes);
									break;
								default:
									break;
								}
						}
						// rebuild directory
						refresh_files_list(FILEFREE);
						numfilesfound = 0;
						selectedFileIndex = -1;
						switch (data.config.xfertype) {
							case USEVFS:
								HandleVFSData(IODIR, extension, &numbytes);
								refresh_files_list(0);
								break;
							case USEDOC:
								HandleDOCData(IODIR, extension, &numbytes);
								refresh_files_list(0);
								break;
							default:
								break;
						}
						transfer_filename[0] = 0;
						field_set_value(form_list_files_filename, transfer_filename);
					}
					break;					
				case form_list_files_closebtn:
					// return empty filename
					transfer_filename[0] = 0;
					field_set_value(form_list_files_filename, transfer_filename);
					FrmGotoForm(form_transfer);
					break;
				default:
					break;
			}
			handled=true;
			break;
		case fldEnterEvent:
			if (io_type == IO_RECEIVE) {
				PlayKeySound();
				handled=true;        	
			}
			break;
		case lstSelectEvent:
			if (event->data.lstSelect.listID == form_list_files_list) {
				if (SetSelectedFile(LstGetSelection(lst))) {
					PlayKeySound();
/*					if (io_type == IO_RECEIVE) {
						ctl_set_visible(form_list_files_loadbtn, true);
						ctl_set_visible(form_list_files_deletebtn, true);
					}
*/
					field_set_value(form_list_files_filename, &filelist[selectedFileIndex * FILENAMESIZE]);
				}
			}
			DrawHorizListLines(7, 28, 14);
			handled=true;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
			menuopen = true;
			handled = false;
			break;
		case frmCloseEvent:
			// get filename with extension
			StrCopy(transfer_filename, field_get_str(form_list_files_filename));
			StrCat(transfer_filename, extension);
			// free memory
			refresh_files_list(FILEFREE);
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

Boolean SetSelectedFile(Int16 lstselection)
{
	if (numfilesfound > 0) {
		selectedFileIndex = lstselection + (currentFilePage * Filenumperpage);
//		HostTraceOutputTL(appErrorClass, "selectedFileIndex-|%hd|", selectedFileIndex);
		return(true);
	} else {
		return(false);
	}
}

void refresh_files_list(Int16 scr)
{
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	Int16 x;
	static Char **items = NULL;
	Int16 nrecs;
	Int16 start;
	Int16 end;
	static Int16 prevNumRecs = 0;
	Char pageString[20];
	Char tmpString[20];
	Int16 FileIndex;

	// Free up each of the previous strings and then free up
	// the array of pointers itself.
	for (x = 0; x < prevNumRecs; x++) {
		MemPtrFree(items[x]);
	}
	if (items) {
		MemPtrFree(items);
		items = NULL;
	}

	// free file list memory	
	if (scr == FILEFREE) if (filelist) FreeMem((void *)&filelist);

	if (scr != FILEFREE) {
		// Get the List pointer
		lst = FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_files_list));

		// check current page exists
		while (currentFilePage > numfilesfound/Filenumperpage) currentFilePage--;

		// Compute the "page" of Files we're currently looking at.
		if (scr > 0) {
			if (((currentFilePage + 1) * Filenumperpage) < numfilesfound) {
				// If there are more waypoints to display, move down one page
				currentFilePage++;
			} else {
				// If at the bottom, wrap to the first page
				currentFilePage = 0;
			}
		} else if (scr < 0) {
			if (currentFilePage > 0) {
				// If not on the first page of waypoints, move up one page 
				currentFilePage--;
			} else {
				// If at the top, wrap to the last page
				if (numfilesfound == 0) {
					currentFilePage = 0;
				} else if (Fmod((double)numfilesfound,(double)Filenumperpage) == 0.0) {
					currentFilePage = (Int16)(numfilesfound/Filenumperpage) - 1;
				} else {
					currentFilePage = (Int16)(numfilesfound/Filenumperpage);
				}
			}
		}

		// Given the current "page", compute the starting
		// and ending index and the number of records.
		start = currentFilePage * Filenumperpage;
		end = ((start + Filenumperpage) > numfilesfound) ? numfilesfound : (start + Filenumperpage);
		nrecs = end - start;

		if (nrecs > 0) {
			
			// We got at least one record so allocate enough 
			// memory to hold nrecs pointers-to-pointers
			items = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// Loop through each waypoint record
			for (x = 0, FileIndex = start; FileIndex < end; FileIndex++, x++) { 
				// Assign each of the nrecs pointers-to-pointers
				//  the address of a newly allocated FILENAMESIZE character array,
				//  retrieve the filename associated with that record,
				//  and copy that name into the array.
				items[x] = (Char *) MemPtrNew(FILENAMESIZE * (sizeof(Char)));
				MemSet(items[x],FILENAMESIZE,0);
				StrCopy(items[x], &filelist[FileIndex * FILENAMESIZE]);
			}
					
			// Reform the list
			LstSetListChoices(lst, items, nrecs);

		} else {
			items = (char **) MemPtrNew(1 * (sizeof(char *)));
			prevNumRecs = 1;		
			items[0] = (char *) MemPtrNew(FILENAMESIZE * (sizeof(char)));
			MemSet(items[0],FILENAMESIZE,0);
			StrNCopy(items[0], "No Files", 12);
			LstSetListChoices(lst, items, 1);
			LstSetSelection(lst, 0);
			selectedFileIndex = -1;
		}

/*		// show or hide buttons
		if ((selectedFileIndex > -1) && (io_type == IO_RECEIVE)) {
			ctl_set_visible(form_list_files_loadbtn, true);
			ctl_set_visible(form_list_files_deletebtn, true);
		} else {
			ctl_set_visible(form_list_files_loadbtn, false);
			ctl_set_visible(form_list_files_deletebtn, false);
		}
		ctl_set_visible(form_list_files_closebtn, true);
*/
		// Create the "Page: # of #" string
		MemSet(pageString,20, 0);
		if (numfilesfound == 0) {
			StrCopy(pageString,StrIToA(tmpString,(currentFilePage)));
		} else {
			StrCopy(pageString,StrIToA(tmpString,(currentFilePage+1)));
		}
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tmpString,(numfilesfound % Filenumperpage) ? 
				(((int)(numfilesfound/Filenumperpage)) + 1) : (int)(numfilesfound/Filenumperpage)));
		field_set_value(form_list_files_page, pageString);
		field_set_value(form_list_files_nrecs, StrIToA(tmpString,numfilesfound));
	
		// Redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_files_list));
		}
	
		// If the currently selected waypoint is on the currently
		//  displayed page, then darken it as if it were selected.  If not then
		//  de-select everything.
		if ((selectedFileIndex >= (currentFilePage * Filenumperpage)) 
			&& (selectedFileIndex < ((currentFilePage * Filenumperpage) + Filenumperpage))) {
			LstSetSelection(lst, selectedFileIndex % Filenumperpage);
			field_set_value(form_list_files_filename, &filelist[selectedFileIndex * FILENAMESIZE]);
		} else {
			LstSetSelection(lst, -1);
		}
		DrawHorizListLines(7, 28, 14);
	} else {
		prevNumRecs = 0;
	}
}

#ifndef TREOSUPPORT
Err SendSMS(Int8 msgtype)
{
	return(0);
}
Boolean TreoDevice()
{
	return(false);
}
#else
Err SendSMS(Int8 msgtype)
{
	Err error=0;

	if (data.config.outputSMS) {
		if (device.TreoPDA) {
			error = TreoSendSMS(data.config.SMSaddress, data.config.SMSouttype, msgtype);
		} else {
			error = ExgSendSMS(data.config.SMSaddress, data.config.SMSouttype, msgtype);
		} 
	}
	return(error);
}

Boolean TreoDevice() 
{
//	Err	err;
	UInt32	value;
//	void*	procP;

	// Make sure that we are running on the Treo
	// Make sure that we are at the appropriate Palm OS version
	if (!FtrGet (hsFtrCreator, hsFtrIDVersion, &value)) {
		if (value < hsMinVersionSupported) {
//			FrmCustomAlert (WarningAlert, "PalmOS version too low", NULL, NULL);
			return(false);
		}
	} else {
//		FrmCustomAlert (WarningAlert, "No hsFtrIDVersion", NULL, NULL);
		return(false);
	}

	// Make sure that we support the HsIndicator function call.
	// We use this test to make sure that we are on a Treo device.
/*
	procP = HsGetTrapAddress (hsSelIndicator);
	if (procP && procP != HsGetTrapAddress(hsSelUnimplemented)) {
		// We're good to go
	} else {
		// Display the error dialog and return with error
//		FrmCustomAlert (WarningAlert, "No hsSelIndicator proc", NULL, NULL);
		return(false);
	}
*/

/*
	err = LoadPhoneLibrary(false)
	if (err) {
		// Not able to load the phone library 
//		FrmCustomAlert (WarningAlert, "Unable to load Phone Library", NULL, NULL);
		return(false);
	}
*/

//	FrmCustomAlert (WarningAlert, "Returning TreoDevice true", NULL, NULL);
	return(true);
}

/****************************************************************/
/* LoadPhoneLibrary */
/**
 * Get a libRef to the Phone library
 *
 * \param   unloadlib   Boolean to know if this is to load or unload
 *                      the phone library
 *
 * \retval  Any errors
 *
 ****************************************************************/
Err LoadPhoneLibrary(Boolean unloadlib)
{
	Err error = 0;

	if (unloadlib) {
	// Release the SMS library
		if (smslib_ref != sysInvalidRefNum) {
			PhnLibClose(smslib_ref);
			smslib_ref=sysInvalidRefNum;
		}
	} else {
		if (smslib_ref == sysInvalidRefNum) {
			error = HsGetPhoneLibrary(&smslib_ref);
			if (error) {
				smslib_ref = sysInvalidRefNum;
				return(error);
			}

			error = PhnLibOpen(smslib_ref);
			if (error) return(error);
		}
	}


	return(error);
}

Err TreoSendSMS(MemPtr address, Int8 outtype, Int8 msgtype)
{
	UInt32 msgID = 0;
	PhnAddressList addList;
	PhnAddressHandle addressH;
	Err error = 0;
	DmOpenRef smsRefNum = 0;	// CDMA workaround
	Char message[161];

	if (SMSGetMessage(message, outtype, msgtype)) {
		// Load the phone libarary
		error = LoadPhoneLibrary(false);
		if (error)
			return(error); 

		// verify the Phone is powered
		if (!PhnLibModulePowered (smslib_ref)) {
			FrmCustomAlert(WarningAlert, "Phone not powered on!", NULL, NULL);
			return(1);
		}

		// verify the Phone is on the network
		if (!PhnLibRegistered (smslib_ref)) {
			FrmCustomAlert(WarningAlert, "Phone not on the network!", NULL, NULL);
			return(1);
		}

		smsRefNum = PhnLibGetDBRef(smslib_ref);		// CDMA workaround

		// now, create the new message
		msgID = PhnLibNewMessage (smslib_ref, kMTOutgoing);
		if (!msgID) {
			FrmCustomAlert(WarningAlert, "Could not create new message!", NULL, NULL);
			PhnLibReleaseDBRef(smslib_ref, smsRefNum);	// CDMA workaround
			return(1);
		}

		// the the owner of this new message to this application
		PhnLibSetOwner (smslib_ref, msgID, appcreator);

		// Get the message to be sent
		SMSGetMessage(message, outtype, msgtype);

		// fill in the text of this message
		PhnLibSetText (smslib_ref, msgID, message, (short) StrLen(message));

		// fill in the address
		addList = PhnLibNewAddressList (smslib_ref);
		if (!addList) {
			FrmCustomAlert(WarningAlert, "Could not create new address list!", NULL, NULL);
			PhnLibReleaseDBRef(smslib_ref, smsRefNum);	// CDMA workaround
			return(1);
		}

		addressH = PhnLibNewAddress (smslib_ref, address, phnLibUnknownID);
		if (!addressH) {
			FrmCustomAlert(WarningAlert, "Could not create new address!", NULL, NULL);
			PhnLibReleaseDBRef(smslib_ref, smsRefNum);	// CDMA workaround
			return(1);
		}

		PhnLibAddAddress (smslib_ref, addList, addressH);
		MemHandleFree (addressH);

		PhnLibSetAddresses (smslib_ref, msgID, addList);

		// and fire the message off!
		error = PhnLibSendMessage (smslib_ref, msgID, true);

		PhnLibReleaseDBRef(smslib_ref, smsRefNum);	// CDMA workaround

		// Unload the phone libarary
		error = LoadPhoneLibrary(true);
	}

	return(error);
}

/***********************************************************************
 *
 * FUNCTION:    PrvSmsExgLibOpen
 *
 * DESCRIPTION: Initialize the link with SMS Library
 *
 * PARAMETERS:  refNumP - pointer to the lib reference number
 *
 * RETURNED:    An error or 0 if not
 *
 ***********************************************************************/
Err PrvSmsExgLibOpen(Boolean unloadlib)
{
	Err	err=0;

	if (unloadlib) {
		/* Close the SMS Exchange Library */
		if (smslib_ref != sysInvalidRefNum) {
			err = ExgLibClose(smslib_ref);
			smslib_ref = sysInvalidRefNum;
		}
	} else {
		if (smslib_ref == sysInvalidRefNum) {
			err = SysLibFind(kSmsLibName, &smslib_ref);
			if (err == sysErrLibNotFound) {
				err = SysLibLoad(sysFileTExgLib, sysFileCSmsLib, &smslib_ref);
				if (err) {
					if (err != sysErrLibNotFound)
						ErrNonFatalDisplay("Exchange SMS Library Load Error");
					smslib_ref = sysInvalidRefNum;
					// Phone driver not installed
					return(err);
				}
			}
		}
		err = ExgLibOpen(smslib_ref);
		if (err) {
			smslib_ref = sysInvalidRefNum;	// Reset the refNum if com can't be open
		}
	}
	return(err);
}

/***********************************************************************
 *
 * FUNCTION:    PrvSmsExgOpenCom
 *
 * DESCRIPTION: Open the com
 *
 * PARAMETERS:  socket  - a pointer to store information for the socket
 *              count   - number of record for this socket
 *              refNumP - pointer to the lib reference number
 *
 * RETURNED:    An error or 0 if not
 *
 ***********************************************************************/
Err PrvSmsExgOpenCom(ExgSocketType* socket, UInt32 count, UInt16 *refNumP)
{
	Err	err;

	if (*refNumP == sysInvalidRefNum)
		err = PrvSmsExgLibOpen(false);

	if (*refNumP != sysInvalidRefNum)
	{
		MemSet(socket, sizeof(ExgSocketType), 0);
		socket->libraryRef = *refNumP;
		socket->count = count;
		err = ExgConnect(socket);
	}

	return err;
}

/***********************************************************************
 *
 * FUNCTION:    PrvSmsExgCloseCom
 *
 * DESCRIPTION: Close the com
 *
 * PARAMETERS:  socket - a pointer to the socket
 *              err    - error durring the communication
 *
 * RETURNED:    An error or 0 if not
 *
 ***********************************************************************/
/*
static Err PrvSmsExgCloseCom(ExgSocketType* socket, Err err)
{
	return ExgDisconnect(socket, err);
}
*/

/***********************************************************************
 *
 * FUNCTION:    ExgSampleSendSMS
 *
 * DESCRIPTION: Send a SMS with the SMS Exchange Library
 *
 * PARAMETERS:  address - destination
 *              outtype - output type GENERIC or PATHAWAY
 *              msgtype - which message to send
 *
 * RETURNED:    error or 0 if not
 *
 ***********************************************************************/
Err ExgSendSMS(MemPtr address, Int8 outtype, Int8 msgtype)
{
	ExgSocketType	socket;
	UInt16			messageLen, messageID;
	Err				err = errNone;
	SmsParamsType 	param;
	UInt32			nSent =0;
	Char		message[161];

	if (SMSGetMessage(message, outtype, msgtype)) {
		//-------------------
		// New connection    
		// This also opens the exchange library
		//-------------------
		err = PrvSmsExgOpenCom(&socket, 1, &smslib_ref);
		if (err)
			return(err);

		messageLen = StrLen(message);
		MemSet(&param, sizeof(param), 0);
	
		param.creator = sysFileCSmsLib;
		// Using kSmsNetworkAuto, all the specific protocol info will be ignored, and default value use instead. 
		param.networkType = kSmsNetworkAuto;
	
		param.data.send.converter = kSmsNoConverter;
		param.data.send.destinationAddress = address;

		/*-----------------------------------------------------*/
		/* Send in text mode whatever the content is, to avoid */
		/* problems with phones that don't accept 8 bits.      */
		/* Some accents will be removed...                     */
		/*-----------------------------------------------------*/
		param.dataCodingScheme = kSmsTextEncoding;

		socket.socketRef = (UInt32)&param;
		socket.length = messageLen;
		err = ExgPut(&socket);
	
		/*-------------------*/
		/* Send data         */
		/*-------------------*/
		if (!err)
		{
			// Send the message to the Sms Lib.
			// This will fill a buffer but doesn't send the message
			while((nSent < messageLen) && !err)
				nSent += ExgSend(&socket, (UInt8*)message + nSent, messageLen - nSent, &err);
		}

		if (err) {
			ExgDisconnect(&socket, err);
			return(err);
		}

		/*-------------------*/
		/* Disconnection     */
		/*-------------------*/
		err = ExgDisconnect(&socket, err);

		/*----------------------*/
		/* Store the message ID */
		/*----------------------*/
		messageID = param.smsID;
	
		// Close the Exchange SMS Library
		PrvSmsExgLibOpen(true);
	}

	return(err);
}

Boolean SMSGetMessage(Char *message, Int8 outtype, Int8 msgtype)
{
	Boolean sendmsg = true;
	Char comment[30];
	Char tempchar[30];
	UInt32 tempsecs;
	DateTimeType tempdt;

	switch(outtype) {
		case SMSOUTGEN:
			switch(msgtype) {
				case SMSSENDTO:
					StrCopy(comment, "Flight Start-\n");
					break;
				case SMSSENDLAND:
					StrCopy(comment, "Flight End-\n");
					break;
				case SMSSENDPERIOD:
					StrCopy(comment, "In-Flight-\n");
					break;
				case SMSSENDPROGEND:
					StrCopy(comment, "Flight End-\n");
					break;
				case SMSSENDNOW:
					StrCopy(comment, "Current Position-\n");
					break;
				default:
					break;
			}
			StrCopy(message, comment);
			StrCat(message, "Time: ");
			// Copy the flight DTG(MMDDYY) into the Start DateTime Structure
			// Copy the Start time UTC (HHMMSS) into the Start DateTime Structure
			StringToDateAndTime(data.logger.gpsdtg, data.logger.gpsutc, &tempdt);

			tempsecs=TimDateTimeToSeconds(&tempdt);
			SecondsToDateOrTimeString(tempsecs, tempchar, 1, (Int32)data.config.timezone);
			StrCat(message, tempchar);
			StrCat(message, "\n");
			StrCat(message, "Posit: ");
			LLToStringDMS(DegMinStringToLatLon(data.logger.gpslat, data.logger.gpslatdir[0]), tempchar, ISLAT);
			StrCat(message, tempchar);
			StrCat(message, " ");
			LLToStringDMS(DegMinStringToLatLon(data.logger.gpslng, data.logger.gpslngdir[0]), tempchar, ISLON);
			StrCat(message, tempchar);
			StrCat(message, "\n");
			StrCat(message, "Alt: ");
			StrCat(message, print_altitude(data.logger.gpsalt));
			StrCat(message, data.input.alttext);
			StrCat(message, "\n");
			break;
		case SMSOUTPW:
			switch(msgtype) {
				case SMSSENDTO:
					StrCopy(comment, "Flight Start\n");
					break;
				case SMSSENDLAND:
					StrCopy(comment, "Flight End\n");
					break;
				case SMSSENDPERIOD:
					StrCopy(comment, "In-Flight\n");
					break;
				case SMSSENDPROGEND:
					StrCopy(comment, "Flight End\n");
					break;
				case SMSSENDNOW:
					StrCopy(comment, "Current Position\n");
					break;
				default:
					break;
			}
			// $PWS,version number, name, icon, color, UTCDate, UTCtime, latitude, longitude, elevation, speed, course, comment
			// $PWS: Message identifier
			StrCopy(message, "$PWS,"); 
			// Version number: should be 1
			StrCat(message, "1,"); 
			// Name: Owner Name of device of Tracking vehicle/person.
			StrCat(message, data.igchinfo.name);
			StrCat(message, ",");
			// Icon: Name of Icon to use to display on screen. ie. 1 or Vehicles:Airplane
			StrCat(message, "1,"); 
			// Color: Color to use for UI display. Hexadecimal RGB format. 00RRGGBB. ie. Black=0,
			// White=00FFFFFF, Blue=000000FF, Red = 00FF0000
			StrCat(message, "00FF0000,"); 
			// UTCdate: UTC Date of position, DDMMYY, ie. 14/04/06. (April 14, 2006)
			StrCat(message, data.logger.gpsdtg); 
			StrCat(message, ",");
			// UTCtime: UTC time of position, HHMMSS.ss ie. 132530.50 (1:25 pm 30.5 seconds)
			StrCat(message, data.logger.gpsutc); 
			StrCat(message, ".00,"); 
			// Latitude: Latitude in Decimal degrees. Greater than 0 is North Latitude. ie. 45.23453, - 78.42453
			StrCat(message, DblToStr(data.input.gpslatdbl, 5));
			StrCat(message, ",");
			// Longitude: Longitude in Decimal degrees. Greater than 0 is East Longitude.
			StrCat(message, DblToStr(data.input.gpslngdbl, 5));
			StrCat(message, ",");
			// Elevation: Elevation in Feet
			StrCat(message, DblToStr(pround(data.logger.gpsalt,0),0));
			StrCat(message, ",");
			// Speed: Speed in Nautical Miles per hour
			StrCat(message, DblToStr(pround(data.input.ground_speed.value, 0), 0));
			StrCat(message, ",");
			// Course: Course in decimal degrees. 0 to 359
			StrCat(message, DblToStr(pround(data.input.magnetic_track.value, 0), 0));
			StrCat(message, ",");
			// Comment: A text comment.
			StrCat(message, comment);
			StrCat(message, "\n");
			break;
		default:
			break;
	}
	return(sendmsg);
}
#endif

