#include <PalmOS.h>
#include "soaring.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarIO.h"
#include "soarForm.h"
#include "soarPLst.h"
#include "soarTask.h"
#include "soarSUA.h"
#include "soarMem.h"
#include "soarCAI.h"
#include "soarWay.h"
#include "soarWind.h"
#include "soarComp.h"
#include "soarEW.h"
#include "soarMap.h"
#include "soarRECO.h"

DmOpenRef config_db;
DmOpenRef logger_db;
DmOpenRef flight_db;
DmOpenRef waypoint_db;
DmOpenRef polar_db;
DmOpenRef task_db;
DmOpenRef suaidx_db;
DmOpenRef suadata_db;
DmOpenRef terrain_db;
DmOpenRef memo_db;
DmOpenRef doc_db;
DmOpenRef sim_db;

extern PolarData *inusePolar;
extern Int16     selectedPolarIndex;
extern PolarData *selectedPolar;
extern RECOData  *recodata;
extern EWMRData *ewmrdata;
extern Char *filelist;
extern UInt16 numfilesfound;

Char NameString[26];

/*****************************************************************************
 * OpenCreateDB - open a database. If it doesn't exist, create it.
 *****************************************************************************/
Int16 OpenCreateDB(DmOpenRef *dbP, 
						UInt16 cardNo, 
						Char* name, 
						UInt32 dbcreator, 
						UInt32 type, 
						UInt16 mode)
{
	if ((*dbP=DmOpenDatabaseByTypeCreator(type,dbcreator,mode)) == 0) {
		DmCreateDatabase(cardNo,name,dbcreator,type,false);
		if ((*dbP=DmOpenDatabaseByTypeCreator(type,dbcreator,mode)) == 0) {
			return (DB_OPEN_FAIL);
		} else {
			return (DB_OPEN_CREATE);
		}
	}
	return (DB_OPEN_SUCCESS);
}

/*****************************************************************************
 * OpenCreateDB2 - open a database and get DB version number.
 *****************************************************************************/
Int16 OpenCreateDB2(DmOpenRef *dbP, 
						UInt16 cardNo, 
						Char *name, 
						UInt32 dbcreator, 
						UInt32 type, 
						UInt16 mode,
						UInt16 curver) 
{
	LocalID   dbID;
	UInt16    dbver;
	MemHandle aih;
	LocalID   ai;

//	HostTraceOutputTL(appErrorClass, "soarDB:About to call DmFindDatabase-name=|%s|", name);
	if ((dbID = DmFindDatabase(cardNo, name)) == 0) {
		// If the database isn't found - Create it.
		DmCreateDatabase(cardNo, name, dbcreator, type, false);
		// Then find the new database to get the dbID
		if ((dbID = DmFindDatabase(cardNo, name)) == 0) {
			return (DB_OPEN_FAIL);
		} else {
			// Then open the new database with the dbID
			if ((*dbP=DmOpenDatabase(cardNo, dbID, mode)) == 0) {
				return (DB_OPEN_FAIL);
			} else {
				// If the newly created database is opened - Set the DB Info
				DmOpenDatabaseInfo(*dbP, &dbID, NULL, NULL, NULL, NULL);
				DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &ai, NULL, NULL, NULL);
				if (ai == 0) {
					aih = DmNewHandle(*dbP, 64);
		
					ai = MemHandleToLocalID(aih);
					DmSetDatabaseInfo(cardNo,dbID,NULL,NULL,&curver,NULL,NULL,NULL,NULL,&ai,NULL,NULL,NULL);
//					HostTraceOutputTL(appErrorClass, "soarDB:%s-curver create |%hu|", name, curver);
				}
				return (DB_OPEN_CREATE);
			}
		}
	} else {
		// If the database is found - Open it.
		*dbP = DmOpenDatabase(cardNo, dbID, mode);
		// Get the DB Info
		DmDatabaseInfo(cardNo,dbID,NULL,NULL,&dbver,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
//		HostTraceOutputTL(appErrorClass, "soarDB:%s-curver|%hu| dbver|%hu|", name, curver, dbver);
		if (curver != dbver) {
			// If it is a different version of the database, clear it and start over
			FrmCustomAlert(WarningAlert, name, "Database Reset", " ");
			// Close the current database
			DmCloseDatabase(*dbP);
			// Delete the database
			DmDeleteDatabase(cardNo, dbID);
			// Create a new version of it
			DmCreateDatabase(cardNo, name, dbcreator, type, false);
			// Then find the new database to get the dbID
			if ((dbID = DmFindDatabase(cardNo, name)) == 0) {
				return (DB_OPEN_FAIL);
			} else {
				// Then open the new database with the dbID
				if ((*dbP=DmOpenDatabase(cardNo, dbID, mode)) == 0) {
					return (DB_OPEN_FAIL);
				} else {
					// If the newly created database is opened - Set the DB Info
					DmOpenDatabaseInfo(*dbP, &dbID, NULL, NULL, NULL, NULL);
					DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &ai, NULL, NULL, NULL);
					if (ai == 0) {
						aih = DmNewHandle(*dbP, 64);
			
						ai = MemHandleToLocalID(aih);
						DmSetDatabaseInfo(cardNo,dbID,NULL,NULL,&curver,NULL,NULL,NULL,NULL,&ai,NULL,NULL,NULL);
//						HostTraceOutputTL(appErrorClass, "soarDB:%s-curver create |%hu|", name, curver);
					}
					return (DB_OPEN_CREATE);
				}
			}
		}
		return (DB_OPEN_SUCCESS);
	}
}

/*****************************************************************************
 * OpenDeleteDB - delete an open.
 *****************************************************************************/
Boolean OpenDeleteDB(DmOpenRef *dbP, UInt16 cardNo, Char *name) 
{
	LocalID   dbID;

	if ((dbID = DmFindDatabase(cardNo, name)) != 0) {
		// Close the current database
		DmCloseDatabase(*dbP);
		// Delete the database
		DmDeleteDatabase(cardNo, dbID);
		return (true);
	} else { 
		return (false);
	}
}

/*****************************************************************************
 * OpenDBCountRecords
 *****************************************************************************/
UInt16 OpenDBCountRecords(DmOpenRef	openDBDB)
{
	if (openDBDB == 0)
		return(NO_RECORD);
	else
		return(DmNumRecords(openDBDB));
}



/*****************************************************************************
 * OpenDBAddRecord - add a record in the open db at the start or
 *		end of the db (according to atStart)
 *****************************************************************************/
UInt16 OpenDBAddRecord(DmOpenRef		openDBDB,
						Int32		size,
						MemPtr		openDBP,
						Boolean		atStart)
{
	MemHandle	recH;
	UInt8		*recP;
	Err			err;
	UInt16		dbIndex = 0;

	if ((dbIndex = OpenDBNewRecord(openDBDB, size, &recH, (MemPtr)&recP, atStart))
			== NO_RECORD)
		return NO_RECORD;

	// write the data
	err = DmWrite(recP, 0, openDBP, size);

	// release record
	OpenDBReleaseRecord(openDBDB, dbIndex, &recH, (MemPtr)&recP, !err);

	// delete on failure
	if (err) {
		OpenDBDeleteRecord(openDBDB, dbIndex);
		return(NO_RECORD);
	} else {
//		HostTraceOutputTL(appErrorClass, "soarDB:dbIndex:|%hu|", dbIndex);
		return(dbIndex);
	}
}



/*****************************************************************************
 * OpenDBNewRecord - create a new record in the open db at the start or
 *		end of the db (according to atStart)
 *****************************************************************************/
