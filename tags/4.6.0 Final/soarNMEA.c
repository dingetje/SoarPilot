#include <PalmOS.h>	// all the system toolbox headers
#include <SerialMgr.h>
#include <MemoryMgr.h>
#include <DLServer.h>
#include "soaring.h"
#include "soarNMEA.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarMap.h"
#include "soarUMap.h"
#include "soarWind.h"
#include "Mathlib.h"
#include "soarMath.h"
#include "soarRECO.h"
#include "soarIO.h"
#include "soarSUA.h"
#include "soarGPSInfo.h"
#include "AddIncs/GarminIncs/GPSLib68K.h"
#include "soarGEOMAG.h"
#include "soarComp.h"
#include "soarWay.h"

#define INVALID 0
#define GPRMC_VALID 1
#define GPRMB_VALID 2
#define GPGGA_VALID 3
#define PGRMZ_VALID 4
#define PGCS_VALID  5
#define PBB50_VALID 6
#define PCAIB_VALID 7
#define PCAID_VALID 8
#define PFSRA_VALID 9
#define LXWP0_VALID 10
#define PMGNST_VALID 11
#define W_VALID 12
#define RECOA_VALID 13
#define RECOB_VALID 14
#define GPGSA_VALID 15
#define PGRME_VALID 16
#define PTAS1_VALID 17
#define GPGSV_VALID 18
#define PILC_VALID  19
#define PFLAU_VALID 20
#define PFLAA_VALID 21
#define PZAN1_VALID 22
#define GPWIN_VALID 23
#define PWES0_VALID 24
#define PWES1_VALID 25

Boolean recv_data = false;	// receiving GPS data
Boolean recv_palt = false;	// receing pressure alititude data
GPSSatDataType *satData;	// Arrays for GPS satellite data
GPSPVTDataType *pvtData;
Boolean updatewind = false;
Boolean updatetime = false;

UInt8 curgpsfixquality;
Int8 curgpsfixtype = -1;

extern Boolean updatemap;
extern RECOData *recodata;
extern GPSSatDataType *satDataFlarm;
extern Boolean draw_log;
extern UInt8 FlarmRX;
extern Int16 FlarmVSep;
extern Int16 FlarmIdx;
extern Boolean Flarmpresent;
extern Boolean FlarmAlarm;
extern UInt32 utcsecs;
extern Boolean ManualDistChg;
extern double MGCBattHours;
extern double MCCurVal;

void nmea_parser(Char* serinp, UInt32 length, Boolean reset)
{

	Int16 state = INVALID;
	static Int16 next = 0;
	static Boolean gpsver2 = false;
	static Boolean gpgsa = false;
	static Boolean gpgga = false;
	static Int16 revert;
	static Char NMEAbuf[85];
	static Boolean gpsephavail = false;
	UInt32 cur = 0;
	double true_track;
	double magnetic_track;
	double groundspeed;
	double bearing_to_destination;
	double distance_to_destination;
	double gpsvar;
	double ballast;
	static Char gpsutc[10] = "000000000";
	Char gpsdtg[7];
	Char gpsstat[5];
	Char gpslat[21];
	Char gpslatdir[5];
	Char gpslng[22];
	Char gpslngdir[5];
	Char gpsvardir[5];
	Char gpsnumsats[5];
	Char tempchar[20];
	DateTimeType tempdt;
	UInt32 tempsecs;
	double tempalt;
	Int8 j;
	Int8 i;
	Boolean readposdata = true;
	static UInt16 DeviationCount=0;
	static Int8 NumGSVMsgs=0;
	static Int8 CurGSVMsg=0;
	static Int8 ExpectedGSVMsg=1;
	static double gpsalt;
	static double gpsgeoidaldiff;
	double tempelev, tempdist;
	Int16 tempnum;
	double RelN, RelE;
	static Int16 FlarmMaxIdx;
	static Int16 FlarmMaxDist;
	char mychksumstr[10];
	UInt16 mychksum = 0;
	double true_bearing, vmg;
	static UInt32 last_GPGGA = 0;
	static UInt32 last_GPRMC = 0;

	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		MemSet(&NMEAbuf,sizeof(NMEAbuf),0);
		gpsver2 = false;
		gpgsa = false;
		return;
	}

	if (data.config.nmeaxfertype == USEIQUESER) {
		readposdata = false;
	}

