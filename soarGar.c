#include <PalmOS.h>
#include "AddIncs/GarminIncs/GPSLib68K.h"
#include "soarGar.h"
#include "soaring.h"
#include "soarForm.h"
#include "soarUMap.h"
#include "soarGEOMAG.h"
#include "soarUtil.h"
#include "soarNMEA.h"
#include "Mathlib.h"
#include "soarMath.h"
#include "soarGPSInfo.h"
#include "soarIO.h"
#include "soarComp.h"

//#define semiToDeg           (double)( 180.0 / 2147483648.0 )
//#define secondsPerFrac      ( 1.0 / 4294967296.0 )      /* Seconds per fractional second    */
//#define secondsPerMinute    60.0                        /* Seconds per minute               */
//#define secondsPerHour      ( secondsPerMinute * 60.0 ) /* Seconds per hour                 */
//#define max( a, b )                     ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

UInt16 iQue_ref;
Boolean GPSDataChanged = false;
Boolean GPSStillOpen = false;

extern GPSSatDataType *satData;
extern GPSPVTDataType *pvtData;
extern Boolean updatemap;
extern Boolean updatewind;
extern Boolean updatetime;
extern Boolean recv_palt;
extern Boolean draw_log;

Int16 iQueEnabledDevice() 
{
	Err err;
	Int16 retval=-1;
	UInt32 deviceID;

	err = SysLibFind(gpsLibName, &iQue_ref);
	if (err != errNone) {
		err = SysLibLoad( gpsLibType, gpsLibCreator, &iQue_ref );
		if ( err ) {
//			FrmCustomAlert(WarningAlert, "Can't load Garmin GPSLIB"," "," ");
			retval = 0;
		} else {
//			FrmCustomAlert(WarningAlert, "GPSLib Loaded New"," "," ");
			SysLibRemove(iQue_ref);
//			FrmCustomAlert(WarningAlert, "GPSLib Removed"," "," ");
		}
	} else {
//		FrmCustomAlert(WarningAlert, "GPSLib Loaded Already-Not loading again"," "," ");
	}

	if (retval == -1) {
		// Determine iQue Type
		err = FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &deviceID);
		switch (deviceID) {
			case '3000':
				retval = iQue3000;
				break;
			case '3200':
				retval = iQue3200;
				break;
			case '3600':
				retval = iQue3600;
				break;
			case '3700':
				retval = iQue3600a;
				break;
			default:
				// If none of the above, still need to return 0 so that other calls will work properly
				retval = 0;
				break;
		}
	}

	return(retval);
}

Boolean XferInitiQue()
{
	Err err=errNone;
	Boolean retval=true;
	UInt16  cardNo;     /* Card number used by SysNotifyUnregister  */
	LocalID dbID;       /* Database ID used by SysNotifyUnregister  */

	err = SysLibFind(gpsLibName, &iQue_ref);
	if (err != errNone) {
		err = SysLibLoad( gpsLibType, gpsLibCreator, &iQue_ref );
		if ( err ) {
			FrmCustomAlert(WarningAlert, "Can't load Garmin GPSLIB"," "," ");
			retval = false;
		} else {
//			FrmCustomAlert(WarningAlert, "GPSLib Loaded New"," "," ");
		}
	} else {
//		FrmCustomAlert(WarningAlert, "GPSLib Loaded Already-Not loading again"," "," ");
	}
	//----------------------------------------------------------
	//Open the connection to the iQue GPS
	//----------------------------------------------------------
	if (GPSStillOpen == false) {
		err = GPSOpen(iQue_ref);
//		FrmCustomAlert(WarningAlert, "GPS Not Still Open-GPS Opened"," "," ");
	} else {
//		FrmCustomAlert(WarningAlert, "GPS Still Open-Not Opening"," "," ");
	}
	if ( err ) {
		FrmCustomAlert(WarningAlert, "Can't open Garmin GPS"," "," ");
		retval = false;
	} else {
		//----------------------------------------------------------
		//Register to recieve notifications from the GPS library
		//----------------------------------------------------------
		err = SysCurAppDatabase( &cardNo, &dbID );
		if ( err ) {
			FrmCustomAlert(WarningAlert, "Can't get App DB Info"," "," ");
			retval = false;
		} else {
			err = SysNotifyRegister( cardNo, dbID, sysNotifyGPSDataEvent, 
											 GpsLibEventCallback, sysNotifyNormalPriority, NULL );
			if ( err ) {
				FrmCustomAlert(WarningAlert, "Can't Register Garmin Notification"," "," ");
				retval = false;
			}
		}
	}
	return(retval);
}

