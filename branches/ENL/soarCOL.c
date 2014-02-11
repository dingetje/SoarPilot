// tab size: 4

/**
* \file soarCOL.c
* \brief SoarPilot Colibri support
*/
#include <PalmOS.h>			// all the system toolbox headers
#include "soarCOL.h"
#include "soarComp.h"
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarDB.h"
#include "soarPLst.h"
#include "soarForm.h"
#include "soarUMap.h"
#include "soarMath.h"
#include "soarCAI.h"
#include "soarWay.h"

// global variables from the main SoarPilot program

extern Char 		c36[37];			///< FAI base36 format - from soarVolk.c
extern Boolean 		BinaryFileXfr; 		///< the format of the requested log download, false - IGC-format, true - LXN-format
extern Int8			CompCmdRes;			///< from soarComp.c to pass back results

// global variables from CAI routines

extern CAILogData 	*cailogdata;		///< CAI logger data
extern Int16 		cainumLogs;			///< CAI number of logs
extern Int16 		selectedCAIFltIndex;///< CAI selected flight index

// Global variables
Char			CRC;					///< CRC checksum

// Global variables for the flight-log download

UInt32			StartPos = 0, 			///< start postion of one flight log in the memory of the logger
				EndPos = 0;				///< end postion of one flight log in the memory of the logger
UInt32			OrigStartPos = 0;		///< original starting position of the log, because StartPos gets incremented
UInt32			CurrentPos = 0;			///< current position in the binary data when generating IGC
UInt16			RemainInBlock;			///< remaining bytes in the current block
UInt16			PosInBlock;				///< current position in the downloaded block
MemInfo			*MeminfoP;				///< memory info downloaded from the logger
UInt8			*LogBuffer;				///< Logger buffer
//Boolean			LXold = false;				// flag to show if filser sentences to be used for older LX devide (LX5000)

/// Competition class strings
Char			*Competition_Class[] =
{
	"STANDARD",
	"15-METER",
	"OPEN",
	"18-METER",
	"WORLD",
	"DOUBLE",
	"MOTOR_GL",""
};

/// GPS datum strings
Char			*Table_GPSdatum[] =
{
	"ADINDAN        ",
	"AFGOOYE        ",
	"AIN EL ABD 1970",
	"COCOS ISLAND   ",
	"ARC 1950       ",
	"ARC 1960       ",
	"ASCENSION 1958 ",
	"ASTRO BEACON E ",
	"AUSTRALIAN 1966",
	"AUSTRALIAN 1984",
	"ASTRO DOS 7/14 ",
	"MARCUS ISLAND  ",
	"TERN ISLAND    ",
	"BELLEVUE (IGN) ",
	"BERMUDA 1957   ",
	"COLOMBIA       ",
	"CAMPO INCHAUSPE",
	"CANTON ASTRO   ",
	"CAPE CANAVERAL ",
	"CAPE (AFRICA)  ",
	"CARTHAGE       ",
	"CHATHAM 1971   ",
	"CHUA ASTRO     ",
	"CORREGO ALEGRE ",
	"DJAKARTA       ",
	"DOS 1968       ",
	"EASTER ISLAND  ",
	"EUROPEAN 1950  ",
	"EUROPEAN 1979  ",
	"FINLAND 1910   ",
	"GANDAJIKA BASE ",
	"NEW ZEALAND '49",
	"OSGB 1936      ",
	"GUAM 1963      ",
	"GUX 1 ASTRO    ",
	"HJOESEY 1955   ",
	"HONG KONG 1962 ",
	"INDIAN/NEPAL   ",
	"INDIAN/VIETNAM ",
	"IRELAND 1965   ",
	"DIEGO GARCIA   ",
	"JOHNSTON 1961  ",
	"KANDAWALA      ",
	"KERGUELEN ISL. ",
	"KERTAU 1948    ",
	"CAYMAN BRAC    ",
	"LIBERIA 1964   ",
	"LUZON/MINDANAO ",
	"LUZON PHILIPPI.",
	"MAHE 1971      ",
	"MARCO ASTRO    ",
	"MASSAWA        ",
	"MERCHICH       ",
	"MIDWAY ASTRO'61",
	"MINNA (NIGERIA)",
	"NAD-1927 ALASKA",
	"NAD-1927 BAHAM.",
	"NAD-1927 CENTR.",
	"NAD-1927 CANAL ",
	"NAD-1927 CANADA",
	"NAD-1927 CARIB.",
	"NAD-1927 CONUS ",
	"NAD-1927 CUBA  ",
	"NAD-1927 GREEN.",
	"NAD-1927 MEXICO",
	"NAD-1927 SALVA.",
	"NAD-1983       ",
	"NAPARIMA       ",
	"MASIRAH ISLAND ",
	"SAUDI ARABIA   ",
	"ARAB EMIRATES  ",
	"OBSERVATORIO'66",
	"OLD EGYIPTIAN  ",
	"OLD HAWAIIAN   ",
	"OMAN           ",
	"CANARY ISLAND  ",
	"PICAIRN 1967   ",
	"PUERTO RICO    ",
	"QATAR NATIONAL ",
	"QORNOQ         ",
	"REUNION        ",
	"ROME 1940      ",
	"RT-90 SWEDEN   ",
	"S.AMERICA  1956",
	"S.AMERICA  1956",
	"SOUTH ASIA     ",
	"CHILEAN 1963   ",
	"SANTO(DOS)     ",
	"SAO BRAZ       ",
	"SAPPER HILL    ",
	"SCHWARZECK     ",
	"SOUTHEAST BASE ",
	"FAIAL          ",
	"TIMBALI 1948   ",
	"TOKYO          ",
	"TRISTAN ASTRO  ",
	"RESERVED       ",
	"VITI LEVU 1916 ",
	"WAKE-ENIWETOK  ",
	"WGS-1972       ",
	"WGS-1984       ",
	"ZANDERIJ       ",
	"CH-1903        "
};


//**************************************************************************************
//**************************************************************************************
//**
//**				Exported functions used by the main SoarPilot program
//**
//**************************************************************************************
//**************************************************************************************

