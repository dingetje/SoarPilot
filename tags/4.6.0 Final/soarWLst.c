#include <PalmOS.h>
#include "soaring.h"
#include "soarWLst.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarMap.h"
#include "soarUMap.h"
#include "soarIO.h"
#include "soarWay.h"
#include "soarDB.h"
#include "soarSTF.h"
#include "soarMath.h"
#include "soarTer.h"

WaypointData 	*selectedWaypoint;
Int16 		selectedWaypointIndex = -1;
WaypointData	*TempWpt;
Int16 		numOfWaypts = 0;
Int16		currentWayptPage = 0;
Boolean 	newWaypt = false;
UInt16  	WayptsortType = SortByDistance;
Boolean 	wayselect = false;

extern double MCCurVal;
extern Boolean recv_data;
extern UInt32 cursecs;
extern Int16 activetskway;
extern Boolean menuopen;

// David Lane - comparison function supplied to DmQuickSort 
Int16 waypt_comparison(WaypointData* rec1, WaypointData* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h)
{
	//Not sure it should be initialized to zero
	Int16 retval=0;
	Boolean tempwpt1 = (StrNCompare(rec1->name, "_WP", 3) == 0);
	Boolean tempwpt2 = (StrNCompare(rec2->name, "_WP", 3) == 0);

	switch ( order ) {
		case SortByLastUsed:
			// PG - Recently Used
			retval = StrCompare(rec2->UsedTime, rec1->UsedTime);
			if (retval == 0) retval = StrCompare(rec1->name, rec2->name);
			break;
		case SortByNameA:
			// David Lane - compare the names of the two waypoints in ascending order
			// PG - allow for temp wpts to be sorted to the bottom
			if ((tempwpt1 && tempwpt2) || (!tempwpt1 && !tempwpt2)) {
				retval = StrCompare(rec1->name, rec2->name);
			} else if (tempwpt1) {
				retval = 1;				
			} else {
				retval = -1;
			}
			break;
		case SortByNameD:
			// David Lane - compare the names of the two waypoints in descending order
			// PG - allow for temp wpts to be sorted to the top
			if ((tempwpt1 && tempwpt2) || (!tempwpt1 && !tempwpt2)) {
				retval = StrCompare(rec2->name, rec1->name);
			} else if (tempwpt1) {
				retval = -1;				
			} else {
				retval = 1;
			}
			break;
		case SortByDistance:
		{
			if (rec1->distance == rec2->distance) {
				retval = StrCompare(rec1->name, rec2->name);
			} else if (rec1->distance < rec2->distance) {
				retval = -1;
			} else {
				retval = 1;
			}
			break;
		}
		case SortByDistApt:
		{
			if ((rec1->type & AIRLAND) && (rec2->type & AIRLAND)) {
				if (rec1->distance == rec2->distance) {
					retval = StrCompare(rec1->name, rec2->name);
				} else if (rec1->distance < rec2->distance) {
					retval = -1;
				} else {
					retval = 1;
				}
			} else if (rec1->type & AIRLAND) {
				retval = -1;
			} else if (rec2->type & AIRLAND) {
				retval = 1;
			} else {
				if (rec1->distance == rec2->distance) {
					retval = StrCompare(rec1->name, rec2->name);
				} else if (rec1->distance < rec2->distance) {
					retval = -1;
				} else {
					retval = 1;
				}
			}
			break;
		}
	}
	return (retval);
}

