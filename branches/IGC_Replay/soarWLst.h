#ifndef SOARWLST_H
#define SOARWLST_H

#define SWAY __attribute__ ((section ("sway")))

#define SortByNameA	0
#define SortByNameD	1
#define SortByDistance	2
#define SortByDistApt	3
#define SortByDistanceD	4
#define SortByLastUsed  5

/*****************************************************************************
 * protos
 *****************************************************************************/
Int16 waypt_comparison(WaypointData* rec1, WaypointData* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h) SWAY;
void refresh_waypoint_list(Int16 scr) SWAY;
void calc_dist_bear_aalt(Int16 index) SWAY;
void calc_dist_bear_aalt2(WaypointData *wayptData) SWAY;
void calc_alt(WaypointData *wayptData) SWAY;
double calc_altld(double distance, double elevation, double startalt, Boolean usesafealt) SWAY;
void calc_waypoint_list_values(Int16 page) SWAY;
void update_waypoint_list_values_event() SWAY;
Boolean save_waypt_fields() SWAY;
Boolean TempWptSave() SWAY;
void InuseWaypointCalcEvent() SWAY;

#endif