/**
* \return true - successful declaration, false - unsuccessful declaration
* \brief Function for sending active task + IGC info to the Colibri, uses global variable 'data'
*/
Boolean COLDeclareTask()
{
	DateTimeType		date;
	UInt32			time;
	UInt16			numWaypts, turnPoints, i;
	UInt8			from, to, numCtrl;
	Boolean			copyStart = false,
				copyFinish = false,
				result = false;
	UInt8			*sendBuffer;

	numWaypts = data.task.numwaypts;

	turnPoints = numWaypts - 2;		// start and finish points are not counted as turnpoints
	
	if (data.task.hastakeoff)
		turnPoints--;			// takeoff point is not turnpoint
	else
	{
		copyStart = true;		// copy the start point as the takeoff point
		numWaypts++;				
	}
	
	if (data.task.haslanding)
		turnPoints--;			// landing point is not turnpoint
	else
	{
		copyFinish = true;		// copy the finish point as the landing point
		numWaypts++;				
	}

	sendBuffer = MemPtrNew(349);
	if (!sendBuffer)
	{
  		CompCmdRes = MEMORYERR;
		return false;
	}

	turnPoints = turnPoints - data.task.numctlpts;

	MemSet(sendBuffer, 349, 0);		// initialize block to send
	
	HandleWaitDialog(true);
		
	// Start assembling of flight info block
	
	// Add pilot name, max 18 chars, padded by spaces
	StrNCopy(&sendBuffer[3], data.igchinfo.name, 18);
	if (StrLen(data.igchinfo.name) < 18)
		for (i = (3 + StrLen(data.igchinfo.name)); i < (3 + 18); i++) sendBuffer[i] = 0x20;

	// Add glider type, max 11 chars, padded by spaces
	StrNCopy(&sendBuffer[22], data.igchinfo.type, 11);
	if (StrLen(data.igchinfo.type) < 11)
		for (i = (22 + StrLen(data.igchinfo.type)); i < (22 + 11); i++) sendBuffer[i] = 0x20;

	// Add registration number, max 7 chars, padded by spaces
	StrNCopy(&sendBuffer[34], data.igchinfo.gid, 7);
	if (StrLen(data.igchinfo.gid) < 7)
		for (i = (34 + StrLen(data.igchinfo.gid)); i < (34 + 7); i++) sendBuffer[i] = 0x20;

	// Add competition ID, max 3 chars, padded by spaces
	StrNCopy(&sendBuffer[42], data.igchinfo.cid, 3);
	if (StrLen(data.igchinfo.cid) < 3)
		for (i = (42 + StrLen(data.igchinfo.cid)); i < (42 + 3); i++) sendBuffer[i] = 0x20;
	
	// 48 razred ???
				
	// Add observer name, max 9 chars, padded by spaces
	StrNCopy(&sendBuffer[47], data.igchinfo.ooid, 9);
	if (StrLen(data.igchinfo.ooid) < 9)
		for (i = (47 + StrLen(data.igchinfo.ooid)); i < (47 + 9); i++) sendBuffer[i] = 0x20;
	
	TimSecondsToDateTime(TimGetSeconds(), &date);
	time = (UInt32) date.hour * 3600;
	time += date.minute * 60;
	time += date.second;

	sendBuffer[121] = (time & 0x00FF0000) >> 16;
	sendBuffer[122] = (time & 0x0000FF00) >> 8;
	sendBuffer[123] = time & 0x000000FF;
	
	sendBuffer[124] = date.day;
	sendBuffer[125] = date.month;
	sendBuffer[126] = date.year % 100;				// last 2 digits
	
	// 127-129 user defined year/month/day
	// 130-131 taskID
	
	sendBuffer[132] = turnPoints;					// number of turning points

	// 133-144 type of points
	sendBuffer[133] = 3;						// take-off point
	sendBuffer[133 + numWaypts - 1 - data.task.numctlpts] = 2;	// landing point
	for (i = 134; i < (133 + numWaypts - 1 - data.task.numctlpts); i++) sendBuffer[i] = 1;

	numCtrl = 0;
	
	// 145-192 longitudes (4 bytes x 12)
	// 193-240 latitudes (4 bytes x 12)
	// 241-348 names (9 chars x 12)
	for (i = 0; i < data.task.numwaypts; i++)
	{
		if (copyStart)
		{	
			if (i == 0)
			{
				COLCopyWaypoint(sendBuffer, 0, 0);		// copy start point into the take-off position as well
				from = 0;
				to = 1;
			}
			else
			{
				from = i;
				to = i + 1 - numCtrl;
			}
		}
		else
		{
			from = i;
			to = i - numCtrl;
		}
		
		if (copyFinish)
		{
			if (i == (data.task.numwaypts - 1))
			{
				COLCopyWaypoint(sendBuffer, i, to);		// copy last point into the landing position as well
				from = i;
				to++;
			}
		}

		if ((data.task.waypttypes[from] & CONTROL) == 0) 
		{
			COLCopyWaypoint(sendBuffer, from, to);
		}
		else
		{
			numCtrl++;
		}
	}

	if (!COLSync())
	{
		HandleWaitDialog(false);
		CompCmdRes = CONNECTERR;
		MemPtrFree(sendBuffer);
		return false;
	}
		
	serial_out(LX_STX);					// initialization sequence for sending of task declaration
	serial_out(LX_PCWRITE);
	
	result =  COLSendBlock(sendBuffer, 349);		// send the declaration block to the Colibri
	
	if (!result)						// error during sending of block
	{
		HandleWaitDialog(false);
		MemPtrFree(sendBuffer);
		return false;
	}

	// send competition class 
	MemSet(sendBuffer, 10, 0);				// initialize block to send

	// Add competition class, max 8 chars
	StrNCopy(sendBuffer, data.igchinfo.cls, 8);		// needs to be from the list, not freetext, ethzsa
	
	serial_out(LX_STX);					// initialization sequence for sending of competition class
	serial_out(LX_CCWRITE);
	
	result =  COLSendBlock(sendBuffer, 9);			// send the competition class block to the Colibri

	HandleWaitDialog(false);

	MemPtrFree(sendBuffer);
	
	return(result);
}

/**
* \param cmd command code, what to do
* \return true - successful flight headers download, false - unsuccessful download
* \brief Function for acquire Colibri flight headers. Uses global variables cailogdata and cainumLogs
*/
Boolean COLGetFlightList(Int16 cmd)
{
	UInt16			numberOfFlights = 0;
	UInt8			*rcvBuffer;
	CAILogData		*caitemp;

	if (cmd == CAIFIFREE)
	{
		if (cailogdata)							// free the previously allocated memory
		{
			MemPtrFree(cailogdata);
			cailogdata = NULL;
		}
		cainumLogs = 0;
		return true;
	}
	
	if (cmd == CAIFISTART)							// download the list of flights stored in the logger
	{
		HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);

		if (COLSync())
		{		
			rcvBuffer = MemPtrNew(95);
			if (!rcvBuffer)						// no free memory
			{
				HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
				cainumLogs = 0;
				return false;
			}
		
			serial_out(LX_STX);
			serial_out(LX_FLLIST);					// order the logger to send flight headers

			do
			{
				if (!COLReadBlock(rcvBuffer, 95, 30))		// wait for the flight header for 30s 
				{
					MemPtrFree(rcvBuffer);			// timeout or error occured, the flight headers cannot be used
					if (cailogdata)
					{
						MemPtrFree(cailogdata);
						cailogdata = NULL;
					}
					cainumLogs = 0;
					HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
					return false;
				}

				if (rcvBuffer[0])							// flight header valid
				{
					if (!cailogdata)
					{								// first flight header
						cailogdata = MemPtrNew(sizeof(CAILogData));
						if (!cailogdata)					// no free memory to store the header
						{
							MemPtrFree(rcvBuffer);
							cainumLogs = 0;
							HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
							return false;
						}
						MemSet(cailogdata, sizeof(CAILogData), 0);		// clear the storage space
					}
					else
					{
						caitemp = MemPtrNew(sizeof(CAILogData) * (numberOfFlights + 1));	// allocate memory for the new header
						if (!caitemp)						// no free memory to store the header
						{
							MemPtrFree(rcvBuffer);
							MemPtrFree(cailogdata);
							cailogdata = NULL;
							cainumLogs = 0;
							HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
							return false;
						}
						
						MemSet(caitemp, sizeof(CAILogData) * (numberOfFlights + 1), 0);		// clear the storage space
						
						MemMove(caitemp, cailogdata, sizeof(CAILogData) * numberOfFlights);	// copy the collected headers to the new array
						
						MemPtrFree(cailogdata);												// free the old array
						cailogdata = caitemp;
					}

					COLStoreFlightHeader(rcvBuffer, numberOfFlights);	// store the flight data into the cailogdata array
					numberOfFlights++;
					HandleWaitDialogUpdate(UPDATEDIALOG, numberOfFlights, -1, "header");
				}
			} while (rcvBuffer[0]);
		
			MemPtrFree(rcvBuffer);
			cainumLogs = numberOfFlights;
			HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
			return true;
		}
		else
		{
			// synching failed
			HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
			cainumLogs = 0;
			cailogdata = NULL;
			return false;
		}
	}
	return false;
}