void refresh_waypoint_list(Int16 scr)
{
	#define FIELDLEN 16

	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3, lst4;
	Int16 x;
	Int16 wayptIndex;
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;
	static Char **items = NULL;
	static Char **dist = NULL;
	static Char **bear = NULL;
	static Char **aalt = NULL;
	Int16 nrecs,i;
	Int16 start;
	Int16 end;
	static Int16 prevNumRecs = 0;
	WaypointData waypoint;
	Char pageString[20];
	Char tmpString[20];

	// David Lane - free up each of the previous strings and then free up
	//  the array of pointers itself.
	for (x = 0; x < prevNumRecs; x++) {
		MemPtrFree(items[x]);
		MemPtrFree(dist[x]);
		MemPtrFree(bear[x]);
		MemPtrFree(aalt[x]);
	}
	if (items) {
		MemPtrFree(items);
		items = NULL;
	}
	if (dist) {
		MemPtrFree(dist);
		dist = NULL;
	}
	if (bear) {
		MemPtrFree(bear);
		bear = NULL;
	}
	if (aalt) {
		MemPtrFree(aalt);
		aalt = NULL;
	}

	if (scr != 9999) {
		// David Lane - get the number of waypoints in the database
		numOfWaypts = DmNumRecords(waypoint_db);

		// surpress un-used waypoint when sorting in last used order
		if (WayptsortType == SortByLastUsed) {
			for (i=0; i<numOfWaypts; i++) {
				OpenDBQueryRecord(waypoint_db, i, &waypoint_hand, &waypoint_ptr);
				MemMove(&waypoint,waypoint_ptr,sizeof(WaypointData));
				MemHandleUnlock(waypoint_hand);
				if (StrLen(waypoint.UsedTime) == 0) break;
			}
			if (i < numOfWaypts) numOfWaypts = i;
			if (selectedWaypointIndex >= numOfWaypts) selectedWaypointIndex = numOfWaypts-1;
		}

		// David Lane - get the List pointer
		lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list));
		lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list2));
		lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list3));
		lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list4));

		// David Lane - lets compute the "page" of waypoints we're
		//  currently looking at.
		if (scr > 0) {
			if (((currentWayptPage + 1) * 7) <  numOfWaypts) {
				// If there are more waypoints to display, move down one page
				currentWayptPage++;
			} else {
				// If at the bottom, wrap to the first page
				currentWayptPage = 0;
			}
		} else if (scr < 0)  {
			if (currentWayptPage > 0) {
				// If not on the first page of waypoints, move up one page 
				currentWayptPage--;
			} else {
				// If at the top, wrap to the last page
				if (numOfWaypts == 0) {
					currentWayptPage = 0;
				} else if (Fmod((double)numOfWaypts,7.0) == 0.0) {
					currentWayptPage = (Int16)(numOfWaypts/7) - 1;
				} else {
					currentWayptPage = (Int16)(numOfWaypts/7);
				}
			}
		}

		calc_waypoint_list_values(currentWayptPage);

		// David Lane - given the current "page", compute the starting
		//  and ending index and the number of records.
		start = currentWayptPage * 7;
		end = ((start + 7) > numOfWaypts) ? numOfWaypts : (start + 7);
		nrecs = end - start;

		if (nrecs > 0) {
			
			// David Lane - we got at least one record so allocate enough 
			//  memory to hold nrecs pointers-to-pointers
			items = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			dist = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			bear = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			aalt = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// David Lane - loop through each waypoint record
			for (x = 0, wayptIndex = start; wayptIndex < end; wayptIndex++, x++) { 
				// David Lane - assign each of the nrecs pointers-to-pointers
				//  the address of a newly allocated 25 character array,
				//  retrieve the waypoint name associated with that record,
				//  and copy that name into the array.
				OpenDBQueryRecord(waypoint_db, wayptIndex, &waypoint_hand, &waypoint_ptr);
				MemMove(&waypoint,waypoint_ptr,sizeof(WaypointData));
				MemHandleUnlock(waypoint_hand);
				items[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				dist[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				bear[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				aalt[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				MemSet(items[x],FIELDLEN,0);
				MemSet(dist[x],FIELDLEN,0);
				MemSet(bear[x],FIELDLEN,0);
				MemSet(aalt[x],FIELDLEN,0);

				// waypoint names
				if ((WayptsortType == SortByDistApt) && ((waypoint.type & AIRPORT) == 0) && (waypoint.type & AIRLAND)) {
					StrCopy(tmpString, "~");
					StrCat(tmpString, waypoint.name);
					StrNCopy(items[x], tmpString, 9);
				} else {
					StrNCopy(items[x], waypoint.name, 9);
				}

				// distances
				if ((waypoint.distance*data.input.disconst) < 999.5) {
					StrCopy(dist[x], print_distance2(waypoint.distance, 1));
					StrCopy(dist[x], leftpad(dist[x], '\031', 5));
				} else if ((waypoint.distance*data.input.disconst) < 9999.5) {
					StrCopy(dist[x], print_distance2(waypoint.distance, 0));
					StrCopy(dist[x], leftpad(dist[x], ' ', 4));
				} else {
					StrCopy(dist[x], "9999");
					StrCopy(dist[x], leftpad(dist[x], '\031', 4));
				}		

				// arrival altitudes
				if ((waypoint.alt*data.input.altconst) > 99999) {
					StrCopy(aalt[x], " 99999");
				} else if ((waypoint.alt*data.input.altconst) < -99999) {
					StrCopy(aalt[x], "-99999");
				} else {
					if (waypoint.alt < 0) {
						StrCopy(aalt[x], "-");
					} else {
						StrCopy(aalt[x], " ");
					}
					StrCat(aalt[x], DblToStr(Fabs(waypoint.alt) * data.input.altconst,0));
					StrCopy(aalt[x], leftpad(aalt[x], '\031', 6));
				}
				if (waypoint.elevation == 0.0) {
					StrCat(aalt[x],"!");
				}

				// bearings
				StrCopy(bear[x], print_direction2(waypoint.bearing));
				StrCat(bear[x], "°");
			}
					
			// David Lane - reform the list
			LstSetListChoices(lst, items, nrecs);
			LstSetListChoices(lst2, dist, nrecs);
			LstSetListChoices(lst3, bear, nrecs);
			LstSetListChoices(lst4, aalt, nrecs);
		} else {
			items = (char **) MemPtrNew(1 * (sizeof(char *)));
			dist = (char **) MemPtrNew(1 * (sizeof(char *)));
			bear = (char **) MemPtrNew(1 * (sizeof(char *)));
			aalt = (char **) MemPtrNew(1 * (sizeof(char *)));
			prevNumRecs = 1;		
			items[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			dist[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			bear[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			aalt[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			MemSet(items[0],FIELDLEN,0);
			MemSet(dist[0],FIELDLEN,0);
			MemSet(bear[0],FIELDLEN,0);
			MemSet(aalt[0],FIELDLEN,0);
			StrNCopy(items[0], "No WPs", 9);
			LstSetListChoices(lst, items, 1);
			LstSetListChoices(lst2, dist, 1);
			LstSetListChoices(lst3, bear, 1);
			LstSetListChoices(lst4, aalt, 1);
			LstSetSelection(lst, 0);
			LstSetSelection(lst2, 0);
			LstSetSelection(lst3, 0);
			LstSetSelection(lst4, 0);
			selectedWaypointIndex = -1;
			ctl_set_visible(form_list_waypt_edit, false);
			ctl_set_visible(form_list_waypt_findbtn, false);
			ctl_set_visible(form_list_waypt_OK, false);
		}
	
		// David Lane - create the "Page: # of #" string
		MemSet(pageString,20, 0);
		if (numOfWaypts == 0) {
			StrCopy(pageString,StrIToA(tmpString,(currentWayptPage)));
		} else {
			StrCopy(pageString,StrIToA(tmpString,(currentWayptPage+1)));
		}
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tmpString,(numOfWaypts % 7) ? (((int)(numOfWaypts/7)) + 1) : (int)(numOfWaypts/7)));
		field_set_value(form_list_waypt_page, pageString);
		field_set_value(form_list_waypt_nrecs, StrIToA(tmpString,numOfWaypts));
	
		// David Lane - redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
			LstDrawList(lst2);
			LstDrawList(lst3);
			LstDrawList(lst4);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_waypt_list));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_waypt_list2));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_waypt_list3));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_waypt_list4));
		}


		// David Lane - if the currently selected waypoint is on the currently
		//  displayed page, then darken it as if it were selected.  If not then
		//  de-select everything.
		if ((selectedWaypointIndex >= (currentWayptPage*7)) && (selectedWaypointIndex < ((currentWayptPage*7) + 7))) {
  		 	LstSetSelection(lst, selectedWaypointIndex % 7);
  		 	LstSetSelection(lst2, selectedWaypointIndex % 7);
  		 	LstSetSelection(lst3, selectedWaypointIndex % 7);
  		 	LstSetSelection(lst4, selectedWaypointIndex % 7);
			//MFH Had to do this to get selectedWaypoint updated after the reselection.
			//    Not sure why it wasn't working with just the LstSetSelection.
			OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &waypoint_hand, &waypoint_ptr);
			MemMove(selectedWaypoint, waypoint_ptr, sizeof(WaypointData));
			MemHandleUnlock(waypoint_hand);
		} else {
  		 	LstSetSelection(lst, -1);
  		 	LstSetSelection(lst2, -1);
  		 	LstSetSelection(lst3, -1);
  		 	LstSetSelection(lst4, -1);
		}

		// draw the horizontal lines
		DrawHorizListLines(7, 44, 14);
	}
}