//	HostTraceOutputTL(appErrorClass, "Inside nmea_parser");
	while (cur<length) {
		state = INVALID;
		while( (cur<length) && (next<85) && (serinp[cur] != '$') && (serinp[cur] != '!') ) {
			NMEAbuf[next]=serinp[cur];
			next++;
			cur++;
		}
//		HostTraceOutputTL(appErrorClass, "NMEAbuf1-|%s|", NMEAbuf);
		if (next >= 85) {
			/* Parsing error start over */
			next=0;
		}
		if ((cur<length) && (serinp[cur] == '$'||serinp[cur] == '!')) {
			cur++;
//			HostTraceOutputTL(appErrorClass, "NMEAbuf2-|%s|", NMEAbuf);
			if (NMEAbuf[0]!='G' && NMEAbuf[0]!='P' && NMEAbuf[0]!='L' && NMEAbuf[0]!='w' && NMEAbuf[0]!='R'){
				/* Parsing error start over*/
				next=0;
			} else {
				NMEAbuf[next]=0;
				if (NMEAbuf[0] == 'w') {
//					HostTraceOutputTL(appErrorClass, "     W");
					if (gpsvalidate(NMEAbuf)) {
						state = W_VALID;
					} else {
						state = INVALID;
//						HostTraceOutputTL(appErrorClass, "   Bad W|%s|", NMEAbuf);
					}
				} else {
					switch (NMEAbuf[2]) {
						case 'A':
							if ((NMEAbuf[3]=='I') && (NMEAbuf[4]=='B')) {
//								HostTraceOutputTL(appErrorClass, "     PCAIB");
								if (gpsvalidate(NMEAbuf)) {
									state = PCAIB_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PCAIB|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='I') && (NMEAbuf[4]=='D')) {
//								HostTraceOutputTL(appErrorClass, "     PCAID");
								if (gpsvalidate(NMEAbuf)) {
									state = PCAID_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PCAID|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='S') && (NMEAbuf[4]=='1')) {
//								HostTraceOutputTL(appErrorClass, "     PTAS1");
								if (gpsvalidate(NMEAbuf)) {
									state = PTAS1_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PTAS1|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='N') && (NMEAbuf[4]=='1')) {
//								HostTraceOutputTL(appErrorClass, "     PZAN1");
								if (gpsvalidate(NMEAbuf)) {
									state = PZAN1_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PZAN1|%s|", NMEAbuf);
								}
							}
							break;
						case 'B': 
							if ((NMEAbuf[3]=='5') && (NMEAbuf[4]=='0')) {
//								HostTraceOutputTL(appErrorClass, "     PBB50");
								if (gpsvalidate(NMEAbuf)) {
									state = PBB50_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PBB50|%s|", NMEAbuf);
								}
							}
							break;
						case 'R': 
							if ((NMEAbuf[3]=='M') && (NMEAbuf[4]=='Z')) {
//								HostTraceOutputTL(appErrorClass, "     PGRMZ");
								if (gpsvalidate(NMEAbuf) && readposdata) {
									state = PGRMZ_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PGRMZ|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='M') && (NMEAbuf[4]=='C')) {
//								HostTraceOutputTL(appErrorClass, "     GPRMC found");
								if (gpsvalidate(NMEAbuf) && readposdata) {
									state = GPRMC_VALID;
									if ((data.config.nmeaxfertype == USEBT) && data.config.echoNMEA && (utcsecs > last_GPRMC + data.config.slowlogint)) {
										last_GPRMC = utcsecs;
										// re-transmit GPRMC data to pass through port
										StrCopy(tempchar, "$");
										TxDataAlt(tempchar);
										TxDataAlt(NMEAbuf);
										// transmit GPRMB data to pass through port
										if (data.input.destination_valid) {
											// construct GPRMB data and to pass through port
											StrCopy(NMEAbuf, "$GPRMB,");
											// GPS fix status
											StrCat(NMEAbuf, data.logger.gpsstat);
											StrCat(NMEAbuf, ",");
											// cross track error
											StrCat(NMEAbuf, "0.00,L,");
											// origin waypoint ID (current position)
											StrCat(NMEAbuf, "POS,");
											// destination waypoint ID
											StrCat(NMEAbuf, Left(data.inuseWaypoint.name,3));
											StrCat(NMEAbuf, ",");
											// waypoint latitude
											LLToStringDM(data.inuseWaypoint.lat, tempchar, true, false, true, 2);
											StrCat(NMEAbuf, Left(tempchar, 7));
											if (data.inuseWaypoint.lat > 0.0) {
												StrCat(NMEAbuf, ",N,");
											} else {
												StrCat(NMEAbuf, ",S,");
											}
											// waypoint longitude
											LLToStringDM(data.inuseWaypoint.lon, tempchar, false, false, true, 2);
											StrCat(NMEAbuf, Left(tempchar,8));
											if (data.inuseWaypoint.lon > 0.0) {
												StrCat(NMEAbuf, ",E,");
											} else {
												StrCat(NMEAbuf, ",W,");
											}
											// range
											if (data.inuseWaypoint.distance < 999.0) {
												StrCat(NMEAbuf, leftpad(DblToStr(data.inuseWaypoint.distance, 1), '0', 3));
											} else {
												StrCat(NMEAbuf, "999.9");
											}
											StrCat(NMEAbuf, ",");
											// bearing (true)
											true_bearing = nice_brg(data.inuseWaypoint.bearing - data.input.deviation.value);
											StrCat(NMEAbuf, leftpad(DblToStr(true_bearing, 1), '0', 3));
											StrCat(NMEAbuf, ",");
											// velocity towards destination waypoint (VMG)
											if ((data.input.ground_speed.valid == VALID) && (data.input.true_track.valid == VALID)) {
												vmg = Cos(DegreesToRadians(true_bearing - data.input.true_track.value)) * data.input.ground_speed.value;
												if (vmg < 0.0) vmg = 0.0;
												StrCat(NMEAbuf, leftpad(DblToStr(vmg, 1), '0', 3));
											} else {
												StrCat(NMEAbuf,"000.0");
											}
											StrCat(NMEAbuf, ",");
											// arrival flag (not arrived)
											StrCat(NMEAbuf, "V");
											// checksum
											mychksum = 0;
											for (i=1; i < StrLen(NMEAbuf); i++) {
												mychksum^=NMEAbuf[i];
											}
											StrIToH(tempchar, mychksum);
											length = StrLen(tempchar);
											mychksumstr[0] = tempchar[length-2];
											mychksumstr[1] = tempchar[length-1];
											mychksumstr[2] = '\0';
											StrCat(NMEAbuf,"*");
											StrCat(NMEAbuf,mychksumstr);
											// end of line
											StrCatEOL(NMEAbuf, USESER);
											// confirm checksum
											if (gpsvalidate(&NMEAbuf[1])) TxDataAlt(NMEAbuf);
										}
									}
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPRMC|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='M') && (NMEAbuf[4]=='B') && !data.config.usepalmway) {
//								HostTraceOutputTL(appErrorClass, "     GPRMB");
								if (gpsvalidate(NMEAbuf)) {
									state = GPRMB_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPRMB|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='M') && (NMEAbuf[4]=='E')) {
//								HostTraceOutputTL(appErrorClass, "     GPRME");
								if (gpsvalidate(NMEAbuf) && readposdata) {
									state = PGRME_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPRME|%s|", NMEAbuf);
								}
							}
							break;
						case 'G': 
							if ((NMEAbuf[3]=='G') && (NMEAbuf[4]=='A')) {
//								HostTraceOutputTL(appErrorClass, "     GPGGA");
								if (data.config.usechksums) {
									if (gpsvalidate(NMEAbuf) && readposdata) {
										state = GPGGA_VALID;
										if ((data.config.nmeaxfertype == USEBT) && data.config.echoNMEA && (utcsecs > last_GPGGA + data.config.slowlogint)) {
											last_GPGGA = utcsecs;
											// re-transmit GPGGA data to pass through port
											StrCopy(tempchar, "$");
											TxDataAlt(tempchar);
											TxDataAlt(NMEAbuf);
										}
									} else {
										state = INVALID;
//										HostTraceOutputTL(appErrorClass, "   Bad GPGGA|%s|", NMEAbuf);
									}
								} else {
									if (readposdata) {
										state = GPGGA_VALID;
									} else {
										state = INVALID;
									}
								}
							} else if ((NMEAbuf[3]=='S') && (NMEAbuf[4]=='A')) {
//								HostTraceOutputTL(appErrorClass, "     GPGSA");
								if (gpsvalidate(NMEAbuf) && readposdata) {
									state = GPGSA_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPGSA|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='S') && (NMEAbuf[4]=='V')) {
//								HostTraceOutputTL(appErrorClass, "     GPGSV");
								if (gpsvalidate(NMEAbuf) && readposdata && (data.config.flightcomp != FLARMCOMP) &&
										FrmGetActiveFormID() == form_sat_status) {
									state = GPGSV_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPGSV|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='N') && (NMEAbuf[4]=='S')) {
//								HostTraceOutputTL(appErrorClass, "     PMGNST");
								if (gpsvalidate(NMEAbuf) && (FrmGetActiveFormID() == form_sat_status)) {
									state = PMGNST_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PMGNST|%s|", NMEAbuf);
								}
							}
							break;
						case 'C': 
							if ((NMEAbuf[3]=='S') && (NMEAbuf[5]=='1')) {
//								HostTraceOutputTL(appErrorClass, "     PGCS");
								if (gpsvalidate(NMEAbuf)) {
									state = PGCS_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PGCS|%s|", NMEAbuf);
								}
							} else if (NMEAbuf[3]=='O') {
								if (NMEAbuf[4]=='A') {
//									HostTraceOutputTL(appErrorClass, "     RECOA");
									if (gpsvalidate(NMEAbuf)) {
										state = RECOA_VALID;
									} else {
										state = INVALID;
//										HostTraceOutputTL(appErrorClass, "   Bad RECOA|%s|", NMEAbuf);
									}
								} else if (NMEAbuf[4]=='B') {
//									HostTraceOutputTL(appErrorClass, "     RECOB");
									if (gpsvalidate(NMEAbuf)) {
										state = RECOB_VALID;
									} else {
										state = INVALID;
//										HostTraceOutputTL(appErrorClass, "   Bad RECOB|%s|", NMEAbuf);
									}
								}
							} 
							break;
						case 'S': 
							if ((NMEAbuf[3]=='R') && (NMEAbuf[4]=='A')) {
//								HostTraceOutputTL(appErrorClass, "     PFSRA");
								if (gpsvalidate(NMEAbuf)) {
									state = PFSRA_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PFSRA|%s|", NMEAbuf);
								}
							}
							break;
						case 'W': 
							if ((NMEAbuf[3]=='P') && (NMEAbuf[4]=='0')) {
//								HostTraceOutputTL(appErrorClass, "     LXWP0");
								if (gpsvalidate(NMEAbuf)) {
									state = LXWP0_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad LXWP0|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='I') && (NMEAbuf[4]=='N')) {
//								HostTraceOutputTL(appErrorClass, "     GPWIN");
								if (gpsvalidate(NMEAbuf)) {
									state = GPWIN_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad GPWIN|%s|", NMEAbuf);
								}
							}
							break;
						case 'L': 
							if ((NMEAbuf[3]=='C')) {
//								HostTraceOutputTL(appErrorClass, "     PILC");
								if (gpsvalidate(NMEAbuf)) {
									state = PILC_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PILC|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='A') && (NMEAbuf[4]=='U')) {
//								HostTraceOutputTL(appErrorClass, "     PFLAU");
								if (gpsvalidate(NMEAbuf)) {
									state = PFLAU_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PFLAU|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='A') && (NMEAbuf[4]=='A')) {
//								HostTraceOutputTL(appErrorClass, "     PFLAA");
								if (gpsvalidate(NMEAbuf)) {
									state = PFLAA_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PFLAA|%s|", NMEAbuf);
								}
							}
							break;
						case 'E':
							if ((NMEAbuf[3]=='S') && (NMEAbuf[4]=='0')) {
//								HostTraceOutputTL(appErrorClass, "     PWES0");
								if (gpsvalidate(NMEAbuf)) {
									state = PWES0_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PWES0|%s|", NMEAbuf);
								}
							} else if ((NMEAbuf[3]=='S') && (NMEAbuf[4]=='1')) {
//								HostTraceOutputTL(appErrorClass, "     PWES1");
								if (gpsvalidate(NMEAbuf)) {
									state = PWES1_VALID;
								} else {
									state = INVALID;
//									HostTraceOutputTL(appErrorClass, "   Bad PWES1|%s|", NMEAbuf);
								}
							}
							break;
						default:
							break;
					}
				}
				next=0;
			}
		}
	
// Parse Valid NMEA Sentences
		switch (state) {

// NMEA183 Sentence to give distance & bearing to target waypoint
// Not Necessary if using internal waypoints
			case GPRMB_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser RMB");
				//	MFH Getting waypoint id
				GetField(NMEAbuf, 5, tempchar);
				tempelev = 0.0;
				StrNCopy(data.input.destination_name, tempchar, 16);
				// decode altitude if in NNNXXX format with XXX = meters / 10
				if ((data.config.wpformat == WPGPWPLALT) && (StrLen(tempchar) == 6)) {
					if (StrChr("0123456789",tempchar[4]) && StrChr("0123456789",tempchar[5]) && StrChr("0123456789",tempchar[6])) {
						tempelev = pround((double)StrAToI(Right(tempchar,3)) * 10 / ALTMETCONST, 0);
//						HostTraceOutputTL(appErrorClass, "tempelev = %s", DblToStr(tempelev,0));
					}
				}				
				// PG getting destination lat
				GetField(NMEAbuf, 6, tempchar);
				StrNCopy(gpslat, tempchar, 20);
				GetField(NMEAbuf, 7, tempchar);
				StrCopy(gpslatdir, tempchar);
				data.input.destination_lat = DegMinStringToLatLon(gpslat, gpslatdir[0]);
//				HostTraceOutputTL(appErrorClass, "Destination Lat |%s|", DblToStr(data.input.destination_lat,2));
				// PG getting destination lon
				GetField(NMEAbuf, 8, tempchar);
				StrNCopy(gpslng, tempchar, 21);
				GetField(NMEAbuf, 9, tempchar);
				StrCopy(gpslngdir, tempchar);
				data.input.destination_lon = DegMinStringToLatLon(gpslng, gpslngdir[0]);
//				HostTraceOutputTL(appErrorClass, "Destination Lon |%s|", DblToStr(data.input.destination_lon,2));
				//	MFH Getting distance to waypoint in nautical miles
				GetField(NMEAbuf, 10, tempchar);
				distance_to_destination=StrToDbl(tempchar);
				//	MFH Getting bearing to waypoint in true degrees then convert to mag
				GetField(NMEAbuf, 11, tempchar);
				bearing_to_destination=StrToDbl(tempchar)+data.input.deviation.value;
				if (bearing_to_destination >= 360.0) {
					bearing_to_destination -= 360.0;
				}
				if (bearing_to_destination < 0.0) {
					bearing_to_destination = 360.0 + bearing_to_destination;
				}
				if (StrLen(tempchar) > 2) {
					// valid goto waypoint from GPS
					data.input.distance_to_destination.value = distance_to_destination; 
					data.input.distance_to_destination.valid = VALID;
					data.input.bearing_to_destination.value = bearing_to_destination;
					data.input.bearing_to_destination.valid = VALID;
					data.input.destination_elev = tempelev;
				} else if (!ManualDistChg) {
					// no valid goto waypoint from GPS
					data.input.destination_name[0] = 0;
					data.input.destination_elev = 0.0;
					data.input.distance_to_destination.valid = VALID;
					data.input.bearing_to_destination.valid = NOT_VALID;
					data.input.bearing_to_destination.value = 000.0;
				} else {
					// manual distance change
					data.input.distance_to_destination.valid = VALID;
					data.input.bearing_to_destination.valid = NOT_VALID;
					data.input.bearing_to_destination.value = 000.0;
				}
				data.input.destination_valid = false;
				data.application.changed = 1;
				break;
			} // End of GPRMB_VALID:
	
// NMEA183 Sentence to give course, speed and determine magnetic variation
// Also gives GPS Time, Status & LAT/LNG for logging
			case GPRMC_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser RMC");

				if (!gpsver2) {
					GetField(NMEAbuf, 1, tempchar);
					if (StrCompare(tempchar, "") == 0) {
						// If the incoming data is blank, then use the previous gpsutc if not blank.
						if (StrCompare(gpsutc, "") == 0) {
							// If both the incoming data and gpsutc are blank, use system time adjusted for the timezone
							SecondsToDateOrTimeString(TimGetSeconds(), gpsutc, 2, data.config.timezone*-1);
						}
					} else {
						if (StrLen(tempchar) > 6) {
							StrNCopy(gpsutc, tempchar, 6);
							gpsutc[6] = '\0';
						} else {
							StrCopy(gpsutc, tempchar);
						}
						if (gpsutc[4] == '6') {
							// This is to fix the time output by some GPS' which
							// sometimes outputs a time of 144060 instead of 144100.
							// Magellan GPS Companion seems to do this.
							StringToDateOrTime(gpsutc, &tempdt, 1);
							tempsecs=TimDateTimeToSeconds(&tempdt);
							SecondsToDateOrTimeString(tempsecs, gpsutc, 2, 0);
						}
					}
					GetField(NMEAbuf, 2, tempchar);
					StrCopy(gpsstat, tempchar);
					GetField(NMEAbuf, 3, tempchar);
					StrNCopy(gpslat, tempchar, 20);
					GetField(NMEAbuf, 4, tempchar);
					StrCopy(gpslatdir, tempchar);
					GetField(NMEAbuf, 5, tempchar);
					StrNCopy(gpslng, tempchar, 21);
					GetField(NMEAbuf, 6, tempchar);
					StrCopy(gpslngdir, tempchar);
		
					StrCopy(gpsnumsats, "XX");
				}
				GetField(NMEAbuf, 7, tempchar);
				if (StrCompare(tempchar, "") == 0) {
					groundspeed = 0.0;
				} else {
					groundspeed = StrToDbl(tempchar);
				}
				GetField(NMEAbuf, 8, tempchar);
				if (StrCompare(tempchar, "") == 0) {
					true_track = 0.0;
				} else {
					true_track = StrToDbl(tempchar);
				}
				GetField(NMEAbuf, 9, tempchar);
				StrCopy(gpsdtg, tempchar);
				GetField(NMEAbuf, 10, tempchar);
				if (StrCompare(tempchar, "") == 0) {
					gpsvar = 0.0;
				} else {
					gpsvar = StrToDbl(tempchar);
				}
				GetField(NMEAbuf, 11, tempchar);
				if (StrCompare(tempchar, "") == 0) {
					StrCopy(gpsvardir, "W");
				} else {
					StrCopy(gpsvardir, tempchar);
				}
	
//				HostTraceOutputTL(appErrorClass, "RMC:true_track:|%s|", DblToStr(true_track, 2));
				if (true_track < 0.0 || true_track > 360.0) {
//					HostTraceOutputTL(appErrorClass, "RMC:setting gpsstat to V");

//					StrCopy(data.logger.gpsstat, "V");
//					StrCopy(data.input.gpsnumsats, gpsnumsats);
//					state = INVALID;
					true_track=nice_brg(true_track);
//					return;
				}
	
				if (true_track == 360.0) {
					true_track = 0.0;
				}
	
				if ((data.config.flightcomp == RECOCOMP) ||  data.config.gpscalcdev) { 
					if (DeviationCount == 0) {
						// Calculate the GPS Variation
						data.input.deviation.value = GetDeviation();
//						HostTraceOutputTL(appErrorClass, "gpsvar-|%s|", DblToStr(gpsvar, 1));
						data.input.deviation.valid = VALID;
					}

					DeviationCount++;
					if (DeviationCount == 120) {
						DeviationCount = 0;
					}
				} else {
					//Mag = True - Var(E) or + Var(W)
					//so
					//Magnetic = True + variation
					//True = Magnetic - variation
					if (StrCompare(gpsvardir, "E")==0) {
						gpsvar *= -1.0;
					}
					data.input.deviation.value = gpsvar;
					data.input.deviation.valid = VALID;
				}

				magnetic_track=nice_brg(true_track + data.input.deviation.value);
		
				data.input.ground_speed.value=groundspeed;
				data.input.ground_speed.valid=VALID;
				if (groundspeed >= 1.0) {
					data.input.magnetic_track.value=magnetic_track;
					data.input.magnetic_track.valid=VALID;
					data.input.true_track.value=true_track;
					data.input.true_track.valid=VALID;
				}
	
				StrCopy(data.logger.gpsdtg, gpsdtg);
	
				if (!gpsver2) {
//					HostTraceOutputTL(appErrorClass, "RMC:setting gpsstat to gpsstat");
					StrCopy(data.logger.gpsstat, gpsstat);
					StrCopy(gpsnumsats, "XX");
					StrCopy(data.input.gpsnumsats, gpsnumsats);
					StrCopy(data.logger.gpsutc, gpsutc);
					if ((StrCompare(data.logger.gpsstat, "A") == 0) && !draw_log) {
						StrCopy(data.logger.gpslat, gpslat);
						StrCopy(data.logger.gpslatdir, gpslatdir);
						data.input.gpslatdbl = DegMinStringToLatLon(data.logger.gpslat, data.logger.gpslatdir[0]);
						data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
						StrCopy(data.logger.gpslng, gpslng);
						StrCopy(data.logger.gpslngdir, gpslngdir);
						data.input.gpslngdbl = DegMinStringToLatLon(data.logger.gpslng, data.logger.gpslngdir[0]);					
					}
					// Calculate Lift Here RMC
					if (data.config.usepalt) {
						CalcLift(data.logger.pressalt, data.logger.gpsutc, -9999.9, NORESET);
//						HostTraceOutputTL(appErrorClass, "Calc Lift GGA-GPSALT");
					} else {
//					} else if (data.config.flightcomp == NOCOMP     ||
//						   data.config.flightcomp == B50COMP    ||
//						   data.config.flightcomp == LXCOMP     ||
//						   data.config.flightcomp == LXVARCOMP  ||
//						   data.config.flightcomp == RECOCOMP   ||
//						   data.config.flightcomp == B50VLKCOMP ||
//						   data.config.flightcomp == B50LXCOMP  ||
//						   data.config.flightcomp == SN10COMP   ||
//						   data.config.flightcomp == VOLKSCOMP  ||
//						   data.config.flightcomp == GPSNAVCOMP ||
//						   data.config.flightcomp == FLARMCOMP  ||
//						   data.config.flightcomp == C302ACOMP  ||
//						   data.config.flightcomp == EWCOMP) {
					// Might be able to use this instead of the above
//					} else if (data.config.flightcomp != C302COMP) {
						// Calculate using gps altitude because these types of computers do not output any alititude values
						// or the operator has chosen not to use the pressure altitude info
						CalcLift(data.logger.gpsalt, data.logger.gpsutc, -9999.9, NORESET);
//						HostTraceOutputTL(appErrorClass, "Calc Lift RMC-NOCOMP||B50COMP||LXCOMP||LXVARCOMP||EWCOMP||B50VLKCOMP||B50LXCOMP||RECOCOMP");
					}
					if (FrmGetActiveFormID() == form_sat_status && curgpsfixtype == -1) {
						if (StrCompare(data.logger.gpsstat, "V") == 0) {
							curgpsfixquality = 0;
						} else {
							curgpsfixquality = 1;
						}
						SetFixStatus();
					}
	
				}
				if ((StrCompare(data.logger.gpsstat, "V") == 0) || draw_log) {
					updatetime = false;
				} else {
					updatemap = true;
					updatetime = true;
					// This is the only place that updatewind should be set otherwise it causes havoc with 
					// the thermal/cruise code
//					HostTraceOutputTL(appErrorClass, "RMC:setting updatewind to true");
					updatewind = true;

				}
				data.application.changed = 1;
				break;

			} //End of GPRMC_VALID
	