/**
* \return true - successful flight download, false - unsuccessful download
* \brief Function for downloading one flight log.
* Uses external variable BinaryFileXfr to decide if download in LXN or IGC format
* Uses external variables cailogdata, cainumLogs, selectedCAIFltIndex
*/
Boolean COLDownloadFlight()
{
	Boolean		result;
	Char		fileName[13];

	HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, "%");
	
	if (COLSync())
	{
		if (selectedCAIFltIndex < cainumLogs)
		{
			MemSet(fileName, 13, 0);
			fileName[0] = c36[cailogdata[selectedCAIFltIndex].startDate.year % 10];		// assemble IGC-file name
			fileName[1] = c36[cailogdata[selectedCAIFltIndex].startDate.month];
			fileName[2] = c36[cailogdata[selectedCAIFltIndex].startDate.day];
			if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//			if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
				fileName[3] = 'L';							// LX navigation ID
			else
				fileName[3] = 'F';							// Filser
													// can be other manufacturer: Posigraph
													// http://www.spsys.demon.co.uk/download.htm
			fileName[4] = c36[cailogdata[selectedCAIFltIndex].SerialNo / 1296];
			fileName[5] = c36[(cailogdata[selectedCAIFltIndex].SerialNo % 1296) / 36];
			fileName[6] = c36[cailogdata[selectedCAIFltIndex].SerialNo % 36];
			fileName[7] = (cailogdata[selectedCAIFltIndex].FlightOfDay < 36 ? c36[cailogdata[selectedCAIFltIndex].FlightOfDay] : '_');

			if (BinaryFileXfr)
			{
				if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//				if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
					StrCat(fileName, ".LXN");					// LX navigation binary format
				else
					StrCat(fileName, ".FIL");					// Filser binary format
			}
			else
				StrCat(fileName, ".IGC");
			
			MeminfoP = MemPtrNew(sizeof(MemInfo));

			if (COLGetMemInfo())
			{
				StartPos = cailogdata[selectedCAIFltIndex].StartTape + (32768 * cailogdata[selectedCAIFltIndex].StartPage);
				EndPos = cailogdata[selectedCAIFltIndex].EndTape + (32768 * cailogdata[selectedCAIFltIndex].EndPage);
				OrigStartPos = StartPos;

				if (EndPos < StartPos)
					EndPos += 32768 * (MeminfoP->endpage - MeminfoP->startpage + 1);

				if (XferInit(fileName, IOOPENTRUNC, data.config.xfertype))		// open output file
				{
					if (BinaryFileXfr)
						result = COLDownloadFlightLXN();		// make the binary download	
					else
						result = COLDownloadFlightIGC();		// convert the downloaded data to IGC-format
	
					XferClose(data.config.xfertype);			// close output file
				}
				else result = false;		// file open was unsuccessful
			}
			else result = false;			// meminfo reception failed

			MemPtrFree(MeminfoP);
		}
		else result = false;				// invalid flight index (bigger than cainumLogs-1)
	}
	else result = false;					// syncing with colibri unsuccessful

	HandleWaitDialogUpdate(STOPDIALOG, 0, 0, "%");
	return result;
}

//***********************************************************************************************
//***********************************************************************************************
//**
//**				Internal functions
//**
//***********************************************************************************************
//***********************************************************************************************


/**
* \brief Function for copying waypoint into sendBuffer as it is required by the Colibri
* Uses global variable struct 'data'
* \param sendBuffer - declaration block under construction (will be sent to Colibri)
* \param from - index of waypoint in data.task struct, this should be copied into the declaration block to send
* \param to - index of waypoint in the declaration block
*/
void COLCopyWaypoint(Char *sendBuffer, UInt8 from, UInt8 to)
{
	Int32		longitude, latitude;
	
	StrNCopy(&sendBuffer[241 + (to * 9)], data.task.wayptnames[from], 8);
	longitude = (Int32) Fabs(data.task.wayptlons[from] * 60000.0);
	latitude = (Int32) Fabs(data.task.wayptlats[from] * 60000.0);
	
	if (data.task.wayptlons[from] < 0) longitude = 1 - longitude;			// west long.
	if (data.task.wayptlats[from] < 0) latitude = 1 - latitude;			// south lat.

	sendBuffer[145 + (to * 4)] = (UInt8) ((longitude & 0xFF000000) >> 24);
	sendBuffer[146 + (to * 4)] = (UInt8) ((longitude & 0x00FF0000) >> 16);
	sendBuffer[147 + (to * 4)] = (UInt8) ((longitude & 0x0000FF00) >> 8);
	sendBuffer[148 + (to * 4)] = (UInt8) (longitude & 0x000000FF);

	sendBuffer[193 + (to * 4)] = (UInt8) ((latitude & 0xFF000000) >> 24);
	sendBuffer[194 + (to * 4)] = (UInt8) ((latitude & 0x00FF0000) >> 16);
	sendBuffer[195 + (to * 4)] = (UInt8) ((latitude & 0x0000FF00) >> 8);
	sendBuffer[196 + (to * 4)] = (UInt8) (latitude & 0x000000FF);
}
//***********************************************************************************************

/**
* \brief Function for calculating CRC-checksum continuously, uses global CRC variable
* \param c - character for including in the CRC value
* \return c (the same character the function was called with)
*/
Char Calc_CRC(Char c)
{
	Int8	tmp, count, tmpc = c;

	for(count = 8; --count >= 0; c <<= 1)
	{
		tmp = CRC ^ c;
		CRC <<= 1;
		if(tmp < 0) CRC ^= CRC_POLY;
	}
	return tmpc;
}

//***********************************************************************************************