void calc_dist_bear_aalt(Int16 index)
{
	MemHandle	waypoint_hand = NULL;
	MemPtr		waypoint_ptr;
	WaypointData	wayptData;
	double 		glide_ratio = 0.0;

	OpenDBQueryRecord(waypoint_db, index, &waypoint_hand, &waypoint_ptr);
	MemMove(&wayptData, waypoint_ptr, sizeof(WaypointData));
	MemHandleUnlock(waypoint_hand);				

	// David Lane - calculate the distance and bearing
	LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, wayptData.lat, wayptData.lon, &wayptData.distance, &wayptData.bearing);
	// Convert to Magnetic
	wayptData.bearing = nice_brg(wayptData.bearing + data.input.deviation.value);

	glide_ratio = (wayptData.distance * FPNM)/data.config.ldforcalcs;
	switch (data.config.alttype) {
		case REQALT:
   		wayptData.alt = glide_ratio + wayptData.elevation + data.config.safealt;
			break;
		case ARVALT:
	  		wayptData.alt = data.input.inusealt - glide_ratio - wayptData.elevation;
			break;
		case DLTALT:
	  		wayptData.alt = data.input.inusealt - (glide_ratio + wayptData.elevation + data.config.safealt);
			break;
		default:
			break;
	}

	// David Lane - write the modified record
	OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), &wayptData, index);
}


