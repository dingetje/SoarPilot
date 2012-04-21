#ifndef SOARIO_H
#define SOARIO_H

#define SIO __attribute__ ((section ("sio")))
#define SSMS __attribute__ ((section ("ssms")))
/*****************************************************************************
 * defines 
 *****************************************************************************/
#define SERIAL_TIMEOUT_DEFAULT  (5 * sysTicksPerSecond)

// Flow Control Defines
#define NOFLOW   0
#define RTS      1 
#define XON      2
#define FLOWBOTH 3

#define USESER     0
#define USEMEMO    1
#define USEIR      2
#define USEVFS     3
#define USEDOC     4
#define USEBT      5
#define USEIQUE    6
#define USEUSB     7
#define USEIQUESER 8
#define USECF      9
#define USEST      10
#define USEEXT     11

#define IOINIT       0
#define IOWRITE      1
#define IOREAD       2
#define IOCLOSE      3
#define IOOPEN       4
#define IOOPENTRUNC  5
#define IODIR        6
#define IOEXIST      7
#define IODELETE     8

#define NO_FILE_TYPE	0
#define WAYPOINTS_DAT	1
#define WAYPOINTS_CUP	2
#define WAYPOINTS_WPL	3
#define TASKS_SPT	4
#define POLARS_SPL	5
#define CONFIG_SCG      6
#define SUADATA_TNP 	7
#define SUADATA_OPENAIR	8
#define WAYPOINTS_OZI	9

#define XFRXFR 		0
#define XFRRECEIVE 	1
#define XFRTRANSMIT 	2
#define XFRDELETE 	3
#define XFRRECONNECT   	4
#define XFRC302		5
#define XFRGPSNAV       6

#define IO_NONE		0
#define IO_RECEIVE	1
#define IO_TRANSMIT	2

#define FILEFREE 9999

#define FILENAMESIZE 64

#define errSettingSerial "Error while trying to set serial port values"

/*****************************************************************************
 * prototypes
 *****************************************************************************/
void ClearSerial() SIO;
Boolean RxData(Int8 xfertype) SIO;
Boolean RxDataOld(void) SIO;
Boolean RxDataNew(void) SIO;
Boolean RxDataMemo(void) SIO;
Boolean RxDataVFS(void) SIO;
Boolean RxDataDOC(void) SIO;
Boolean XferInit(Char *baudrate, UInt16 flowctrl, Int8 xfertype) SIO;
Boolean XferInitOld(UInt32 baudrate, UInt16 flowctrl) SIO;
Boolean XferInitNew(UInt32 baudrate, UInt16 flowctrl, Int8 xfertype) SIO;
Boolean XferInitAlt(UInt32 baudrate, UInt16 flowctrl, Int8 xfertype) SIO;
Boolean XferInitMemo(Char *fileName, UInt32 inittype) SIO;
Boolean XferInitVFS(Char *fileName, UInt32 inittype) SIO;
Boolean XferInitDOC(Char *fileName, UInt32 inittype) SIO;
void XferClose(Int8 xfertype) SIO;
void XferCloseOld(void) SIO;
void XferCloseNew(void) SIO;
void XferCloseAlt(void) SIO;
void XferCloseMemo(void) SIO;
void XferCloseVFS(void) SIO;
void XferCloseDOC(void) SIO;
Boolean TxData(char *data, Int8 xfertype) SIO;
Boolean TxDataOld(char *data) SIO;
Boolean TxDataNew(Char *data) SIO;
Boolean TxDataAlt(Char *data) SIO;
Boolean TxDataMemo(Char *data) SIO;
Boolean TxDataVFS(Char *data) SIO;
Boolean TxDataDOC(Char *data) SIO;
void ShowSerialError(void) SIO;
Boolean OpenPortNew (UInt32 baudrate, Int8 xfertype) SIO;
Boolean VFSFileCopy(Char *oldpath, Char *newpath) SIO;
Boolean HandleVFSData(Int8 cmd, Char *data, UInt32 *numBytes) SIO;
Boolean FindVolRefNum(UInt16 *volRefNum) SIO;
Err CardMountHandler(SysNotifyParamType *notifyParamsP) SIO;
Err CardUnMountHandler(SysNotifyParamType *notifyParamsP) SIO;
Boolean WaitFor(Char *prompt, Int8 xfertype) SIO;
Boolean WaitForC(Char prompt, Int8 xfertype) SIO;
Boolean GetDataEOL(Char *buf, Int8 xfertype) SIO;
Boolean GetData(Char *buf, UInt32 datalen, Int8 xfertype) SIO;
Boolean GetDataOld(Char *buf, UInt32 datalen) SIO;
Boolean GetDataNew(Char *buf, UInt32 datalen) SIO;
Err SrmReceiveWaitNew(UInt16 portId, UInt32 bytes, Int32 timeout, UInt32* numBytesPending) SIO;
Err SerReceiveWaitNew(UInt16 portId, UInt32 bytes, Int32 timeout, UInt32* numBytesPending) SIO;
Boolean serial_out(UInt8 data) SIO;
Boolean serial_in(UInt8 *data) SIO;
Boolean USBEnabledDevice() SIO;
void SetDefaultFilename(Int8 filetype, Boolean withextention) SIO;
Boolean form_list_files_event_handler(EventPtr event) SIO;
Boolean SetSelectedFile(Int16 lstselection) SIO;
void refresh_files_list(Int16 scr) SIO;
Err SendSMS(Int8 msgtype) SSMS;
Boolean TreoDevice() SSMS;
Err LoadPhoneLibrary(Boolean unloadlib) SSMS;
Err TreoSendSMS(MemPtr address, Int8 outtype, Int8 msgtype) SSMS;
Boolean SMSGetMessage(Char *message, Int8 outtype, Int8 msgtype) SSMS;
Err PrvSmsExgLibOpen(Boolean unloadlib) SSMS;
Err PrvSmsExgOpenCom(ExgSocketType* socket, UInt32 count, UInt16 *refNumP) SSMS;
Err ExgSendSMS(MemPtr address, Int8 outtype, Int8 msgtype) SSMS;

#endif