UInt16 OpenDBNewRecord(DmOpenRef		openDBDB,
						Int32		size,
						MemHandle	*openDBH,
						MemPtr		*openDBP,
						Boolean		atStart)
{
	UInt16	dbIndex = 0;

	// check for an open db
 	if (openDBDB == 0)
		return false;

	if (atStart)
		dbIndex = 0;
	else
		dbIndex = DmNumRecords(openDBDB);

	*openDBP = NULL;

	// create and write a new record
	if ((*openDBH = DmNewRecord(openDBDB, &dbIndex, size)) == NULL ||
			(*openDBP = MemHandleLock(*openDBH)) == NULL) {

		// couldn't create properly, back out
		OpenDBReleaseRecord(openDBDB, dbIndex, openDBH, openDBP, false);

		return NO_RECORD;
	}

	return dbIndex;
}



/*****************************************************************************
 * OpenDBGetRecord - get a record from the OpenDB
 *****************************************************************************/
Boolean OpenDBGetRecord(DmOpenRef	openDBDB,
						Int16			index,
						MemHandle	*openDBH,
						MemPtr		*openDBP)
{
	// init the ptr up front
	*openDBP = NULL;

	// check for an open db
 	if (openDBDB == 0)
		return false;

	// check for bad index
	if (index >= DmNumRecords(openDBDB))
		return false;

	// get the record
	if ((*openDBH = DmGetRecord(openDBDB, index)) == NULL ||
			(*openDBP = MemHandleLock(*openDBH)) == NULL) {

		// couldn't get properly, back out
		OpenDBReleaseRecord(openDBDB, index, openDBH, openDBP, false);

		return false;
	}

	return true;
}



/*****************************************************************************
 * OpenDBQueryRecord - get a read-only record from the OpenDB
 *****************************************************************************/
Boolean OpenDBQueryRecord(DmOpenRef	openDBDB,
						Int16			index,
						MemHandle	*openDBH,
						MemPtr		*openDBP)
{
	// init the ptr up front
	*openDBP = NULL;

	// check for an open db
 	if (openDBDB == 0)
		return(false);

	// check for bad index
	if (index >= DmNumRecords(openDBDB))
		return(false);

	// get the record
	if ((*openDBH = DmQueryRecord(openDBDB, index)) == NULL ||
			(*openDBP = MemHandleLock(*openDBH)) == NULL) {

		// couldn't get properly, back out
		OpenDBReleaseRecord(openDBDB, index, openDBH, openDBP, false);

		return(false);
	}

	return(true);
}



/*****************************************************************************
 * OpenDBReleaseRecord - release a record from the openDB db.  
 *****************************************************************************/
Boolean OpenDBReleaseRecord(DmOpenRef	openDBDB,
							Int16			index,
							MemHandle	*openDBH,
							MemPtr		*openDBP,
							Boolean		save)
{
	// check for an open db
 	if (openDBDB == 0)
		return false;

	// check for bad index
	if (index >= DmNumRecords(openDBDB))
		return false;

	// unlock first
	if (*openDBP)
		MemHandleUnlock(*openDBH);

	// release the record and close the database
	DmReleaseRecord(openDBDB, index, save);

	// clean up
	*openDBP = NULL;
	*openDBH = NULL;

	return true;
}



/*****************************************************************************
 * OpenDBDeleteRecord - delete a record from the openDB db.  
 *****************************************************************************/
Boolean OpenDBDeleteRecord(DmOpenRef	openDBDB, Int16			index)
{
	// check for an open db
 	if (openDBDB == 0)
		return false;

	// check for bad index and delete
	if (index >= DmNumRecords(openDBDB) ||
			DmRemoveRecord(openDBDB, index) != 0)
		return false;

	return true;
}



/*****************************************************************************
 * OpenDBEmpty - delete the records from the openDB db.  
 *****************************************************************************/
Boolean OpenDBEmpty(DmOpenRef	openDBDB)
{
	Int16		index;
	Boolean	result = true;

	// check for an open db
	if (openDBDB == 0)
		return false;

	// delete backwards
	for (index = DmNumRecords(openDBDB) - 1; index > -1 && result; index--)
		if (DmRemoveRecord(openDBDB, index) != 0)
			result = false;

	return result;
}


/*****************************************************************************
 * OpenDBSetBackup
 *    set the bit in a database so the database will or will not be backed up
 *		by the default backup conduit
 * Parameters:
 *		DmOpenRef	dbRef			- reference to the OPEN database
 *		Boolean		setForBackup	- set for backup or not
 *****************************************************************************/
void OpenDBSetBackup(DmOpenRef	openDBRef,
					Boolean		setForBackup)
{
	UInt16	cardNo, dbAttrs;
	LocalID dbID;

	// get the dbID then get the attributes
	DmOpenDatabaseInfo(openDBRef, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &dbAttrs,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	// set the attribute
	if (setForBackup)
		// back up the database
		dbAttrs |= dmHdrAttrBackup;
	else
		// DON'T backup the database
		dbAttrs &= ~dmHdrAttrBackup;

	// save the attributes
	DmSetDatabaseInfo(cardNo, dbID, NULL, &dbAttrs,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}


/*****************************************************************************
 * OpenDBUpdateRecord - update a record.  NULL records are written out
 *	as empty
 *	Return: false if dbIndex out of range or write fails, true otherwise
 *****************************************************************************/
Boolean OpenDBUpdateRecord(DmOpenRef openDBDB,
						Int32		size,
						MemPtr	openDBP,
						UInt16	dbIndex)
{
	MemHandle	dbH = NULL;
	MemPtr		dbP = NULL;
	Boolean		result = true;

	// fetch and write the record
	if (dbIndex < DmNumRecords(openDBDB) &&
			(dbH = DmGetRecord(openDBDB, dbIndex)) != NULL &&
			(dbP = MemHandleLock(dbH)) != NULL) {
		if (openDBP) {
//			DebugWriteCheck(dbP, 0, size);
			DmWrite(dbP, 0, openDBP, size);
		} else {
			// NULL data, so write 0s
			DmSet(dbP, 0, size, 0);
		}
	} else {
		result = false;
	}

	// clean up
	if (dbP)
		MemHandleUnlock(dbH);
	if (dbH)
		DmReleaseRecord(openDBDB, dbIndex, result);

	return(result);
}

/*****************************************************************************
 * OpenDBUpdateAppInfo - update the App Info data for a database
 *	Return: false - If AppInfo Chuck not found otherwise true
 *****************************************************************************/
Boolean OpenDBUpdateAppInfo(DmOpenRef openDBDB, 
									 UInt16 cardNo, 
									 double	appInfoData)
{
	LocalID ai = DmGetAppInfoID(openDBDB);
	double *dblptr;
	Boolean result=true;

	if (ai) {
		dblptr = MemLocalIDToLockedPtr(ai, CARD_NO);
		DmSet(dblptr, 0, 64, 0);
		DmWrite(dblptr, 0, &appInfoData, sizeof(double));
		MemPtrUnlock(dblptr);

	} else {
		result = false;
	}

	return(result);
}

/*****************************************************************************
 * OpenDBGetAppInfo - update the App Info data for a database
 *	Return: false - If AppInfo Chuck not found otherwise true
 *****************************************************************************/
Boolean OpenDBGetAppInfo(DmOpenRef openDBDB, 
									 UInt16 cardNo, 
									 double *appInfoData)
{
	LocalID ai = DmGetAppInfoID(openDBDB);
	double *dblptr;
	Boolean result=true;;

	if (ai) {
		dblptr = MemLocalIDToLockedPtr(ai, CARD_NO);
		*appInfoData = *dblptr;
		MemPtrUnlock(dblptr);
		
	} else {
		result = false;
	}

	return(result);
}

/************************************************************
 *
 *  FUNCTION: MemoAppInfoInit
 *
 *  DESCRIPTION: Create an app info chunk if missing.  Set
 *		the strings to a default.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *************************************************************/
Err MemoAppInfoInit(DmOpenRef dbP)
{
	UInt16 				cardNo;
	MemHandle			h;
	LocalID 				dbID;
	LocalID 				appInfoID;
	MemoAppInfoPtr 	nilP = 0;
	MemoAppInfoPtr		appInfoP;
	
	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return dmErrInvalidParam;
	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
			NULL, NULL, &appInfoID, NULL, NULL, NULL))
		return dmErrInvalidParam;
	
	if (appInfoID == 0)
		{
		h = DmNewHandle (dbP, sizeof (MemoAppInfoType));
		if (! h) return dmErrMemError;
		
		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);
		}
	
	appInfoP = MemLocalIDToLockedPtr(appInfoID, cardNo);

	// Clear the app info block.
	DmSet (appInfoP, 0, sizeof(MemoAppInfoType), 0);

	// Initialize the categories.
	CategoryInitialize ((AppInfoPtr) appInfoP, LocalizedAppInfoStr);

	// Initialize the sort order.
	DmSet (appInfoP, (UInt32)&nilP->sortOrder, sizeof(appInfoP->sortOrder), 
		soAlphabetic);

	MemPtrUnlock(appInfoP);

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:     MemoGetDatabase
 *
 * DESCRIPTION:  Get the application's database.  Open the database if it
 * exists, create it if neccessary.
 *
 * PARAMETERS:   *dbPP - pointer to a database ref (DmOpenRef) to be set
 *					  mode - how to open the database (dmModeReadWrite)
 *
 * RETURNED:     Err - zero if no error, else the error
 *
 ***********************************************************************/