/**
* \brief Function for syncing the Colibri
* \return true - 3 consecutive correct answer from Colibri,
* false - 10 consecutive incorrect (or missing) answer from Colibri
*/
Boolean COLSync()
{
	UInt16		cnt = 0,
				failedcnt = 0,
				timeoutcnt;
	UInt8		c;

	serial_out(LX_SYN);
	ClearSerial();
	SysTaskDelay(SysTicksPerSecond()/2);
	
	serial_out(LX_SYN);
	ClearSerial();
	SysTaskDelay(SysTicksPerSecond()/2);
	
	do
	{
		serial_out(LX_SYN);
		
		c = 0;
		timeoutcnt = 0;
		while(!serial_in(&c) && (timeoutcnt < 5))		// wait 1 sec. for each LX_ACK
		{
			timeoutcnt++;
			SysTaskDelay(SysTicksPerSecond()/5);		// 0.2 sec delay
		}
		
		if (c != LX_ACK)
		{
			cnt = 0;		// Colibri did not send LX_ACK as answer
			failedcnt++;
		}
		else
		{
			cnt++;			// Colibri connected and answers correctly
			failedcnt = 0;
		}
						
		SysTaskDelay(SysTicksPerSecond()*3/4);
		ClearSerial();							// clear buffer
	} while ((cnt < 3) && (failedcnt < 10));
	
	if (cnt == 3)				
		return true;
	else
		return false;
}
//***********************************************************************************************
/**
* \brief Function for keeping the Colibri active
* \return true - correct answer from Colibri,
* false - incorrect (or missing) answer from Colibri
*/
Boolean COLPing()
{
	UInt16	timeoutcnt;
	UInt8	c;

	serial_out(LX_SYN);
	ClearSerial();
	SysTaskDelay(SysTicksPerSecond()/2);
        serial_out(LX_SYN);
        
	c = 0;
	timeoutcnt = 0;
	while(!serial_in(&c) && (timeoutcnt < 5))		// wait 1 sec. for each LX_ACK
	{
		timeoutcnt++;
		SysTaskDelay(SysTicksPerSecond()/5);		// 0.2 sec delay
	}

	if (c != LX_ACK)
		return(false);
	else
	        return(true);
}
//***********************************************************************************************

/**
* \brief Function to send a data block to the Colibri
* after the block is sent, a CRC-byte is appended, too
* \param block - pointer to the block to send
* \param size -	number of bytes to send out
* \return true - successful sending (LX_ACK received after checksum sent)
* false - unsuccessful sending (NAK received, timeout, etc...)
*/
Boolean COLSendBlock(UInt8 *block, UInt16 size)
{
	UInt32		i;
	UInt16		timeoutcnt = 0;
	Char		c;
	Boolean		rcvd = false;
	
	CRC = 0xFF;
	
	for(i = 0; i < size; i++)
	{
		serial_out(Calc_CRC(block[i]));
	}
	
	serial_out(CRC);						// send the calculated checksum
	
	do
	{
		if (serial_in(&c))
		{
			rcvd = true;
		}
		else
		{
			timeoutcnt++;
			SysTaskDelay(SysTicksPerSecond()/5);
		}
	} while ((timeoutcnt < 100) && !rcvd);			// wait for byte from the serial port for 20s
	
	if (rcvd && (c == LX_ACK))
		return true;								// LX_ACK is received after the block was sent
	else
		return false;								// error occured (timeout, CRC, etc...)
}

//***********************************************************************************************
/**
* \brief Function to receive a data block from the Colibri
* after the block is received, CRC-check is done
* \param block - pointer to the block where the data can be stored after reception
* \param size -	number of bytes to receive, the CRC-byte is not included
* \param timeout - time to wait for the next byte, in seconds
* \return true - successful (received checksum correct)
* false - unsuccessful reception (checksum incorrect, timeout, etc...)
*/
Boolean COLReadBlock(UInt8 *block, UInt16 size, UInt16 timeout)
{
	UInt16		cnt = 0,
				timeoutcnt = 0,
				timeoutThreshold = timeout * 5;
	Char		c;
	
	CRC = 0xFF;
	MemSet(block, size, 0);					//clear the buffer
	
	do
	{
		if (serial_in(&c))
		{
			timeoutcnt = 0;
			if (cnt < size)
				block[cnt] = Calc_CRC(c);
			cnt++;
		}
		else
		{
			timeoutcnt++;
			SysTaskDelay(SysTicksPerSecond()/5);
		}
	} while ((timeoutcnt < timeoutThreshold) && (cnt < (size + 1)));			// wait for data, or timeout after the predefined time
	
	if (timeoutcnt == timeoutThreshold)
		return false;
	else
	{
		if (c == CRC)
			return true;
		else
			return false;
	}
}
//***********************************************************************************************

/**
* \brief Function for downloading one block of a flight log
* \param blockStart - starting position of the block in the Colibri's memory
* \param blockEnd - end position of the block in the Colibri's memory
* \param buffer - buffer for the received block
* \return 0	- unsuccessful flight block download
* other	- successful download, returns the size of the block
*/
UInt16 COLGetFlightBlock(UInt32 blockStart, UInt32 blockEnd, UInt8 *buffer)
{
	Boolean			result, done;
	UInt16			i, offset, blockSize = 0, currentPercent;
	UInt16			sizes[16];
	UInt8			blockLimits[6];
	UInt32			percent = (EndPos - OrigStartPos + 256)/100;

	blockLimits[0] = (UInt8) ((blockStart % 32768) & 0xFF);
	blockLimits[1] = (UInt8) ((blockStart % 32768) >> 8);
	blockLimits[3] = (UInt8) ((blockEnd % 32768) & 0xFF);
	blockLimits[4] = (UInt8) ((blockEnd % 32768) >> 8);

	blockLimits[2] = (UInt8) (blockStart / 32768);
	if (blockLimits[2] > MeminfoP->endpage)
		blockLimits[2] -= MeminfoP->endpage - MeminfoP->startpage + 1;

	blockLimits[5] = (UInt8) (blockEnd / 32768);
	if (blockLimits[5] > MeminfoP->endpage)
		blockLimits[5] -= MeminfoP->endpage - MeminfoP->startpage + 1;
	
	serial_out(LX_STX);
	serial_out(LX_FLTPOS);

	result = COLSendBlock(blockLimits, 6);		// select block to download

	if (!result) return 0;						// problem occured

	serial_out(LX_STX);
	serial_out(LX_BSIZES);
	result = COLReadBlock(buffer, 32, 5);		// read 32-byte block from colibri, 5sec timeout

	if (!result) return 0;						// problem occured

	for (i = 0; i < 16; i++)
	{
		sizes[i] = (256 * buffer[2*i]) + buffer[(2*i) + 1];
		blockSize += sizes[i];
	}

	done = false;
	i = 0;
	offset = 0;
	currentPercent = blockStart - OrigStartPos;
	
	do
	{
		if (BinaryFileXfr)
		{																			// we need to update the progress dialog only
			currentPercent += sizes[i];												// in case of LXN-file download, IGC download
			HandleWaitDialogUpdate(UPDATEDIALOG, 100, currentPercent/percent, "%");	// has its own progress calculation
		}

		serial_out(LX_STX);
		serial_out(LX_FLB0 + i);

		result = COLReadBlock(&buffer[offset], sizes[i], 30);		// receive flight blocks from colibri, 30sec timeout

		if (!result) return 0;
		
		offset += sizes[i];
		
		i++;
		if (i < 16)
			done = (sizes[i] == 0);

	} while(!done && (i < 16));

	return blockSize;
}
//***********************************************************************************************

/**
* \brief Function for downloading the decrypt block at the end of the flight-log download
* \param buffer	- pointer to the buffer where the data can be stored after reception
* \return true - successful decrypt block download
* false	- unsuccessful download
*/
Boolean COLGetDecryptBlock(UInt8 *buffer)
{
	Boolean			result;

	serial_out(LX_STX);
	serial_out(LX_DECRYPT);

	result = COLReadBlock(buffer, 256, 30);					// download decrypt block from colibri, 30sec timeout
	
	return result;
}
//***********************************************************************************************