void XferCloseiQue()
{
	Err err;
	UInt16  cardNo;     /* Card number used by SysNotifyUnregister  */
	LocalID dbID;       /* Database ID used by SysNotifyUnregister  */

	//----------------------------------------------------------
	//Unregister notifications from the GPS library
	//----------------------------------------------------------
	err = SysCurAppDatabase( &cardNo, &dbID );
	if ( err ) {
		FrmCustomAlert(WarningAlert, "Can't get App DB Info"," "," ");
	} else {
		err = SysNotifyUnregister( cardNo, dbID, sysNotifyGPSDataEvent, sysNotifyNormalPriority );
		if ( err ) {
			FrmCustomAlert(WarningAlert, "Can't Unregister Garmin Notification"," "," ");
		}
	}


	//----------------------------------------------------------
	//Close GPS Library; unload it if we're the last app using it.
	//----------------------------------------------------------
	// Close the library.
	err = GPSClose(iQue_ref);
	if( err != gpsErrStillOpen ) {
		GPSStillOpen = false;
//		FrmCustomAlert(WarningAlert, "GPS Closed"," "," ");
		if (GPSStillOpen==false) {
			SysLibRemove(iQue_ref);
//			FrmCustomAlert(WarningAlert, "GPSLib Removed"," "," ");
		} else {
//			FrmCustomAlert(WarningAlert, "GPSLib Not Removed-Not Loaded Here or Still Open"," "," ");
		}
	} else {
		GPSStillOpen = true;
//		FrmCustomAlert(WarningAlert, "GPS Still Open-Not Closed"," "," ");
	}
}

/******************************************************************************
*
*   PROCEDURE NAME:
*       GpsLibEventCallback - Display Change Event Callback
*
*   DESCRIPTION:
*       Gets called when a GPS Library Event Notification occurs.
*
******************************************************************************/
Err GpsLibEventCallback( SysNotifyParamType *notifyParamsP )
{
	// Have to set this to false when the data gets changed
	GPSDataChanged = true;

	//----------------------------------------------------------
	//Per the documentation return 0.
	//----------------------------------------------------------
	return(0);

} /* GpsLibEventCallback() */