// MFH - Same as calc_dist_bear_aalt except you pass in a pointer to a WaypointData structure
//       vice a db index.
void calc_dist_bear_aalt2(WaypointData *wayptData)
{
	double glide_ratio = 0.0;

	// David Lane - calculate the distance and bearing
	LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, wayptData->lat, wayptData->lon, &wayptData->distance, &wayptData->bearing);
	// Convert to Magnetic
	wayptData->bearing = nice_brg(wayptData->bearing + data.input.deviation.value);

	glide_ratio = (wayptData->distance * FPNM)/data.config.ldforcalcs;
	switch (data.config.alttype) {
		case REQALT:
   		wayptData->alt = glide_ratio + wayptData->elevation + data.config.safealt;
			break;
		case ARVALT:
   		wayptData->alt = data.input.inusealt - glide_ratio - wayptData->elevation;
			break;
		case DLTALT:
   		wayptData->alt = data.input.inusealt - (glide_ratio + wayptData->elevation + data.config.safealt);
			break;
		default:
			break;
	}
}

void calc_alt(WaypointData *wayptData)
{
	double glide_ratio = 0.0;

	glide_ratio = (wayptData->distance * FPNM)/data.config.ldforcalcs;
	switch (data.config.alttype) {
		case REQALT:
   		wayptData->alt = glide_ratio + wayptData->elevation + data.config.safealt;
			break;
		case ARVALT:
   		wayptData->alt = data.input.inusealt - glide_ratio - wayptData->elevation;
			break;
		case DLTALT:
   		wayptData->alt = data.input.inusealt - (glide_ratio + wayptData->elevation + data.config.safealt);
			break;
		default:
			break;
	}
}

// Given a distance and a destination elevation, calculate the needed altitude value
// based on the current L/D setting
double calc_altld(double distance, double elevation, double startalt, Boolean usesafealt)
{
	double glide_ratio = 0.0;
	double alt;
	double safealt;

	if (usesafealt) {
		safealt = data.config.safealt;
	} else {
		safealt = 0.0;
	}

	glide_ratio = (distance * FPNM)/data.config.ldforcalcs;
	switch (data.config.alttype) {
		case REQALT:
   		alt = glide_ratio + elevation + safealt;
			break;
		case ARVALT:
   		alt = startalt - glide_ratio - elevation;
			break;
		case DLTALT:
   		alt = startalt - (glide_ratio + elevation + safealt);
			break;
		default:
   		alt = glide_ratio + elevation + safealt;
			break;
	}
	return(alt);
}

void calc_waypoint_list_values(Int16 page)
{
	Int16			wayptIndex;
	Int16 			numOfWaypts;
	Int16			start = 0;
	Int16			end = 0;
	double			WaySTF=0.0;
	double			startalt;
	MemHandle		waypoint_hand = NULL;
	MemPtr			waypoint_ptr;
	WaypointData		wayptData;

	numOfWaypts = DmNumRecords(waypoint_db);
	if (numOfWaypts == 0) {
		return;
	}

//     	HostTraceOutputTL(appErrorClass, "Calculating Whole Waypoint List");

	start = page * 7;
	end = ((start + 7) > numOfWaypts) ? numOfWaypts : (start + 7);

	// David Lane - go through the records in the Waypoint database 
	for (wayptIndex = start; wayptIndex < end; wayptIndex++) {
		
		OpenDBQueryRecord(waypoint_db, wayptIndex, &waypoint_hand, &waypoint_ptr);
		MemMove(&wayptData, waypoint_ptr, sizeof(WaypointData));
		MemHandleUnlock(waypoint_hand);				

		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, wayptData.lat, wayptData.lon, &wayptData.distance, &wayptData.bearing);
		// Convert to Magnetic
		wayptData.bearing = nice_brg(wayptData.bearing + data.input.deviation.value);

		startalt = data.input.inusealt;

		if (CalcSTFSpdAlt(MCCurVal, wayptData.distance, wayptData.bearing, wayptData.elevation, startalt, true, &WaySTF, &wayptData.alt)==false) {
			// Calling this again because the CalcSTFSpdAlt returned false
			calc_dist_bear_aalt2(&wayptData);
		}
		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), &wayptData, wayptIndex);
	}

	// MFH - Resort page only if sorting by Distance or Distance/Airport
	// MFH - Does not do the sort if in Waypoint Select Mode
	if (!wayselect && ((WayptsortType == SortByDistance) || (WayptsortType == SortByDistApt))) {
//		HostTraceOutputTL(appErrorClass, "Sort Waypoints1");
		DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
		if (selectedWaypointIndex != -1) {
			selectedWaypointIndex = FindWayptRecordByName(selectedWaypoint->name);
		}
	}

}