// NMEA183 V2 Sentence to give GPS Altitude in meters
// as well as Number of SATS tracked and Height above Ellipsoid
			case GPGGA_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser GGA");
				gpsver2 = true;
				gpgga = true;
				revert = 0;
				GetField(NMEAbuf, 1, tempchar);
				if (StrCompare(tempchar, "") == 0) {
					// If the incoming data is blank, then use the previous gpsutc if not blank.
					if (StrCompare(gpsutc, "") == 0) {
						// If both the incoming data and gpsutc are blank, use system time adjusted for the timezone
						SecondsToDateOrTimeString(TimGetSeconds(), gpsutc, 2, data.config.timezone*-1);
//						HostTraceOutputTL(appErrorClass, "     Setting gpsutc to system utc time");
					}
				} else {
					if (StrLen(tempchar) > 6) {
						StrNCopy(gpsutc, tempchar, 6);
						gpsutc[6] = '\0';
					} else {
						StrCopy(gpsutc, tempchar);
					}
					if (gpsutc[4] == '6') {
						// This is to fix the time output by some GPS' which
						// sometimes outputs a time of 144060 instead of 144100.
						// Magellan GPS Companion seems to do this.
						StringToDateOrTime(gpsutc, &tempdt, 1);
						tempsecs=TimDateTimeToSeconds(&tempdt);
						SecondsToDateOrTimeString(tempsecs, gpsutc, 2, 0);
					}
				}