void GetiQueInfo()
{
	Char tempchar[25];
	Char nsew[2];
	UInt32 seconds;
	double utcSeconds;
	UInt8 numSats=0;        // max Number of Satellites   
	UInt8 numDisplayableSats=GPSMAXSATS;
	UInt8 i;
	static double gpsvar=0.0;
	static UInt16 DeviationCount=0;
	static Char prevgpsutc[10];

#ifdef NMEALOG
	Char tempchar2[25];
	Err gpsresp;

	gpsresp = GPSGetPVT(iQue_ref, pvtData);

//	StrCopy(tempchar2, "gpsresp-");
//	StrIToA(tempchar, gpsresp);
//	StrCat(tempchar, "\n");
//	StrCat(tempchar2, tempchar);
//	if (device.VFSCapable) TxData(tempchar2, USEVFS);

	StrCopy(tempchar2, "fix-");
	StrIToA(tempchar, pvtData->status.fix);
	StrCat(tempchar, "\n");
	StrCat(tempchar2, tempchar);
	if (device.VFSCapable) TxData(tempchar2, USEVFS);

//	StrCopy(tempchar2, "mode-");
//	StrIToA(tempchar, pvtData->status.mode);
//	StrCat(tempchar, "\n");
//	StrCat(tempchar2, tempchar);
//	if (device.VFSCapable) TxData(tempchar2, USEVFS);

	StrCopy(tempchar2, "GPSDChgd-");
	StrIToA(tempchar, GPSDataChanged);
	StrCat(tempchar, "\n");
	StrCat(tempchar2, tempchar);
	if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif



	if (GPSDataChanged) {
		if ( GPSGetPVT(iQue_ref, pvtData) != gpsErrNone ) {

#ifdef NMEALOG
			StrCopy(tempchar2, "1\n");
			if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

			if (pvtData->status.fix == gpsFixUnusable || pvtData->status.fix == gpsFixInvalid) {
				StrCopy(data.logger.gpsstat, "V");
			} else {
				StrCopy(data.logger.gpsstat, "A");
			}

/*
			if (!data.config.useiquesim) {
				//----------------------------------------------------------
				//Get satellite data
				//----------------------------------------------------------
				numSats = GPSGetMaxSatellites( iQue_ref );
				if ( GPSGetSatellites( iQue_ref, satData ) != 0 ) {
					MemSet( satData, numSats * sizeof( GPSSatDataType ), -1 );
				}
			}
*/
			data.application.changed = 1;
		} else if ((pvtData->status.mode != gpsModeSim) || data.config.useiquesim) {

#ifdef NMEALOG
			StrCopy(tempchar2, "2\n");
			if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

			// Determine the GPS UTC
			// data.logger.gpsutc - text
			/*----------------------------------------------------------
			Determine number of UTC seconds since the beginning of the
			current day.
			----------------------------------------------------------*/
			utcSeconds = pvtData->time.seconds + ( pvtData->time.fracSeconds * secondsPerFrac ); 

//			HostTraceOutputTL(appErrorClass, "utcSeconds-|%s|", DblToStr(utcSeconds, 1));

			/*----------------------------------------------------------
			Round the UTC seconds because fractional seconds aren't
			needed for this application.
			----------------------------------------------------------*/
			seconds = (UInt32)pround(utcSeconds, 0);
//			HostTraceOutputTL(appErrorClass, "seconds-|%hu|", seconds);
			SecondsToDateOrTimeString(seconds, data.logger.gpsutc, 2, 0);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpsutc-|%s|", data.logger.gpsutc);

			// Determine the GPS DTG
			// data.logger.gpsdtg - text
			SecondsToDateOrTimeString(TimGetSeconds(), data.logger.gpsdtg, 4, 0);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpsdtg-|%s|", data.logger.gpsdtg);

			// Determine the GPS Status (A or V)
			// data.logger.gpsstat - text
			if (pvtData->status.fix == gpsFixUnusable || pvtData->status.fix == gpsFixInvalid) {
				StrCopy(data.logger.gpsstat, "V");
				// Did this to try to workaround the fact that the iQue outputs a time
				// of 12 midnite when the satellite signal is lost.
				StrCopy(data.logger.gpsutc, prevgpsutc);
			} else {
				StrCopy(data.logger.gpsstat, "A");
				StrCopy(prevgpsutc, data.logger.gpsutc);
			}

			// Put the one sigma estimated position into the epe value;
			// Multiplied by 2 to get a 2 sigma value
			data.input.eph = (double)pvtData->status.eph * 2.0;

			// Determine the Number of Active GPS Satellites
			// data.input.gpsnumsats - text
			// data.input.siu - Int8
			//----------------------------------------------------------
			// Set up the Satellite Array
			//----------------------------------------------------------
			numSats = GPSGetMaxSatellites( iQue_ref );

			/*----------------------------------------------------------
			Get satellite data
			----------------------------------------------------------*/
			if ( GPSGetSatellites( iQue_ref, satData ) != 0 ) {
				MemSet( satData, numSats * sizeof( GPSSatDataType ), 0xff );
			}

			//-----------------------------------------------------------------------
			//Iterate through the satellites and and determine how many are active.
			//-----------------------------------------------------------------------
			numSats = 0;
			for ( i = 0; i < numDisplayableSats; ++i ) {
				//--------------------------------------------------
				//Get data
				//--------------------------------------------------
				if (satData[i].status & gpsSatUsedMask) {
					data.input.svns[i] = (Int8)satData[i].svid;
					numSats++;
				} else {
					data.input.svns[i] = -1;
				}
			}
			data.input.siu = (Int8)numSats;
			StrIToA(data.input.gpsnumsats, numSats);
			StrCopy(data.input.gpsnumsats, leftpad(data.input.gpsnumsats, '0', 2));

			if (pvtData->status.fix > gpsFixInvalid) {
			// Get the Latitude
			if (!draw_log) data.input.gpslatdbl = (double)pvtData->position.lat * semiToDeg;
			data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
			StrPrintF(nsew, "%c", LLToStringDM(data.input.gpslatdbl, tempchar, ISLAT, false, true, 4));
			tempchar[StrLen(tempchar)-1] = '\0';
			StrCopy(data.logger.gpslat, tempchar);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpslat-|%s|", data.logger.gpslat);
			StrCopy(data.logger.gpslngdir, nsew);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpslatdir-|%s|", data.logger.gpslatdir);

			// Get the Longitude
			if (!draw_log) data.input.gpslngdbl = (double)pvtData->position.lon * semiToDeg;
			StrPrintF(nsew, "%c", LLToStringDM(data.input.gpslngdbl, tempchar, ISLON, false, true, 4));
			tempchar[StrLen(tempchar)-1] = '\0';
			StrCopy(data.logger.gpslng, tempchar);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpslng-|%s|", data.logger.gpslng);
			StrCopy(data.logger.gpslngdir, nsew);
//			HostTraceOutputTL(appErrorClass, "data.logger.gpslngdir-|%s|", data.logger.gpslngdir);

			// Get the GPS Altitude (MSL)
			//Altitude in meters divided by ALTMETCONST to convert to feet.
			data.logger.gpsalt = (double)pvtData->position.altMSL/ALTMETCONST;
			// Using WGS84 altitude for now because it is more accurate
//			data.logger.gpsalt = (double)pvtData->position.altWGS84/ALTMETCONST;

			// Finding the altitude difference between MSL and WGS84 to put into
			// the geoidal difference variable mostly just for display.
			data.input.gpsgeoidaldiff = 
						(double)(pvtData->position.altWGS84 - pvtData->position.altMSL) / ALTMETCONST;

			if ((data.config.pressaltsrc == GPSALT) || (!recv_palt)) {
//				data.logger.pressalt = data.logger.gpsalt;
// PG : Code changes for QNH pressure adjustment to get accurate FL, or if no press altitude data received yet
				data.logger.pressalt = data.logger.gpsalt + data.input.QNHaltcorrect;
// PG : ends
			}

			// Called with gps altitude but it really just uses the current "up" 
			// value  that is put into the vario value
//			if (data.config.flightcomp != C302COMP) {
				CalcLift(data.logger.gpsalt, data.logger.gpsutc, -9999.9, NORESET);
//			}
//			HostTraceOutputTL(appErrorClass, "Calc Lift Garmin");

			// Get the ground speed
			// Have to convert from m/s to knots
			data.input.ground_speed.value=(double)pvtData->velocity.speed / AIRMPSCONST;
			data.input.ground_speed.valid=VALID;
			if (data.input.ground_speed.value != 0.0) {
				// Get the true track direction
				data.input.true_track.value=nice_brg(RadiansToDegrees((double)pvtData->velocity.track));
//				HostTraceOutputTL(appErrorClass, "data.input.true_track.value-|%s|", 
//											DblToStr(data.input.true_track.value, 1));
				data.input.true_track.valid=VALID;

				if (DeviationCount == 0) {
					// Calculate the GPS Variation
					gpsvar = GetDeviation();
//					HostTraceOutputTL(appErrorClass, "gpsvar-|%s|", DblToStr(gpsvar, 1));
				}

				DeviationCount++;
				if (DeviationCount == 120) {
					DeviationCount = 0;
				}

				// Calculate the magnetic track direction
				data.input.magnetic_track.value = nice_brg(data.input.true_track.value + gpsvar);
				data.input.magnetic_track.valid=VALID;
//				HostTraceOutputTL(appErrorClass, "data.input.magnetic_track.value-|%s|", 
//											DblToStr(data.input.magnetic_track.value, 1));
			}
			// Get the vertical speed
			// Have to convert from m/s to knots
			data.input.vario=(double)pvtData->velocity.up / AIRMPSCONST;
			/* Save Maximum Altitude  in feet*/
			if (data.input.maxalt < data.logger.gpsalt) {
				data.input.maxalt = data.logger.gpsalt;
			}
			/* Save Minimum Altitude  in feet*/
			if (data.input.minalt > data.logger.gpsalt) {
				data.input.minalt = data.logger.gpsalt;
			}

			}

			if ((StrCompare(data.logger.gpsstat, "V") == 0) || draw_log) {
				updatetime = false;
			} else {
				updatemap = true;
				updatetime = true;
				// This is the only place that updatewind should be set otherwise it causes havoc with
				// the thermal/cruise code
//				HostTraceOutputTL(appErrorClass, "Garmin:setting updatewind to true");
				updatewind = true;
			}

			data.application.changed = 1;
		} else {

#ifdef NMEALOG
			StrCopy(tempchar2, "3\n");
			if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

			StrCopy(data.logger.gpsstat, "V");
			StrCopy(data.input.gpsnumsats, "00");
			pvtData->status.fix = gpsFixUnusable;
/*
			if (data.config.useiquesim) {
				//----------------------------------------------------------
				//Get satellite data
				//----------------------------------------------------------
				numSats = GPSGetMaxSatellites( iQue_ref );
				if ( GPSGetSatellites( iQue_ref, satData ) != 0 ) {
					MemSet( satData, numSats * sizeof( GPSSatDataType ), -1 );
				}
			}
*/

			data.application.changed = 1;
		}
	} else {

#ifdef NMEALOG
		StrCopy(tempchar2, "4\n");
		if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

		if (!data.config.useiquesim) {

#ifdef NMEALOG
			StrCopy(tempchar2, "5\n");
			if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

			if ( GPSGetPVT( iQue_ref, pvtData ) != gpsErrNone ) {

#ifdef NMEALOG
				StrCopy(tempchar2, "6\n");
				if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

			pvtData->status.fix = gpsFixUnusable;
				MemSet( satData, numSats * sizeof( GPSSatDataType ), 0xff );
			} else if (pvtData->status.mode != gpsModeSim) {

#ifdef NMEALOG
				StrCopy(tempchar2, "7\n");
				if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

				//----------------------------------------------------------
				//Get satellite data
				//----------------------------------------------------------
				numSats = GPSGetMaxSatellites( iQue_ref );
				if ( GPSGetSatellites( iQue_ref, satData ) != 0 ) {
					MemSet( satData, numSats * sizeof( GPSSatDataType ), 0xff );
				}
				if (pvtData->status.fix == gpsFixUnusable || pvtData->status.fix == gpsFixInvalid) {
					StrCopy(data.logger.gpsstat, "V");
				} else {
					StrCopy(data.logger.gpsstat, "A");
				}
			} else {

#ifdef NMEALOG
				StrCopy(tempchar2, "8\n");
				if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

				StrCopy(data.logger.gpsstat, "V");
				StrCopy(data.input.gpsnumsats, "00");
				pvtData->status.fix = gpsFixUnusable;
				numSats = 0;
				MemSet( satData, numSats * sizeof( GPSSatDataType ), 0xff );
			}
			data.application.changed = 1;
		}
	}
	GPSDataChanged = false;

#ifdef NMEALOG
	StrCopy(tempchar2, "------\n");
	if (device.VFSCapable) TxData(tempchar2, USEVFS);
#endif

	return;
}