// David Lane - this function is called by the event loop.
//  It steps through a seven record portion of the waypoint database
//  and calculates the distance and bearing from each waypoint to the
//  current point reported by the GPS.  
//
//  The seven record portion is either the list we're looking at
//  on the waypoint form, or is "the next" page in a wrap-around
//  algorithm which allows for the entire database to be calculated
//  (eventually).
//
//  When we do "wrap-around", meaning we've gotten to all the records
//  in the database, we can display the Distance button thereby
//  allowing a user to sort by distance.
//
void update_waypoint_list_values_event()
{
	Int16		wayptIndex;
	Int16 		numOfWaypts;
	Int16		start = 0;
	Int16		end = 0;
	static Int16	currentPage = 0;
	static Int16	myPage = 0;
	static Int16	page = 0;
	double		WaySTF=0.0;
	double		startalt;
	MemHandle	waypoint_hand = NULL;
	MemPtr		waypoint_ptr;
	WaypointData	wayptData;

	numOfWaypts = DmNumRecords(waypoint_db);
	if (numOfWaypts == 0) {
		return;
	}

//	HostTraceOutputTL(appErrorClass, "Updating Waypoint List : Ct %s",DblToStr(start,0));
	start = page * 7;
	end = ((start + 7) > numOfWaypts) ? numOfWaypts : (start + 7);

	// David Lane - go through the records in the Waypoint database 
	for (wayptIndex = start; wayptIndex < end; wayptIndex++) {
		
		OpenDBQueryRecord(waypoint_db, wayptIndex, &waypoint_hand, &waypoint_ptr);
		MemMove(&wayptData, waypoint_ptr, sizeof(WaypointData));
		MemHandleUnlock(waypoint_hand);				

		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, wayptData.lat, wayptData.lon, &wayptData.distance, &wayptData.bearing);
		// Convert to Magnetic
		wayptData.bearing = nice_brg(wayptData.bearing + data.input.deviation.value);

		startalt = data.input.inusealt;

		if (CalcSTFSpdAlt(MCCurVal, wayptData.distance, wayptData.bearing, wayptData.elevation, startalt, true, &WaySTF, &wayptData.alt)==false) {
			// Calling this again because the CalcSTFSpdAlt returned false
			calc_dist_bear_aalt2(&wayptData);
		}
		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), &wayptData, wayptIndex);
	}

	if (currentPage == 0) {
		page = currentWayptPage;
		currentPage++;
		if (FrmGetActiveFormID() == form_list_waypt && !menuopen) {
			// update waypoint list
			refresh_waypoint_list(0);
		}
	} else {
 	   	if (++myPage > (int)(numOfWaypts/7)) {
			myPage = 0;
		}
		page = myPage;
		currentPage++;
		// Checks the first page every 7 calculations of other pages
		// this is useful for Dist and Dist/Apt modes to ensure the nearest waypoints are updated regularly
		if (currentPage == 7) currentPage = 0;
	}
}