//				HostTraceOutputTL(appErrorClass, "     gpsutc=|%s|", gpsutc);
				GetField(NMEAbuf, 2, tempchar);
				StrNCopy(gpslat, tempchar, 20);
				GetField(NMEAbuf, 3, tempchar);
				StrCopy(gpslatdir, tempchar);
				GetField(NMEAbuf, 4, tempchar);
				StrNCopy(gpslng, tempchar, 21);
				GetField(NMEAbuf, 5, tempchar);
				StrCopy(gpslngdir, tempchar);
				/* Fix Quality */
				GetField(NMEAbuf, 6, tempchar);
				curgpsfixquality = StrAToI(tempchar);
				if (curgpsfixquality <= 0) {
//					HostTraceOutputTL(appErrorClass, "GGA:setting gpsstat to V");
					StrCopy(gpsstat, "V");
				} else {
//					HostTraceOutputTL(appErrorClass, "GGA:setting gpsstat to A");
					StrCopy(gpsstat, "A");
				}
	
				/* Number of SATS Tracked */
				GetField(NMEAbuf, 7, tempchar);
				StrCopy(gpsnumsats, tempchar);

				/* Horizontal Dillution of Precision (HDOP) in meters */
				GetField(NMEAbuf, 8, tempchar);
//				data.input.eph = StrToDbl(tempchar);

				// Only want to use the calculated value if the Garmin
				// eph value is not present
				if (!gpsephavail) {
					// Using this formula to get a rough 2-sigma ehp value
					data.input.eph = StrToDbl(tempchar) * 5.1 * 2.0;
				}

				/* Altitude in meters above MSL */
				GetField(NMEAbuf, 9, tempchar);
				gpsalt = StrToDbl(tempchar);
		
				/* Height above ellipsoid */
				GetField(NMEAbuf, 11, tempchar);
				gpsgeoidaldiff = StrToDbl(tempchar);
				if (data.config.gpsaltref == GPSWGS84) {
					data.input.gpsgeoidaldiff = gpsgeoidaldiff;
				} else {
					data.input.gpsgeoidaldiff = 0.0;
				}
	
//				HostTraceOutputTL(appErrorClass, "GGA:setting gpsstat to gpsstat");
				StrCopy(data.logger.gpsstat, gpsstat);
				StrCopy(data.input.gpsnumsats, gpsnumsats);
				data.input.siu = (Int8)StrAToI(gpsnumsats);
/* simulate midnight in SeeYou
				tempnum = StrToDbl(ToNumeric(Left(gpsutc,2)))+22;
				StrCopy(tempchar, DblToStr(tempnum,0));
				StrCat(tempchar, Right(gpsutc,4));
				StrCopy(gpsutc, tempchar);
*/
				StrCopy(data.logger.gpsutc, gpsutc);
				if ((StrCompare(data.logger.gpsstat, "A") == 0) && !draw_log) {
					StrCopy(data.logger.gpslat, gpslat);
					StrCopy(data.logger.gpslatdir, gpslatdir);
					data.input.gpslatdbl = DegMinStringToLatLon(data.logger.gpslat, data.logger.gpslatdir[0]);
					data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
					StrCopy(data.logger.gpslng, gpslng);
					StrCopy(data.logger.gpslngdir, gpslngdir);
					data.input.gpslngdbl = DegMinStringToLatLon(data.logger.gpslng, data.logger.gpslngdir[0]);
				}
	
				//Altitude in meters divided by ALTMETCONST to convert to feet.
				//Normally the gpsgeoidaldiff would be added to convert the MSL value in field 9 to get
				//HAE.
				//however we aren't displaying HAE anymore.  Instead this value is now subtracted to 
				//account those GPS' which put HAE in field 9 and the correction value in field 11.  By
				//subtracting the value we end up with an MSL value.
				data.logger.gpsalt = (data.config.gpsmsladj*ALTMETCONST+gpsalt-data.input.gpsgeoidaldiff)/ALTMETCONST;
				if (data.config.pressaltsrc == GPSALT) {
					data.logger.pressalt = gpsalt/ALTMETCONST;
				}
	
				// Made the conversion to feet here so that it could be used above without conversion
				data.input.gpsgeoidaldiff /= ALTMETCONST;
				gpsgeoidaldiff /= ALTMETCONST; // Contains the info from the NMEA stream in feet
				gpsalt /= ALTMETCONST; // Contains the info from the NMEA in feet

				// Calculate Lift Here GGA
				if (data.config.usepalt) {
					CalcLift(data.logger.pressalt, data.logger.gpsutc, -9999.9, NORESET);
//					HostTraceOutputTL(appErrorClass, "Calc Lift GGA-GPSALT");
				} else {
//				} else if (data.config.flightcomp == NOCOMP     ||
//					   data.config.flightcomp == B50COMP    ||
//					   data.config.flightcomp == LXCOMP     ||
//					   data.config.flightcomp == LXVARCOMP  ||
//					   data.config.flightcomp == RECOCOMP   || //Not sure if this is needed
//					   data.config.flightcomp == B50VLKCOMP ||
//					   data.config.flightcomp == B50LXCOMP  ||
//					   data.config.flightcomp == SN10COMP   ||
//					   data.config.flightcomp == VOLKSCOMP  ||
//					   data.config.flightcomp == GPSNAVCOMP ||
//					   data.config.flightcomp == FLARMCOMP  ||
//					   data.config.flightcomp == C302ACOMP  ||
//					   data.config.flightcomp == EWCOMP) {
				// Might be able to use this instead of the above
//				} else if (data.config.flightcomp != C302COMP) {
					// Calculate using gps altitude because these types of computers do not output any alititude values
					// or the operator has chosen not to use the pressure altitude info
					CalcLift(data.logger.gpsalt, data.logger.gpsutc, -9999.9, NORESET);
//					HostTraceOutputTL(appErrorClass, "Calc Lift GGA-NOCOMP||B50COMP||LXCOMP||LXVARCOMP||EWCOMP||B50VLKCOMP||B50LXCOMP||RECOCOMP");
				}
	
				if ((StrCompare(data.logger.gpsstat, "V") == 0) || draw_log) {
					updatetime = false;
				} else {
					updatemap = true;
					updatetime = true;
				}

				if (FrmGetActiveFormID() == form_sat_status && !gpgsa) {
					// Have to fake the fixtype based on the number of satellites
					if (data.input.siu <= 3) {
						curgpsfixtype = 2;
					} else {
						curgpsfixtype = 3;
					}
					SetFixStatus();
				}
				data.application.changed = 1;
				break;
			} //End of GPGGA_VALID:

// NMEA183 V2 Sentence to give the list of SVN's being used in the
// current satellite fix.
			case GPGSA_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser GSA");
				gpgsa = true;
				GetField(NMEAbuf, 2, tempchar);
				curgpsfixtype = (Int8)StrAToI(tempchar);
				for (i=0; i<12; i++) {
					GetField(NMEAbuf, i+3, tempchar);
					if(StrLen(tempchar) > 0) {
						data.input.svns[i] = (Int8)StrAToI(tempchar);
					} else {
						data.input.svns[i] = -1;
					}
				}

				// force iPhone fix type
				if (device.StyleTapPDA == STIPHONE) {
					if (gpgga) {
						curgpsfixtype = 3;
					} else {
						curgpsfixtype = 2;
					}
					SetFixStatus();
				}

				data.application.changed = 1;
				break;
			} // End of GPGSA_VALID:
	
// NMEA183 V2 Sentence to get the stats for each satellite being used
			case GPGSV_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser GSV");
				GetField(NMEAbuf, 1, tempchar);
				NumGSVMsgs = (Int8)StrAToI(tempchar);
				GetField(NMEAbuf, 2, tempchar);
				CurGSVMsg = (Int8)StrAToI(tempchar);
//				HostTraceOutputTL(appErrorClass, "CurGSVMsg-|%hd|", (Int16)CurGSVMsg);
//				HostTraceOutputTL(appErrorClass, "NumGSVMsgs-|%hd|", (Int16)NumGSVMsgs);
				if (CurGSVMsg == ExpectedGSVMsg) {
//					HostTraceOutputTL(appErrorClass, "ExpectedGSVMsg Found-|%hd|", (Int16)ExpectedGSVMsg);
					if (ExpectedGSVMsg == NumGSVMsgs) {
						ExpectedGSVMsg = 1;
					} else {
						ExpectedGSVMsg++;
					}
					j = (CurGSVMsg-1)*4;
					for (i=0; i<4; i++) {
						satData[j+i].status = 0;
						satData[j+i].status |= satNoDif;
						satData[j+i].status |= satNoEph;
						GetField(NMEAbuf, i*4+4, tempchar);
						if(StrLen(tempchar) > 1) {
							satData[j+i].svid = (Int8)StrAToI(tempchar);
						} else {
							satData[j+i].svid = gpsInvalidSVID;
						}
						GetField(NMEAbuf, i*4+5, tempchar);
						if(StrLen(tempchar) > 0) {
							satData[j+i].elevation = (float)DegreesToRadians(StrToDbl(tempchar));
						}
						GetField(NMEAbuf, i*4+6, tempchar);
						if(StrLen(tempchar) > 0) {
							satData[j+i].azimuth = (float)DegreesToRadians(StrToDbl(tempchar));
						}
						GetField(NMEAbuf, i*4+7, tempchar);
						if(StrLen(tempchar) > 0) {
							if (StrLen(tempchar) > 1) {
								satData[j+i].snr = (float)StrToDbl(tempchar) * 100.0;
								if (satData[j+i].snr == 0.0) {
									// Setting to -100 to indicate no signal
									satData[j+i].snr = -100.0;
								}
							} else {
								// Setting to -100 to indicate no signal
								satData[j+i].snr = -100.0;
							}
						}
					}
				} else {
					ExpectedGSVMsg = 1;
				}
				// Compare the Sats being tracked to the ones used for the fix to
				// decide whether to set the ephemeris status
				for (i=0; i<GPSMAXSATS; i++) {
					if (satData[i].svid != gpsInvalidSVID) {
						for (j=0; j<GPSMAXSATS; j++) {
							if (data.input.svns[j] != -1) {
								if (satData[i].svid == data.input.svns[j]) {
									satData[i].status |= satEph;
								}
							}
						}
					}
				}

				// Just wrapped this up so I can use it elsewhere
				SetFixStatus();
				data.application.changed = 1;

				break;
			} // End of GPGSV_VALID:
	