Err MemoGetDatabase (DmOpenRef *dbPP, UInt16 mode)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 cardNo;
	LocalID dbID;

	*dbPP = NULL;

	// Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (memoDBType, sysFileCMemo, mode);
	if (!dbP) {
		error = DmCreateDatabase (0, memoDBName, sysFileCMemo, memoDBType, false);
		if (error)
			return error;
		
		dbP = DmOpenDatabaseByTypeCreator(memoDBType, sysFileCMemo, mode);
		if (!dbP)
			return (1);

		// Set the backup bit.  This is to aid syncs with non Palm software.
		OpenDBSetBackup(dbP, true);
		
		error = MemoAppInfoInit (dbP);
	if (error) {
			DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL);
	DmCloseDatabase(dbP);
	DmDeleteDatabase(cardNo, dbID);
	return error;
		}
	}
	
	*dbPP = dbP;
	return 0;
}

Boolean HandleMemoData(Int8 action, Char *data, UInt32 *numBytes)
{
	static Char *memodata=NULL;
	static MemHandle memohand=NULL;
	MemHandle output_hand;
	MemPtr output_ptr;
	Int16 x = 0;
	static Int16 nrecs;
	Boolean found=false;
	static UInt16 recidx=0;
	Boolean retval = true;
	static UInt32 curReadChar=0;
	static UInt16 datalength=0;
	UInt32 endReadChar=0;
	static Char memofilename[26];
	static UInt16 numfiles=0;
	Boolean exitloop=false;

	switch (action) {
		case IOINIT:
			curReadChar = 0;
			datalength = 0;
			memohand = MemHandleNew(memoMaxLength);
			memodata = MemHandleLock(memohand);
			MemSet(memodata,sizeof(memodata),0);

			// Opens the Memo Database
//			HostTraceOutputTL(appErrorClass, "soarDB:Opening MemoDB");
			if (MemoGetDatabase(&memo_db, DB_RW_MODE) == 0) {
				retval = true;
			} else {
				retval = false;
			}

			nrecs = DmNumRecords(memo_db);
//			HostTraceOutputTL(appErrorClass, "soarDB:nrecs|%hd", nrecs);

			while((x < nrecs)  && (exitloop == false)) {
//				HostTraceOutputTL(appErrorClass, "soarDB:Getting Existing Record-x=|%hd|", x);
				if (OpenDBQueryRecord(memo_db, x, &output_hand, &output_ptr)) {
//					HostTraceOutputTL(appErrorClass, "soarDB:Calling MemMove");
					MemMove(memodata, output_ptr, memoMaxLength);
//					HostTraceOutputTL(appErrorClass, "soarDB:Calling MemHandleUnlock");
					MemHandleUnlock(output_hand);

//					HostTraceOutputTL(appErrorClass, "soarDB:Calling StrLen");
					if (StrLen(memodata) > 0) {
						StrNCopy(NameString, memodata, StrLen(data)+1);
						NameString[StrLen(data)+1] = '\0';
						// check filename for match
//						HostTraceOutputTL(appErrorClass, "soarDB:Filename Required %s",NameString);
//						HostTraceOutputTL(appErrorClass, "soarDB:Filename to Check %s",data);
						if (StrNCompare(trim(NameString, '*', true), data, StrLen(data)) == 0) {
//							HostTraceOutputTL(appErrorClass, "soarDB:Found it %s",NameString);
							recidx = x;
							if (*numBytes == IOOPENTRUNC) {
								// delete a record if writing new data
								OpenDBDeleteRecord(memo_db, recidx);
								x--;
								nrecs--;
							} else {
								// stop with first file found if reading data
								exitloop = true;
							}
							found = true;
						}
					}
				} else {
//					HostTraceOutputTL(appErrorClass, "soarDB:Didn't find it %s",NameString);
					found = false;
				}
				x++;
			}

			if (*numBytes == IOOPENTRUNC) {
				//make new file using data as the base filename
				StrCopy(memodata, "*");
				StrCat(memodata, data);
				// save file name and zero files counter
				StrCopy(memofilename, memodata);
				StrCat(memodata, "\r\n");
				numfiles = 0;
//				HostTraceOutputTL(appErrorClass, "soarDB:Adding New Record w/filename:|%s|", memodata);
				recidx = OpenDBAddRecord(memo_db, memoMaxLength, memodata, ATEND);
//				HostTraceOutputTL(appErrorClass, "soarDB:Added recidx:|%hu|", recidx);
				retval = true;
			} else if (!found && *numBytes == IOOPEN) {
				retval = false;
			}

			datalength = StrLen(memodata);	
			break;
		case IOWRITE:
			// Acumulates text to copy into a MemoPad file
			if ((StrLen(memodata)+StrLen(data)) < memoMaxLength) {
				if (StrLen(memodata) == 0) {
					StrCopy(memodata, data);
				} else {
					StrCat(memodata, data);
				}
			} else {
				// If more data has been acumulated than will fit in a Notepad
				// add the MemoPad entry and then start on a new one
//				HostTraceOutputTL(appErrorClass, "soarDB:Adding New record-2");
				recidx = OpenDBAddRecord(memo_db, memoMaxLength, memodata, ATEND);
				MemSet(memodata,sizeof(memodata),0);
				// put filename and sequence number
				numfiles++;
				StrCopy(memodata, memofilename);
				StrCat(memodata, leftpad(DblToStr(pround(numfiles,0),0), '0', 2));
				StrCat(memodata, "\r\n");
				// Copy leftover data into memodata for the next MemoPad file
				StrCat(memodata, data);
			}
//			HostTraceOutputTL(appErrorClass, "soarDB:Updating memo data-memodata|%s|", memodata);
			OpenDBUpdateRecord(memo_db, memoMaxLength, memodata, recidx);
			retval = true;
			break;
		case IOREAD:
			// Check to see if at end of memo
			if (curReadChar == datalength) {
				// At end of memo
//				HostTraceOutputTL(appErrorClass, "At EOF");

				// look for more files in the memo database with the same name
				exitloop = false;
				x = recidx + 1;
				while((x < nrecs)  && (exitloop == false)) {
//					HostTraceOutputTL(appErrorClass, "soarDB:Getting Existing Record-x=|%hd|", x);
					if (OpenDBQueryRecord(memo_db, x, &output_hand, &output_ptr)) {
//						HostTraceOutputTL(appErrorClass, "soarDB:Calling MemMove");
						MemMove(memodata, output_ptr, memoMaxLength);
//						HostTraceOutputTL(appErrorClass, "soarDB:Calling MemHandleUnlock");
						MemHandleUnlock(output_hand);

//						HostTraceOutputTL(appErrorClass, "soarDB:Filename Required %s",NameString);
						if (StrLen(memodata) > 0) {
							//StrNCopy(NameString, memodata, StrLen(data)+1);
							//NameString[StrLen(data)+1] = '\0';
							StrNCopy(data, memodata, StrLen(NameString)+1);
							trim(data, '*', true);
//							HostTraceOutputTL(appErrorClass, "soarDB:Filename to Check %s",data);
							// check filename for match
							if (StrNCompare(trim(NameString, '*', true), data, StrLen(data)) == 0) {
//								HostTraceOutputTL(appErrorClass, "soarDB:Found it %s",NameString);
								recidx = x;
								// stop with first file found because reading data
								exitloop = true;
								found = true;
							}
						}
					} else {
//						HostTraceOutputTL(appErrorClass, "soarDB:Didn't find it %s",NameString);
						found = false;
					}
					x++;
				}
				if (found == 0 ) {
					// exit if no more files found
					return(false);
				} else {
					// set up to read next file
 					curReadChar = 0;
					datalength = StrLen(memodata);
				}
			} 

			// Calculate new stopping point based on the number of bytes requested
			endReadChar = curReadChar + *numBytes;

			// Check to see if there are fewer characters left to read 
			// than have been requested.
			if (endReadChar > datalength) {
				// If there are fewer, change end point and number of bytes read
				endReadChar = datalength;
				*numBytes = datalength - curReadChar;
			}

			// Copy the correct number of characters into the data pointer
			// passed in
			StrNCopy(data, &memodata[curReadChar], *numBytes);
//			HostTraceOutputTL(appErrorClass, "data: |%s|", data);

			// Move the current read point to the new location
//			HostTraceOutputTL(appErrorClass, "Moving current read pointer");
			curReadChar += *numBytes;

			if (curReadChar == datalength && data[*numBytes-1] != '\n') {
				data[*numBytes] = '\n';
				(*numBytes)++;
//				curReadChar++;
			}
			data[*numBytes] = '\0';
//			HostTraceOutputTL(appErrorClass, "Returning true");
			retval = true;

			break;
		case IOCLOSE:
//			HostTraceOutputTL(appErrorClass, "soarDB:Closing memo database");
			DmCloseDatabase(memo_db);
			MemHandleUnlock(memohand);
			MemHandleFree(memohand);
			retval = true;
			break;
	}
	return(retval);
}