Boolean save_waypt_fields()
{
	char *str;
	Boolean dbempty = true;
	Char waypointname[15];
	double UTMn, UTMe;
	Char UTMz[5];
	
	// Check to see if there are any waypoints already in the database
	if (OpenDBCountRecords(waypoint_db)) {
		dbempty = false;
	}

	selectedWaypoint->elevation = field_get_double(form_av_waypt_elev)/data.input.altconst;

	if (data.config.llfmt == LLUTM) {
		UTMe = field_get_double(form_av_waypt_lat);
		UTMn = field_get_double(form_av_waypt_lon);
		StrCopy(UTMz, field_get_str(form_av_waypt_utmzone));
		UTMtoLL(UTMWGS84, UTMn, UTMe, UTMz, &selectedWaypoint->lat,  &selectedWaypoint->lon);
	} else {
		str = field_get_str(form_av_waypt_lat);
		if (str){
			if (data.config.llfmt == LLDM) {
				selectedWaypoint->lat = DegMinColonStringToLatLon(str);
			} else {
				selectedWaypoint->lat = DegMinSecColonStringToLatLon(str);
			}
			if ((selectedWaypoint->lat == 0.0) && (data.input.homeLat < 0.0)) selectedWaypoint->lat = -0.0000001;
		}
		str = field_get_str(form_av_waypt_lon);
		if (str) {
			if (data.config.llfmt == LLDM) {
				selectedWaypoint->lon = DegMinColonStringToLatLon(str);
			} else {
				selectedWaypoint->lon = DegMinSecColonStringToLatLon(str);
			}
			if ((selectedWaypoint->lon == 0.0) && (data.input.homeLon < 0.0)) selectedWaypoint->lon = -0.0000001;
		}
	}

	selectedWaypoint->type = NULL;

	if (ctl_get_value(form_av_waypt_typeRF)) selectedWaypoint->type|= REFWPT;		
	//selectedWaypoint->type &= REFWPT;
	if (selectedWaypoint->type & REFWPT) {
		ClearWayptREFStatus();
		data.input.refLat = selectedWaypoint->lat;
		data.input.refLon = selectedWaypoint->lon;
		if (data.config.RefWptRadial) {
			data.input.refwpttype = REFRADIAL;
		} else {
			data.input.refwpttype = REFBEARING;
		}
	}
	if (ctl_get_value(form_av_waypt_typeA)) selectedWaypoint->type|= AIRPORT;
	if (ctl_get_value(form_av_waypt_typeT)) selectedWaypoint->type|= TURN;
	if (ctl_get_value(form_av_waypt_typeL)) selectedWaypoint->type|= LAND;
	if (ctl_get_value(form_av_waypt_typeS)) selectedWaypoint->type|= START;
	if (ctl_get_value(form_av_waypt_typeF)) selectedWaypoint->type|= FINISH;
	if (ctl_get_value(form_av_waypt_typeM)) selectedWaypoint->type|= MARK;
	if (ctl_get_value(form_av_waypt_typeTH)) selectedWaypoint->type|= THRML;
	if (ctl_get_value(form_av_waypt_typeH) || dbempty) {
		if (!dbempty) {
			ClearWayptHOMEStatus();
		}
		selectedWaypoint->type|= HOME;
		HomeWayptInitialize(selectedWaypoint);
		// if not receiving GPS data, update all waypoint distance and bearings to new home waypoint
		if (!recv_data) {
			calcallwaypoints(data.input.homeLat, data.input.homeLon);
			selectedWaypoint->distance = 0.0;
			selectedWaypoint->bearing = 0.0;
		}
		data.input.terelev = GetTerrainElev(data.input.gpslatdbl, data.input.gpslngdbl);
	}
	
	if ((selectedWaypoint->type & AIRPORT) || (selectedWaypoint->type & LAND)) selectedWaypoint->type |= AIRLAND;
	if (ctl_get_value(form_av_waypt_typeAR)) {
		selectedWaypoint->type|= AREA;
		selectedWaypoint->arearadial1mag = ctl_get_value(form_av_waypt_arearadial1m);
		selectedWaypoint->arearadial2mag = ctl_get_value(form_av_waypt_arearadial2m);
		selectedWaypoint->arearadial1 = (UInt16)nice_brg(field_get_long(form_av_waypt_arearadial1));
		selectedWaypoint->arearadial2 = (UInt16)nice_brg(field_get_long(form_av_waypt_arearadial2));
		selectedWaypoint->arearadius = Fabs(field_get_double(form_av_waypt_arearadius)/data.input.disconst);
		selectedWaypoint->arearadius2 = Fabs(field_get_double(form_av_waypt_arearadius2)/data.input.disconst);
		// if set to Magnetic, convert entry from Magnetic to True for storage
		if (selectedWaypoint->arearadial1mag) {
			selectedWaypoint->arearadial1 = (UInt16)nice_brg(selectedWaypoint->arearadial1 - data.input.deviation.value);
		}
		if (selectedWaypoint->arearadial2mag) {
			selectedWaypoint->arearadial2 = (UInt16)nice_brg(selectedWaypoint->arearadial2 - data.input.deviation.value);
		}
	} else {
		selectedWaypoint->arearadial1 = 0;
		selectedWaypoint->arearadial2 = 0;
		selectedWaypoint->arearadius = 0.0;
		selectedWaypoint->arearadius2 = 0.0;
		selectedWaypoint->arearadial1mag = false;
		selectedWaypoint->arearadial2mag = false;
	}

	StrCopy(selectedWaypoint->rmks, NoComma(field_get_str(form_av_waypt_remark)," "));

	// do error checks

	// check for empty name
	StrCopy(waypointname, field_get_str(form_av_waypt_name));
	if (StrLen(trim(NoComma(waypointname," "),' ',true)) == 0) {
		warning->type = Wgeneric;
		StrCopy(warning->line1, "Waypoint name");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);
		return(false);
	}
	StrCopy(selectedWaypoint->name, trim(NoComma(waypointname," "),' ',true));

	if (selectedWaypoint->type & AREA) {
		// check for non-zero max-R
		if (selectedWaypoint->arearadius == 0) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius cannot be Zero");
			StrCopy(warning->line2, "for an AREA Waypoint");
			FrmPopupForm(form_warning);
			return(false);
		}
		// check for max R > min R
		if (selectedWaypoint->arearadius2 >= selectedWaypoint->arearadius) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius must be bigger");
			StrCopy(warning->line2, "than Min Radius");
			FrmPopupForm(form_warning);
			return(false);
		}
	}
	
	if (newWaypt) {
		OpenDBAddRecord(waypoint_db, sizeof(WaypointData), selectedWaypoint, true);
	} else {
		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), selectedWaypoint, selectedWaypointIndex);
	}

	DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
	selectedWaypointIndex = FindWayptRecordByName(selectedWaypoint->name);

	return(true);
}