// Garmin Proprietary Sentence to give GPS Altitude in feet
// Used by Flarm and EWMR also
			case PGRMZ_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser RMZ");
				if ((gpsver2) && (data.config.pressaltsrc != GARALT) && (data.config.pressaltsrc != FLARMALT) && (data.config.pressaltsrc != EWMRALT)) {
					revert++;
					if (revert == 4) {
						gpsver2 = false;
						gpsephavail = false;
						revert = 0;
					}
				} else {
					GetField(NMEAbuf, 1, tempchar);
					gpsalt = StrToDbl(tempchar);
					if (!gpsver2) {
						data.input.gpsgeoidaldiff = 0.0;
						// Leaving this adding the data.input.gpsgeoidaldiff because it doesn't matter
						data.logger.gpsalt = data.config.gpsmsladj*ALTMETCONST+gpsalt+data.input.gpsgeoidaldiff; 
						if (data.config.pressaltsrc == GPSALT) {
							data.logger.pressalt = gpsalt;
						}
					}
					if ((data.config.pressaltsrc == GARALT) || (data.config.pressaltsrc == FLARMALT) || (data.config.pressaltsrc == EWMRALT)) {
						data.logger.pressalt = gpsalt;
						recv_palt = true;
					}
					data.application.changed = 1;
				}
				break;
			} // End of PGRMZ_VALID:
	
// Garmin Proprietary Sentence to give GPS Estimated Error Values
			case PGRME_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser RME");
				GetField(NMEAbuf, 1, tempchar);
				data.input.eph = StrToDbl(tempchar);
				gpsephavail = true;
		
				data.application.changed = 1;
				break;
			} // End of PGRME_VALID:
	