/**
* \brief Function for storing the received flight header data into the given index of the cailogdata array
* Uses global variable array 'cailogdata'
* \param buffer - pointer to the buffer where the received flight header is stored
* \param ind - index of the 'cailogdata' array, where the flight details should be extracted
*/
void COLStoreFlightHeader(UInt8* buffer, UInt16 ind)
{
	cailogdata[ind].index = ind;

	cailogdata[ind].startDate.day = ((buffer[9] - 48) * 10) + (buffer[10] - 48);			// convert ASCII to integers
	cailogdata[ind].startDate.month = ((buffer[12] - 48) * 10) + (buffer[13] - 48);
	cailogdata[ind].startDate.year = ((buffer[15] - 48) * 10) + (buffer[16] - 48);

	if (buffer[18] == 0x20)											// hour starts with space
		cailogdata[ind].startTime.hour = (Int16) (buffer[19] - 48);
	else
		cailogdata[ind].startTime.hour = (Int16) (((buffer[18] - 48) * 10) + (buffer[19] - 48));
	cailogdata[ind].startTime.min = (Int16) (((buffer[21] - 48) * 10) + (buffer[22] - 48));		// convert ASCII to integers
	cailogdata[ind].startTime.sec = (Int16) (((buffer[24] - 48) * 10) + (buffer[25] - 48));

//	cailogdata[ind].endDate;
						
	if (buffer[27] == 0x20)											// hour starts with space
		cailogdata[ind].endTime.hour = (Int16) (buffer[28] - 48);
	else
		cailogdata[ind].endTime.hour = (Int16) (((buffer[27] - 48) * 10) + (buffer[28] - 48));
	cailogdata[ind].endTime.min = (Int16) (((buffer[30] - 48) * 10) + (buffer[31] - 48));		// convert ASCII to integers
	cailogdata[ind].endTime.sec = (Int16) (((buffer[33] - 48) * 10) + (buffer[34] - 48));

	StrNCopy(cailogdata[ind].pilotName, &buffer[40], 24);
	cailogdata[ind].FlightOfDay = (Int16) buffer[94];
	
	cailogdata[ind].StartTape = (((UInt16) buffer[1]) * 256) + buffer[2];
	cailogdata[ind].StartPage = buffer[4];
	cailogdata[ind].EndTape = (((UInt16) buffer[5]) * 256) + buffer[6];
	cailogdata[ind].EndPage = buffer[8];
	
	cailogdata[ind].SerialNo = (((UInt16) buffer[91]) * 256) + buffer[92];
}
//***********************************************************************************************

/**
* \brief Function for downloading the memory information of the Colibri (or other LX-device)
* \return true - successful memory info download
* false	- unsuccessful download
*/
Boolean COLGetMemInfo()
{
	UInt8		*tempBuffer;
	Boolean		valid = false;
	
	tempBuffer = MemPtrNew(6);

	serial_out(LX_STX);
	serial_out(LX_MEMINFO);

	valid = COLReadBlock(tempBuffer, 6, 4);						// read 6 bytes from the serial port
	if (valid)
	{
		MeminfoP->startpos = (tempBuffer[0] * 256) + tempBuffer[1];
		MeminfoP->startpage = tempBuffer[2];
		MeminfoP->endpos = (tempBuffer[3] * 256) + tempBuffer[4];
		MeminfoP->endpage = tempBuffer[5];
	}
	else
	{
		MeminfoP->startpos = 0;
		MeminfoP->startpage = 8;
		MeminfoP->endpos = (UInt16) 32768;
		MeminfoP->endpage = 15;
	}
	MemPtrFree(tempBuffer);
	return valid;
}

/**
* \brief Function for downloading one flight log in binary-format
* \return true - successful flight download
* false	- unsuccessful flight download
*/
Boolean COLDownloadFlightLXN()
{
	Boolean		result = false;
	UInt8		*buffer;
	UInt32		rcvBlockSize;

	buffer = MemPtrNew(FLIGHTBLOCKSIZE);
	if (buffer)
	{
		do
		{
			if (COLSync())
			{
				StartPos += FLIGHTBLOCKSIZE;
				rcvBlockSize = COLGetFlightBlock(StartPos - FLIGHTBLOCKSIZE, (StartPos<EndPos?StartPos:EndPos), buffer);
				if (rcvBlockSize)
				{
					result = HandleVFSData(IOWRITE, buffer, &rcvBlockSize);
				}
				else {
					result = false;		// problem during downloading the block
				}
			}
			else {
				// didn't sync
				result = false;
			}

		} while ((StartPos < EndPos) && result);

		MemPtrFree(buffer);
	}
	// succesful so far?
	if (result)
	{
		buffer = MemPtrNew(256);
		rcvBlockSize = 256;
		if (buffer)
		{
			if (COLSync())
			{
				result = COLGetDecryptBlock(buffer);							// get decrypt block
				if (result)
				{
					HandleWaitDialogUpdate(UPDATEDIALOG, 100, 100, "%");
					result = HandleVFSData(IOWRITE, buffer, &rcvBlockSize);		// output of the received data to file
				}
			}
			else {
				result = false;		// synching with the logger failed
			}
			MemPtrFree(buffer);
		}
		else {
			result = false;
		}
	}

	return result;
}
//***********************************************************************************************