// save entered waypoint details to a temp waypoint
Boolean TempWptSave() {

	char *str;
	Char waypointname[15];
	double UTMn, UTMe;
	Char UTMz[5];
	
	TempWpt->elevation = field_get_double(form_av_waypt_elev)/data.input.altconst;
	TempWpt->alt = selectedWaypoint->alt;
	TempWpt->distance = selectedWaypoint->distance;
	TempWpt->bearing = selectedWaypoint->bearing;

	if (data.config.llfmt == LLUTM) {
		UTMe = field_get_double(form_av_waypt_lat);
		UTMn = field_get_double(form_av_waypt_lon);
		StrCopy(UTMz, field_get_str(form_av_waypt_utmzone));
		UTMtoLL(UTMWGS84, UTMn, UTMe, UTMz, &TempWpt->lat,  &TempWpt->lon);
	} else {
		str = field_get_str(form_av_waypt_lat);
		if (str){
			if (data.config.llfmt == LLDM) {
				TempWpt->lat = DegMinColonStringToLatLon(str);
			} else {
				TempWpt->lat = DegMinSecColonStringToLatLon(str);
			}
			if ((TempWpt->lat == 0.0) && (data.input.homeLat < 0.0)) TempWpt->lat = -0.0000001;
		}
		str = field_get_str(form_av_waypt_lon);
		if (str) {
			if (data.config.llfmt == LLDM) {
				TempWpt->lon = DegMinColonStringToLatLon(str);
			} else {
				TempWpt->lon = DegMinSecColonStringToLatLon(str);
			}
			if ((TempWpt->lon == 0.0) && (data.input.homeLon < 0.0)) TempWpt->lon = -0.0000001;
		}
	}

	TempWpt->type = NULL; 
	if (selectedWaypoint->type & REFWPT) TempWpt->type |= REFWPT;
	if (ctl_get_value(form_av_waypt_typeA)) TempWpt->type|= AIRPORT;
	if (ctl_get_value(form_av_waypt_typeT)) TempWpt->type|= TURN;
	if (ctl_get_value(form_av_waypt_typeL)) TempWpt->type|= LAND;
	if (ctl_get_value(form_av_waypt_typeS)) TempWpt->type|= START;
	if (ctl_get_value(form_av_waypt_typeF)) TempWpt->type|= FINISH;
	if (ctl_get_value(form_av_waypt_typeM)) TempWpt->type|= MARK;
	if (ctl_get_value(form_av_waypt_typeTH)) TempWpt->type|= THRML;
	if (ctl_get_value(form_av_waypt_typeH)) TempWpt->type|= HOME;

	if ((TempWpt->type & AIRPORT) || (TempWpt->type & LAND)) TempWpt->type |= AIRLAND;
	if (ctl_get_value(form_av_waypt_typeAR)) {
		TempWpt->type|= AREA;
		TempWpt->arearadial1mag = ctl_get_value(form_av_waypt_arearadial1m);
		TempWpt->arearadial2mag = ctl_get_value(form_av_waypt_arearadial2m);
		TempWpt->arearadial1 = (UInt16)nice_brg(field_get_long(form_av_waypt_arearadial1));
		TempWpt->arearadial2 = (UInt16)nice_brg(field_get_long(form_av_waypt_arearadial2));
		TempWpt->arearadius = Fabs(field_get_double(form_av_waypt_arearadius)/data.input.disconst);
		TempWpt->arearadius2 = Fabs(field_get_double(form_av_waypt_arearadius2)/data.input.disconst);
		// save AREA attributes for correct display of radii on return
		selectedWaypoint->type|= AREA;
		selectedWaypoint->arearadius = TempWpt->arearadius;
		selectedWaypoint->arearadius2 = TempWpt->arearadius2;
		// if set to Magnetic, convert entry from Magnetic to True for storage
		if (TempWpt->arearadial1mag) {
			TempWpt->arearadial1 = (UInt16)nice_brg(TempWpt->arearadial1 - data.input.deviation.value);
		}
		if (TempWpt->arearadial2mag) {
			TempWpt->arearadial2 = (UInt16)nice_brg(TempWpt->arearadial2 - data.input.deviation.value);
		}
	} else {
		TempWpt->arearadial1 = 0;
		TempWpt->arearadial2 = 0;
		TempWpt->arearadius = 0.0;
		TempWpt->arearadius2 = 0.0;
		TempWpt->arearadial1mag = false;
		TempWpt->arearadial2mag = false;
	}

	str = field_get_str(form_av_waypt_remark);
	if (str) {
		StrCopy(TempWpt->rmks, NoComma(str," "));
	}

	TempWpt->rwdir = selectedWaypoint->rwdir;
	TempWpt->rwlen = selectedWaypoint->rwlen;
	StrCopy(TempWpt->freq, NoComma(selectedWaypoint->freq," "));
	StrCopy(TempWpt->geninfo, NoComma(selectedWaypoint->geninfo," "));
	StrCopy(TempWpt->UsedTime, NoComma(selectedWaypoint->UsedTime," "));

	// do error checks

	// check for empty name
	StrCopy(waypointname, field_get_str(form_av_waypt_name));
	if (StrLen(trim(NoComma(waypointname," "),' ',true)) == 0 ) {
		warning->type = Wgeneric;
		StrCopy(warning->line1, "Waypoint name");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);
		return(false);
	}
	StrCopy(TempWpt->name, trim(NoComma(waypointname," "),' ',true));

	if (TempWpt->type & AREA) {
		// check for non-zero max-R
		if (TempWpt->arearadius == 0) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius cannot be Zero");
			StrCopy(warning->line2, "for an AREA Waypoint");
			FrmPopupForm(form_warning);
			return(false);
		}
		// check for max R > min R
		if (TempWpt->arearadius2 >= TempWpt->arearadius) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius must be bigger");
			StrCopy(warning->line2, "than Min Radius");
			FrmPopupForm(form_warning);
			return(false);
		}
	}
	