// Volkslogger Proprietary Sentence
			case PGCS_VALID:
			{
				/* Volkslogger Pressure Alt (Hexidecimal) in meters */
				if (data.config.pressaltsrc == VOLKSALT) {
					GetField(NMEAbuf, 3, tempchar);
					data.logger.pressalt = ((double)Hex2Dec2(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} // End of PGCS_VALID:
	
// Zander SR940 Proprietary Sentence
			case PZAN1_VALID:
			{
				/* Zander Pressure Alt in meters */
				if (data.config.pressaltsrc == ZANDERALT) {
					GetField(NMEAbuf, 1, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} // End of PZAN1_VALID:
	
// PosiGraph Proprietary Sentence
			case GPWIN_VALID:
			{
				/* PosiGraph Pressure Alt in meters - 5159 = 515.9m */
				if (data.config.pressaltsrc == POSIALT) {
					GetField(NMEAbuf, 3, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) / 10.0 / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} // End of GPWIN_VALID:
	
// Borgelt B50 Proprietary Sentence
			case PBB50_VALID:
			{
				if (data.config.flightcomp == B50COMP ||
					data.config.flightcomp == B50VLKCOMP || 
					data.config.flightcomp == B50LXCOMP ||
					data.config.flightcomp == WESTBCOMP) {
					// msr: PBB50 sentence implementation
					// B50 true air speed in knots
					GetField(NMEAbuf, 1, tempchar);
					data.input.true_airspeed.value = StrToDbl(tempchar);
					data.input.true_airspeed.valid = VALID;
					// B50 vario in knots
					GetField(NMEAbuf, 2, tempchar);
					data.input.vario = StrToDbl(tempchar);
					// B50 MacCready in knots
					GetField(NMEAbuf, 3, tempchar);
					data.input.compmc = StrToDbl(tempchar);
					// B50 indicated air speed in knots
					GetField(NMEAbuf, 4, tempchar);
					data.input.ind_airspeed = (int)Sqrt(StrToDbl(tempchar));
					if (data.config.flightcomp != WESTBCOMP) {
						// B50 bugs degradation
						GetField(NMEAbuf, 5, tempchar);
						data.input.bugs = 1.0 - (StrToDbl(tempchar)/100.0);
						// B50 ballast weight
						GetField(NMEAbuf, 6, tempchar);
						if (data.polar.maxwater > 0) {
							ballast = (StrToDbl(tempchar) - 1.0)*data.polar.maxdrywt /(data.polar.maxwater*PPGW);
							data.input.ballast = (ballast < 0.0 ? 0.0 : (ballast > 1.0 ? 1.0 : ballast));
						} else
							data.input.ballast = 0;
					}
					// B50 cruise(1)/climb(0) switch
					GetField(NMEAbuf, 7, tempchar);
					data.input.cruise = StrToDbl(tempchar) == 0;
					data.application.changed = 1;
					// B50 pressure altitude
					if ((data.config.pressaltsrc == B500ALT) && GetField(NMEAbuf, 9, tempchar)) {
						data.logger.pressalt = StrToDbl(tempchar) == 0;
						recv_palt = true;
						data.application.changed = 1;
					}
				}
				break;
			} // End of PBB50_VALID:
	
// ILEC SN10 Proprietary Sentences
			case PILC_VALID:
			{
				if (data.config.flightcomp == SN10COMP) {
					GetField(NMEAbuf, 1, tempchar);
					if (StrCompare(tempchar, "PDA1") == 0) {
						if (data.config.pressaltsrc == SN10ALT) {
							// Pressure Alt in meters as an integer */
							GetField(NMEAbuf, 2, tempchar);
							data.logger.pressalt = (StrToDbl(tempchar)) / ALTMETCONST;
							recv_palt = true;	
						}
						// Total energy vario (meters per second xx.xx)
						GetField(NMEAbuf, 3, tempchar);
						data.input.vario = (StrToDbl(tempchar)) / AIRMPSCONST;
						// Wind direction (wind is from, true, integer) 
						GetField(NMEAbuf, 4, tempchar);
						data.input.compwnddir = StrToDbl(tempchar);
//						HostTraceOutputTL(appErrorClass, "VWind Dir-|%s|", tempchar);

						// Wind strength (kph, integer)
						GetField(NMEAbuf, 5, tempchar);
						data.input.compwndspd = (StrToDbl(tempchar)) / SPDKPHCONST;
//						HostTraceOutputTL(appErrorClass, "VWind Spd-|%s|", tempchar);
						data.application.changed = 1;
					} else if (StrCompare(tempchar, "SET") == 0) {
						// SN10 altimeter setting in mB
						GetField(NMEAbuf, 2, tempchar);
						data.input.compqnh = StrToDbl(tempchar); // it is a pressure value
						data.application.changed = 1;
					} else if (StrCompare(tempchar, "POLAR") == 0) {
						data.application.changed = 1;
					}
				}
				break;
			} // End of PILC_VALID:
	
// Cambridge Priprietary Sentence for current
// Destination waypoint name and elevation
// Not currently being used
			case PCAIB_VALID:
			{
	
//				data.application.changed = 1;
				break;
			} //End of PCAIB_VALID:
	
// Cambridge Priprietary Sentence for Pressure Altitude
			case PCAID_VALID:
			{
				if (data.config.pressaltsrc == C302ALT || data.config.pressaltsrc == C302AALT) {
				/* Cambridge 302 Pressure Alt in meters */
					GetField(NMEAbuf, 2, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;	
				}
				break;
			} // End of PCAID_VALID:
	
// Cambridge proprietary sentence
			case W_VALID:
			{
				if (data.config.flightcomp == C302COMP) {
					// NETD302 Vector Wind in degrees
					// 302 reports the direction wind is going TO
					// not the direction FROM
					GetField(NMEAbuf, 1, tempchar);
					data.input.compwnddir = RecipCse(StrToDbl(tempchar));
//					HostTraceOutputTL(appErrorClass, "VWind Dir-|%s|", tempchar);
					// NETD302 Vector Wind speed in 10th of m/s
					GetField(NMEAbuf, 2, tempchar);
					data.input.compwndspd = (StrToDbl(tempchar)/10.0) / AIRMPSCONST;
//					HostTraceOutputTL(appErrorClass, "VWind Spd-|%s|", tempchar);
					// NETD302 Vector Wind age in seconds
					GetField(NMEAbuf, 3, tempchar);
//					HostTraceOutputTL(appErrorClass, "VWind Age-|%s|", tempchar);
					// NETD302 Component Wind in m/s + 500 (500 = 0)
					GetField(NMEAbuf, 4, tempchar);
					data.input.compheadwind = ((StrToDbl(tempchar) - 500.0) / 10.0) / AIRMPSCONST;
//					HostTraceOutputTL(appErrorClass, "CWind Spd-|%s|", tempchar);
					// NETD302 True Altitude in Meters + 1000
					GetField(NMEAbuf, 5, tempchar);
//					HostTraceOutputTL(appErrorClass, "TAlt-|%s|", tempchar);
					data.input.comptruealt = (StrToDbl(tempchar) - 1000.0) / ALTMETCONST;
//					HostTraceOutputTL(appErrorClass, "data.input.comptruealt-|%s|", DblToStr(data.input.comptruealt, 1));
					// 302 QNH Setting - Not sure if this is a pressure or an altitude
					GetField(NMEAbuf, 6, tempchar);
					data.input.compqnh = StrToDbl(tempchar); // it is a pressure value
//					HostTraceOutputTL(appErrorClass, "QNH-|%s|", tempchar);
					// NETD302 True Air Speed in 100ths of m/s
					GetField(NMEAbuf, 7, tempchar);
					data.input.true_airspeed.value = (StrToDbl(tempchar) / 100.0) / AIRMPSCONST;
					data.input.true_airspeed.valid = VALID;
//					HostTraceOutputTL(appErrorClass, "TAS-|%s|", tempchar);
					// NETD302 Vario in 10ths of knots + 200
					GetField(NMEAbuf, 8, tempchar);
					data.input.vario = (StrToDbl(tempchar) - 200.0) / 10.0;
//					HostTraceOutputTL(appErrorClass, "Vario-|%s|", tempchar);
					// NETD302 Averager in 10ths of knots + 200
					GetField(NMEAbuf, 9, tempchar);
					data.input.varioavg = (StrToDbl(tempchar) - 200.0) / 10.0;
//					HostTraceOutputTL(appErrorClass, "VAvg-|%s|", tempchar);
					// NETD302 Relative Varios in 10 of knots + 200
					GetField(NMEAbuf, 10, tempchar);
					data.input.variorel = (StrToDbl(tempchar) - 200.0) / 10.0;
//					HostTraceOutputTL(appErrorClass, "RVario-|%s|", tempchar);
					// 302 MacCready in 10ths of knots
					GetField(NMEAbuf, 11, tempchar);
					data.input.compmc = StrToDbl(tempchar) / 10.0;
//					HostTraceOutputTL(appErrorClass, "MC-|%s|", tempchar);
					// 302 ballast in percentage of capacity
					GetField(NMEAbuf, 12, tempchar);
					data.input.ballast = StrToDbl(tempchar)/100.0;
//					HostTraceOutputTL(appErrorClass, "Ballast-|%s|", tempchar);
					// 302 bugs degradation
					GetField(NMEAbuf, 13, tempchar);
					data.input.bugs = (StrToDbl(tempchar)/100.0);
//					HostTraceOutputTL(appErrorClass, "Bugs-|%s|", tempchar);
//					HostTraceOutputTL(appErrorClass, "data.input.bugs-|%s|", DblToStr(data.input.bugs, 2));
					// Doesn't really matter which altitude is used,
					// will use the C302 vario & averager values
					CalcLift(data.input.comptruealt, data.logger.gpsutc, -9999.9, NORESET);
//					HostTraceOutputTL(appErrorClass, "Calc Lift W");

//					HostTraceOutputTL(appErrorClass, "--------------------------------");
					data.application.changed = 1;
				}
				break;
			} // End of W_VALID:
	
// GPS Companion proprietary sentence
// for battery status
			case PMGNST_VALID:
			{
				// Battery voltage level (0-999) in arbitrary units (bbb)
//				GetField(NMEAbuf, 4, tempchar);
				// Hours of operation left on battery (approximate) (hh.h)
				GetField(NMEAbuf, 5, tempchar);
				MGCBattHours = StrToDbl(tempchar);
//				data.application.changed = 1;
				break;
			} // End of PMGNST_VALID:
	
// LX/Colibri Proprietary Sentence for current
// Pressure Alt in meters
			case PFSRA_VALID:
			{
				if (data.config.pressaltsrc == LXALT) {
					GetField(NMEAbuf, 1, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} // End of PFSRA_VALID:

// LX/Colibri Proprietary LXWP0 Sentence
// Pressure Alt in meters
// Vario in m/s
			case LXWP0_VALID:
			{
				if (data.config.pressaltsrc == LXALT) {
					GetField(NMEAbuf, 3, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				if (data.config.flightcomp == LXCOMP ||
				    data.config.flightcomp == LXVARCOMP || 
				    data.config.flightcomp == FILSERCOMP || // need to check this PG			
				    data.config.flightcomp == B50LXCOMP) {
					// LX / Colibri Vector Wind in degrees True
					GetField(NMEAbuf, 11, tempchar);
					data.input.compwnddir = nice_brg(StrToDbl(tempchar)+data.input.deviation.value);
//					HostTraceOutputTL(appErrorClass, "VWind Dir-|%s|", tempchar);
					// LX / Colibri Vector Wind speed in kph
					GetField(NMEAbuf, 12, tempchar);
					data.input.compwndspd = StrToDbl(tempchar)/SPDKPHCONST;
//					HostTraceOutputTL(appErrorClass, "VWind Spd-|%s|", tempchar);
					data.application.changed = 1;
				}
				if (data.config.flightcomp == LXVARCOMP) {
					GetField(NMEAbuf, 4, tempchar);
					data.input.vario = (StrToDbl(tempchar)) / AIRMPSCONST;
				}
				break;
			} //End of LXWP0_VALID:

// Tasman Proprietary PTAS1 Sentence
// Vario in knots * 10
// Average Vario in knots * 10
// Pressure Alt in meters
// $SPTAS1,xxx,yyy,zzzzz,aaa*CS<CR><LF>
//	xxx     CV or current vario +200 (knots/10, 0-19.9)
// yyy     AV or averasge vario +200 (knots/10, 0-19.9)
// zzzzz   Barometric altitude feet +2000 (1013.25mb=0ft or 29.92 inHg)
//         note: no QNH setting available
// aaa     TAS knots 0-200
// *       end of string
// CS      XOR all bytes afer $ and before*
// <CR><LF> Carrage return,line feed
			case PTAS1_VALID:
			{
				if (data.config.flightcomp == TASCOMP ||
					 data.config.flightcomp == EWMRSDTASCOMP) {
					// Tasman Vario Value  +200 in knots
					GetField(NMEAbuf, 1, tempchar);
					data.input.vario = (StrToDbl(tempchar) - 200.0) / 10.0;
//					HostTraceOutputTL(appErrorClass, "Vario-|%s|", tempchar);
					// Tasman Average Vario Value +200 in knots
					GetField(NMEAbuf, 2, tempchar);
					data.input.varioavg = (StrToDbl(tempchar) - 200.0) / 10.0;
//					HostTraceOutputTL(appErrorClass, "VWind Spd-|%s|", tempchar);
					// Tasman true air speed in knots
					GetField(NMEAbuf, 4, tempchar);
					data.input.true_airspeed.value = StrToDbl(tempchar);
					data.input.true_airspeed.valid = VALID;

					// This is an estimate of TAS
					// Increase your indicated airspeed by 2% per thousand feet of altitude
					// Saving this formula for possible future use.
					// data.input.true_airspeed.value = data.input.ind_airspeed + (.02 * data.logger.gpsalt / 1000.0);
					// data.input.true_airspeed.valid = VALID;

					// Doesn't really matter which altitude is used,
					// will use the Tasman vario & averager values
					CalcLift(data.logger.gpsalt, data.logger.gpsutc, -9999.9, NORESET);
//					HostTraceOutputTL(appErrorClass, "Calc Lift PTAS1");
					data.application.changed = 1;
				}
				if (data.config.pressaltsrc == TASALT) {
					// Tasman Barometric altitude in feet +2000 
					GetField(NMEAbuf, 3, tempchar);
					data.logger.pressalt = (StrToDbl(tempchar)) - 2000.0;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} //End of PTAS1_VALID:

// RECOA Proprietary Sentences
			case RECOA_VALID:
			{
				// Chip Temp * 10.0 in Celcius
				GetField(NMEAbuf, 1, tempchar);
				recodata->temp = (double)(StrAToI(tempchar)) / 10.0; 

				// Power Voltage = Input Register value * 0.11758
				GetField(NMEAbuf, 2, tempchar);
				recodata->voltage = (double)(StrAToI(tempchar)) * 0.11758; 

				// Current RECO ENL Value
				GetField(NMEAbuf, 3, tempchar);
				recodata->enl = (Int16)(StrAToI(tempchar)); 

				// RECO Firmware Version
				GetField(NMEAbuf, 4, tempchar);
				StrCopy(recodata->recofirmwarever, tempchar);

				// RECO Serial Number
				GetField(NMEAbuf, 5, tempchar);
				if (StrLen(tempchar) > 3) {
					recodata->recoserial[0] = tempchar[0];
					recodata->recoserial[1] = tempchar[StrLen(tempchar)-2];
					recodata->recoserial[2] = tempchar[StrLen(tempchar)-1];
					recodata->recoserial[3] = '\0';
				} else {
					StrCopy(recodata->recoserial, tempchar);
				}

				// RECO GPS Serial Number
				GetField(NMEAbuf, 6, tempchar);
				StrCopy(data.igchinfo.gpsser, tempchar);

				// RECO GPS Model 
				GetField(NMEAbuf, 7, tempchar);
				StrCopy(data.igchinfo.gpsmodel, tempchar);

				data.application.changed = 1;
				break;
			} // End of RECOA_VALID:

// RECOB Proprietary Sentence
// Example: $RECOB,6.23,95232*7F
//	first is system timestamp - format is [0-60s].[0-127] -
//	the value after dot is 1/127 [s] -
//	divide by 127 to get decimal part of the seconds
//	second contains the value of current pressure;
//	recalculated into altitude value using barometric formula in the SoaringPilot
			case RECOB_VALID:
			{
				Int16 intsecs;
				double fracsecs;
				double tmpdbl;
//				double intpart;

				if (data.config.pressaltsrc == RECOALT) {
					GetField(NMEAbuf, 1, tempchar);
					// Returns the current system timestamp
					tmpdbl = StrToDbl(tempchar);
					intsecs = (Int16)tmpdbl;
					if (tmpdbl < 10.0) {
						tempchar[0] = '0';
						tempchar[1] = '0';
					} else {
						tempchar[0] = '0';
						tempchar[1] = '0';
						tempchar[2] = '0';
					}
					fracsecs = StrAToI(tempchar) / 127.0;
					recodata->cursecs = (double)intsecs + fracsecs;
					if (recodata->cursecs >= 60.0) {
						recodata->cursecs -= 60.0;
					}

					GetField(NMEAbuf, 2, tempchar);
					// Returns pressure altitude in feet
					data.logger.pressalt = RECOB2Alt(tempchar);
					recv_palt = true;

					CalcLift(data.logger.pressalt, data.logger.gpsutc, recodata->cursecs, NORESET);
					data.application.changed = 1;
				}
				break;
			} // End of RECOB_VALID:

// Flarm Specific Sentences
// Operating Status and Alarm Data
// PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>, <RelativeVSep>,<RelativeHDist>
// Note: Will always be active without FLARM computer setting, so can be used in combination with other varios
			case PFLAU_VALID:
			{
				// Operating Status and Alarm Data
				Flarmpresent = true;
//				HostTraceOutputTL(appErrorClass, NMEAbuf);

				// copy traffic data from satDataFlarm array
				MemMove(satData, satDataFlarm, sizeof(GPSSatDataType));

				// no. of devices being received
				GetField(NMEAbuf, 1, tempchar);
				FlarmRX = StrAToI(tempchar);
				 
				// Alarm Level
				GetField(NMEAbuf, 5, tempchar);
				tempnum = StrAToI(tempchar);
				if (tempnum <= 1 ) {
					satData[0].status = 0;
					satData[0].snr = -1;
					FlarmAlarm = false;
//					HostTraceOutputTL(appErrorClass, "Clear Flarm Alarm <= 1");
				} else if (tempnum == 2) {
					satData[0].status = 0;
					satData[0].snr = 1;
					FlarmAlarm = true;
//					HostTraceOutputTL(appErrorClass, "Set Flarm Alarm == 2");
				} else {
					satData[0].status = 1;
					satData[0].snr = 1;
					FlarmAlarm = true;
//					HostTraceOutputTL(appErrorClass, "Set Flarm Alarm > 2");
				}

				// Check for Traffic Data
				GetField(NMEAbuf, 6, tempchar);
				if (StrLen(tempchar) == 0) {
					// no traffic in message
					satData[0].svid = gpsInvalidSVID;

				} else {					
					// Relative Bearing
					tempnum = StrAToI(tempchar);
					satData[0].azimuth = nice_brg(tempnum) * degToRad;

					// Alarm Type
					GetField(NMEAbuf, 7, tempchar);
					tempnum = StrAToI(tempchar);
					if (tempnum == 3) {
						// obstacle
						satData[0].svid = 0; 
					} else {
						// aircraft - level by default
						satData[0].svid = 2;
					}
					
					// Relative VSep
					GetField(NMEAbuf, 8, tempchar);
					FlarmVSep = StrAToI(tempchar);

					// Relative HDist
					GetField(NMEAbuf, 9, tempchar);
					satData[0].elevation = StrAToI(tempchar);

					// if not an obstacle, decide if Above, Below or Level
					// level is within +5/-10 deg of horiz 
					// 1/10 ~= sin(5 deg)
					if (satData[0].svid != 0) {
						if (FlarmVSep > (satData[0].elevation/10)) {
							// above
							satData[0].svid = 3;
						} else if (FlarmVSep < -(satData[0].elevation/5)) {
							// below	
							satData[0].svid = 1;
						}
					}
				}

				// update display
//				if (FrmGetActiveFormID() == form_sat_status) {
//					DrawSatStatus(gpsalt, gpsgeoidaldiff, MGCBattHours, false);
//				}

				// clear data
//				MemSet(satData, GPSMAXSATS * sizeof(GPSSatDataType), 0xff);
				MemSet(satDataFlarm, GPSMAXSATS * sizeof(GPSSatDataType), 0xff);
				FlarmIdx = 1;
				FlarmMaxIdx = 1;
				FlarmMaxDist = -1;

				data.application.changed = 1;
				break;
			} // End of PFLAU_VALID:

// Flarm Specific Sentences
// Traffic Data
// PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,<ID-Type>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
// Note: Will always be active without FLARM computer setting, so can be used in combination with other varios
			case PFLAA_VALID:
			{
				// Traffic Data
//				HostTraceOutputTL(appErrorClass, NMEAbuf);

				// Relative North
				GetField(NMEAbuf, 2, tempchar);
				RelN = StrAToI(tempchar);

				// Relative East
				GetField(NMEAbuf, 3, tempchar);
				RelE = StrAToI(tempchar);

				// Distance
				tempdist = Sqrt(RelN*RelN + RelE*RelE);

				// Check if more than GPSMAXSATS traffic items
				if (FlarmIdx >= GPSMAXSATS) {
					if (tempdist >= FlarmMaxDist) {
						// replace farthest item
						FlarmIdx = FlarmMaxIdx;
						FlarmMaxIdx = 0;
						FlarmMaxDist = tempdist;
					}
				}

				if (FlarmIdx < GPSMAXSATS) {

					// Alarm Level
					GetField(NMEAbuf, 1, tempchar);
					tempnum = StrAToI(tempchar);
					if (tempnum <= 1 ) {
						satDataFlarm[FlarmIdx].status = 0;
						satDataFlarm[FlarmIdx].snr = -1;
					} else if (tempnum == 2) {
						satDataFlarm[FlarmIdx].status = 0;
						satDataFlarm[FlarmIdx].snr = 1;
					} else {
						satDataFlarm[FlarmIdx].status = 1;
						satDataFlarm[FlarmIdx].snr = 1;
					}

					// Bearing
					satDataFlarm[FlarmIdx].azimuth = Atan2(RelE, RelN) - DegreesToRadians(data.input.true_track.value);

					// Distance
					satDataFlarm[FlarmIdx].elevation = tempdist;
					if (satDataFlarm[FlarmIdx].elevation > FlarmMaxDist) {
						FlarmMaxDist = satDataFlarm[FlarmIdx].elevation;
						FlarmMaxIdx = FlarmIdx;
					}

					// VSep
					GetField(NMEAbuf, 4, tempchar);
					tempnum = StrAToI(tempchar);

					// Type
					GetField(NMEAbuf, 11, tempchar);
					if (StrCompare(tempchar, "F") == 0) {
						// obstacle
						satDataFlarm[FlarmIdx].svid = 0; 
					} else {
						// aircraft - level by default
						satDataFlarm[FlarmIdx].svid = 2;
					}

					// if not an obstacle, decide if Above, Below or Level
					// level is within +/-5 deg of horiz 
					// 1/10 ~= sin(5 deg)
					if (satData[FlarmIdx].svid != 0) {
						if (tempnum > (satDataFlarm[FlarmIdx].elevation/10)) {
							// above
							satDataFlarm[FlarmIdx].svid = 3;
						} else if (tempnum < -(satDataFlarm[FlarmIdx].elevation/10)) {
							// below	
							satDataFlarm[FlarmIdx].svid = 1;
						}
					}

					// Climb Rate
					GetField(NMEAbuf, 10, tempchar);
					if (StrLen(tempchar) > 0) {
						// in knots
						tempdist = StrToDbl(tempchar) / AIRMPSCONST;
						if (tempdist > 0.5 * MCCurVal) {
							// climbing if more than 50% of MC value
						satData[FlarmIdx].svid += 4;
						}
					}

					// set max distance item index
					if (FlarmMaxIdx == 0) {
						FlarmMaxIdx = FlarmIdx;
						FlarmIdx = GPSMAXSATS;
					}

					// Increment index
					FlarmIdx++;
				}
				break;
			} // End of PFLAA_VALID:

// Westerboer Specific Sentences
//$PWES0,DD,VVVV,MMMM,NNNN,BBBB,SSSS,AAAAA,QQQQQ,IIII,TTTT,UUU,CCC*CS<CR><LF>
                        case PWES0_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser PWES0");
				if (data.config.pressaltsrc == WESTBALT) {
					GetField(NMEAbuf, 7, tempchar);
					data.logger.pressalt = ((double)StrAToI(tempchar)) / ALTMETCONST;
					data.application.changed = 1;
					recv_palt = true;
				}
				break;
			} // End of PWES0_VALID:

// Westerboer Specific Sentences
//$PWES1,DD,MM,S,AAA,F,V,LLL,BB,U*CS<CR><LF>
                        case PWES1_VALID:
			{
//				HostTraceOutputTL(appErrorClass, "     Parser PWES1");
				// data duplicated in PBB50 sentence so processed there.
				break;
			} // End of PWES1_VALID:

			default:
				break;
		}

		// reset valid sentence flag
		state = INVALID;

		// Find current altitude
		if (data.config.flightcomp == C302COMP) {
			tempalt = data.input.comptruealt;
		} else {
			tempalt = data.logger.gpsalt;
		}
		// Save Maximum Altitude  in feet
		if (data.input.maxalt < tempalt) {
			data.input.maxalt = tempalt;
		}
		// Save Minimum Altitude  in feet
		if (data.input.minalt > tempalt) {
			data.input.minalt = tempalt;
		}
	}

	// Modify pressure altitude by QNH setting for non pressure altitude sources
	// Or if no pressure altitude data received yet
	if ((data.config.pressaltsrc == GPSALT) || (!recv_palt)) {
		data.logger.pressalt = data.logger.gpsalt + data.input.QNHaltcorrect;
	}

	//	HostTraceOutputTL(appErrorClass,"P Alt? %s",DblToStr(recv_palt,0));
	//	HostTraceOutputTL(appErrorClass, "Leaving nmea_parser");
	//	HostTraceOutputTL(appErrorClass, "--------------------------");
}

Boolean gpsvalidate(Char* gpssentence)
{
	char mychksumstr[10];
	char gpschksumstr[10];
	char tempchar [10];
	UInt16 mychksum = 0;
	UInt16 i;
	UInt16 length;

	length = StrLen(gpssentence);

//	HostTraceOutputTL(appErrorClass, "gpssentence:|%s|", gpssentence);
	if ((StrChr(gpssentence, '*') != NULL) && (gpssentence[length-5] == '*')) {
		for (i=0; i < length-5; i++) {
			mychksum^=gpssentence[i];
		}
		gpschksumstr[0] = gpssentence[length-4];
		gpschksumstr[1] = gpssentence[length-3];
		gpschksumstr[2] = '\0';
//		HostTraceOutputTL(appErrorClass, "gpschecksumstr:|%s|", gpschksumstr);

		StrIToH(tempchar, mychksum);
//		HostTraceOutputTL(appErrorClass, "mychksum:|%s|", tempchar);
		length = StrLen(tempchar);
		mychksumstr[0] = tempchar[length-2];
		mychksumstr[1] = tempchar[length-1];
		mychksumstr[2] = '\0';

		if (StrCompare(gpschksumstr, mychksumstr) == 0) {
//			HostTraceOutputTL(appErrorClass, "Checksum is GOOD!");
			return(true);
		} else {
//			HostTraceOutputTL(appErrorClass, "WARNING: Checksum mismatch!");
//			HostTraceOutputTL(appErrorClass, "gpschksumstr:|%s|", gpschksumstr);
//			HostTraceOutputTL(appErrorClass, "mychksumstr:|%s|", mychksumstr);
			return(false);
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "WARNING: No Checksum found!");
		return(false);
	}
}

void CalcLift(double curalt, Char *utc, double inputsecs, Int8 command)
{
	// Variables for Current Lift Calc
	//DateTimeType curdt;
	double liftcursecs, timediff;
	double altdiff = 0.0;
	double polarsink, speed;
	static double lastsecs = 0.0;
	static double lastinputsecs = 0.0;
	static double lastalt = 0.0;
	static double firstsecs = 0.0;

	// Variables for Average Lift Calc
	Int16 i;
	double liftsum = 0.0;
	double distsum = 0.0;
	double polarsum = 0.0;
	static Int16 liftidx = 0;
	static Int16 maxliftidx = 0;
	static Boolean atmaxliftidx = false;
	static double liftarray[360];
	static double distarray[360];
	static double polararray[360];
	static double lastlat = INVALID_LAT_LON, lastlon = INVALID_LAT_LON;
	double tempdbl;

//	HostTraceOutputTL(appErrorClass, "inputsecs:|%s|", DblToStr(inputsecs, 2));
	// Reset the average lift calculations
	if (command == RESET) {
		liftidx = 0;
		maxliftidx = 0;
		atmaxliftidx = false;
		firstsecs = 0.0;
		return;
	} 

	// Reset the average lift calculation 
	if (command == PREFILL) {
		liftidx = 0;
		maxliftidx = 0;
		liftarray[liftidx] = 0.0; //curalt; was curalt? don't know wny
		distarray[liftidx] = 0.0;
		polararray[liftidx] = 0.0;
		liftidx++;
		maxliftidx = liftidx;
		atmaxliftidx = false;
		firstsecs = 0.0;
		return;
	}
	
	/* Calculate current lift in Knots */
	if (inputsecs == -9999.9 && data.config.flightcomp == RECOCOMP) {
		// If using a RECO Computer/Logger then I will not calculate the
		// lift information and await information from the RECOB sentence
		return;
	} else if (inputsecs != -9999.9) {
		if (lastinputsecs == 0.0) {
			lastinputsecs = inputsecs;
		}
		// If inputsecs contains real numbers then that means that a RECO
		// is attached so I will use that information
		timediff = inputsecs - lastinputsecs;
		// This is to allow for the case where a ReCo is being used
		// and the seconds go from near 60 second back to near zero.
		if (timediff < 0.0) {
			timediff += 60.0;
		}
//		HostTraceOutputTL(appErrorClass, "lastinputsecs:|%s|", DblToStr(lastinputsecs, 2));
//		HostTraceOutputTL(appErrorClass, "timediff1:|%s|", DblToStr(timediff, 2));
//		HostTraceOutputTL(appErrorClass, "lastsecs:|%s|", DblToStr(lastsecs, 2));
		liftcursecs = lastsecs + timediff;
	} else {
//		HostTraceOutputTL(appErrorClass, "utc:|%s|", utc);
		//StringToDateOrTime(utc, &curdt, 1);
		//liftcursecs = (double)TimDateTimeToSeconds(&curdt);
		liftcursecs = utcsecs;
	}

//	HostTraceOutputTL(appErrorClass, "liftcursecs:|%s|", DblToStr(liftcursecs, 2));
	if (lastsecs == 0.0) {
		lastsecs = liftcursecs;
		lastalt = curalt;
	}

	if (firstsecs == 0.0) {
		firstsecs = liftcursecs;
	}

	timediff = liftcursecs - lastsecs;
	// This is to allow for the case where a ReCo is being used
	// and the seconds go from near 60 second back to near zero.
	// Shouldn't need this now
//	if (timediff < 0.0) {
//		timediff += 60.0;
//	}
//	HostTraceOutputTL(appErrorClass, "timediff2:|%s|", DblToStr(timediff, 2));
//	HostTraceOutputTL(appErrorClass, "curalt:|%s|", DblToStr(curalt, 1));
//	HostTraceOutputTL(appErrorClass, "lastalt:|%s|", DblToStr(lastalt, 1));
	altdiff = curalt - lastalt;
	lastalt = curalt;
	lastsecs = liftcursecs;
	lastinputsecs = inputsecs;
//	HostTraceOutputTL(appErrorClass, "altdiff:|%s|", DblToStr(altdiff, 1));

	// If using a C302, just copy the current vario and averager values then return;
	if (data.config.flightcomp == C302COMP ||
		data.config.flightcomp == TASCOMP ||
		data.config.flightcomp == EWMRSDTASCOMP) {
		data.input.curlift = data.input.vario;
		data.input.avglift = data.input.varioavg;
		return;
	}

	// These devices just output vario but not average vario so we must
	// still calculate the average values.
	if (data.config.flightcomp == B50COMP ||
		 data.config.flightcomp == B50VLKCOMP ||
		 data.config.flightcomp == B50LXCOMP ||
		 data.config.flightcomp == SN10COMP ||
		 data.config.flightcomp == LXVARCOMP ||
		 data.config.flightcomp == IQUECOMP ||
		 data.config.flightcomp == WESTBCOMP) {
		// msr: use vario from flight computer when available
		data.input.curlift = data.input.vario;
	} else if (altdiff == 0.0 || timediff == 0.0) {
		data.input.curlift = 0.0;
	} else {
		data.input.curlift = altdiff / (timediff / 60.0) / AIRFPMCONST;
	}
//	HostTraceOutputTL(appErrorClass, "data.input.curlift:|%s|", DblToStr(data.input.curlift, 2));

	/* Calculate distance from last point */
	if (lastlat != INVALID_LAT_LON || lastlon != INVALID_LAT_LON) { // check for at least one valid position
		LatLonToRange(lastlat, lastlon, data.input.gpslatdbl, data.input.gpslngdbl, &tempdbl);
	} else {
		tempdbl = 0.0;
	}
	distarray[liftidx] = tempdbl;
	lastlat = data.input.gpslatdbl;
	lastlon = data.input.gpslngdbl;

	/* Calculate sink due to polar */
	speed = data.input.true_airspeed.value; // either "real" data, or calculated from ground speed and wind
	polarsink = data.polar.a*speed*speed + data.polar.b*speed + data.polar.c;
	data.input.nettolift = data.input.curlift - polarsink;
	
	/* Calculate average lift of all samples in the last 30 seconds in Knots */
	liftarray[liftidx] = data.input.curlift;
	polararray[liftidx] = polarsink;
	liftidx++;

//	HostTraceOutputTL(appErrorClass, "liftidx1:|%hd|", liftidx);
	if (atmaxliftidx == false) {
		maxliftidx = liftidx;
//		HostTraceOutputTL(appErrorClass, "liftcursecs-firstsecs:|%s|", DblToStr(liftcursecs-firstsecs, 2));
		if ((liftcursecs-firstsecs) >= 30.0) {
			atmaxliftidx = true;
		}
	}
//	HostTraceOutputTL(appErrorClass, "maxliftidx:|%hd|", maxliftidx);
	if ((atmaxliftidx && (liftidx == maxliftidx)) || (liftidx == 360)) {
		liftidx = 0;
		atmaxliftidx = true;
	}
//	HostTraceOutputTL(appErrorClass, "liftidx2:|%hd|", liftidx);
	for (i=0; i < maxliftidx; i++) {
		liftsum += liftarray[i];
		distsum += distarray[i];
		polarsum += polararray[i];
	}
//	HostTraceOutputTL(appErrorClass, "liftsum:|%s|", DblToStr(liftsum, 2));
	if (liftsum == 0.0 ) {
		data.input.avglift = 0.0;
	} else {
		data.input.avglift = liftsum / maxliftidx;
		data.input.avgnettolift = (liftsum - polarsum) / maxliftidx; 
	}
	data.input.distdone = distsum;

//	HostTraceOutputTL(appErrorClass, "data.input.avglift:  |%s|", print_vertical_speed2(data.input.avglift, 2));
//	HostTraceOutputTL(appErrorClass, "data.input.nettolift:|%s|", print_vertical_speed2(data.input.nettolift, 2));
//	HostTraceOutputTL(appErrorClass, "-----------------------------");

	return;
}

void SetFixStatus()
{
	switch (curgpsfixquality) {
		case 0: // Invalid Fix
			pvtData->status.fix = gpsFixInvalid;
			break;
		case 1:	// GPS Fix either 2D or 3D
			if (curgpsfixtype == -1) {
				// Just marking it as a valid fix
				pvtData->status.fix = gpsFixValid;
			} else if (curgpsfixtype == 2) {
				// 2D Fix
				pvtData->status.fix = gpsFix2D;
			} else if (curgpsfixtype == 3) {
				// 3D Fix
				pvtData->status.fix = gpsFix3D;
			} else {
				// No Fix
				pvtData->status.fix = gpsFixInvalid;
			}
			break;
		case 2:	// DGPS Fix either 2D or 3D
			if (curgpsfixtype == 2) {
				// 2D Fix
				pvtData->status.fix = gpsFix2DDiff;
			} else if (curgpsfixtype == 3) {
				// 3D Fix
				pvtData->status.fix = gpsFix3DDiff;
			} else {
				// No Fix
				pvtData->status.fix = gpsFixInvalid;
			}
			break;
	}
	return;
}