/**
* \brief Function for downloading one flight log in IGC-format
* \return true - successful flight download
* false	- unsuccessful flight download
*/
Boolean COLDownloadFlightIGC()
{
	Boolean			retVal, done = false, result = false;
	UInt8			igcBuffer[256];
	UInt8			decryptBlock[256];
	UInt8			i, h, fixExtNum = 0, extNum = 0, event = 0;
	UInt8			extendData[32];
	Int16			ftab[16], etab[16];
	UInt16			extData, g, previousPercent = 0, currentPercent = 0;
	UInt32			j, k;
	UInt32			percent = (EndPos - OrigStartPos)/100;
	Int32			lat, lon;
	Char			textBuffer[400];
	LX_flightInfo	flightInfo;
	LX_fixData		fixData;
	LX_originData	origData;
	LX_headerInfo	headerData;

	CurrentPos = 0;			// first position of the log
	
	if (COLSync())
	{
		if (!COLGetDecryptBlock(decryptBlock)) return false;	// get decrypt block
	}
	else
		return false;
	
	fixData.st = 0;
	fixData.lat = 0;
	fixData.lon = 0;
	fixData.t = 0;
	fixData.dt = 0;
	fixData.aa = 0;
	fixData.ga = 0;
	fixData.enl = 0;

	origData.flag = 0;
	origData.time = 0;
	origData.lat = 0;
	origData.lon = 0;
	
	while(!done)
	{
		if (!COLGetBinaryDataForIGC(&i, 1))		// read 1 byte from the LXN-file
		{
			done = true;
			result = false;
			break;
		}
		igcBuffer[0] = i;
		
		currentPercent = CurrentPos/percent;
		if (currentPercent > previousPercent)
		{
			HandleWaitDialogUpdate(UPDATEDIALOG, 100, currentPercent, "%");
			previousPercent = currentPercent;
		}

		if (!i)
		{
			j = 0;
			retVal = COLGetBinaryDataForIGC(&i, 1);
			while ((i == 0) && retVal)
			{
				j++;
				retVal = COLGetBinaryDataForIGC(&i, 1);
			}

			if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//			if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
				StrPrintF(textBuffer, "LLXNEMPTY%ld\r\n", j);			// LX navigation L-record
			else
				StrPrintF(textBuffer, "LFILEMPTY%ld\r\n", j);			// Filser L-record
			
			if (!TxData(textBuffer, data.config.xfertype))
			{
				done = true;
				result = false;
				break;
			}
		}

		if(i <= LX_MAX_LSTRING)
		{
			if (!COLGetBinaryDataForIGC(igcBuffer, i))
			{
				done = true;
				result = false;
				break;
			}
			igcBuffer[i] = 0;

			// check for logger ID, then decide if older LX version
//			if (igcBuffer contains "HFFTYFRTYPE")
//			{
//				if (igcBuffer contains "FILSER")
//				or (ignBuffer contains "LX5")
//				{
//					LXold = true;               	
//				}
//			}

			StrPrintF(textBuffer, "%s\r\n", igcBuffer);
			if (!TxData(textBuffer, data.config.xfertype))
			{
				done = true;
				result = false;
				break;
			}
		}
		else switch(i)
		{
			case LX_START:
				if (!COLGetBinaryDataForIGC(igcBuffer, 9))		// struct s_start (L = 10)
				{
					done = true;
					result = false;
					break;
				}
				headerData.flt_nr = igcBuffer[8];
				break;
				
			case LX_DATUM:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 4))	// struct s_datum (L = 5)
				{
					done = true;
					result = false;
					break;
				}
				headerData.day = igcBuffer[1];
				headerData.month = igcBuffer[2];
				headerData.year = (igcBuffer[3] * 256) + igcBuffer[4];
				break;
				
			case LX_SER_NR:
				if (!COLGetBinaryDataForIGC(igcBuffer, 9))		// struct s_start (L = 10)
				{
					done = true;
					result = false;
					break;
				}
				headerData.ser_nr = StrAToI(&igcBuffer[3]);
				StrPrintF(textBuffer, "A%sFLIGHT:%d\r\n", igcBuffer, headerData.flt_nr);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFDTE%02d%02d%02d\r\n", headerData.day, headerData.month, headerData.year%100);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_FLIGHT_INFO:
				if (!COLGetBinaryDataForIGC((UInt8 *) &flightInfo, sizeof(LX_flightInfo)))	// struct LX_flightInfo
				{
					done = true;
					result = false;
					break;
				}
				if (flightInfo.razred == 7) break;

				StrPrintF(textBuffer, "HFFXA%03d\r\n", flightInfo.fix_accuracy);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFPLTPILOT:%s\r\n", flightInfo.pilot);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGTYGLIDERTYPE:%s\r\n", flightInfo.glider);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGIDGLIDERID:%s\r\n", flightInfo.reg_num);
				if (flightInfo.gpsdatum != 255)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "HFDTM%03dGPSDATUM:%s\r\n", flightInfo.gpsdatum, Table_GPSdatum[flightInfo.gpsdatum]);
				}
				else
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "HFDTM%03dGPSDATUM:UNKNOWN\r\n", flightInfo.gpsdatum);
				}
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFCIDCOMPETITIONID:%s\r\n", flightInfo.cmp_num);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFCCLCOMPETITIONCLASS:%s\r\n", Competition_Class[flightInfo.razred]);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGPSGPS:%s\r\n", flightInfo.gps);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_COMPETITION_CLASS:
				if (!COLGetBinaryDataForIGC(igcBuffer, 9))
				{
					done = true;
					result = false;
					break;
				}
				if (flightInfo.razred != 7) break;

				StrPrintF(textBuffer, "HFFXA%03d\r\n", flightInfo.fix_accuracy);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFPLTPILOT:%s\r\n", flightInfo.pilot);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGTYGLIDERTYPE:%s\r\n", flightInfo.glider);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGIDGLIDERID:%s\r\n", flightInfo.reg_num);
				if (flightInfo.gpsdatum != 255)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "HFDTM%03dGPSDATUM:%s\r\n", flightInfo.gpsdatum, Table_GPSdatum[flightInfo.gpsdatum]);
				}
				else
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "HFDTM%03dGPSDATUM:UNKNOWN\r\n", flightInfo.gpsdatum);
				}
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFCIDCOMPETITIONID:%s\r\n", flightInfo.cmp_num);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFCCLCOMPETITIONCLASS:%s\r\n", igcBuffer);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFGPSGPS:%s\r\n", flightInfo.gps);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_SHVERSION:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 2))		// struct s_version (L = 3)
				{
					done = true;
					result = false;
					break;
				}
				StrPrintF(textBuffer, "HFRFWFIRMWAREVERSION:%1d.%1d\r\n", igcBuffer[2]/10, igcBuffer[2]%10);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "HFRHWHARDWAREVERSION:%1d.%1d\r\n", igcBuffer[1]/10, igcBuffer[1]%10);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_FIXEXT_INFO:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 4))		// struct s_extend (L = 5)
				{
					done = true;
					result = false;
					break;
				}
				extData = ((igcBuffer[3] * 256) + igcBuffer[4]);
				fixExtNum = 0;
				for(j = 1; j < 65535; j *= 2) if (extData & j) fixExtNum++;
				
				if (fixExtNum > 0)
					StrPrintF(textBuffer, "I%02d", fixExtNum);
				else
					break;
				
				i = 36;
				j = 0;
				if (extData & LX_FXA)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dFXA", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_VXA)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVXA", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_RPM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dRPM", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_GSP)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dGSP", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_IAS)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dIAS", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_TAS)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTAS", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_HDM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dHDM", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_HDT)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dHDT", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_TRM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTRM", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_TRT)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTRT", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_TEN)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTEN", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_WDI)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dWDI", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_WVE)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dWVE", i, i+4);
					i += 5;
					ftab[j] = 5;
					j++;
				}
				if (extData & LX_ENL)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dENL", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				if (extData & LX_VAR)
				{
					if (data.config.flightcomp == LXCOMP) {
						StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVAR", i, i+2);
						ftab[j] = 3;
					} else {
						StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVAR", i, i+4);
						ftab[j] = 5;
					}
					i += 3;
					j++;
				}
				if (extData & LX_XX3)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dXX3", i, i+2);
					i += 3;
					ftab[j] = 3;
					j++;
				}
				
				StrPrintF(&textBuffer[StrLen(textBuffer)], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_EXTEND_INFO:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 4))		// struct s_extend (L = 5)
				{
					done = true;
					result = false;
					break;
				}
				extData = ((igcBuffer[3] * 256) + igcBuffer[4]);
				extNum = 0;
				for(j = 1; j < 65535; j *= 2) if (extData & j) extNum++;
				
				if (extNum > 0)
					StrPrintF(textBuffer, "J%02d", extNum);
				else
					break;

				i = 8;
				j = 0;
				if (extData & LX_FXA)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dFXA", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_VXA)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVXA", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_RPM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dRPM", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_GSP)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dGSP", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_IAS)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dIAS", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_TAS)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTAS", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_HDM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dHDM", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_HDT)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dHDT", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_TRM)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTRM", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_TRT)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTRT", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_TEN)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dTEN", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_WDI)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dWDI", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_WVE)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dWVE", i, i+4);
					i += 5;
					etab[j] = 5;
					j++;
				}
				if (extData & LX_ENL)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dENL", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				if (extData & LX_VAR)
				{
					if (data.config.flightcomp == LXCOMP) {
						StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVAR", i, i+2);
						etab[j] = 3;
					} else {
						StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dVAR", i, i+4);
						etab[j] = 5;
					}
					i += 3;
					j++;
				}
				if (extData & LX_XX3)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02dXX3", i, i+2);
					i += 3;
					etab[j] = 3;
					j++;
				}
				
				StrPrintF(&textBuffer[StrLen(textBuffer)], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_FIXEXT:
				textBuffer[0] = 0;		// for returning correct length (zero) at the first time
				if (!COLGetBinaryDataForIGC(extendData, fixExtNum*2))
				{
					done = true;
					result = false;
					break;
				}
				for(i = 0; i < fixExtNum; i++)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%0*u", ftab[i], (extendData[2*i] * 256) + extendData[(2*i)+1]);
// does this need to change to "%0*d" for the VAR record????
				}

				StrPrintF(&textBuffer[StrLen(textBuffer)], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_EXTEND:
				if (!COLGetBinaryDataForIGC(&h, 1))
				{
					done = true;
					result = false;
					break;
				}
				k = ((Int32) h) + fixData.t;
				if (!COLGetBinaryDataForIGC(extendData, extNum*2))
				{
					done = true;
					result = false;
					break;
				}
				StrPrintF(textBuffer, "K%02ld%02ld%02ld", k/3600, (k%3600)/60, (k%60));

				for(i = 0; i < extNum; i++)
				{
					StrPrintF(&textBuffer[StrLen(textBuffer)], "%0*u", etab[i], (extendData[2*i] * 256) + extendData[(2*i)+1]);
				}
				StrPrintF(&textBuffer[StrLen(textBuffer)], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_TASK:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 229))	// struct s_task (L = 230)
				{
					done = true;
					result = false;
					break;
				}
				MemMove(&k, &igcBuffer[1], 4);

				StrPrintF(textBuffer, "C%02d%02d%02d", igcBuffer[5], igcBuffer[6], igcBuffer[7]);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "%02ld%02ld%02ld", k/3600, (k%3600)/60, k%60);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "%02d%02d%02d", igcBuffer[8], igcBuffer[9], igcBuffer[10]);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "%04d%02d\r\n", ((igcBuffer[11] * 256) + igcBuffer[12]), (Int8)igcBuffer[13]);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
					break;
				}
				
				for(i = 0; i < 12; i++)				// max. number of turnpoints
					if (igcBuffer[14 + i])
					{
						MemMove(&lon, &igcBuffer[26 + (i * 4)], 4);
						MemMove(&lat, &igcBuffer[74 + (i * 4)], 4);

						if (lat >= 0)
							textBuffer[399] = 'N';
						else
							textBuffer[399] = 'S';
						
						if (lon >= 0)
							textBuffer[398] = 'E';
						else
							textBuffer[398] = 'W';

						StrPrintF(textBuffer, "C%02ld%05ld%c",
										lat >= 0 ? (lat/60000):((-1) * (lat/60000)),
										lat >= 0 ? (lat%60000):((-1) * (lat%60000)),
										textBuffer[399]);

						StrPrintF(&textBuffer[StrLen(textBuffer)], "%03ld%05ld%c",
										lon >= 0 ? (lon/60000):((-1) * (lon/60000)),
										lon >= 0 ? (lon%60000):((-1) * (lon%60000)),
										textBuffer[398]);

						StrPrintF(&textBuffer[StrLen(textBuffer)], "%s\r\n", &igcBuffer[122 + (i * 9)]);
						if (!TxData(textBuffer, data.config.xfertype))
						{
							done = true;
							result = false;
							break;
						}
					}
				break;
			
			case LX_EVENT:
				if (!COLGetBinaryDataForIGC(igcBuffer, 9))		// struct s_start (L = 10)
				{
					done = true;
					result = false;
					break;
				}
				igcBuffer[9] = 0;
				event = 1;
				break;
			
			case LX_POSITION_OK:
			case LX_POSITION_BAD:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 10))		// struct s_pos (L = 11)
				{
					done = true;
					result = false;
					break;
				}
				fixData.lat = ((igcBuffer[3] * 256) + igcBuffer[4]) + origData.lat;		// s_pos.lat + orig.lat
				fixData.lon = ((igcBuffer[5] * 256) + igcBuffer[6]) + origData.lon;		// s_pos.lon + orig.lon
				fixData.t = ((igcBuffer[1] * 256) + igcBuffer[2]) + origData.time;		// s_pos.ura + orig.time
				MemMove(&fixData.aa, &igcBuffer[7], 2);									// s_pos.aalt
				MemMove(&fixData.ga, &igcBuffer[9], 2);									// s_pos.galt
				fixData.st = (i == LX_POSITION_OK?'A':'V');
				
				if (event)
				{
					StrPrintF(textBuffer, "E%02ld%02ld%02ld%s\r\n", fixData.t/3600, (fixData.t%3600)/60, (fixData.t%60), igcBuffer);
					if (!TxData(textBuffer, data.config.xfertype))
					{
						done = true;
						result = false;
						break;
					}
					event = 0;
				}
				
				if (fixData.t < 0) event = 0;

				if (fixData.lat >= 0)
					textBuffer[399] = 'N';
				else
					textBuffer[399] = 'S';
				
				if (fixData.lon >= 0)
					textBuffer[398] = 'E';
				else
					textBuffer[398] = 'W';

				StrPrintF(textBuffer, "B%02ld%02ld%02ld", fixData.t/3600, (fixData.t%3600)/60, fixData.t%60);
				StrPrintF(&textBuffer[7], "%02ld%05ld%c",
										fixData.lat >= 0 ? (fixData.lat/60000):((-1) * (fixData.lat/60000)),
										fixData.lat >= 0 ? (fixData.lat%60000):((-1) * (fixData.lat%60000)),
										textBuffer[399]);
				StrPrintF(&textBuffer[15], "%03ld%05ld%c%c",
										fixData.lon >= 0 ? (fixData.lon/60000):((-1) * (fixData.lon/60000)),
										fixData.lon >= 0 ? (fixData.lon%60000):((-1) * (fixData.lon%60000)),
										textBuffer[398],
										fixData.st);
				
				if (fixData.aa < 0)
					StrPrintF(&textBuffer[25], "-%04d", ((-1) * fixData.aa));
				else
					StrPrintF(&textBuffer[25], "%05d", fixData.aa);
				
				if (fixData.ga < 0)
					StrPrintF(&textBuffer[30], "-%04d", ((-1) * fixData.ga));
				else
					StrPrintF(&textBuffer[30], "%05d", fixData.ga);
				
				if (!fixExtNum) StrPrintF(&textBuffer[35], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_ORIGIN:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 12))		// struct s_orig (L = 13)
				{
					done = true;
					result = false;
					break;
				}
				MemMove(&origData.lat, &igcBuffer[5], 4);
				MemMove(&origData.lon, &igcBuffer[9], 4);
				MemMove(&origData.time, &igcBuffer[1], 4);
				
				if (origData.lat >= 0)
					textBuffer[399] = 'N';
				else
					textBuffer[399] = 'S';
				
				if (origData.lon >= 0)
					textBuffer[398] = 'E';
				else
					textBuffer[398] = 'W';

				if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//				if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
					StrPrintF(textBuffer, "LLXNORIGIN%02ld%02ld%02ld", origData.time/3600, (origData.time%3600)/60, origData.time%60);
				else
					StrPrintF(textBuffer, "LFILORIGIN%02ld%02ld%02ld", origData.time/3600, (origData.time%3600)/60, origData.time%60);

				StrPrintF(&textBuffer[StrLen(textBuffer)], "%02ld%05ld%c",
										origData.lat >= 0 ? (origData.lat/60000):((-1) * (origData.lat/60000)),
										origData.lat >= 0 ? (origData.lat%60000):((-1) * (origData.lat%60000)),
										textBuffer[399]);
				StrPrintF(&textBuffer[StrLen(textBuffer)], "%03ld%05ld%c\r\n",
										origData.lon >= 0 ? (origData.lon/60000):((-1) * (origData.lon/60000)),
										origData.lon >= 0 ? (origData.lon%60000):((-1) * (origData.lon%60000)),
										textBuffer[398]);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_END:
				done = true;
				result = true;
				break;
			
			case LX_SECURITY_OLD:
				if (!COLGetBinaryDataForIGC(igcBuffer, 22))
				{
					done = true;
					result = false;
					break;
				}
				StrPrintF(textBuffer, "G%22.22s\r\n", igcBuffer);
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			case LX_SECURITY:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], 66))		// struct s_security (L = 67)
				{
					done = true;
					result = false;
					break;
				}
				switch (igcBuffer[2])
				{
					case LX_HIGH_SECURITY:	StrPrintF(textBuffer, "G2"); break;
					case LX_MED_SECURITY:	StrPrintF(textBuffer, "G1"); break;
					case LX_LOW_SECURITY:	StrPrintF(textBuffer, "G0"); break;
				}
				for(i = 0; i < igcBuffer[1]; i++)
				{
					g = StrLen(textBuffer);
					StrPrintF(&textBuffer[g], "%x", igcBuffer[3+i]);
					textBuffer[g] = textBuffer[g+2];					// get rid of the 2 zeros at the beginning
					textBuffer[g+1] = textBuffer[g+3];
					textBuffer[g+2] = textBuffer[g+4];
				}
				StrPrintF(&textBuffer[StrLen(textBuffer)], "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
			
			default:
				if (!COLGetBinaryDataForIGC(&igcBuffer[1], decryptBlock[i]))
				{
					done = true;
					result = false;
					break;
				}
				if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//				if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
					StrPrintF(textBuffer, "LLXNUNKNOWN");
				else
					StrPrintF(textBuffer, "LFILUNKNOWN");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
					break;
				}
				for(i = 0; i < decryptBlock[i]+1; i++)
				{
					if (!(i%20) && i)
					{
						if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP))