return(true);
}

void InuseWaypointCalcEvent()
{
	double poirange, poibearing;

	if (data.config.usepalmway && data.input.destination_valid) {
//		HostTraceOutputTL(appErrorClass, "Update inuseWaypoint Event : %lu", cursecs%86400);
		if (data.config.accuratedistcalc) {// && (data.config.earthmodel != EMSPHERE)) {
			// accurate range calculation
			LatLonToRangeBearingEll(data.input.gpslatdbl, data.input.gpslngdbl, data.inuseWaypoint.lat, data.inuseWaypoint.lon, &poirange, &poibearing);
		} else {
			// faster approximate range calculation
			LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.inuseWaypoint.lat, data.inuseWaypoint.lon, &poirange, &poibearing);
		}
		// LatLonToRangeBearing returns a True bearing
		// Converted to Magnetic
		poibearing = nice_brg(poibearing + data.input.deviation.value);
		data.inuseWaypoint.distance = poirange;
		data.inuseWaypoint.bearing = poibearing;

		// update destination data
		data.input.destination_lat = data.inuseWaypoint.lat;
		data.input.destination_lon = data.inuseWaypoint.lon;
		data.input.destination_type = data.inuseWaypoint.type;
		data.input.destination_elev = data.inuseWaypoint.elevation;
		StrCopy(data.input.destination_name, data.inuseWaypoint.name);
		// update destination distance and bearing
		data.input.distance_to_destination.valid = VALID;
		data.input.distance_to_destination.value = poirange;
		data.input.bearing_to_destination.valid = VALID;
		data.input.bearing_to_destination.value = poibearing;
	}

	return;
}