Boolean HandleDOCData(Int8 action, Char *data, UInt32 *numBytes)
{
	static Char *docdata=NULL;
	static MemHandle dochand=NULL;
	MemHandle output_hand;
	MemPtr output_ptr;
	UInt16 nrecs=0;
	Boolean found=false;
	static UInt16 recidx=0;
	Boolean retval = true;
	static UInt32 curReadChar=0;
	static UInt16 datalength=0;
	UInt32 endReadChar=0;
	Int16 dberr;
	static palmdoc_record0 pdocrec0;
	static Boolean firsttime=true;
	static UInt16 curDocRec=0;
	UInt32 leninc, lenover;

	DmSearchStateType state;
	Boolean latestVer = false;
	UInt16 card;
	LocalID currentDB;
	Char nameP[FILENAMESIZE];
	Char *prev_filelist = NULL;
	Int16 prev_numfilesfound = 0;
	Char extension[5];

	switch (action) {
		case IODELETE:
			// delete a file
			if ((currentDB = DmFindDatabase(CARD_NO, data)) != 0) {
				DmDeleteDatabase(CARD_NO, currentDB);
				return(true);
			} else {
				return(false);
			}
		case IOEXIST:
			// test if a filename exists
			if ((currentDB = DmFindDatabase(CARD_NO, data)) == 0) {
				return(true);
			} else {	
				return(false);
			}
		case IODIR:
			// list the DOC databases on the Palm that have the required extension (data)
			numfilesfound = 0;
			dberr = DmGetNextDatabaseByTypeCreator(true,  &state, docDBType, docCreator, latestVer, &card, &currentDB);
			while (!dberr && currentDB) {
				// do something with currentDB
				dberr = DmDatabaseInfo(card, currentDB, nameP, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
//				HostTraceOutputTL(appErrorClass, "File =|%s|", nameP);
				// check file extension (ignore case)
				StrToLower(extension , Right(nameP, 4));
				if (StrCompare(extension, data) == 0) {
//					HostTraceOutputTL(appErrorClass, "Add File to List");
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
				// now get the next DB
				dberr = DmGetNextDatabaseByTypeCreator(false, &state, docDBType, docCreator, latestVer, &card, &currentDB);
			}
			quicksort(filelist, numfilesfound, FILENAMESIZE, 1);
			*numBytes = numfilesfound;
			break;
		case IOINIT:
			if (StrLen(data) < 5) {
				// exit if filename too short
				retval = false;
				break;
			}
			dochand = MemHandleNew(docrecMaxLength+1);
			docdata = MemHandleLock(dochand);
			MemSet(docdata, docrecMaxLength+1, '\0');
			doc_db = 0;
			// Opens the DOC Database
//			HostTraceOutputTL(appErrorClass, "soarDB:Opening DOC Database:|%s|", data);
			dberr=OpenCreateDB2(&doc_db, CARD_NO, data, docCreator, docDBType, DB_RW_MODE, DOCDBVER);

			retval = true;
			found = true;
			if (dberr == DB_OPEN_SUCCESS) {
//				HostTraceOutputTL(appErrorClass, "soarDB:Found and Opened DOC File");
			}
			if (dberr == DB_OPEN_FAIL) {
				ErrDisplay("Could Not Create DOC File");
				retval = false;
				found = false;
			}
			if (dberr==DB_OPEN_CREATE) {
				OpenDBSetBackup(doc_db, true);

//				HostTraceOutputTL(appErrorClass, "soarDB:New DOC Database-Adding Record0");
				MemSet(&pdocrec0, sizeof(pdocrec0), 0);
				pdocrec0.version = 1;
				pdocrec0.doc_size = 0;
				pdocrec0.num_recs = 0;
				pdocrec0.rec_size = docrecMaxLength;
				OpenDBAddRecord(doc_db, sizeof(pdocrec0), &pdocrec0, ATSTART);

				found = true;

				// Even though it was created, it wasn't initially found
				if (*numBytes==IOOPEN) {
					found = false;
					retval = false;
					// Delete the newly created database because it wasn't needed for an
					// IOOPEN call
					OpenDeleteDB(&doc_db, CARD_NO, data);
					doc_db = NULL;
				}
			}

			//Poplulate pdocrec0 structure
//			HostTraceOutputTL(appErrorClass, "soarDB:Getting Record0 Data");
			MemSet(&pdocrec0, sizeof(pdocrec0), 0);
			if (found && OpenDBQueryRecord(doc_db, 0, &output_hand, &output_ptr)) {
//				HostTraceOutputTL(appErrorClass, "soarDB:Found Record0 Moving the Data");
				MemMove(&pdocrec0, output_ptr, sizeof(pdocrec0));
				MemHandleUnlock(output_hand);
				if (pdocrec0.version != 1) {
					FrmCustomAlert(WarningAlert, "Compressed DOC Files Not Supported"," "," ");
					found = false;
					retval = false;
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "soarDB:Didn't find record 0");
				found = false;
				retval = false;
			}


			if (found && *numBytes == IOOPENTRUNC) {
				nrecs = pdocrec0.num_recs;
//				HostTraceOutputTL(appErrorClass, "soarDB:nrecs|%hu|", nrecs);
				if (nrecs > 0) {
//					HostTraceOutputTL(appErrorClass, "soarDB:Deleting Existing Records");
					// Clear the current doc file
					OpenDBEmpty(doc_db);

					// Put a new record0 into the doc file
					MemSet(&pdocrec0,sizeof(pdocrec0),0);
					pdocrec0.version = 1;
					pdocrec0.doc_size = 0;
					pdocrec0.num_recs = 0;
					pdocrec0.rec_size = docrecMaxLength;
					OpenDBAddRecord(doc_db, sizeof(pdocrec0), &pdocrec0, ATSTART);

				}
				// Add first record empty
				recidx = OpenDBAddRecord(doc_db, StrLen(docdata), docdata, false);
//				HostTraceOutputTL(appErrorClass, "soarDB:Added new empty record at recidx:|%hu|", recidx);

				// Update the record0 info to account for the new data
				pdocrec0.num_recs = 1;
//				HostTraceOutputTL(appErrorClass, "soarDB:Updating Record0");
				OpenDBUpdateRecord(doc_db, sizeof(pdocrec0), &pdocrec0, 0);

				retval = true;
			} else if (!found && *numBytes == IOOPEN) {
				retval = false;
			}
			break;
		case IOWRITE:
			// Acumulates text to copy into a DOC file
//			HostTraceOutputTL(appErrorClass, "soarDB:StrLen(data)-|%hu|", StrLen(data));
//			HostTraceOutputTL(appErrorClass, "soarDB:StrLen(docdata)-|%hu|", StrLen(docdata));
			leninc = StrLen(data);
			if ((StrLen(docdata)+StrLen(data)) < docrecMaxLength) {
				if (StrLen(docdata) == 0) {
					StrCopy(docdata, data);
				} else {
					StrCat(docdata, data);
				}
			} else {
				// If more data has been acumulated than will fit in a one DOC record
				// record is not blank so fill remaining spaces in existing record
//				HostTraceOutputTL(appErrorClass, "soarDB:Data  %s", data);
//				HostTraceOutputTL(appErrorClass, "soarDB:StrLen docdata  %s",DblToStr(StrLen(docdata),0));
//				HostTraceOutputTL(appErrorClass, "soarDB:StrLen data  %s",DblToStr(StrLen(data),0));
				lenover =  StrLen(docdata)+StrLen(data)-docrecMaxLength;
//				HostTraceOutputTL(appErrorClass, "soarDB:lenover %s",DblToStr(lenover,0));
//				HostTraceOutputTL(appErrorClass, "soarDB:copied %s", Left(data, StrLen(data)-lenover));
				StrCat(docdata, Left(data, StrLen(data)-lenover));
//				HostTraceOutputTL(appErrorClass, "soarDB:StrLen docdata  %s",DblToStr(StrLen(docdata),0));
				// delete and re-create the record
				OpenDBDeleteRecord(doc_db, recidx);
				recidx = OpenDBAddRecord(doc_db, StrLen(docdata), docdata, ATEND);

				// add the DOC record entry and then start on a new one
//				HostTraceOutputTL(appErrorClass, "soarDB:Adding New record-2");
//				MemSet(docdata,docrecMaxLength,' ');
				recidx = OpenDBAddRecord(doc_db, docrecMaxLength, docdata, ATEND);
				pdocrec0.num_recs++;
//				HostTraceOutputTL(appErrorClass, "soarDB:pdocrec0.num_recs-|%hu|", pdocrec0.num_recs);

				// Copy leftover data into docdata for the next doc record
//				HostTraceOutputTL(appErrorClass, "soarDB:Data  %s", Right(data, lenover));
				StrCopy(docdata, Right(data, lenover));
//				HostTraceOutputTL(appErrorClass, "soarDB:StrLen(data)-|%hu|", StrLen(data));
//				HostTraceOutputTL(appErrorClass, "soarDB:StrLen(docdata)2-|%hu|", StrLen(docdata));
//				HostTraceOutputTL(appErrorClass, "----------");
			}
//			HostTraceOutputTL(appErrorClass, "soarDB:Updating doc data-docdata|%s|", docdata);
			OpenDBDeleteRecord(doc_db, recidx);

			recidx = OpenDBAddRecord(doc_db, StrLen(docdata),   docdata, ATEND);
// alternative to include the \0 string terminator in the record
//			recidx = OpenDBAddRecord(doc_db, StrLen(docdata)+1, docdata, ATEND);

			// Update the record0 info to account for the new data
//			pdocrec0.doc_size += (UInt32)StrLen(data);
			pdocrec0.doc_size += leninc;
//			HostTraceOutputTL(appErrorClass, "soarDB:Updating Record0");
//			HostTraceOutputTL(appErrorClass, "soarDB:---------------------------------------------");
			OpenDBUpdateRecord(doc_db, sizeof(pdocrec0), &pdocrec0, 0);
			retval = true;
			break;
		case IOREAD:
			
			if (pdocrec0.num_recs <= 0) {
				FrmCustomAlert(WarningAlert, "Must have 1 or more records in the DOC File"," "," ");
				retval = false;
				break;
			}
			if (pdocrec0.doc_size <= 0) {
				FrmCustomAlert(WarningAlert, "DOC data size must be > 0"," "," ");
				retval = false;
				break;
			}

			// If first time through or at end of current doc record
			if (firsttime || curReadChar == datalength) {
				if (firsttime) {
					curDocRec = 1;
				}

				if (curDocRec > pdocrec0.num_recs) {
					firsttime = true;
					retval = false;
					break;
				} 

				curReadChar = 0;

				// Get First Record from Doc file and put in docdata
				if (OpenDBQueryRecord(doc_db, curDocRec, &output_hand, &output_ptr)) {
//					HostTraceOutputTL(appErrorClass, "soarDB:Calling MemMove");
					MemMove(docdata, output_ptr, docrecMaxLength);
//					HostTraceOutputTL(appErrorClass, "soarDB:Calling MemHandleUnlock");
					MemHandleUnlock(output_hand);
//					HostTraceOutputTL(appErrorClass, "docdata: |%s|", docdata);
				} else {
//					HostTraceOutputTL(appErrorClass, "soarDB:Didn't find it");
					retval = false;
					break;
				}

				// Set datalength to length of record
				datalength = StrLen(docdata);

				// Increment curDocRec
				curDocRec++;

				firsttime = false;
			}
				
			// Calculate new stopping point based on the number of bytes requested
			endReadChar = curReadChar + *numBytes;

			// Check to see if there are fewer characters left to read 
			// than have been requested.
			if (endReadChar > datalength) {
				// If there are fewer, change end point and number of bytes read
				endReadChar = datalength;
				*numBytes = datalength - curReadChar;
			}

			// Copy the correct number of characters into the data pointer
			// passed in
			StrNCopy(data, &docdata[curReadChar], *numBytes);
//			HostTraceOutputTL(appErrorClass, "data: |%s|", data);
//			HostTraceOutputTL(appErrorClass, "-----------------------");

			// Move the current read point to the new location
//			HostTraceOutputTL(appErrorClass, "Moving current read pointer");
			curReadChar += *numBytes;

			if (curDocRec > pdocrec0.num_recs && curReadChar == datalength && data[(*numBytes)-1] != '\n') {
//				HostTraceOutputTL(appErrorClass, "DOC Read: Adding newline");
				data[*numBytes] = '\n';
				(*numBytes)++;
			}
			data[*numBytes] = '\0';

//			HostTraceOutputTL(appErrorClass, "DOC Read:Returning true");
			retval = true;

			break;
		case IOCLOSE:
//			HostTraceOutputTL(appErrorClass, "soarDB:Closing doc database");
			if (doc_db) {
				DmCloseDatabase(doc_db);
			}
			MemHandleUnlock(dochand);
			MemHandleFree(dochand);
			retval = true;
			break;
	}
	return(retval);
}

Boolean OpenAllDatabases(void)
{
	Int16 dberr;
	MemHandle record_handle;
	MemPtr record_ptr;
	CAIAddData *caidata;

	/* Configuration data */
	dberr=OpenCreateDB2(&config_db, CARD_NO, config_db_name, appcreator, 
				config_db_type, DB_RW_MODE, (UInt16)(CONFIGDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create config database");
		XferClose(data.config.nmeaxfertype);
		return(true);
	}

	if (dberr==DB_OPEN_CREATE) {
		// config data
		data.config.bugfactor=1.0;
		data.config.pctwater=0.0;
		data.config.safealt=0.0;
		data.config.spdunits=NAUTICAL;
		data.config.disunits=NAUTICAL;
		data.config.altunits=NAUTICAL;
		data.config.lftunits=NAUTICAL;
		data.config.wgtunits=NAUTICAL;
		data.config.wtrunits=NAUTICAL;
		data.config.logstartspd=20.0;
		data.config.logstopspd=10.0;
		data.config.logstoptime=300;
		data.config.logonoff=true;
		data.config.logautooff=true;
		data.config.mapscaleidx=10;
		data.config.timezone=0;
		data.config.nmeaspeed=2;
		data.config.dataspeed=2;
		data.config.flowctrl=false;
		data.config.alttype=REQALT;
		data.config.defaulttoFG=true;
		data.config.usecalchw=true;
		data.config.usepalmway=true;
		data.config.setmcval=true;
		data.config.optforspd=true;
		data.config.ldforcalcs=25;
		data.config.btmlabels=true;
		data.config.trktrail=true;
		data.config.thzoom=THZOOMFIX;
		data.config.thonoff=true;
		data.config.wayonoff=true;
		data.config.wayline=true;
		data.config.waymaxlen=12;
		data.config.numtrkpts=20;
		data.config.thnumtrkpts=20;
		data.config.pressaltsrc=0;
		data.config.usepalt=false;
		data.config.altreftype=MSL;
		data.config.taskonoff=true;
		data.config.taskdrawtype=2;
		data.config.FlrmCopyIGC=true;
		data.config.usechksums=true;
		data.config.starttype=TSKLINE;
		data.config.startrad=1.0;
		data.config.startdir=0.0;
		data.config.startdirmag=false;
		data.config.startdirauto=true;
		data.config.turntype=FAI;
		data.config.turnfairad=1.0;
		data.config.turncircrad=0.25;
		data.config.finishtype=TSKLINE;
		data.config.finishrad=1.0;
		data.config.finishdir=0.0;
		data.config.finishdirmag=false;
		data.config.finishdirauto=true;
		data.config.qfealt=0.0;
		data.config.usetas=false;
		data.config.flightcomp=0;
		data.config.earthmodel=EMWGS84;
		// 1.8.8 Additions Start Here
		data.config.usegpstime=false;
		data.config.SUAmaxalt=35000.0;
		data.config.suaactivetypes=SUAALL;
		data.config.windarrow=true;
		data.config.inrngcalc=true;
		if (device.CardPresent) {
			data.config.xfertype=USEVFS;
		} else {
			data.config.xfertype=USEDOC;
		}
		data.config.hwposlabel=true;
		// 1.9.1 Additions Start Here
		data.config.mapcircrad1=5.0;
		data.config.mapcircrad2=10.0;
		data.config.tskzoom=true;
		data.config.nodatatime=300;
		// 1.9.3 Additions Start Here
		data.config.slowlogint=4;
		data.config.fastlogint=1;
		data.config.keysoundon=false;
		data.config.mcrngmult=1;
		data.config.nmeaxfertype=USESER;
		// 1.9.7 Additions Start Here
		data.config.llfmt=LLDMS;
		// 1.9.8 Additions Start Here
		data.config.maporient=TRACKUP;
		data.config.thmaporient=NORTHUP;
		data.config.useiquesim=false;
		data.config.useiqueser=false;
		// 2.0.0 Additions Start Here
		data.config.horiz_dist=1.0;
		data.config.vert_dist=500.0;
		data.config.CheckSUAWarn=true;
		data.config.suawarntypes=SUAALL;
		data.config.wndunits=NAUTICAL;
		data.config.SUArewarn = 0.33;
		data.config.stcurdevcreator = 0;
		data.config.thzoomscale = THZOOMSCALELO;
		data.config.autodismisstime = 120;
		data.config.declutter = 0.0;
		data.config.keepSUA = false;
		data.config.TaskColour.r = 0;
		data.config.TaskColour.g = 0;
		data.config.TaskColour.b = 255;
		data.config.SUAColour.r = 255;
		data.config.SUAColour.g = 192;
		data.config.SUAColour.b = 0;
		data.config.SUAwarnColour.r = 255;
		data.config.SUAwarnColour.g = 0;
		data.config.SUAwarnColour.b = 0;
		data.config.SectorColour.r = 255; 
		data.config.SectorColour.g = 0;
		data.config.SectorColour.b = 255;
		data.config.BoldTask = false;
		data.config.BoldSUA = false;
		data.config.BoldSector = false;
		data.config.QNHunits = MILLIBARS;
		data.config.SUAdispalt = 35000.0;
		data.config.shownextwpt = NEXTOFF;
		data.config.AATmode = AAT_NORMAL;
		data.config.wpformat = WPCAI;
		data.config.tskspdunits=NAUTICAL;
		data.config.SUAlookahead = 0;
		data.config.sectormaporient = NORTHUP;
		data.config.thmapscaleidx = 10;
		data.config.listlineshoriz = 0;
		data.config.listlinesvert = 2;
		data.config.gpsmsladj = 0.0;
		data.config.gpsaltref = GPSMSL;
		data.config.ctllinevis = true;
		data.config.gpscalcdev = false;
		data.config.Flarmscaleidx = 7;
		data.config.showrgs = false;
		data.config.calcwind = true;
		data.config.tskrestarts = true;
		data.config.usefgterrain = true;
		data.config.defaulthome = true;
		data.config.SUAdisponlywarned = false;
		data.config.SUAhighlightwarned = true;
		data.config.RefWptRadial = false;
		data.config.windprofileorient = NORTHUP;
		data.config.SinkColour.r = 255; 
		data.config.SinkColour.g = 0;
		data.config.SinkColour.b = 0;
		data.config.WeakColour.r = 0; 
		data.config.WeakColour.g = 0;
		data.config.WeakColour.b = 255;
		data.config.StrongColour.r = 0; 
		data.config.StrongColour.g = 255;
		data.config.StrongColour.b = 0;
		data.config.dynamictrkcolour = false;
		data.config.useinputwinds = false;
		StrCopy(data.config.config_file, "Default");
		StrCopy(data.config.waypt_file, "None");
		StrCopy(data.config.SUA_file, "None");
		// default main screen chain
		data.config.screenchain[0]=form_final_glide;
		data.config.screenchain[1]=form_flarm_traffic;	
		data.config.screenchain[2]=form_flt_info;	
		data.config.screenchain[3]=form_wind_disp;	
		data.config.screenchain[4]=form_wind_3dinfo;	
		data.config.screenchain[5]=form_list_waypt;	
		data.config.screenchain[6]=form_set_fg;	
		data.config.screenchain[7]=form_set_qnh;	
		data.config.screenchain[8]=form_set_task;	
		data.config.screenchain[9]=form_moving_map;
		data.config.profilescaleidx = 1;
		data.config.thermalprofile = true;
		data.config.showQFEQNHonstartup = false;
		data.config.outputSMS = false;
		data.config.SMSouttype = SMSOUTGEN;
		data.config.SMSsendtype = 0;
		data.config.SMSsendint = 300;
		StrCopy(data.config.SMSaddress, "");
		data.config.defaultrules.rulesactive = false;
		data.config.defaultrules.startaltref = AGL;
		data.config.defaultrules.finishaltref = AGL;
		data.config.totalenergy = false;
		data.config.netto = false;
		data.config.output_wypts = false;
		data.config.output_tasks = true;
		data.config.leftaction = 1;
		data.config.autostart = false;
		data.config.declaretasks = true;
		data.config.fgtostartalt = false;
		data.config.accuratedistcalc = true;
		data.config.WayptColour.r = 0;
		data.config.WayptColour.g = 0;
		data.config.WayptColour.b = 0;
		data.config.BoldWaypt = false;
		data.config.thermal_turns = 1;
		data.config.SUAformat = SUATNP;
		data.config.mapRHfield = MAPFGA;
		data.config.FGalert = false;
		data.config.echoNMEA = false;
		data.config.autoIGCxfer = false;
		data.config.declaretoSD = false;
		data.config.MCbutton = NORMAL;
		// AGM: default preferred BT GPS address is 00:00:00:00:00 -> search BT devices
		data.config.BTAddr[0] = 0;
		data.config.BTAddr[1] = 0;
		data.config.BTAddr[2] = 0;
		data.config.BTAddr[3] = 0;
		data.config.BTAddr[4] = 0;
		data.config.BTAddr[5] = 0;
		OpenDBAddRecord(config_db, sizeof(ConfigFlight), &data.config, true);

		// IGC info
		StrCopy(data.igchinfo.name, "");
		StrCopy(data.igchinfo.type, "");
		StrCopy(data.igchinfo.gid, "");
		StrCopy(data.igchinfo.cid, "");
		StrCopy(data.igchinfo.cls, "");
		StrCopy(data.igchinfo.site, "");
		StrCopy(data.igchinfo.ooid, "");
		StrCopy(data.igchinfo.gpsmodel, "");
		StrCopy(data.igchinfo.gpsser, "");
		OpenDBAddRecord(config_db, sizeof(IGCHInfo), &data.igchinfo, false);

		//Initialize CAI Additional Data
		AllocMem((void *)&caidata, sizeof(CAIAddData));
		caidata->stfdb = 0.0;
		caidata->arvrad = 1.00;
		caidata->apprad = 0.10;
		caidata->tbtwn = 5;
		caidata->dalt = true;
		caidata->sinktone = false;
		caidata->tefg = false;
		caidata->tempunits = FARENHEIT; //NAUTICAL=Farenheit METRIC=Celcius
		caidata->barounits = MILLIBARS; 
		caidata->variotype = CAI_VARIO_SNETTO; 
		caidata->pilotinfo = true;
		caidata->gliderinfo = true;
		OpenDBAddRecord(config_db, sizeof(CAIAddData), caidata, false);
		FreeMem((void *)&caidata);

		// Initialize the RECO Stored Data
		recodata->temp = 0.0;
		recodata->voltage = 0.0;
		StrCopy(recodata->recofirmwarever, "NotSet");
		StrCopy(recodata->recoserial, "NotSet");
		OpenDBAddRecord(config_db, sizeof(RECOData), recodata, false);

		// Initialize the EWMR Stored Data
		StrCopy(ewmrdata->modelname, "MicroRecorder");
		StrCopy(ewmrdata->serialnumber, "560");
		StrCopy(ewmrdata->firmwarever, "6.60");
		StrCopy(ewmrdata->hardwarever, "5.1");
		StrCopy(ewmrdata->secstatus, "SECURE");
		StrCopy(ewmrdata->enlfitted, "N");
		StrCopy(ewmrdata->debuglevel, "4");
		ewmrdata->updaterate=4; //seconds
		ewmrdata->datarate=38400; //baud - readonly
		ewmrdata->aotime=90; //Time in minutes before unit shuts off if no alt or pos chg
		ewmrdata->aoaltchg=50.0/ALTMETCONST; //Altitude change in meters to keep unit from shutting off
		ewmrdata->aospdchg=5.0/SPDKPHCONST; //Velocity change in kph to keep unit from shutting off
		ewmrdata->startfilespd=20.0/SPDKPHCONST; //Speed in kph at which logging starts
		ewmrdata->startfilepaltchg=20.0/ALTMETCONST; //Pressure altitude change in meters at which logging starts
		ewmrdata->filesync=1; //Transfer ALL files (0) or LAST file (1)
		StrCopy(ewmrdata->pilotname, "N/A");
		StrCopy(ewmrdata->cid, "N/A");
		StrCopy(ewmrdata->atype, "N/A");
		StrCopy(ewmrdata->gid, "N/A");
		ewmrdata->pilotinfo = true; // Save/Overwrite pilot info
		ewmrdata->gliderinfo = true; // Save/Overwrite glider info
		OpenDBAddRecord(config_db, sizeof(EWMRData), ewmrdata, false);

		OpenDBSetBackup(config_db, true);

	}

	/* Retrieve Config Info */
	OpenDBQueryRecord(config_db, CONFIG_REC, &record_handle, &record_ptr);
	MemMove(&data.config,record_ptr,sizeof(ConfigFlight));
	MemHandleUnlock(record_handle);

	/* Retrieve IGC Header Info */
	OpenDBQueryRecord(config_db, IGCHINFO_REC, &record_handle, &record_ptr);
	MemMove(&data.igchinfo,record_ptr,sizeof(IGCHInfo));
	MemHandleUnlock(record_handle);

	/* Retrieve CAI Info */
//	OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
//	MemMove(caidata, record_ptr, sizeof(CAIAddData));
//	MemHandleUnlock(record_handle);

	/* Retrieve RECO Info */
	OpenDBQueryRecord(config_db, RECOINFO_REC, &record_handle, &record_ptr);
	MemMove(recodata, record_ptr, sizeof(RECOData));
	MemHandleUnlock(record_handle);
	// Always set these to zero at startup
	recodata->temp = 0.0;
	recodata->voltage = 0.0;
/*
	// ReCo Test Values
	recodata->temp = 31.8;
	recodata->voltage = 12.22832;
	StrCopy(recodata->recofirmwarever, "V3.0");
	StrCopy(recodata->recoserial, "s0601");
*/
	/* Retrieve EWMR Info */
	OpenDBQueryRecord(config_db, EWMRINFO_REC, &record_handle, &record_ptr);
	MemMove(ewmrdata, record_ptr, sizeof(EWMRData));
	MemHandleUnlock(record_handle);

	/* overwrite QNH units if C302 computer active */
//	HostTraceOutputTL(appErrorClass, "About to set QNHunits");
	if (data.config.flightcomp == C302COMP) {
		AllocMem((void *)&caidata, sizeof(CAIAddData));
		OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
		MemMove(caidata, record_ptr, sizeof(CAIAddData));
		MemHandleUnlock(record_handle);
	
		data.config.QNHunits = caidata->barounits;

		FreeMem((void *)&caidata);
	}
//	HostTraceOutputTL(appErrorClass, "Done setting QNHunits");

	/* Sets the units constant, text and precedent values */
	set_unit_constants();

	/* Logging Data */
	dberr=OpenCreateDB2(&logger_db, CARD_NO, logger_db_name, appcreator, 
								logger_db_type, DB_RW_MODE, (UInt16)(LOGGERDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create logger database");
		return(true);
	}
	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(logger_db, true);
	}

	/* Flight Data */
	dberr=OpenCreateDB2(&flight_db, CARD_NO, flight_db_name, appcreator, 
								flight_db_type, DB_RW_MODE, (UInt16)(FLIGHTDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create flight database");
	}
	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(flight_db, true);
	}

	/* Waypoint Data */
	dberr=OpenCreateDB2(&waypoint_db, CARD_NO, waypoint_db_name, appcreator, 
								waypoint_db_type, DB_RW_MODE, (UInt16)(WAYPOINTDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create waypoint database");
	}
	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(waypoint_db, true);
	}

	/* Glider Polar Data */
	MemSet(&data.polar, sizeof(PolarData), 0);
	dberr=OpenCreateDB2(&polar_db, CARD_NO, polar_db_name, appcreator, 
								polar_db_type, DB_RW_MODE, (UInt16)(POLARDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create polar database");
	}

	// David Lane - if the database is brand new create a Default entry
	if (dberr == DB_OPEN_CREATE) {
		StrCopy(data.polar.name, "Default");
		// MFH Do we need to set these?
		data.polar.v1 = 40.0;
		data.polar.w1 = -1.8;
		data.polar.v2 = 58.0;
		data.polar.w2 = -3.3;
		data.polar.v3 = 77.0;
		data.polar.w3 = -6.6;
		data.polar.maxdrywt = 575.0;
		data.polar.maxwater = 0.0;
		CalcPolarABC(&data.polar, data.config.bugfactor);
		OpenDBAddRecord(polar_db, sizeof(PolarData), &data.polar, true);
		OpenDBSetBackup(polar_db, true);
	} 
	
	// David Lane - if the database already has records then read the first
	if ( DmNumRecords(polar_db) )
	{
		OpenDBQueryRecord(polar_db, POLAR_REC, &record_handle, &record_ptr);
		MemMove(&data.polar,record_ptr,sizeof(PolarData));
		MemHandleUnlock(record_handle);
		CalcPolarABC(&data.polar, data.config.bugfactor);
	} else {
		// David Lane - if the database exists but is empty then create a Default entry
		StrCopy(data.polar.name, "Default");
		//MFH Do we need to set these?
		data.polar.v1 = 40.0;
		data.polar.w1 = -1.8;
		data.polar.v2 = 58.0;
		data.polar.w2 = -3.3;
		data.polar.v3 = 77.0;
		data.polar.w3 = -6.6;
		data.polar.maxdrywt = 575;
		data.polar.maxwater = 0.0;
		CalcPolarABC(&data.polar, data.config.bugfactor);
		OpenDBAddRecord(polar_db, sizeof(PolarData), &data.polar, true);
	}
	*inusePolar = data.polar;
	*selectedPolar = data.polar;
	selectedPolarIndex = 0;

	/* Task Data */
	dberr=OpenCreateDB2(&task_db, CARD_NO, task_db_name, appcreator, task_db_type, DB_RW_MODE, (UInt16)(TASKDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create task database");
		return(true);
	}
	if (dberr==DB_OPEN_CREATE) {
		StrCopy(data.task.name, "Active Task");
		data.task.numwaypts = 0;
		data.task.numctlpts =0;
		data.task.hastakeoff = false;
		data.task.haslanding = false;

		OpenDBAddRecord(task_db, sizeof(TaskData), &data.task, true);
		OpenDBSetBackup(task_db, true);
	}
	OpenDBQueryRecord(task_db, TASKACTIVE_REC, &record_handle, &record_ptr);
	MemMove(&data.task,record_ptr,sizeof(TaskData));
	MemHandleUnlock(record_handle);

	data.task.numwaypts = 0;
	data.task.numctlpts =0;

	/* SUA Index Data */
	dberr=OpenCreateDB2(&suaidx_db, CARD_NO, suaidx_db_name, appcreator, 
								suaidx_db_type, DB_RW_MODE, (UInt16)(SUAIDXDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create SUA index database");
		return(true);
	}
	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(suaidx_db, true);
	}

	/* SUA Data */
	dberr=OpenCreateDB2(&suadata_db, CARD_NO, suadata_db_name, appcreator, 
								suadata_db_type, DB_RW_MODE, (UInt16)(SUADBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create SUA database");
	}

	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(suadata_db, true);
	}

	/* Terrain Data */
	dberr=OpenCreateDB2(&terrain_db, CARD_NO, terrain_db_name, appcreator, 
								terrain_db_type, DB_RW_MODE, (UInt16)(TERRAINDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create Terrain database");
	}

	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(terrain_db, true);
	} 

	/* simulator data, IGC playback */
	dberr=OpenCreateDB2(&sim_db, CARD_NO, sim_db_name, appcreator, 
								sim_db_type, DB_RW_MODE, (UInt16)(SIMDBVER*10.0));
	if (dberr == DB_OPEN_FAIL) {
		ErrDisplay("Could not create simulator database");
	}
	if (dberr==DB_OPEN_CREATE) {
		OpenDBSetBackup(sim_db, true);
	}
	
	return(false);
}

void CloseAllDatabases(void)
{
	// Close all open databases
	DmCloseDatabase(config_db);
	DmCloseDatabase(logger_db);
	DmCloseDatabase(flight_db);
	DmCloseDatabase(waypoint_db);
	DmCloseDatabase(polar_db);
	DmCloseDatabase(task_db);
	DmCloseDatabase(suaidx_db);
	DmCloseDatabase(suadata_db);
	DmCloseDatabase(terrain_db);
	// delete the sim database, it's faster to recreate
	OpenDeleteDB(&sim_db, CARD_NO, sim_db_name);
}

void DebugWriteCheck(MemPtr recordP, UInt32 offset, UInt32 bytes)
{

	Err err;

	err = DmWriteCheck (recordP, offset, bytes);
	if (err)
	{
		const char * errorType;
		char errorStr[84];

		switch (err)
		{
		case (dmErrNotValidRecord):
			errorType = "dmErrNotValidRecord";
			break;
		case (dmErrWriteOutOfBounds):
			errorType = "dmErrWriteOutOfBounds";
			break;
		default:
			errorType = "UnknownError";
		}
	StrPrintF(errorStr, "DmWriteCheck caused ErrorType %s:0x%x", errorType, err);
	ErrFatalDisplayIf(err, errorStr);
	}
}