//						if (!LXold && ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)))
							StrPrintF(textBuffer, "\r\nLLXNUNKNOWN");
						else
							StrPrintF(textBuffer, "\r\nLFILUNKNOWN");
						if (!TxData(textBuffer, data.config.xfertype))
						{
							done = true;
							result = false;
							break;
						}
					}
					StrPrintF(textBuffer, "%02x", igcBuffer[i]);
					if (!TxData(textBuffer, data.config.xfertype))
					{
						done = true;
						result = false;
						break;
					}
				}
				StrPrintF(textBuffer, "\r\n");
				if (!TxData(textBuffer, data.config.xfertype))
				{
					done = true;
					result = false;
				}
				break;
		}
	}
	
	if (LogBuffer) MemPtrFree(LogBuffer);
	if (result)
	{
		textBuffer[0] = 0;
		result = TxData(textBuffer, data.config.xfertype);	// write the remaining part of the buffer into the output file
	}
	return result;
}
//***********************************************************************************************

/**
* \brief Function for receiving a binary data block from the logger
* \param data - pointer to the buffer where the received data is to be stored
* \param numBytes - number of requested bytes
* \return true - successful read
* false	- unsuccessful read
*/
Boolean COLGetBinaryDataForIGC(UInt8 *data, UInt32 numBytes)
{
	Boolean		retVal;
	UInt16		rcvdBytes;
	UInt32		blockEnd;

	if (!CurrentPos)
	{									// the first call of the function
		LogBuffer = MemPtrNew(FLIGHTBLOCKSIZE);
		if (LogBuffer)
		{
			if (COLSync())
			{
				blockEnd = StartPos + FLIGHTBLOCKSIZE;
				rcvdBytes = COLGetFlightBlock(blockEnd - FLIGHTBLOCKSIZE, (blockEnd<EndPos?blockEnd:EndPos), LogBuffer);
				if (rcvdBytes)
				{
					if (rcvdBytes >= numBytes)
					{									// enough bytes read from the logger
						MemMove(data, LogBuffer, numBytes);
						CurrentPos = numBytes;
						RemainInBlock = rcvdBytes - numBytes;
						PosInBlock = numBytes;
						retVal = true;
					}
					else retVal = false;	// not enough bytes read from the logger
				}
				else retVal = false;		// problem during downloading the block
			}
			else retVal = false;			// synching with logger failed
		}
		else retVal = false;				// not enough memory to allocate buffer
	}
	else
	{
		if ((StartPos + CurrentPos + 1) <= EndPos)
		{
			if (RemainInBlock >= numBytes)
			{								// enough bytes in the previously downloaded block, no need to get the next block
				MemMove(data, &LogBuffer[PosInBlock], numBytes);
				CurrentPos += numBytes;
				RemainInBlock -= numBytes;
				PosInBlock += numBytes;
				retVal = true;
			}
			else
			{								// not enough bytes in the previous block, need to download a new block from the logger
				if (COLSync())
				{
					blockEnd = StartPos + CurrentPos + FLIGHTBLOCKSIZE;
					rcvdBytes = COLGetFlightBlock(blockEnd - FLIGHTBLOCKSIZE, (blockEnd<EndPos?blockEnd:EndPos), LogBuffer);
					if (rcvdBytes)
					{
						if (rcvdBytes >= numBytes)
						{									// enough bytes read from the logger
							MemMove(data, LogBuffer, numBytes);
							CurrentPos += numBytes;
							RemainInBlock = rcvdBytes - numBytes;
							PosInBlock = numBytes;
							retVal = true;
						}
						else {
							retVal = false;	// not enough bytes read from the logger
						}
					}
					else {
						retVal = false;		// problem during downloading the block
					}
				}
				else {
					retVal = false;			// synching with logger failed
				}
			}
		}
		else {
			retVal = false;					// already reached the end of log
		}
	}
	
	if ((!retVal) || ((StartPos + CurrentPos + 1) > EndPos))
	{														// end of log or fault occured
		if (LogBuffer)
		{
			MemPtrFree(LogBuffer);
			LogBuffer = NULL;
		}
	}
	return retVal;
}

