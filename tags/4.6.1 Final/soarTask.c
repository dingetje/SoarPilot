#include <PalmOS.h>
#include "soaring.h"
#include "soarTask.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarUMap.h"
#include "soarIO.h"
#include "soarSTF.h"
#include "soarComp.h"
#include "soarWind.h"
#include "soarLog.h"
#include "soarWLst.h"
#include "soarWay.h"
#include "soarMath.h"
#include "soarMem.h"
#include "soarTer.h"
#include "soarSUA.h"

TaskData *tsk;
TaskData *edittsk;
TaskData *temptsk;
TaskData *tsktoedit;

Int16 	numOfTasks = 0;
Int16   taskIndex = 0;
Int16 	selectedTaskWayIdx = 0;
Int16	tskWayAddIdx = -1;
Int16	numWaypts;
Boolean addWayToTask = false;
Boolean dispactive = true;
Int16   activetskway = -1;
Int16   activectlpts = 0;
Int16	currentTaskPage = 0;
Int16   currentTaskListPage = 0;
Boolean minnumWayavail = false;
Int16  	minnumWaypts=2;

Boolean mustActivateAsNew = true;
Boolean tasknotfinished = true;
Boolean manualzoomchg = false;
double zoommapscale = 999.9;
Char start_utc[10] = "NT";
Char start_dtg[10 ]= "NT";
UInt32 start_utcsecs = 0;
double start_height = 0.0;

Boolean manchgwpt = false;
double mantskdist = 0.0;
UInt32 manstartsecs = 0;

UInt16 tasksortType = SortByNameA;
double sectorlat, sectorlon, sectorscale;
Boolean activetasksector = false;

double tskspdconst;
Boolean updatesectordists;
UInt32 abovestartheighttime = 0;
Boolean abovemaxstartheight = false;
Boolean startheightwarned1 = false;
Boolean startheightwarned2 = false;

Boolean starttooearly = false;
Boolean settaskreadonly = false;
Int8 settaskstats = STATSDIST;
Boolean exittaskreadonly = false;
Boolean settaskshowpct = false;
UInt32 warnedgotostart = 0;
Int16 TOff=0;
double taskctrlat, taskctrlon;
double taskmapscale;
Int16 inareasector = -1;
Int16 lastareasector = -1;
Boolean defaultrules = false;
Boolean starttask = false;
Int8 AATdist;
double speedtostart = 0.0;
double startrange, startbearing;
Char timeinfo[15] = "";
Boolean skipnewflight = false;
UInt8 glidingtoheight = ELEVATION;
Boolean set_task_changed = false;
Boolean goingtotaskedit = false;
Boolean goingtomenu = false;
Boolean gototskwpt = false;
Int16 requestedtskidx;
TaskSave tsksav;
Boolean tgtchg = false;

extern Boolean recv_data;
extern Boolean inflight;
extern double  MCCurVal;
extern Boolean taskonhold;
extern UInt32  origform;
extern double  curmapscale;
extern double  actualmapscale;
extern IndexedColorType indexGreen, indexRed, indexSector, indexBlack, indexTask;
extern SUAAlertData *suaalert;
extern SUAAlertRet *suaalertret;
extern WaypointData *selectedWaypoint;
extern WaypointData *TempWpt;
extern Int16 selectedWaypointIndex;
extern Boolean menuopen;
extern Boolean saveqfe;
extern Boolean updatemap;
extern FlightData *fltdata;
extern Boolean allowgenalerttap;
extern UInt32 warning_time;
extern double *tskterheights;
extern Int32 prevnumtskter;
extern Int32 numtskter;
extern Boolean tskoffterrain;
extern Boolean tskcrash;
extern Boolean terrainvalid;
extern Int32  rxrecs;
extern char rxtype[15];
extern DateTimeType curtime;
extern Int16 activefltindex;
extern UInt32 origform2;
extern UInt32 cursecs;
extern UInt32 gpssecs;
extern UInt32 utcsecs;
extern Boolean draw_log;
extern double logmapscale;
extern Int8 logmapscaleidx;
extern Int8 logmaporient;
extern Boolean draw_task;
extern double calcstfhwind;
extern Int8 CompCmd;
extern UInt8 mapmode;
extern Boolean taskpreview;
extern Int16 defaultscreen;
extern Boolean savtaskonhold;
extern Boolean ManualDistChg;
extern Char buf[PARSELEN];

void refresh_task_details(Int16 action)
{
	#define FIELDLEN 16

	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3, lst4;
	Int16 x, y, z;
	MemHandle mem_hand;
	MemPtr mem_ptr;
	static Char **items = NULL;
	static Char **bear = NULL;
	static Char **dist = NULL;
	static Char **waytype = NULL;
	static Int16 prevNumRecs = 0;
	// Did This Because of Stack Problems on PalmOS30
	WaypointData  tskWayAdd;
	Char tempchar[50];
	UInt16 AlertResp = 0;
	Int16 start=0, end=0, nrecs=0;
	Boolean prevTLvalue = false;
	Int8 toofewresp = true;
	Int16 TAKEOFFSET=0, LANDOFFSET=0;
	Int16 namelen, findincr;
	Int16 hrs, mins, hrs1, mins1;
	Char tempname[13];
	Boolean isAAT = false;
//	UInt32 starttime;
	UInt32 tempsecs;
	double legspeed;
	minnumWaypts = 2;
	
	if (action == TASKFREE) {
		// Free up each of the previous strings and then free up
		// the array of pointers itself.
		for (x = 0; x < prevNumRecs; x++) {
			MemPtrFree(items[x]);
			MemPtrFree(bear[x]);
			MemPtrFree(dist[x]);
			MemPtrFree(waytype[x]);
		}
		if (items) {
			MemPtrFree(items);
			items = NULL;
		}
		if (bear) {
			MemPtrFree(bear);
			bear = NULL;
		}
		if (dist) {
			MemPtrFree(dist);
			dist = NULL;
		}
		if (waytype) {
			MemPtrFree(waytype);
			waytype = NULL;
		}
		prevNumRecs = 0;
		return;
	}

	if (!settaskreadonly) {
		// Get the number of tasks in the database
		MemSet(tsk,sizeof(TaskData),0);
		numOfTasks = DmNumRecords(task_db);

		// Get number of waypoints for the given task before any changes
		OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
		MemMove(tsk, mem_ptr, sizeof(TaskData));
		MemHandleUnlock(mem_hand);
	}

	numWaypts = tsk->numwaypts;
	if (tsk->hastakeoff) {
		minnumWaypts++;
		TAKEOFFSET = 1;
	} else {
		TAKEOFFSET = 0;
	}
	if (tsk->haslanding) {
		minnumWaypts++;
		LANDOFFSET = numWaypts - 1;
	} else {
		LANDOFFSET = numWaypts;
	}
	if (numWaypts >= minnumWaypts) {
		minnumWayavail = true;
	} else {
		minnumWayavail = false;
	}

	// If at the bottom of a task, go to the next task
	if ((action == TASKPGDWN) && (selectedTaskWayIdx == numWaypts-1)) {
	// if not more pages of current task go to next task
//	if ((action == TASKPGDWN) && ((int)((selectedTaskWayIdx+7)/7) > (int)(numWaypts/7))) {
		if (!settaskreadonly) action = TASKNXT;
	}
	// If at the top of a task, go to the previous task
	if ((action == TASKPGUP) && (selectedTaskWayIdx <= 0)) {
	// if on the first 7 waypoints of the task go to the previous task
//	if ((action == TASKPGUP) && (selectedTaskWayIdx < 7)) {
		if (!settaskreadonly) action = TASKPRV;
	}

//	HostTraceOutputTL(appErrorClass, "action =|%hd|", action);
	switch (action) {
		case TASKDISP:
			// display the current active Task / waypoint?
			if (dispactive) {
				//taskIndex = 0;
				if (activetskway >= 0) {
					selectedTaskWayIdx = activetskway;
				}
			}
			break;
		case TASKACTV:
//			HostTraceOutputTL(appErrorClass, "Activate Task");
			// Activate the current task making it the Active task
			// If it is the active task, then reactivate it

			// re-calculate task distances
			// don't re-calc distances or targets to allow for "rubber banding" in area waypoints
			// set by user in task planning stage before declaration
			CalcTaskDists(tsk, false, false, true);

			if (tsk->ttldist == 0.0) {
				if ((tsk->numwaypts == 0) &&
				   ((data.config.flightcomp == EWCOMP) ||
				    (data.config.flightcomp == FLARMCOMP) ||
				    (data.config.flightcomp == VOLKSCOMP ||
				    (data.config.flightcomp == B50VLKCOMP)))) {
					// clear declared task in supported loggers
					CompCmd = CLRQUESTION;
				} else {
					/// popup warning for zero distance task
					warning->type = Wgeneric;
					StrCopy(warning->line1, "Cannot Activate Task");
					StrCopy(warning->line2, "Distance is Zero");
					FrmPopupForm(form_warning);
				}
			} else {
				toofewresp = CheckForTooFew();
				if (toofewresp == TFOKUP) {
					taskonhold = false;
					savtaskonhold = false;
					if (taskIndex == 0) {
						if (mustActivateAsNew) {
							addWayToTask = false;
							mustActivateAsNew = false;
						} else {
							addWayToTask = true;
						}
					} else {
						taskIndex = 0;
						mustActivateAsNew = false;
					}

					// update task record
//					HostTraceOutputTL(appErrorClass, "Task Name: %s",tsk->name);
					StrCopy(tempname, tsk->name);
					StrCopy(tsk->name, "Active Task");
					StrCopy(data.task.name, tsk->name);
					CalcBisectors(tsk);
					CalcStartFinishDirs(tsk);
					set_task_changed = false;	// clear changed flag
					OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
//					HostTraceOutputTL(appErrorClass, "actv:taskIndex =|%hd|", taskIndex);

					// task declaration time
					if (recv_data) {
						StrCopy(data.activetask.declareutc, data.logger.gpsutc);
						StrCopy(data.activetask.declaredtg, data.logger.gpsdtg);
					} else {
						// Had to multiply timezone by -1 and apply to convert Palm time back to UTC
						StrCopy(data.activetask.declareutc, SecondsToDateOrTimeString(TimGetSeconds(), tempchar, 2, data.config.timezone*-1));
						StrCopy(data.activetask.declaredtg, SecondsToDateOrTimeString(TimGetSeconds(), tempchar, 4, data.config.timezone*-1));
					}

					// load terrain on task
					tskoffterrain = !loadtskterrain(tsk);

					// copy task
//					HostTraceOutputTL(appErrorClass, "Copy to data.task");
					data.task.numwaypts = tsk->numwaypts;
					data.task.numctlpts = tsk->numctlpts;
 					data.task.hastakeoff = tsk->hastakeoff;
					data.task.haslanding = tsk->haslanding;
					data.task.ttldist = tsk->ttldist;
					data.task.aataimtype = tsk->aataimtype;
					// task waypoints
					for (x = 0; x < (Int16)tsk->numwaypts; x++) {
						StrCopy(data.task.wayptnames[x], tsk->wayptnames[x]);
						z = FindWayptRecordByName(tsk->wayptnames[x]);
						StrCopy(data.task.remarks[x], tsk->remarks[x]);
						data.task.wayptidxs[x] = z;
						data.task.wayptlats[x] = tsk->wayptlats[x];
						data.task.wayptlons[x] = tsk->wayptlons[x];
						data.task.elevations[x] = tsk->elevations[x];
						data.task.distances[x] = tsk->distances[x];
						data.task.bearings[x] = tsk->bearings[x];
						data.task.sectbear1[x] = tsk->sectbear1[x];
						data.task.sectbear2[x] = tsk->sectbear2[x];
						data.task.arearadii[x] = tsk->arearadii[x];
						data.task.arearadii2[x] = tsk->arearadii2[x];
						data.task.waypttypes[x] = tsk->waypttypes[x];
						data.task.targetlats[x] = tsk->targetlats[x];
						data.task.targetlons[x] = tsk->targetlons[x];
						data.task.distlats[x] = tsk->distlats[x];
						data.task.distlons[x] = tsk->distlons[x];
						data.task.terrainidx[x] = tsk->terrainidx[x];
						if (z > -1) {
							// update last used time stamp
							OpenDBQueryRecord(waypoint_db, z, &mem_hand, &mem_ptr);
							MemMove(&tskWayAdd, mem_ptr, sizeof(WaypointData));
							MemHandleUnlock(mem_hand);
							// replaced Palm function to handle months/days < 10 correctly i.e. "09" not "9"
							TimSecondsToDateTime(cursecs, &curtime);
							StrCopy(tskWayAdd.UsedTime, leftpad(DblToStr(curtime.year,0), '0', 2));
							StrCat( tskWayAdd.UsedTime, "/");
							StrCat( tskWayAdd.UsedTime, leftpad(DblToStr(curtime.month,0), '0', 2));
							StrCat( tskWayAdd.UsedTime, "/");
							StrCat( tskWayAdd.UsedTime, leftpad(DblToStr(curtime.day,0), '0', 2));
							OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), &tskWayAdd, z);
//							HostTraceOutputTL(appErrorClass, "actv:%s",data.task.wayptnames[x]);
						}
					}
					// copy task rules
					data.task.rulesactive = tsk->rulesactive;
					data.task.startwarnheight = tsk->startwarnheight;
					data.task.maxstartheight = tsk->maxstartheight;
					data.task.timebelowstart = tsk->timebelowstart;
					data.task.mintasktime = tsk->mintasktime;
					data.task.finishheight = tsk->finishheight;
					data.task.fgto1000m = tsk->fgto1000m;
					data.task.startaltref = tsk->startaltref;
					data.task.finishaltref = tsk->finishaltref;
					data.task.startlocaltime = tsk->startlocaltime;
					data.task.startonentry = tsk->startonentry;
					data.task.warnbeforestart = tsk->warnbeforestart;
					data.task.inclcyldist = tsk->inclcyldist;
					// reset task rules flags
					startheightwarned1 = false;
					startheightwarned2 = false;
					abovemaxstartheight = false;
					starttooearly = false;
					warnedgotostart = 0;
					timeinfo[0] = '\0';
					data.activetask.FGstatus = BELOW_FG;

					// reset distances in area waypoints
					for (x = 0; x < (Int16)data.task.numwaypts; x++) {
						if (data.task.waypttypes[x] & AREA) {
							data.activetask.maxareadist[x] = -999.9;
							data.activetask.distlats[x] = data.task.targetlats[x];
							data.activetask.distlons[x] = data.task.targetlons[x];
							data.task.distlats[x] = data.task.targetlats[x];
							data.task.distlons[x] = data.task.targetlons[x];
						}
					}
					data.activetask.arearng1 = -999.9;
					data.activetask.arearng2 = -999.9;
					data.activetask.AATtgtdist = 0.0;
					inareasector = -1;
					lastareasector = -1;
					if (data.config.AATmode == AAT_MTURN_ON) data.config.AATmode = AAT_MTURN;

					// calculate total height needed for task
					data.activetask.FGAalt = CalcRemainingTskFGA(&data.task, TAKEOFFSET, data.input.inusealt, MCCurVal);

					// calculate start height in MSL or AGL (above start waypoint elevation)
					data.activetask.startheight = calcstartheight();
					data.activetask.finishheight = calcfinishheight();

					// activate / re-activate the task
					if (addWayToTask && ((data.activetask.tskstartsecs > 0))) {
//						HostTraceOutputTL(appErrorClass, "actv:TSKREACTIVATE");
						HandleTask(TSKREACTIVATE);
					} else {
//						HostTraceOutputTL(appErrorClass, "actv:TSKNEW");
						manchgwpt = false;
						mantskdist = 0.0;
						manstartsecs = 0;
						// save task data from flight
						StrCopy(tsksav.tskstartutc, data.flight.tskstartutc);
						StrCopy(tsksav.tskstartdtg, data.flight.tskstartdtg);
						StrCopy(tsksav.tskstoputc, data.flight.tskstoputc);
						StrCopy(tsksav.tskstopdtg, data.flight.tskstopdtg);
						tsksav.tskdist = data.flight.tskdist;
						HandleTask(TSKNEW);
					}
					addWayToTask = false;

					// declare to SP log file
					StrCopy(data.flight.flttask.name, tempname);
					if (!skipnewflight && !CopyTaskToFlight()) {
						// task already declared to this flight
						question->type = Qnewflight;
						FrmPopupForm(form_question);
					} else if (skipnewflight) {
						// update task rules
						CopyTaskRulesToFlight();
					}
					skipnewflight = false;
				}
			}
			break;
		case TASKNEW:
			// Create a new/empty Task and move the display to it
			x = TSKFOUND;
			findincr = 1;
			MemSet(tsk, sizeof(TaskData), 0);
			while (x!=TSKNOTFOUND) {
				StrCopy(tsk->name, "NewTsk");
				StrNCat(tsk->name, StrIToA(tempchar, findincr), sizeof(tsk->name));
				x = FindTaskRecordByName(tsk->name, &mem_hand, &mem_ptr);
				findincr++;
				if (findincr == 100) {
					x = TSKNOTFOUND; // force exit from loop, allows for 99 new tasks!!
				}
			}
			// assign default task rules from config file
			AssignDefaultRules(tsk);
			// Since all area targets default to center
			// set to aim type to AATctr
			tsk->aataimtype = AATctr;

			taskIndex = OpenDBAddRecord(task_db, sizeof(TaskData), tsk, false);
			numOfTasks = DmNumRecords(task_db);
//			HostTraceOutputTL(appErrorClass, "new:taskIndex =|%hd|", taskIndex);
			selectedTaskWayIdx = 0;
			minnumWaypts = 2;
			minnumWayavail = false;
			break;
		case TASKCOPY:
			// Copy the current task to a new task entry and move to it
			if ((CheckForTooFew() != TFCANCEL) && (numWaypts != 0)) {
				x = TSKFOUND;
				findincr = 1;
				while (x != TSKNOTFOUND) {
					namelen = StrLen(tsk->name);
//					HostTraceOutputTL(appErrorClass, "Full name=|%s|", tsk->name);
//					HostTraceOutputTL(appErrorClass, "Name Len=|%hd|", namelen);

					//check for active task name
					if (StrCompare(tsk->name,"Active Task") == 0) {
						StrCopy(tsk->name, "Active");
					}

					// cut short the name to allow a suffix to be added
					if (namelen > 9) {
						StrCopy(tsk->name,Mid(tsk->name,9,1));
					}

					// look for previous suffix
					if (StrCompare(Mid(tsk->name,1,namelen-1),"-") == 0) {
						StrCopy(tsk->name,Mid(tsk->name,namelen-1,1));

					} else if (StrCompare(Mid(tsk->name,1,namelen-2),"-") == 0) {
						StrCopy(tsk->name,Mid(tsk->name,namelen-2,1));
					} else {
						// assume no previous suffix
						StrCat(tsk->name, "-");
					}

					StrNCat(tsk->name, StrIToA(tempchar, findincr), sizeof(tsk->name));
					x = FindTaskRecordByName(tsk->name, &mem_hand, &mem_ptr);
					findincr++;

					if (findincr == 100) {
						x = TSKNOTFOUND; // force exit from loop, allows for 99 copies!!
					}
				}

				taskIndex = OpenDBAddRecord(task_db, sizeof(TaskData), tsk, false);
				numOfTasks = DmNumRecords(task_db);
				// resort task list and find copied task
				DmQuickSort(task_db, (DmComparF*)task_comparison, tasksortType);
				taskIndex = FindTaskRecordByName(tsk->name, &mem_hand, &mem_ptr);
//				HostTraceOutputTL(appErrorClass, "copy:taskIndex =|%hd|", taskIndex);
				selectedTaskWayIdx = 0;
			}
			break;
		case TASKCLRACT: //Clear the Active Task without opening the Alert Window
				taskIndex = 0;
				// Do not want a break here!
				if (data.task.numwaypts > 0) {
					// Clear the active task from final glide info
					HandleTask(TSKDEACTIVATE);
					data.task.numwaypts = 0;
					data.task.numctlpts = 0;
				}
				mustActivateAsNew = true;
				break;
		case TASKDEL:
			// Delete the active task from the Task db
			// If it is the active task, simply clear it
			if (taskIndex != 0) {
				question->type = Qdeltask;
				FrmPopupForm(form_question);
			} else {
				question->type = Qclrtask;
				FrmPopupForm(form_question);
			}
			break;
		case TASKDELALL:
			{
			// Delete All Tasks but the Active Task
			for (x=(numOfTasks-1); x>0; x--) {
					OpenDBDeleteRecord(task_db, x);
			}
			numOfTasks = DmNumRecords(task_db);
			taskIndex = 0;
			selectedTaskWayIdx = 0;
			}
			break;
		case TASKADDAFTER:
			// Add a new waypoint to the current task after the one that is highlighted
			if (tskWayAddIdx != -1) {
				MemSet(&tskWayAdd,sizeof(WaypointData),0);
				OpenDBQueryRecord(waypoint_db, tskWayAddIdx, &mem_hand, &mem_ptr);
				MemMove(&tskWayAdd, mem_ptr, sizeof(WaypointData));
				MemHandleUnlock(mem_hand);
				if (numWaypts == 0 || selectedTaskWayIdx == numWaypts-1) {
					// If the turnpoint list is empty or at the end of the list
					StrCopy(tsk->wayptnames[numWaypts], tskWayAdd.name);
					StrCopy(tsk->remarks[numWaypts], tskWayAdd.rmks);
					tsk->wayptidxs[numWaypts] = tskWayAddIdx;
					tsk->wayptlats[numWaypts] = tskWayAdd.lat;
					tsk->wayptlons[numWaypts] = tskWayAdd.lon;
					tsk->elevations[numWaypts] = tskWayAdd.elevation;
					tsk->waypttypes[numWaypts] = tskWayAdd.type;
					if (tskWayAdd.type & AREA) {
//						HostTraceOutputTL(appErrorClass, "Adding an Area Waypoint-numWaypts:|%hd|", numWaypts);
						tsk->sectbear1[numWaypts] = tskWayAdd.arearadial1;
						tsk->sectbear2[numWaypts] = tskWayAdd.arearadial2;
						tsk->arearadii[numWaypts] = tskWayAdd.arearadius;
						tsk->arearadii2[numWaypts] = tskWayAdd.arearadius2;
//						if (tsk->aataimtype != AATctr) {
//							HostTraceOutputTL(appErrorClass, "Setting aataimtype to AATusr");
//							tsk->aataimtype = AATusr;
//						}
					} else {
//						HostTraceOutputTL(appErrorClass, "Not an Area Waypoint-numWaypts:|%hd|", numWaypts);
						tsk->sectbear1[numWaypts] = 0;
						tsk->sectbear2[numWaypts] = 0;
						tsk->arearadii[numWaypts] = 0.0;
						tsk->arearadii2[numWaypts] = 0.0;
					}
					CalcWptDists(tsk, numWaypts, true, true);
				} else {
					for (x=numWaypts; x > selectedTaskWayIdx; x--) {
//						HostTraceOutputTL(appErrorClass, "Adding remainder of waypoints-x:|%hd|", x);
						MoveTaskWaypt(tsk, x, x-1);
					}
					StrCopy(tsk->wayptnames[selectedTaskWayIdx+1], tskWayAdd.name);
					StrCopy(tsk->remarks[selectedTaskWayIdx+1], tskWayAdd.rmks);
					tsk->wayptidxs[selectedTaskWayIdx+1] = tskWayAddIdx;
					tsk->wayptlats[selectedTaskWayIdx+1] = tskWayAdd.lat;
					tsk->wayptlons[selectedTaskWayIdx+1] = tskWayAdd.lon;
					tsk->elevations[selectedTaskWayIdx+1] = tskWayAdd.elevation;
					tsk->waypttypes[selectedTaskWayIdx+1] = tskWayAdd.type;
					if (tskWayAdd.type & AREA) {
//						HostTraceOutputTL(appErrorClass, "Adding an Area Waypoint2-selectedTaskWayIdx+1:|%hd|", selectedTaskWayIdx+1);
						tsk->sectbear1[selectedTaskWayIdx+1] = tskWayAdd.arearadial1;
						tsk->sectbear2[selectedTaskWayIdx+1] = tskWayAdd.arearadial2;
						tsk->arearadii[selectedTaskWayIdx+1] = tskWayAdd.arearadius;
						tsk->arearadii2[selectedTaskWayIdx+1] = tskWayAdd.arearadius2;
						if (tsk->aataimtype != AATctr) {
//							HostTraceOutputTL(appErrorClass, "Setting aataimtype to AATusr3");
							tsk->aataimtype = AATusr;
						}
					} else {
//						HostTraceOutputTL(appErrorClass, "Not an Area Waypoint2-selectedTaskWayIdx+1:|%hd|", selectedTaskWayIdx+1);
						tsk->sectbear1[selectedTaskWayIdx+1] = 0;
						tsk->sectbear2[selectedTaskWayIdx+1] = 0;
						tsk->arearadii[selectedTaskWayIdx+1] = 0.0;
						tsk->arearadii2[selectedTaskWayIdx+1] = 0.0;
					}
					CalcWptDists(tsk, selectedTaskWayIdx+1, true, true);
				}
//				HostTraceOutputTL(appErrorClass, "-------------------------------------");
				selectedTaskWayIdx++;
				numWaypts++;
				tsk->numwaypts = numWaypts;
				if (taskIndex == 0) set_task_changed = true;
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			}
			break;
		case TASKADDBEFORE:
			// Add a new waypoint to the current task before the one that is highlighted
			if (tskWayAddIdx != -1) {
//				HostTraceOutputTL(appErrorClass, "TASKADDBEFORE-selectedTaskWayIdx:|%hd|", selectedTaskWayIdx);
				MemSet(&tskWayAdd,sizeof(WaypointData),0);
				OpenDBQueryRecord(waypoint_db, tskWayAddIdx, &mem_hand, &mem_ptr);
				MemMove(&tskWayAdd, mem_ptr, sizeof(WaypointData));
				MemHandleUnlock(mem_hand);
				for (x=numWaypts; x > selectedTaskWayIdx; x--) {
					MoveTaskWaypt(tsk, x, x-1);
				}
				StrCopy(tsk->wayptnames[selectedTaskWayIdx], tskWayAdd.name);
				StrCopy(tsk->remarks[selectedTaskWayIdx], tskWayAdd.rmks);
				tsk->wayptidxs[selectedTaskWayIdx] = tskWayAddIdx;
				tsk->wayptlats[selectedTaskWayIdx] = tskWayAdd.lat;
				tsk->wayptlons[selectedTaskWayIdx] = tskWayAdd.lon;
				tsk->elevations[selectedTaskWayIdx] = tskWayAdd.elevation;
				tsk->waypttypes[selectedTaskWayIdx] = tskWayAdd.type;
				if (tskWayAdd.type & AREA) {
//					HostTraceOutputTL(appErrorClass, "Adding an Area Waypoint3");
					tsk->sectbear1[selectedTaskWayIdx] = tskWayAdd.arearadial1;
					tsk->sectbear2[selectedTaskWayIdx] = tskWayAdd.arearadial2;
					tsk->arearadii[selectedTaskWayIdx] = tskWayAdd.arearadius;
					tsk->arearadii2[selectedTaskWayIdx] = tskWayAdd.arearadius2;
					if (tsk->aataimtype != AATctr) {
//						HostTraceOutputTL(appErrorClass, "Setting aataimtype to AATusr");
						tsk->aataimtype = AATusr;
					}
				} else {
//					HostTraceOutputTL(appErrorClass, "Not an Area Waypoint3");
					tsk->sectbear1[selectedTaskWayIdx] = 0;
					tsk->sectbear2[selectedTaskWayIdx] = 0;
					tsk->arearadii[selectedTaskWayIdx] = 0.0;
					tsk->arearadii2[selectedTaskWayIdx] = 0.0;
				}
				CalcWptDists(tsk, selectedTaskWayIdx, true, true);
				numWaypts++;
				tsk->numwaypts = numWaypts;
				if (taskIndex == 0) set_task_changed = true;
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			}
			break;
		case TASKUP:
			// Move up one entry in the waypoint list of the current task
			if (selectedTaskWayIdx != 0) {
				selectedTaskWayIdx--;
//				HostTraceOutputTL(appErrorClass, "up:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			}
			break;
		case TASKDWN:
			// Move down one entry in the waypoint list of the current task
			if (selectedTaskWayIdx < (numWaypts-1)) {
				selectedTaskWayIdx++;
//				HostTraceOutputTL(appErrorClass, "dwn:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			}
			break;
		case TASKPGUP:
			// Move up one page (7 lines) of entries in the waypoint list of the current task
			StrCopy(tsk->name, NoComma(field_get_str(form_set_task_name)," "));
			if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			if (selectedTaskWayIdx >= 7) {
				// Move up seven lines
				selectedTaskWayIdx -= 7;
//				HostTraceOutputTL(appErrorClass, "up:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			} else {
				// Move to first item
				selectedTaskWayIdx = 0;
			}
			break;
		case TASKPGDWN:
			// Move down one page (7 lines) of entries in the waypoint list of the current task
			StrCopy(tsk->name, NoComma(field_get_str(form_set_task_name)," "));
			if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			if ((selectedTaskWayIdx+7) < numWaypts) {
				// Move down seven lines
				selectedTaskWayIdx += 7;
			} else {
				// Move to last item
				selectedTaskWayIdx = numWaypts-1;
			}
//			HostTraceOutputTL(appErrorClass, "dwn:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			break;
		case TASKREM:
			// Remove the currently selected waypoint from the current task
//			HostTraceOutputTL(appErrorClass, "rem:numwpts =           |%hd|", numWaypts);
//			HostTraceOutputTL(appErrorClass, "rem:activetskway =      |%hd|", activetskway);
//			HostTraceOutputTL(appErrorClass, "rem:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
//			HostTraceOutputTL(appErrorClass, "rem:taskIndex =         |%hd|", taskIndex);
			if ((taskIndex == 0) && ((selectedTaskWayIdx < (inareasector!=-1?inareasector:activetskway)) || (activetskway >= numWaypts-1)) && (numWaypts >= 1)) {
				warning->type = Wgeneric;
				StrCopy(warning->line1, "Cannot Delete Active or");
				StrCopy(warning->line2, "Accomplished Waypoints");
				FrmPopupForm(form_warning);
				break;
			}
			if (selectedTaskWayIdx != -1) {
				AlertResp = OK; // question here for delete waypoint confirmation
				if (AlertResp == OK) {
					if (selectedTaskWayIdx != (numWaypts-1)) {
						for (x=selectedTaskWayIdx+1; x < numWaypts; x++) {
							MoveTaskWaypt(tsk, x-1, x);
						}
					}
					numWaypts--;
					tsk->numwaypts = numWaypts;
					if (taskIndex == 0) set_task_changed = true;
					OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
					selectedTaskWayIdx--;
					if (selectedTaskWayIdx < 0) {
						selectedTaskWayIdx = 0;
					}
					if (selectedTaskWayIdx == numWaypts) {
						selectedTaskWayIdx = numWaypts - 1;
					}
					if (taskIndex == 0) {
						if (numWaypts == 0) {
							mustActivateAsNew = true;
//							HostTraceOutputTL(appErrorClass, "rem:TSKDEACTIVATE");
							HandleTask(TSKDEACTIVATE);
						} else {
//							HostTraceOutputTL(appErrorClass, "rem:TSKREMWAY");
							HandleTask(TSKREMWAY);
						}
					}
//					HostTraceOutputTL(appErrorClass, "rem:numWaypts =|%hd|", numWaypts);
//					HostTraceOutputTL(appErrorClass, "rem:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
				}
			}
			break;
		case TASKPRV:
			// Move to the previous task
			if (CheckForTooFew() != TFCANCEL) {
				if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
					OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				}
				if (taskIndex > 0) {
					taskIndex--;
				} else {
					taskIndex = numOfTasks-1;
				}
				selectedTaskWayIdx = 0;
//				HostTraceOutputTL(appErrorClass, "prv:taskIndex =|%hd|", taskIndex);
			}
			break;
		case TASKNXT:
			// Move to the next task
			if (CheckForTooFew() != TFCANCEL) {
				if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
					OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				}
				if (taskIndex < (numOfTasks-1)) {
					taskIndex++;
//					HostTraceOutputTL(appErrorClass, "nxt:taskIndex =|%hd|", taskIndex);
				} else {
					taskIndex = 0;
				}
				selectedTaskWayIdx = 0;
			}
			break;
		case TASKCLOSEDEL:
			// Performed when the task window is closed the current info should be discarded
			if (taskIndex != 0) {
//				HostTraceOutputTL(appErrorClass, "closedel:taskIndex=|%hd|", taskIndex);
				if (!settaskreadonly) OpenDBDeleteRecord(task_db, taskIndex);
				numOfTasks = DmNumRecords(task_db);
				// Setting taskIndex = 0 ensures that all actions outside of the Task Editor
				// are executed against the Active Task slot.
				taskIndex = 0;
				selectedTaskWayIdx = 0;
			} else {
//				HostTraceOutputTL(appErrorClass, "closedel:taskIndex=|%hd|", taskIndex);
				numWaypts = 0;
				tsk->numwaypts = numWaypts;
				if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				// Clear the active task from final glide info
				data.task.numwaypts = tsk->numwaypts;
				data.task.numctlpts = tsk->numctlpts;
				mustActivateAsNew = true;
//				HostTraceOutputTL(appErrorClass, "Calling TSKDEACTIVATE");
				HandleTask(TSKDEACTIVATE);
			}
			break;
		case TASKCLOSEUPDATE:
//			HostTraceOutputTL(appErrorClass, "closeupdate:taskIndex=|%hd|", taskIndex);
			StrCopy(tsk->name, NoComma(field_get_str(form_set_task_name)," "));
			if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			break;
		case TASKMVUP:
			// Switch the current entry with the one above
			if ((taskIndex == 0) && (selectedTaskWayIdx <= activetskway)) {
				warning->type = Wgeneric;
				StrCopy(warning->line1, "Cannot Move Before");
				StrCopy(warning->line2, "Active Waypoint");
				FrmPopupForm(form_warning);
				break;
			}
			if (selectedTaskWayIdx > 0) {
				x = selectedTaskWayIdx;
				MoveTaskWaypt(tsk, TSKTEMPSLOT, x-1);
				MoveTaskWaypt(tsk, x-1, x);
				MoveTaskWaypt(tsk, x, TSKTEMPSLOT);
				if (taskIndex == 0) set_task_changed = true;
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				selectedTaskWayIdx--;
//				HostTraceOutputTL(appErrorClass, "mvup:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			}
			break;
		case TASKMVDWN:
			// Switch the current entry with the one below
			if (selectedTaskWayIdx < numWaypts-1) {
				x = selectedTaskWayIdx;
				MoveTaskWaypt(tsk, TSKTEMPSLOT, x+1);
				MoveTaskWaypt(tsk, x+1, x);
				MoveTaskWaypt(tsk, x, TSKTEMPSLOT);
				if (taskIndex == 0) set_task_changed = true;
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				selectedTaskWayIdx++;
//				HostTraceOutputTL(appErrorClass, "mvup:selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
			}
			break;
		case TASKREV:
			if (taskIndex == 0 && mustActivateAsNew == false) {
				warning->type = Wgeneric;
				StrCopy(warning->line1, "Cannot Reverse An");
				StrCopy(warning->line2, "Active Task");
				FrmPopupForm(form_warning);
				break;
			}
			// Reverse the order of the current task
			if (numWaypts > 1) {
				for (x=0; x < (Int16)(numWaypts/2); x++) {
					MoveTaskWaypt(tsk, TSKTEMPSLOT, x);
					MoveTaskWaypt(tsk, x, numWaypts-1-x);
					MoveTaskWaypt(tsk, numWaypts-1-x, TSKTEMPSLOT);
				}
				if (taskIndex == 0) set_task_changed = true;
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			}
			break;
		case TASKSAVETL:
			// Save the current takeoff and landing pushbutton values
			prevTLvalue = tsk->hastakeoff;
			tsk->hastakeoff = ctl_get_value(form_set_task_hastakeoff);
			if (tsk->hastakeoff && !prevTLvalue) {
				 minnumWaypts++;
			} else if (!tsk->hastakeoff && prevTLvalue && (minnumWaypts > 2)) {
				 minnumWaypts--;
			}
			prevTLvalue = tsk->haslanding;
			tsk->haslanding = ctl_get_value(form_set_task_haslanding);
			if (tsk->haslanding && !prevTLvalue) {
				 minnumWaypts++;
			} else if (!tsk->haslanding && prevTLvalue && (minnumWaypts > 2)) {
				 minnumWaypts--;
			}
			// Ensure we have enough turnpoints before calculating the Start & Finish Dirs
			if (numWaypts >= minnumWaypts) {
				minnumWayavail = true;
				CalcTaskDists(tsk, false, false, false);
				CalcBisectors(tsk);
				CalcStartFinishDirs(tsk);
			} else {
				
				minnumWayavail = false;
			}
			if (taskIndex == 0) set_task_changed = true;
			OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
			break;
		case TASKSETWPT:
			manchgwpt = true;
			taskonhold = false;
			savtaskonhold = false;
			if (data.activetask.tskstartsecs == 0) {
				// force manual start
				HandleTask(TSKFORCESTART);
			}
			// set active waypoint
			activetskway = requestedtskidx;
			lastareasector = -1;
			for (x=0; x<requestedtskidx; x++) {
				data.flight.invalidturn[x] = true;
				data.flight.timeatturn[x] = utcsecs;
				if (data.task.waypttypes[x] & AREAEXIT) lastareasector = x;
			}
			CheckTurn();
			HandleTaskAutoZoom(0.0, 0.0, true);
			break;
		default:
			break;
	}

	// Update the screen
	if (action != TASKCLOSEDEL && action != TASKCLOSEUPDATE && action != TASKCLRACT && action != TASKUPNUM
	&& (FrmGetActiveFormID() == form_set_task)) {
		if (!settaskreadonly) {
			// Get number of waypoints for the given task after any changes
			OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
			MemMove(tsk,mem_ptr,sizeof(TaskData));
			MemHandleUnlock(mem_hand);
		}

		numWaypts = tsk->numwaypts;
		minnumWaypts = 2;
		if (tsk->hastakeoff) {
			minnumWaypts++;
			TAKEOFFSET = 1;
		} else {
			TAKEOFFSET = 0;
		}
		if (tsk->haslanding) {
			minnumWaypts++;
			LANDOFFSET = numWaypts - 1;
		} else {
			LANDOFFSET = numWaypts;
		}
		
		if (!settaskreadonly) {
			if (taskIndex == 0 && mustActivateAsNew == false) {
				//Activate Button Should Say "REACT"
				ctl_set_label(form_set_task_actvbtn, "REACT");
			} else {
				//Activate Button Should Say "ACT"
				ctl_set_label(form_set_task_actvbtn, "ACT");
			}
		}

		// Get the List pointer
		lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list));
		lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list2));
		lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list3));
		lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list4));

		// Free up each of the previous strings and then free up
		// the array of pointers itself.
		for (x = 0; x < prevNumRecs; x++) {
			MemPtrFree(items[x]);
			MemPtrFree(bear[x]);
			MemPtrFree(dist[x]);
			MemPtrFree(waytype[x]);
		}
		if (items) {
			MemPtrFree(items);
			items = NULL;
		}
		if (bear) {
			MemPtrFree(bear);
			bear = NULL;
		}
		if (dist) {
			MemPtrFree(dist);
			dist = NULL;
		}
		if (waytype) {
			MemPtrFree(waytype);
			waytype = NULL;
		}

		field_set_enable(form_set_task_name, true);
		StrCopy(tsk->name, NoComma(tsk->name," "));
		field_set_value(form_set_task_name, tsk->name);
		if (taskIndex == 0) {
			field_set_enable(form_set_task_name, false);
		}

		if (numWaypts > 0) {
			// Compute page to display based on selectedTaskWayIdx;
			currentTaskPage = (Int16)(selectedTaskWayIdx / 7);

			// Compute start and end indexes of the page to display
			start = currentTaskPage * 7;
			end = ((start + 7) > numWaypts) ? numWaypts : (start + 7);
			nrecs = end - start;
			y = 0;
			
			// We got at least one record so allocate enough
			// memory to hold nrecs pointers-to-pointers
			items =   (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			bear =    (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			dist =    (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			waytype = (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// update task distances and bearings
			setAATdist(tsk->aataimtype);
			CalcTaskDists(tsk, false, false, false);
			CalcBisectors(tsk);
			CalcStartFinishDirs(tsk);
			if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);

			switch (settaskstats) {
				case STATSDIST:
					// Display distances and bearings
					for (x = 0; x < numWaypts; x++) {
						if (x >= start && x < end) {
							items[y] =   (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							bear[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							dist[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							waytype[y] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							MemSet(items[y],FIELDLEN,0);
							MemSet(bear[y],FIELDLEN,0);
							MemSet(dist[y],FIELDLEN,0);
							MemSet(waytype[y],FIELDLEN,0);
							StrNCopy(items[y], tsk->wayptnames[x], 12);
							if (x == 0) {
								StrCopy(bear[y], leftpad("-", '\031', 4));
								StrCopy(dist[y], leftpad("-", '\031', 5));
								// check for start of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else {
									StrCopy(waytype[y], " ");
								}

							} else {
								StrCopy(bear[y], print_direction2(tsk->bearings[x]));
								StrCat(bear[y], "°");

								if (!settaskshowpct) {
									// show distances
//									HostTraceOutputTL(appErrorClass, "Distance %s", DblToStr(tsk->distances[x]*data.input.disconst,5));
									if (tsk->distances[x] <= 0.0) {
										StrCopy(dist[y],leftpad("-", '\031', 5));
										StrCopy(bear[y],leftpad("-", '\031', 4));
									} else if ((tsk->distances[x]*data.input.disconst) < 10.0) {
										StrCopy(dist[y], DblToStr(tsk->distances[x]*data.input.disconst, 1));
										StrCopy(dist[y], leftpad(dist[y], '\031', 5));
									} else if ((tsk->distances[x]*data.input.disconst) < 1000.0) {
										StrCopy(dist[y], DblToStr(tsk->distances[x]*data.input.disconst, 1));
										StrCopy(dist[y], leftpad(dist[y], '\031', 5));
									} else if ((tsk->distances[x]*data.input.disconst) < 10000.0) {
										StrCopy(dist[y], leftpad(DblToStr(tsk->distances[x]*data.input.disconst, 0), ' ',5));
									} else if ((tsk->distances[x]*data.input.disconst) < 100000.0) {
										StrCopy(dist[y], DblToStr(tsk->distances[x]*data.input.disconst, 0));
									} else {
										StrCopy(dist[y], "99999");
									}
								} else {
									// show percentages
									if (tsk->distances[x]/tsk->ttldist*100 == 100.0) {
										StrCopy(dist[y], "100%");
									} else {
										StrCopy(dist[y], DblToStr(pround(tsk->distances[x]/tsk->ttldist*100.0, 1), 1));
										StrCat(dist[y], "%");
										StrCopy(dist[y], leftpad(dist[y], '\031', 5));
									}
								}

								// check for start and finish of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else if (x == LANDOFFSET-1) {
									StrCopy(waytype[y], "F");
								} else if (x == LANDOFFSET) {
									StrCopy(waytype[y], " ");
								// check for type of task waypoint
								} else if (tsk->waypttypes[x] & CONTROL) {
									StrCopy(waytype[y], "C");
								} else if (tsk->waypttypes[x] & AREA) {
									StrCopy(waytype[y], "A");
								} else {
									StrCopy(waytype[y], "T");
								}
							}
							// next waypoint
							y++;
						}
						// check for area task
						if (tsk->waypttypes[x] & AREA) {
						 	isAAT = true;
						}
					}
					// total task distance
					if ((tsk->ttldist*data.input.disconst) < 10.0) {
						field_set_value(form_set_task_ttldist, DblToStr(tsk->ttldist*data.input.disconst, 1));
					} else if ((tsk->ttldist*data.input.disconst) < 1000.0) {
						field_set_value(form_set_task_ttldist, DblToStr(tsk->ttldist*data.input.disconst, 1));
					} else {
						field_set_value(form_set_task_ttldist, DblToStr(tsk->ttldist*data.input.disconst, 0));
					}
					field_set_value(form_set_task_units, data.input.distext);
					break;
				case STATSSPEED:
					// Display speeds
//					starttime = fltdata->timeatturn[TAKEOFFSET];
					for (x = 0; x < numWaypts; x++) {
						if (x >= start && x < end) {
							items[y] =   (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							bear[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							dist[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							waytype[y] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							MemSet(items[y],FIELDLEN,0);
							MemSet(bear[y],FIELDLEN,0);
							MemSet(dist[y],FIELDLEN,0);
							MemSet(waytype[y],FIELDLEN,0);
							StrNCopy(items[y], tsk->wayptnames[x], 12);
							if (x == 0) {
								StrCopy(bear[y], leftpad("-", '\031', 4));
								StrCopy(dist[y], leftpad("-", '\031', 5));
								// check for start of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else {
									StrCopy(waytype[y], " ");
								}

							} else {
								z = x-1;
								while (tsk->waypttypes[z] & CONTROL) z--;
								if ((fltdata->timeatturn[x] > 0) && !fltdata->invalidturn[x]) {
									// task leg speed
									legspeed = tsk->distances[x]/(double)(fltdata->timeatturn[x]-fltdata->timeatturn[z])*3600.0;
									if (legspeed <= 0.0) {
										StrCopy(dist[y],leftpad("-", '\031', 5));
										StrCopy(bear[y],leftpad("-", '\031', 4));
									} else if ((legspeed*data.input.tskspdconst) < 10.0) {
										StrCopy(dist[y], DblToStr(legspeed*data.input.tskspdconst, 1));
										StrCopy(dist[y], leftpad(dist[y], '\031', 5));
									} else if ((legspeed*data.input.tskspdconst) < 1000.0) {
										StrCopy(dist[y], DblToStr(legspeed*data.input.tskspdconst, 1));
										StrCopy(dist[y], leftpad(dist[y], '\031', 5));
									} else {
										StrCopy(dist[y], "99999");
									}
								} else {
									StrCopy(dist[y], leftpad("-", '\31', 5));
								}
								// check for start and finish of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else if (x == LANDOFFSET-1) {
									StrCopy(waytype[y], "F");
								} else if (x == LANDOFFSET) {
									StrCopy(waytype[y], " ");
								// check for type of task waypoint
								} else if (tsk->waypttypes[x] & CONTROL) {
									StrCopy(waytype[y], "C");
								} else if (tsk->waypttypes[x] & AREA) {
									StrCopy(waytype[y], "A");
								} else {
									StrCopy(waytype[y], "T");
								}
							}
							// check reached points and for invalidturn i.e. manually advanced to next waypoint
							if (fltdata->invalidturn[x] && !(tsk->waypttypes[x] & CONTROL)) {
								StrCopy(bear[y], "Man");
							} else if (fltdata->timeatturn[x] > 0) {
								StrCopy(bear[y], "  OK");
							} else {
								StrCopy(bear[y], leftpad("-", '\031', 4));
							}
							// next waypoint
							y++;
						}
					}
					// overall task speed
					field_set_value(form_set_task_ttldist, DblToStr(pround(fltdata->tskspeed*data.input.tskspdconst,1),1));
					field_set_value(form_set_task_units, data.input.tskspdtext);
					break;
				case STATSTIME:
					// Display times
//					starttime = fltdata->timeatturn[TAKEOFFSET];
					for (x = 0; x < numWaypts; x++) {
						if (x >= start && x < end) {
							items[y] =   (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							bear[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							dist[y] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							waytype[y] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
							MemSet(items[y],FIELDLEN,0);
							MemSet(bear[y],FIELDLEN,0);
							MemSet(dist[y],FIELDLEN,0);
							MemSet(waytype[y],FIELDLEN,0);
							StrNCopy(items[y], tsk->wayptnames[x], 12);
							if (x == 0) {
								StrCopy(bear[y], leftpad("-", '\031', 4));
								if (fltdata->timeatturn[x] > 0) {
									// time at turn
									tempsecs = fltdata->timeatturn[x] + data.config.timezone*3600; 
									hrs = (tempsecs%86400)/3600;
									mins = (tempsecs%3600)/60;
									StrCopy(dist[y], DblToStr(hrs,0));
									StrCat(dist[y], ":");
									StrCat(dist[y], leftpad(DblToStr(mins,0),'0',2));
								} else {
									StrCopy(dist[y], leftpad("-", '\031', 5));
								}
								// check for start of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else {
									StrCopy(waytype[y], " ");
								}
							} else {
								z = x-1;
								while (tsk->waypttypes[z] & CONTROL) z--;
								if (fltdata->timeatturn[x] > 0 ) {
									// time at turn
									tempsecs = fltdata->timeatturn[x] + data.config.timezone*3600; 
									hrs = (tempsecs%86400)/3600;
									mins = (tempsecs%3600)/60;
									StrCopy(dist[y], DblToStr(hrs,0));
									StrCat(dist[y], ":");
									StrCat(dist[y], leftpad(DblToStr(mins,0),'0',2));
									// task leg time mins
									tempsecs = fltdata->timeatturn[z] + data.config.timezone*3600; 
									hrs1 = (tempsecs%86400)/3600;
									mins1 = (tempsecs%3600)/60;
									legspeed = hrs*60+mins - (hrs1*60+mins1);
									if (legspeed <= 999) {
										StrCopy(bear[y], leftpad(DblToStr(legspeed,0), '\031', 3));
									} else {
										StrCopy(bear[y], "999");
									}
								} else {
									StrCopy(bear[y], leftpad("-", '\031', 4));
									StrCopy(dist[y], leftpad("-", '\031', 5));
								}
								// check for start and finish of task
								if (x == TAKEOFFSET) {
									StrCopy(waytype[y], "S");
								} else if (x == LANDOFFSET-1) {
									StrCopy(waytype[y], "F");
								} else if (x == LANDOFFSET) {
									StrCopy(waytype[y], " ");
								// check for type of task waypoint
								} else if (tsk->waypttypes[x] & CONTROL) {
									StrCopy(waytype[y], "C");
								} else if (tsk->waypttypes[x] & AREA) {
									StrCopy(waytype[y], "A");
								} else {
									StrCopy(waytype[y], "T");
								}
							}
							y++;
						}
					}
					// overall task time
					field_set_value(form_set_task_ttldist, " ");
					field_set_value(form_set_task_units, "mins");

					break;
				default:
					break;
			}
			
			// Reform the list
			LstSetListChoices(lst, items, nrecs);
			LstSetListChoices(lst2, bear, nrecs);
			LstSetListChoices(lst3, dist, nrecs);
			LstSetListChoices(lst4, waytype, nrecs);

			//Update the takeoff and landing pushbuttons
			ctl_set_value(form_set_task_hastakeoff, tsk->hastakeoff);
			ctl_set_value(form_set_task_haslanding, tsk->haslanding);
		} else {
			items =   (Char **) MemPtrNew(1 * (sizeof(Char *)));
			bear =    (Char **) MemPtrNew(1 * (sizeof(Char *)));
			dist =    (Char **) MemPtrNew(1 * (sizeof(Char *)));
			waytype = (Char **) MemPtrNew(1 * (sizeof(Char *)));
			prevNumRecs = 1;
			items[0] =   (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			bear[0] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			dist[0] =    (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			waytype[0] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			MemSet(items[0],FIELDLEN,0);
			MemSet(bear[0],FIELDLEN,0);
			MemSet(dist[0],FIELDLEN,0);
			MemSet(waytype[0],FIELDLEN,0);
			StrNCopy(items[0], "No Waypoints", 12);
			LstSetListChoices(lst, items, 1);
			LstSetListChoices(lst2, bear, 1);
			LstSetListChoices(lst3, dist, 1);
			LstSetListChoices(lst4, waytype, 1);
			field_set_value(form_set_task_ttldist, "0.0");
			ctl_set_value(form_set_task_hastakeoff, false);
			ctl_set_value(form_set_task_haslanding, false);
			selectedTaskWayIdx = -1;
		}

		// Redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
			LstDrawList(lst2);
			LstDrawList(lst3);
			LstDrawList(lst4);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_set_task_list));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_set_task_list2));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_set_task_list3));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_set_task_list4));
		}

		// select the item
		LstSetSelection(lst, (selectedTaskWayIdx % 7));
		LstSetSelection(lst2, (selectedTaskWayIdx % 7));
		LstSetSelection(lst3, (selectedTaskWayIdx % 7));
		LstSetSelection(lst4, (selectedTaskWayIdx % 7));

		// draw the horizontal lines
		DrawHorizListLines(7, 43, 14);

		// strike through name for achieved waypoints in active task
		if ((taskIndex == 0) && (data.activetask.tskstartsecs > 0)) {
			for (x = currentTaskPage*7; x <= activetskway; x++) {
				y = x - currentTaskPage*7;
				if (y < 7) {
					if ((x < activetskway) || !tasknotfinished) {
						if (x == selectedTaskWayIdx) {
							WinEraseLine(2, 36+y*14, 78, 36+y*14);
						} else {
							WinDrawLine(2, 36+y*14, 78, 36+y*14);
						}
					}
				}
			}
		}
		
		// display button if area task
		if (isAAT) {
			ctl_set_visible(form_set_task_AATdist_pop1, true);
			if (tsk->aataimtype != AATusr) lst_set_selection(form_set_task_AATdist_pop2, tsk->aataimtype);
			switch(tsk->aataimtype) {
				case AATmax:
					ctl_set_label(form_set_task_AATdist_pop1, "Max");
					break;
				case AATctr:
					ctl_set_label(form_set_task_AATdist_pop1, "Ctr");
					break;
				case AATmin:
					ctl_set_label(form_set_task_AATdist_pop1, "Min");
					break;
				case AATusr:
					ctl_set_label(form_set_task_AATdist_pop1, "Usr");
					lst_set_selection(form_set_task_AATdist_pop2, -1);
					break;
			}
		} else {
			ctl_set_visible(form_set_task_AATdist_pop1, false);
		}
		
	} else {
		// Just update the task data
		// Get number of waypoints for the given task after any changes
//		HostTraceOutputTL(appErrorClass, "Refresh_task_details Before tsk->numwaypts-|%hd|", tsk->numwaypts);
		if (!settaskreadonly) {
			OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
			MemMove(tsk,mem_ptr,sizeof(TaskData));
			MemHandleUnlock(mem_hand);
		}
//		HostTraceOutputTL(appErrorClass, "Refresh_task_details After tsk->numwaypts-|%hd|", tsk->numwaypts);
		if (tsk->numwaypts > 0) {
			CalcTaskDists(tsk, false, false, false);
			CalcBisectors(tsk);
			CalcStartFinishDirs(tsk);
		} else {
			tsk->ttldist = -1.0;
			selectedTaskWayIdx = -1;
		}
//		HostTraceOutputTL(appErrorClass, "Refresh_task_details Before Update tsk->numwaypts-|%hd|", tsk->numwaypts);
		if (!settaskreadonly) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
	}

/* PG deleted after new screen format
	if (action != TASKCLOSEDEL && action !=TASKCLOSEUPDATE && action != TASKCLRACT && (FrmGetActiveFormID() == form_set_task)) {
		// Display the number of waypoints in the current task
		MemSet(pageString, 20, 0);
		StrCopy(pageString,StrIToA(tempchar, selectedTaskWayIdx+1));
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tempchar, numWaypts));
		field_set_value(form_set_task_page, pageString);
	}
*/
}

void CalcWptDists(TaskData *task, Int16 x, Boolean updatedists, Boolean updatetargets)
// calculates the distance points and target points for area waypoints
{
	double temprng, tempbrg, templat, templon;

	if (task->waypttypes[x] & AREA)  {
		if (updatedists || updatetargets) {
			if (task->sectbear1[x] != task->sectbear2[x]) {
				// set distance to centre of area
				if (task->sectbear2[x] < task->sectbear1[x]) {
					tempbrg = (task->sectbear1[x] + task->sectbear2[x]+360.0)/2.0;
				} else {
					tempbrg = (task->sectbear1[x] + task->sectbear2[x])/2.0;
				}
				temprng = (task->arearadii[x] + task->arearadii2[x])/2.0;
				RangeBearingToLatLonLinearInterp(task->wayptlats[x], task->wayptlons[x], temprng, tempbrg, &templat, &templon);
			} else {
				// set distance to waypoint
				templat = task->wayptlats[x];
				templon = task->wayptlons[x];
			}

			// copy to dist / target waypoints
			if (updatedists) {
				if (task == &data.task) {
					data.activetask.maxareadist[x] = -999.9;
					data.activetask.arearng1 = -999.9;
					data.activetask.arearng2 = -999.9;
				}
				task->distlats[x] = templat;
				task->distlons[x] = templon;
			}
			if (updatetargets) {
				task->targetlats[x] = templat;
				task->targetlons[x] = templon;
			}
		}
	} else {
		// distance and target waypoints always equal to waypoints for non-area points
		if (task == &data.task) {
			data.activetask.maxareadist[x] = -999.9;
			data.activetask.arearng1 = -999.9;
			data.activetask.arearng2 = -999.9;
		}
		task->distlats[x] = task->wayptlats[x];
		task->distlons[x] = task->wayptlons[x];
		task->targetlats[x] = task->wayptlats[x];
		task->targetlons[x] = task->wayptlons[x];
		task->arearadii2[x] = 0.0;
		// update radii if cylinder type for start/turnpoint/finish
		if (x == (task->hastakeoff?1:0)) {
			// start
			if (data.config.starttype == CYLINDER) task->arearadii[x] = data.config.startrad;
		} else if (x == (task->haslanding?task->numwaypts-2:task->numwaypts-1)) {
			// finish
			if (data.config.finishtype == CYLINDER) task->arearadii[x] = data.config.finishrad;
		} else if (data.config.turntype == CYLINDER) {
			// turnpoint
			task->arearadii[x] = data.config.turncircrad;
		} else {
			task->arearadii[x] = 0.0;
		}
	}
	return;
}

void CalcTaskDists(TaskData *task, Boolean updatedists, Boolean updatetargets, Boolean updateattribs)
{
	Int16 TAKEOFFSET, LANDOFFSET;
	double prevLat=0.0, prevLon=0.0;
	double curRng, curBrg;
	Int16 x, lastx = 0;

	// update the task data : distance, bearings and total distance
	if (task->numwaypts > 0) {

//		HostTraceOutputTL(appErrorClass, "Calculating Task Distances");

		// check takeoff and landing flags
		if (task->hastakeoff) {
			TAKEOFFSET = 1;
		} else {
			TAKEOFFSET = 0;
		}
		if (task->haslanding) {
			LANDOFFSET = task->numwaypts - 1;
		} else {
			LANDOFFSET = task->numwaypts;
		}

		// zero task distance and no. of control points
		task->ttldist = 0.0;
		task->numctlpts = 0;

		// Loop through each task waypoint
		for (x = 0; x < (Int16)task->numwaypts; x++) {

			// check area & control status to false : area and control points only allowed between TAKEOFF and LANDING
			if (updateattribs) {
				if (x <= TAKEOFFSET) {
					if (task->waypttypes[x] & CONTROL) task->waypttypes[x] = task->waypttypes[x] ^ CONTROL;
					if (task->waypttypes[x] & AREA) task->waypttypes[x] = task->waypttypes[x] ^ AREA;
				}
				if (x >= LANDOFFSET-1) {
					if (task->waypttypes[x] & CONTROL) task->waypttypes[x] = task->waypttypes[x] ^ CONTROL;
					if (task->waypttypes[x] & AREA) task->waypttypes[x] = task->waypttypes[x] ^ AREA;
				}
			}

			// set up distances for point
			CalcWptDists(task, x, updatedists, updatetargets);

			if (x > 0) {
				if ((task->waypttypes[x] & CONTROL) == 0) {
					// Calculate leg distance and bearing
					LatLonToRangeBearingEll(prevLat, prevLon, task->distlats[x], task->distlons[x], &curRng, &curBrg);

					// Converted to Magnetic
					curBrg = nice_brg(curBrg + data.input.deviation.value);

					// check for same waypoint as start i.e. concentric case
					if ((x == TAKEOFFSET+1+task->numctlpts) && (data.config.starttype == CYLINDER) && (task->distlats[x] == prevLat) && (task->distlons[x] == prevLon)) {
						// update current waypoint distance
						curRng = Fabs(task->arearadii[lastx]-task->arearadii[x]);
//						curRng = task->arearadii[lastx];
						// This code would be used if turnpoint radii should not be included
						if ((data.config.turntype == CYLINDER) || (data.config.turntype == BOTH)) {
							if ((!task->rulesactive || !task->inclcyldist) && !(task->waypttypes[x] & AREA)) {
								curRng = Fabs(task->arearadii[lastx]-task->arearadii[x]);
							} else {
								if (task->arearadii[lastx] > task->arearadii[x]) {
									curRng =  task->arearadii[lastx];
								} else {
									curRng =  task->arearadii[x];
								}
//								curRng -= data.config.turncircrad;
							}
						}
						curBrg = 0.0;
					} else {
						// adjust leg distance and add to task total distance if a valid waypoint
						// Adjust distance for circle start
						if ((data.config.starttype == CYLINDER) && (x == TAKEOFFSET+1) && (curRng != 0.0))
						if (!task->rulesactive || !task->inclcyldist) {
							curRng -= data.config.startrad;
						}
						// Adjust distance for circle finish
						if ((data.config.finishtype == CYLINDER) && (x == LANDOFFSET-1) && (curRng != 0.0))
						if (!task->rulesactive || !task->inclcyldist) {
							curRng -= data.config.finishrad;
						}
						// This code would be used if turnpoint radii should not be included
						if (((data.config.turntype == CYLINDER) || (data.config.turntype == BOTH)) && (curRng != 0.0)) 
							if (!task->rulesactive || !task->inclcyldist) {
								if (!(task->waypttypes[lastx] & AREA) && (lastx != TAKEOFFSET)) {
									curRng -= data.config.turncircrad;
								}
								if (!(task->waypttypes[x] & AREA) && (x != LANDOFFSET-1)) {
									curRng -= data.config.turncircrad;
								}
							}
					}

					// check leg distance
					if (curRng < 0.0) curRng = 0.0;

					// Update total distance and leg distance and bearing
					task->distances[x] = curRng;
					task->bearings[x] = curBrg;
					if (x > TAKEOFFSET && x < LANDOFFSET) {
						task->ttldist += curRng;
					}

				} else {
					// Control Waypoint
					task->distances[x] = 0.0;
					task->bearings[x] = 0.0;
					task->numctlpts++;
				}
				
			} else {
				// 1st waypoint in task
				task->distances[x] = 0.0;
				task->bearings[x] = 0.0;
			}

			// Update last lat, Lon pair if not a control point
			if ((task->waypttypes[x] & CONTROL) == 0 ) {
				prevLat = task->distlats[x];
				prevLon = task->distlons[x];
				lastx = x;
			}

//			HostTraceOutputT(appErrorClass, "%s", DblToStr(x,0));
//			HostTraceOutputT(appErrorClass, " : %s", task->wayptnames[x]);
//			HostTraceOutputTL(appErrorClass, " : %s", print_distance2(task->distances[x],2));
		}

	} else {
		// No waypoints in task
		task->ttldist = 0.0;
		task->numctlpts = 0;
	}

	// check total distance
	if (task->ttldist < 0.0) task->ttldist = 0.0;

	return;
}

void DrawTask(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng, double maxdist, double startalt, TaskData *tsk, double mapscale, Boolean ismap)
{
	Int16 x, TAKEOFFSET=0, LANDOFFSET=0;
	Int16 cur=-1, nxt=-1, prv=-1;
	Int16 plotX=0, plotY=0;
	Int16 prevplotX=SCREEN.GLIDERX, prevplotY=SCREEN.GLIDERY;
	Int16 sect1, sect2;
	double poirange, poibearing;
	double startdir, finishdir;
	Boolean plotvalstat=false, prevplotvalstat=false;
	Boolean drawalltask = false;
	double tmpalt, tmpstf;
	double boldline = 1.0;
	Int16 saveplotX=0, saveplotY=0;
	Int16 tempplotX=0, tempplotY=0;
	Boolean isctlpt=false;
	Int16 plotX2=0, plotY2=0;

	if (tsk->hastakeoff) {
		TAKEOFFSET = 1;
	}
	if (tsk->haslanding) {
		LANDOFFSET = tsk->numwaypts - 1;
	} else {
		LANDOFFSET = tsk->numwaypts;
	}

	if (tsk->numwaypts > 0) {
		// Set variables to support the number of task points to draw
		switch (data.config.taskdrawtype) {
			case 0:
				drawalltask = true;
				prv = cur = nxt = -1;
				break;
			case 1:
				prv = -1;
				cur = activetskway;
				nxt = -1;
				break;
			case 2:
				prv = activetskway-1;
				cur = activetskway;
				nxt = -1;
				break;
			case 3:
				prv = activetskway-1;
				cur = activetskway;
				nxt = activetskway+1;
				break;
		}

		// force to draw all in waypoint sector view
		if (!ismap) {
			drawalltask = true;
			prv = cur = nxt = -1;
		}

/*		// draw line / FAI / arc end points
		plotvalstat = CalcPlotValues(gliderLat, gliderLon, data.input.startendlat1, data.input.startendlon1, xratio, yratio,
				&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
		DrawPOI(plotX, plotY, "A", (SCREEN.SRES*3), 0, false, false, TARGET, false, mapscale, false, false, false);
		plotvalstat = CalcPlotValues(gliderLat, gliderLon, data.input.startendlat2, data.input.startendlon2, xratio, yratio,
				&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
		DrawPOI(plotX, plotY, "B", (SCREEN.SRES*3), 0, false, false, TARGET, false, mapscale, false, false, false);
*/
		// cycle through each waypoint
		for (x=0; x < (Int16)tsk->numwaypts; x++) {

			// Draw the remainder of the waypoints
			// Returns bearing in magnetic
			plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->wayptlats[x], tsk->wayptlons[x], xratio, yratio,
						&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, true);

			// Draw actual waypoint dot and name only if not already being drawn as a waypoint or declutter could be active
			// changed to always draw the task waypoint names
			if ((data.config.wayonoff == false) || (mapscale >= data.config.declutter) || (tsk->wayptidxs[x] == -1)) {
				if (poirange < maxdist) {
					if (data.config.inrngcalc) {
						CalcSTFSpdAlt2(MCCurVal, poirange, poibearing, &tmpstf, &tmpalt);
						tmpalt = ConvertAltType(tsk->elevations[x], startalt, true, REQALT, tmpalt);
						if (tmpalt <= startalt) {
							DrawPOI(plotX, plotY, tsk->wayptnames[x], (SCREEN.SRES*3),
									data.config.waymaxlen, true, false, tsk->waypttypes[x], true, mapscale,
										(tsk->waypttypes[x] & AIRPORT), (tsk->waypttypes[x] & AIRLAND), (tmpalt+data.config.safealt > startalt));
						} else {
							DrawPOI(plotX, plotY, tsk->wayptnames[x], (SCREEN.SRES*3),
									data.config.waymaxlen, false, false, tsk->waypttypes[x], true, mapscale, false, false, false);
						}
					} else {
						DrawPOI(plotX, plotY, tsk->wayptnames[x], (SCREEN.SRES*3),
								data.config.waymaxlen, true, false, tsk->waypttypes[x], true, mapscale, (tsk->waypttypes[x] & AIRPORT), (tsk->waypttypes[x] & AIRLAND), false);
					}
				} else {
					DrawPOI(plotX, plotY, tsk->wayptnames[x], (SCREEN.SRES*3),
							data.config.waymaxlen, false, false, tsk->waypttypes[x], true, mapscale, false, false, false);
				}
			}

			if (tsk->waypttypes[x] & AREA) {
				// calculate points for target and max penetrations point
				if (data.config.AATmode == AAT_UPDLEGS) {
					// draw lines from max penetration points
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->targetlats[x], tsk->targetlons[x], xratio, yratio,
							&plotX2, &plotY2, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->distlats[x], tsk->distlons[x], xratio, yratio,
							&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					// draw target point and max penetration point in areas
					DrawPOI(plotX, plotY, " ", (SCREEN.SRES*3), 0, false, false, MAXPENPT, false, mapscale, false, false, false);
					DrawPOI(plotX2, plotY2, " ", (SCREEN.SRES*3), 0, false, false, TARGET, false, mapscale, false, false, false);
				} else {
					// draw lines from target points
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->distlats[x], tsk->distlons[x], xratio, yratio,
							&plotX2, &plotY2, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->targetlats[x], tsk->targetlons[x], xratio, yratio,
							&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					// draw target point and max penetration point in areas
					DrawPOI(plotX2, plotY2, " ", (SCREEN.SRES*3), 0, false, false, MAXPENPT, false, mapscale, false, false, false);
					DrawPOI(plotX, plotY, " ", (SCREEN.SRES*3), 0, false, false, TARGET, false, mapscale, false, false, false);
				}
			}

			// Draw task sectors, lines and/or cylinders
			// set to colour for sectors
			if (device.colorCapable) {
				WinSetForeColor(indexSector);
			}
			// check for bold line
			if (device.HiDensityScrPresent && data.config.BoldSector) {
				WinSetCoordinateSystem(kCoordinatesStandard);
				boldline = SCREEN.SRES;
			}

			// sectors drawn based on waypoint
			plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->wayptlats[x], tsk->wayptlons[x], xratio, yratio,
						&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, true);

			if (plotvalstat && (drawalltask || x == prv || x == cur || x == nxt)) {
				if (x == TAKEOFFSET) {
					// Draw the Start & First Waypoints
					switch (data.config.starttype) {
						case TSKLINE:
							// This converts the startdir to Magnetic for drawing
							startdir = nice_brg(data.config.startdir + data.input.deviation.value);

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.startrad/boldline,
											(Int16)(nice_brg((startdir-90.0))),
											(Int16)(nice_brg((startdir+90.0))),
											ENDSONLY);
							break;
						case FAI:
							// This converts the startdir to Magnetic for drawing
							startdir = RecipCse(nice_brg(data.config.startdir + data.input.deviation.value));

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.startrad/boldline,
											(Int16)(nice_brg((startdir-45.0))),
											(Int16)(nice_brg((startdir+45.0))),
											ALLSECTOR);
							break;
						case CYLINDER:
							//MFH Need to make WinDrawCircle accept radius values in Nautical Miles
							WinDrawCircle((Int32)plotX/boldline, (Int32)plotY/boldline, (Int32)(data.config.startrad*xratio)/boldline, SOLID);
							break;
						case ARC:
							// This converts the startdir to Magnetic for drawing
							startdir = RecipCse(nice_brg(data.config.startdir + data.input.deviation.value));

							// draw relative to first waypoint in task
							plotvalstat = CalcPlotValues(gliderLat, gliderLon, data.input.startlat, data.input.startlon, xratio, yratio,
										&tempplotX, &tempplotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, tempplotX/boldline, tempplotY/boldline, 0.0, data.input.startrad/boldline,
											(Int16)(nice_brg((startdir-data.input.startsect))),
											(Int16)(nice_brg((startdir+data.input.startsect))),
											NOENDS);
							break;
						default:
							break;
					}
				} else if (x == LANDOFFSET-1) {
					// If last waypoint in task & Finish
					switch (data.config.finishtype) {
						case TSKLINE:
							// This converts the startdir back to Magnetic for drawing
							finishdir = nice_brg(data.config.finishdir + data.input.deviation.value);

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.finishrad/boldline,
											(Int16)(nice_brg((finishdir-90.0))),
											(Int16)(nice_brg((finishdir+90.0))),
											ENDSONLY);
							break;
						case FAI:
							// This converts the startdir back to Magnetic for drawing
							finishdir = nice_brg(data.config.finishdir + data.input.deviation.value);

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.finishrad/boldline,
											(Int16)(nice_brg((finishdir-45.0))),
											(Int16)(nice_brg((finishdir+45.0))),
											ALLSECTOR);
							break;
						case CYLINDER:
							WinDrawCircle((Int32)plotX/boldline, (Int32)plotY/boldline, (Int32)(data.config.finishrad*xratio)/boldline, SOLID);
							break;
/*						case ARC:
							// This converts the startdir to Magnetic for drawing
							finishdir = nice_brg(data.config.finishdir + data.input.deviation.value);

							// draw relative to last waypoint in task
							plotvalstat = CalcPlotValues(gliderLat, gliderLon, data.input.finishlat, data.input.finishlon, xratio, yratio,
										&tempplotX, &tempplotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);

							// Expects bearing to be magnetic
							WinDrawSector(xratio, yratio, tempplotX/boldline, tempplotY/boldline, 0.0, data.input.finishrad/boldline,
											(Int16)(nice_brg((finishdir-data.input.finishsect))),
											(Int16)(nice_brg((finishdir+data.input.finishsect))),
											NOENDS);
							break;
*/
						default:
							break;
					}
				} else if (x > TAKEOFFSET && x < (LANDOFFSET-1)) {
					// Draw the Turnpoints
					if ((tsk->arearadii[x] > 0.0) && (tsk->waypttypes[x] & AREA) && ((tsk->waypttypes[x] & CONTROL) == 0)) {
						// Area Turnpoints
						// Convert from True to Magnetic
						sect1 = (Int16)(nice_brg((((double)(tsk->sectbear1[x]))+data.input.deviation.value)));
						sect2 = (Int16)(nice_brg((((double)(tsk->sectbear2[x]))+data.input.deviation.value)));
						// Expects sector bearings to be in magnetic
						WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, tsk->arearadii2[x]/boldline, tsk->arearadii[x]/boldline, sect1, sect2, ALLSECTOR);
					} else if (tsk->waypttypes[x] & CONTROL) {
						// currently draw nothing for CONTROL points (could use same as area turnpoints
					} else {
						// If not the last waypoint in task, draw a turnpoint
						switch (data.config.turntype) {
							case FAI:
								sect1 = (Int16)nice_brg((double)tsk->sectbear1[x]+data.input.deviation.value);
								sect2 = (Int16)nice_brg((double)tsk->sectbear2[x]+data.input.deviation.value);
								// Expects sector bearings to be in magnetic
								WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.turnfairad/boldline, sect1, sect2, ALLSECTOR);
								break;
							case CYLINDER:
								WinDrawCircle((Int32)plotX/boldline, (Int32)plotY/boldline,(Int32)(data.config.turncircrad*xratio)/boldline, SOLID);
								break;
							case BOTH:
								sect1 = (Int16)nice_brg((double)tsk->sectbear1[x]+data.input.deviation.value);
								sect2 = (Int16)nice_brg((double)tsk->sectbear2[x]+data.input.deviation.value);
								WinDrawCircle((Int32)plotX/boldline, (Int32)plotY/boldline, (Int32)(data.config.turncircrad*xratio)/boldline, SOLID);
								if (data.config.turncircrad < data.config.turnfairad) {
									// Expects sector bearings to be in magnetic
									WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, data.config.turncircrad/boldline, data.config.turnfairad/boldline,
													sect1, sect2, NOBOTTOM);
								} else {
									// Expects sector bearings to be in magnetic
									WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, data.config.turnfairad/boldline,
													sect1, sect2, ALLSECTOR);
								}
								break;
							default:
								break;
						}
					}
				}
			}

			// Draw the connecting line between the current and previous points
			// set colour to task lines
			if (device.colorCapable) {
				WinSetForeColor(indexTask);
			}
			
			// check for bold line
			if (device.HiDensityScrPresent && data.config.BoldTask) {
				WinSetCoordinateSystem(kCoordinatesStandard);
				boldline = SCREEN.SRES;
			} else {
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(device.HiDensityScrPresent);
				}
				boldline = 1.0;
			}

			if (tsk->waypttypes[x] & AREA) {
				// calculate point to draw line to/from
				if (data.config.AATmode == AAT_UPDLEGS) {
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->distlats[x], tsk->distlons[x], xratio, yratio,
							&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
				} else {
					plotvalstat = CalcPlotValues(gliderLat, gliderLon, tsk->targetlats[x], tsk->targetlons[x], xratio, yratio,
							&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
				}
			}
			
			if (plotvalstat && prevplotvalstat) {
				// plot dashed line for takeoff, landing and control points
				if ((x == TAKEOFFSET || x == LANDOFFSET) || (tsk->waypttypes[x] & CONTROL) || (tsk->waypttypes[x-1] & CONTROL)) {
					if (data.config.ctllinevis) WinDrawClippedLine(plotX/boldline, plotY/boldline, prevplotX/boldline, prevplotY/boldline, DASHED);
					if ((tsk->waypttypes[x] & CONTROL) && !isctlpt) {
						// save last non control point co-ords
						saveplotX = prevplotX;
						saveplotY = prevplotY;
						isctlpt = true;
					}
				} else {
					// draw solid line
					WinDrawClippedLine(plotX/boldline, plotY/boldline, prevplotX/boldline, prevplotY/boldline, SOLID);
				}
			}
			// check if need to draw addional solid line skipping control points
			if (((tsk->waypttypes[x] & CONTROL) == 0) && isctlpt) {
				WinDrawClippedLine(plotX/boldline, plotY/boldline, saveplotX/boldline, saveplotY/boldline, SOLID);
				isctlpt = false;
			}

			// Have to reset this back again before plotting waypoints
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(device.HiDensityScrPresent);
			}
			boldline = 1.0;

			prevplotX = plotX;
			prevplotY = plotY;
			prevplotvalstat = plotvalstat;
		}
	}

	// reset colour to black and normal lines
	if (device.colorCapable) {
		WinSetForeColor(indexBlack);
	}
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
	}
	return;
}

void HandleTask(Int16 action)
{
	double poirange, poibearing;
	Int16 TAKEOFFSET=0, LANDOFFSET=0;
	double startfinishdir=0.0;
	double sect1, sect2;
	static Boolean wasinsector=false;
	static Boolean startedonentry=false;
	double rng1, rng2;
//	double rng3, rng4;
	Int16 x,y,z;
	double tempdbl;
	double start_stf;
	double distadj;
	static Boolean manualstart = false;
	Int32 timetogo = 0;
	Char tempchar[10];
	Char intchar[10];
	double temprng, tempbrg;
	Int32 timetogotostart;
	double tgtrange;
	
	static Int8 inturnsector = 0;
	static Int8 infinishsector = 0;
	static Int8 instartsector = 0;
	static Int8 inareactlsector = 0;

//	HostTraceOutputTL(appErrorClass, "Handle Task Begin");	

	// check for takeoff waypoint
	if (data.task.hastakeoff) {
		TAKEOFFSET = 1;
	}
	
	// update range and bearing to active waypoint
	LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[activetskway], data.task.wayptlons[activetskway], &poirange, &poibearing);
	poibearing = nice_brg(poibearing + data.input.deviation.value);
	data.activetask.tasktgtlat = data.task.targetlats[activetskway];
	data.activetask.tasktgtlon = data.task.targetlons[activetskway];

	// adjust target to nearest point on start line or arc
	if (activetskway == TAKEOFFSET) {
//		HostTraceOutputTL(appErrorClass, "adjust start");
		switch (data.config.starttype) {
			case TSKLINE:
				dist2seg(data.input.gpslatdbl, data.input.gpslngdbl, data.input.startendlat1, data.input.startendlon1, data.input.startendlat2, data.input.startendlon2, &data.activetask.tasktgtlat, &data.activetask.tasktgtlon);
				break;
			case ARC:
				// to be implemented
				break;
			default:
				break;
		}
	}

	// calculate range to target
	if ((data.config.AATmode == AAT_MTURN_ON) && (inareasector > 0)) {
		LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[inareasector], data.task.targetlons[inareasector], &tgtrange);
	} else {
		LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[activetskway], data.task.targetlons[activetskway], &tgtrange);
	}
	
	// find magnetic bearing to task start waypoint
	if (data.config.starttype != ARC) {
		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[TAKEOFFSET], data.task.wayptlons[TAKEOFFSET], &startrange, &startbearing);
	} else {
		// ARC depends on first waypoint, so calculate distance/bearing from first waypoint
		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.startlat, data.input.startlon, &startrange, &startbearing);
	}
	startbearing = nice_brg(startbearing + data.input.deviation.value);

	// check for landing waypoint
	if (data.task.haslanding) {
		LANDOFFSET = data.task.numwaypts - 1;
	} else {
		LANDOFFSET = data.task.numwaypts;
	}
	// reset timeinfo string
	timeinfo[0] = 0;

	if ((data.task.numwaypts > 0) || action) {  // This ensures that there is an active flight going on before updating the task info

		switch (action) {
			case TSKFORCESTART:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKFORCESTART");
				manualstart = true;
				StrCopy(start_utc, data.logger.gpsutc);
				StrCopy(start_dtg, data.logger.gpsdtg);
				start_utcsecs = utcsecs;
				start_height = data.input.inusealt;
			case TSKSTART:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKSTART");
				starttask = true;
			case TSKNORMAL:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKNORMAL");
				if (inflight) {

					if (tasknotfinished) {

					// only auto zoom if start is active waypoint
					if (activetskway == TAKEOFFSET) {
						HandleTaskAutoZoom(tgtrange, data.config.startrad, false);
					}

//************************************* Always Check for Task Start and Re-start if task not completed
					switch (data.config.starttype) {
						case TSKLINE:
							startfinishdir = RecipCse(nice_brg(data.config.startdir + data.input.deviation.value));
							if (ProcessStart(nice_brg(startfinishdir-90.0), nice_brg(startfinishdir+90.0), 0, data.config.startrad, startbearing, startrange)) {
								// get start time / height
								StrCopy(start_utc, data.logger.gpsutc);
								StrCopy(start_dtg, data.logger.gpsdtg);
								start_utcsecs = utcsecs;
								start_height = data.input.inusealt;
//								HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
								// decide start or re-start
								task_start_alert(data.activetask.tskstartsecs > 0);
							}
							break;
						case FAI:
							startfinishdir = RecipCse(nice_brg(data.config.startdir + data.input.deviation.value));
							if (InSectorTest(nice_brg(startfinishdir-45.0), nice_brg(startfinishdir+45.0), 0, data.config.startrad, startbearing, startrange)) {
								if (instartsector >= LOGGEDPTS) {
									if (data.task.rulesactive && data.task.startonentry && !wasinsector) {
										if (!startedonentry) {
											//  start task on entry to sector
											// get start time / height
											StrCopy(start_utc, data.logger.gpsutc);
											StrCopy(start_dtg, data.logger.gpsdtg);
											start_utcsecs = utcsecs;
											start_height = data.input.inusealt;
//											HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
											// decide start or re-start
											task_start_alert(data.activetask.tskstartsecs > 0);
											startedonentry = true;
										}
									} else {
										if (!wasinsector) {
//											HostTraceOutputTL(appErrorClass, "In FAI Sector");
											if (data.activetask.tskstartsecs == 0) {
												PlayTurnSound();
												activetskway = TAKEOFFSET+1;
												// check for first turnpoint being identical to the start, ignoring control points
												x = activetskway;
												while (data.task.waypttypes[x] & CONTROL) x++;
												if (data.task.distances[x] < 0.01) activetskway++;
												CheckTurn();
												// update range and bearing to next point
												LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[activetskway], data.task.wayptlons[activetskway], &poirange, &poibearing);
												poibearing = nice_brg(poibearing + data.input.deviation.value);
											}
										}
										wasinsector = true;
									}
									instartsector = 0;
//									HostTraceOutputTL(appErrorClass, "HandleTask:FAI Start Achieved!");
								} else {
									instartsector++;
//									HostTraceOutputTL(appErrorClass, "In Start Sector Num:%s",DblToStr(instartsector,0));
								}
//								HostTraceOutputTL(appErrorClass, "In Start Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//								HostTraceOutputTL(appErrorClass, "In Start Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
							} else if (wasinsector) {
								// get start time / height
								StrCopy(start_utc, data.logger.gpsutc);
								StrCopy(start_dtg, data.logger.gpsdtg);
								start_utcsecs = utcsecs;
								start_height = data.input.inusealt;
//								HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
								// decide start or re-start
								task_start_alert(data.activetask.tskstartsecs > 0);
								wasinsector = false;
								instartsector = 0;
							} else {
								wasinsector = false;
								startedonentry = false;
								instartsector = 0;
							}
							break;
						case CYLINDER:
							if (InSectorTest(0, 0, 0, data.config.startrad, startbearing, startrange)) {
								if (instartsector >= LOGGEDPTS) {
									if (data.task.rulesactive && data.task.startonentry && !wasinsector) {
										if (!startedonentry) {
											//  start task on entry to sector
											// get start time / height
											StrCopy(start_utc, data.logger.gpsutc);
											StrCopy(start_dtg, data.logger.gpsdtg);
											start_utcsecs = utcsecs;
											start_height = data.input.inusealt;
//											HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
											// decide start or re-start
											task_start_alert(data.activetask.tskstartsecs > 0);
											startedonentry = true;
										}
									} else {
										if (!wasinsector) {
//											HostTraceOutputTL(appErrorClass, "In CYL Sector");
											if (data.activetask.tskstartsecs == 0) {
												PlayTurnSound();
												activetskway = TAKEOFFSET+1;
												// check for first turnpoint being identical to the start, ignoring control points
												x = activetskway;
												while (data.task.waypttypes[x] & CONTROL) x++;
												if (data.task.distances[x] < 0.01) activetskway++;
												CheckTurn();
												// update range and bearing to next point
												LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[activetskway], data.task.wayptlons[activetskway], &poirange, &poibearing);
												poibearing = nice_brg(poibearing + data.input.deviation.value);
											}
										}
										wasinsector = true;
									}
									instartsector = 0;
//									HostTraceOutputTL(appErrorClass, "HandleTask:Cylinder Start Achieved!");
								} else {
									instartsector++;
//									HostTraceOutputTL(appErrorClass, "In Start Sector Num:%s",DblToStr(instartsector,0));
								}
//								HostTraceOutputTL(appErrorClass, "In Start Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//								HostTraceOutputTL(appErrorClass, "In Start Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
							} else if (wasinsector) {
								// get start time / height
								StrCopy(start_utc, data.logger.gpsutc);
								StrCopy(start_dtg, data.logger.gpsdtg);
								start_utcsecs = utcsecs;
								start_height = data.input.inusealt;
//								HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
								// decide start or re-start
								task_start_alert(data.activetask.tskstartsecs > 0);
								wasinsector = false;
								instartsector = 0;
							} else {
								wasinsector = false;
								startedonentry = false;
								instartsector = 0;
							}
							break;
						case ARC:
							startfinishdir = RecipCse(nice_brg(data.config.startdir + data.input.deviation.value));
							if (ProcessArcStart(nice_brg(startfinishdir-data.input.startsect), nice_brg(startfinishdir+data.input.startsect), 0, data.input.startrad, startbearing, startrange)) {
								// get start time / height
								StrCopy(start_utc, data.logger.gpsutc);
								StrCopy(start_dtg, data.logger.gpsdtg);
								start_utcsecs = utcsecs;
								start_height = data.input.inusealt;
//								HostTraceOutputTL(appErrorClass, "Start Time %s",start_utc);
								// decide start or re-start
								task_start_alert(data.activetask.tskstartsecs > 0);
							}
							break;
						default:
							break;
					}}

//************************************* Do valid start checks

					// check for auto-dismiss of re-start task warning
					if ((suaalert->alerttype == TASKSTART_ALERT) && (data.application.form_id == form_genalert) && (data.config.autodismisstime > 0)) {
						if (cursecs > warning_time + data.config.autodismisstime) {
							// automatically dismiss the warning and close window
//							HostTraceOutputTL(appErrorClass, "Re-Start Task autodismiss");
							warning_time = 0xffffffff;
							HandleWaitDialogWin(0);
							// alert response
							suaalertret->valid = true;
							suaalertret->btnselected = 6;
							suaalertret->alerttype = TASKSTART_ALERT;
							suaalertret->alertidxret = -1;
							suaalertret->priority = suaalert->priority;
						}
					}
				
					// Check start height rules
					if (data.task.rulesactive && (data.task.maxstartheight > 0.0)) {
						if (data.input.inusealt > data.activetask.startheight) {
							// above max start height
							abovestartheighttime = cursecs;
							abovemaxstartheight = true;
						} else if (cursecs > abovestartheighttime + data.task.timebelowstart) {
							// below max start height for long enough
							abovemaxstartheight = false;
						}
					} else {
						// max start height rule n/a
						abovemaxstartheight = false;
						abovestartheighttime = 0;
					}

					// Check for being within the start warn height
					if (data.task.rulesactive && (data.task.maxstartheight > 0.0) && (data.task.startwarnheight > 0.0)) {
						if ((data.activetask.tskstartsecs == 0) && (data.input.inusealt > (data.activetask.startheight - data.task.startwarnheight))) {
							// warn only before 1st start
//							HostTraceOutputTL(appErrorClass, "Start Height Warning!");
							if (!startheightwarned1 || (!startheightwarned2 && (data.input.inusealt > data.activetask.startheight))) {
								task_invalid_start_alert(data.input.inusealt > data.activetask.startheight, false, false, false);
							}
						} else {
							if (data.input.inusealt < data.activetask.startheight - data.task.startwarnheight) startheightwarned2 = false;
							if (data.input.inusealt < data.activetask.startheight - 2*data.task.startwarnheight) startheightwarned1 = false;
						}
					}

					// Check for early start
//					HostTraceOutputTL(appErrorClass, "time %s", DblToStr(cursecs%86400,0));
//					HostTraceOutputTL(appErrorClass, "earliest %s", DblToStr(data.task.startlocaltime*60,0));
					if (data.task.rulesactive && ((Int32)(cursecs%86400) < (data.task.startlocaltime*60))) {
						starttooearly = true;
					} else {
						starttooearly = false;
					}

					// handle the return from the task alert window
					if ((suaalertret->alerttype == TASKSTART_ALERT) && suaalertret->valid) {
						if (suaalertret->priority == GOTOSTART) {
//							HostTraceOutputTL(appErrorClass, "Goto Start");
							starttask = false;
							// don't give another goto start warning for 5 mins
							warnedgotostart = cursecs + 300;
						} else if ((suaalertret->priority != INVALIDSTART) && ((suaalertret->btnselected == 0) || (suaalertret->btnselected == 6))) {
							// set task start flag if "YES" button selected or tapped in window
//							HostTraceOutputTL(appErrorClass, "Confirmed START!");
							starttask = true;
						} else if ((suaalertret->priority == INVALIDSTART) && ((suaalertret->btnselected == 0) || (suaalertret->btnselected == 6))) {
//							HostTraceOutputTL(appErrorClass, "Forced START!");
							starttask = true;
							abovemaxstartheight = false;
							starttooearly = false;
						} else {
//							HostTraceOutputTL(appErrorClass, "Denined Start");
							starttask = false;
//							// return target waypoint to start
//							activetskway = TAKEOFFSET;
							wasinsector = false;
						}

						// clear response
						suaalertret->alerttype = NULL_ALERT;
						suaalertret->alertidxret = -1;
						suaalertret->valid = false;
						suaalertret->btnselected = -1;
					}

//************************************* Accurate Start Range and Time Adjustment
					if (!taskonhold && data.task.rulesactive && (activetskway == TAKEOFFSET) && (data.task.startlocaltime > 0)) {
						// calculate exact distance start sector
						switch (data.config.starttype) {
							case TSKLINE:
								// nearest point on start line, or an end
								startrange = Sqrt(dist2seg(data.input.gpslatdbl, data.input.gpslngdbl, data.input.startendlat1, data.input.startendlon1, data.input.startendlat2, data.input.startendlon2, &data.activetask.tasktgtlat, &data.activetask.tasktgtlon));
								LatLonToBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.activetask.tasktgtlat, data.activetask.tasktgtlon, &startbearing);
								break;
							case FAI:
								// simple case: to start point
								break;
							case CYLINDER:
								// simple case: direct line to edge of start cylinder
								startrange = Fabs(startrange - data.config.startrad);
								break;
							case ARC:
								// nearest point on start arc, or an end
								LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.startlat, data.input.startlon, &temprng, &tempbrg);
								if (InSectorTest(RecipCse(data.config.startdir-data.input.startsect), RecipCse(data.config.startdir+data.input.startsect), 0.0, 2.0, tempbrg, 1.0)) {
									startrange = Fabs(temprng - data.input.startrad);
									startbearing = tempbrg;
									RangeBearingToLatLonLinearInterp(data.input.gpslatdbl, data.input.gpslngdbl, startrange, (temprng>data.input.startrad?startbearing:RecipCse(startbearing)), &data.activetask.tasktgtlat, &data.activetask.tasktgtlon);
								} else {
									startrange = Sqrt(dist2seg(data.input.gpslatdbl, data.input.gpslngdbl, data.input.startendlat1, data.input.startendlon1, data.input.startendlat2, data.input.startendlon2, &data.activetask.tasktgtlat, &data.activetask.tasktgtlon));
									LatLonToBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.activetask.tasktgtlat, data.activetask.tasktgtlon, &startbearing);
								}
								break;
							default:
								break;
						}
//						HostTraceOutputTL(appErrorClass, "Start Bearing %s", DblToStr(startbearing, 1));
//						HostTraceOutputTL(appErrorClass, "Start Dist    %s", print_distance2(startrange, 1));

						if (!data.task.warnbeforestart) {
							timetogo = data.task.startlocaltime*60 - (Int32)(cursecs%86400);
						} else {
							// calculate STF to start point, and time to reach start point
							CalcSTFSpdAlt2(MCCurVal, startrange, RecipCse(startbearing), &start_stf, &tempdbl);
							timetogotostart = (startrange / (start_stf - calcstfhwind) * 3600.0); // add additional 30 secs to turn round?
							if (data.task.startlocaltime*60 - (Int32)(cursecs%86400) <= 0) {
								speedtostart = 0.0;
							} else {
								speedtostart = startrange/(data.task.startlocaltime*60 - (Int32)(cursecs%86400))*3600.0;
							}
							if (!data.config.showrgs) speedtostart += calcstfhwind;
//								HostTraceOutputTL(appErrorClass, "Start STF     %s", print_horizontal_speed2(start_stf, 1));
//								HostTraceOutputTL(appErrorClass, "Duration to Start %s", DblToStr(timetogotostart, 0));
//								HostTraceOutputTL(appErrorClass, "Time to Start %s", DblToStr(data.task.startlocaltime*60 - (Int32)(cursecs%86400), 0));
//								HostTraceOutputTL(appErrorClass, "Speed to Start %s", DblToStr(speedtostart, 0));
//								HostTraceOutputTL(appErrorClass, "Headwind      %s", DblToStr(calcstfhwind, 1));
							// compare current time with start time minus time to get to start point
							timetogo =  data.task.startlocaltime*60 - (Int32)(cursecs%86400) - timetogotostart;
						}
						// set up start time info string
						if (data.task.startlocaltime*60 - (Int32)(cursecs%86400) < 0) {
							StrCopy(timeinfo, "Start! ");
						} else {
							if (timetogo < 0) StrCat(timeinfo, "(");
							// show the time to go until the start gate opens (max 99:59)
							if (Fabs(timetogo) >= 100*60) timetogo = 100*60-1;
							StrCopy(tempchar, "00");
							StrCat(tempchar, StrIToA(intchar, Fabs(timetogo/60)));
							StrCat(timeinfo, Right(tempchar, 2));
							StrCat(timeinfo, ":");
							StrCopy(tempchar, "00");
							StrCat(tempchar, StrIToA(intchar, Fabs(timetogo%60)));
							StrCat(timeinfo, Right(tempchar, 2));
							if (timetogo < 0) StrCat(timeinfo, ")");
							if (data.task.warnbeforestart) {
								// display required speed to reach start on-time
								StrCat(timeinfo, "  ");
								if ((speedtostart < 0.0) || (speedtostart > 150.0)) {
									StrCat(timeinfo, "Max");
								} else {
									StrCat(timeinfo, print_horizontal_speed2(speedtostart,0));
								}
								if ((timetogo <= 0) && (cursecs > warnedgotostart)) {
									// issue goto to start warning
//										HostTraceOutputTL(appErrorClass, "Goto Start Warning");
									task_invalid_start_alert(false, true, false, true);
								}
							}
						}
					}

					// Check for invalid start and popup warning
					if (data.task.rulesactive && starttask && !manualstart) {
						if (abovemaxstartheight) {
//							HostTraceOutputTL(appErrorClass, "Invalid Start! - Above Height Rules");
							task_invalid_start_alert(data.input.inusealt > data.activetask.startheight, false, true, false);
							starttask = false;
						} else if (starttooearly) {
//							HostTraceOutputTL(appErrorClass, "Invalid Start! - Too Early");
//							HostTraceOutputTL(appErrorClass, "time %s", DblToStr(cursecs%86400,0));
//							HostTraceOutputTL(appErrorClass, "earliest %s", DblToStr(data.task.startlocaltime*60,0));
							task_invalid_start_alert(false, true, true, false);
							starttask = false;
						}
					}

					// Check for task start or re-start
					if (starttask) {
//						HostTraceOutputTL(appErrorClass, "START! %s",start_utc);
						PlayStartSound();
						tasknotfinished = true;
						activetskway = TAKEOFFSET+1;
						// check for first turnpoint being identical to the start, ignoring control points
						x = activetskway;
						while (data.task.waypttypes[x] & CONTROL) x++;
						if (data.task.distances[x] < 0.01) activetskway++;
						// reset times and distances in area waypoints
						for (x = 0; x < (Int16)data.task.numwaypts; x++) {
							data.flight.timeatturn[x] = 0;
							data.flight.invalidturn[x] = false;
							if (data.task.waypttypes[x] & AREA) {
								data.activetask.maxareadist[x] = -999.9;
								data.activetask.arearng1 = -999.9;
								data.activetask.arearng2 = -999.9;
								data.task.distlats[x] = data.task.targetlats[x];
								data.task.distlons[x] = data.task.targetlons[x];
							}
						}
						if (manualstart) data.flight.invalidturn[TAKEOFFSET] = true;
						data.flight.flttask.numwaypts = data.task.numwaypts - data.task.numctlpts;
						StrCopy(data.flight.tskstartutc, start_utc);
						StrCopy(data.flight.tskstartdtg, start_dtg);
						data.activetask.tskstartsecs = start_utcsecs;
						data.activetask.tskstopsecs = start_utcsecs;
						data.activetask.TOTsecs = 0;
						StrCopy(data.flight.tskstoputc, "NT");
						StrCopy(data.flight.tskstopdtg, "NT");
						data.flight.timeatturn[TAKEOFFSET] = start_utcsecs;
						data.flight.tskdist = 0.0;
						data.flight.tskspeed = 0.0;
						data.flight.tskairspeed = 0.0;
						data.flight.startheight = start_height;
						data.activetask.finishheight = calcfinishheight();
						if (activefltindex >= 0) {
							OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
							data.input.logfltupd = cursecs;
						}
						CheckTurn();
						// Reset the Auto Task Zoom
						HandleTaskAutoZoom(0.0, 0.0, true);
						starttask = false;
						wasinsector = false;
						abovemaxstartheight = false;
						// update range and bearing to next point
						LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[activetskway], data.task.wayptlons[activetskway], &poirange, &poibearing);
						poibearing = nice_brg(poibearing + data.input.deviation.value);
					}
					
//************************************* If on the last turnpoint, handle the Task Finish
					if ((activetskway == LANDOFFSET-1) && (data.config.AATmode != AAT_MTURN_ON)) {
						if (tasknotfinished) {

						HandleTaskAutoZoom(tgtrange, data.config.finishrad, false);
						switch (data.config.finishtype) {
							case TSKLINE:
								// Reciprocal Converted from True to Magnetic
								startfinishdir = RecipCse(nice_brg(data.config.finishdir + data.input.deviation.value));
								if (ProcessFinish(nice_brg(startfinishdir-90.0), nice_brg(startfinishdir+90.0), 0, data.config.finishrad, poibearing, poirange)) {
									if (data.task.haslanding) {
										activetskway++;
									}
									if (tasknotfinished && (data.task.numwaypts > 0)) {
//										HostTraceOutputTL(appErrorClass, "Task Finish:Line");
										StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
										StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
										data.activetask.tskstopsecs = utcsecs;
										data.flight.timeatturn[activetskway] = utcsecs;
										data.flight.tskdist = data.task.ttldist;
										data.flight.tskspeed = CalcCompletedTskSpeed();
										data.flight.flttask.ttldist = data.task.ttldist;
										if (activefltindex >= 0) {
											OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
											data.input.logfltupd = cursecs;
										}
									}
									CheckTurn();
									PlayFinishSound();
									tasknotfinished = false;
									mustActivateAsNew = true;
									// Reset the Auto Task Zoom
									HandleTaskAutoZoom(0.0, 0.0, true);
								}
								// nearest point on finish line, or an end
								dist2seg(data.input.gpslatdbl, data.input.gpslngdbl, data.input.finishendlat1, data.input.finishendlon1, data.input.finishendlat2, data.input.finishendlon2, &data.activetask.tasktgtlat, &data.activetask.tasktgtlon);
								break;
							case FAI:
								// Reciprocal Converted from True to Magnetic
								startfinishdir = nice_brg(data.config.finishdir + data.input.deviation.value);
								if (InSectorTest(nice_brg(startfinishdir-45.0), nice_brg(startfinishdir+45.0), 0, data.config.finishrad, poibearing, poirange)) {
									if (infinishsector >= LOGGEDPTS) {
										if (data.task.haslanding) {
											activetskway++;
										}
										if (tasknotfinished && (data.task.numwaypts > 0)) {
//											HostTraceOutputTL(appErrorClass, "Task Finish:FAI");
											StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
											StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
											data.activetask.tskstopsecs = utcsecs;
											data.flight.timeatturn[activetskway] = utcsecs;
											data.flight.tskdist = data.task.ttldist;
											data.flight.tskspeed = CalcCompletedTskSpeed();
											data.flight.flttask.ttldist = data.task.ttldist;
											if (activefltindex >= 0) {
												OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
												data.input.logfltupd = cursecs;
											}
										}
										CheckTurn();
										PlayFinishSound();
										tasknotfinished = false;
										mustActivateAsNew = true;
										// Reset the Auto Task Zoom
										HandleTaskAutoZoom(0.0, 0.0, true);

										infinishsector = 0;
//										HostTraceOutputTL(appErrorClass, "HandleTask:FAI Finish Achieved!");
									} else {
										infinishsector++;
//										HostTraceOutputTL(appErrorClass, "In Finish Sector Num:%s",DblToStr(infinishsector,0));
									}
//									HostTraceOutputTL(appErrorClass, "In Finish Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//									HostTraceOutputTL(appErrorClass, "In Finish Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
								} else {
									infinishsector = 0;
								}
								break;
							case CYLINDER:
								if (InSectorTest(0, 0, 0, data.config.finishrad, poibearing, poirange)) {
									if (infinishsector >= LOGGEDPTS) {
										if (data.task.haslanding) {
											activetskway++;
										}
										if (tasknotfinished && (data.task.numwaypts > 0)) {
//											HostTraceOutputTL(appErrorClass, "Task Finish:Cylinder");
											StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
											StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
											data.activetask.tskstopsecs = utcsecs;
											data.flight.timeatturn[activetskway] = utcsecs;
											data.flight.tskdist = data.task.ttldist;
											data.flight.tskspeed = CalcCompletedTskSpeed();
											data.flight.flttask.ttldist = data.task.ttldist;
											if (activefltindex >= 0) {
												OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
												data.input.logfltupd = cursecs;
											}
										}
										CheckTurn();
										PlayFinishSound();
										tasknotfinished = false;
										mustActivateAsNew = true;
										// Reset the Auto Task Zoom
										HandleTaskAutoZoom(0.0, 0.0, true);

										infinishsector = 0;
//										HostTraceOutputTL(appErrorClass, "HandleTask:FAI Cylinder Achieved!");
									} else {
										infinishsector++;
//										HostTraceOutputTL(appErrorClass, "In Finish Sector Num:%s",DblToStr(infinishsector,0));
									}
//									HostTraceOutputTL(appErrorClass, "In Finish Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//									HostTraceOutputTL(appErrorClass, "In Finish Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
								} else {
									infinishsector = 0;
								}
								break;
//							case ARC:
//								// need to add logic for arc finishes
//								break;
							default:
								break;
						}
						} // from if (tasknotfinished)
					} else if (activetskway == LANDOFFSET) {
						// If it gets here, a landing point has been defined.
						// Don't want to do anything.

//********************************************* Handle AREA / CONTROL waypoint
					} else if ((data.task.waypttypes[activetskway] & AREA) || (data.task.waypttypes[activetskway] & CONTROL)) {
//						HostTraceOutputTL(appErrorClass, "HandleTask:Area / Control Waypoint");
						HandleTaskAutoZoom(tgtrange, data.task.arearadii[activetskway], false); // AREA

						// This converts from True to Magnetic for the comparison
						sect1 = nice_brg(data.task.sectbear1[activetskway] + data.input.deviation.value);
						sect2 = nice_brg(data.task.sectbear2[activetskway] + data.input.deviation.value);
						// Since poibearing is in magnetic, should pass all as magnetic
						if (InSectorTest(sect1, sect2, data.task.arearadii2[activetskway], data.task.arearadii[activetskway], poibearing, poirange)) {
							if (inareactlsector >= LOGGEDPTS) {
								HandleTurn();
								// Reset the Auto Task Zoom
								HandleTaskAutoZoom(0.0, 0.0, true);
								inareactlsector = 0;
							} else {
								inareactlsector++;
//								HostTraceOutputTL(appErrorClass, "In Area/Ctl Sector Num:%s",DblToStr(inareactlsector,0));
							}
//							HostTraceOutputTL(appErrorClass, "In Area/Ctl Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//							HostTraceOutputTL(appErrorClass, "In Area/Ctl Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
						} else {
							inareactlsector = 0;
						}

//********************************************* Handle normal turnpoints in the Task
					} else if ((activetskway != TAKEOFFSET) && (data.config.AATmode != AAT_MTURN_ON)) {
//						HostTraceOutputTL(appErrorClass, "HandleTask:Switching on turntype: |%hd|", (Int16)data.config.turntype);

						switch (data.config.turntype) {
							case FAI:
//								HostTraceOutputTL(appErrorClass, "HandleTask:FAI Waypoint");
								HandleTaskAutoZoom(tgtrange, data.config.turnfairad, false);
								// This converts from True to Magnetic for the comparison
								sect1 = nice_brg(data.task.sectbear1[activetskway] + data.input.deviation.value);
								sect2 = nice_brg(data.task.sectbear2[activetskway] + data.input.deviation.value);
								// Since poibearing is in magnetic, should pass all as magnetic
								if (InSectorTest(sect1, sect2, 0, data.config.turnfairad, poibearing, poirange)) {
									if (inturnsector >= LOGGEDPTS) {
										HandleTurn();
										// Reset the Auto Task Zoom
										HandleTaskAutoZoom(0.0, 0.0, true);
										inturnsector = 0;
//										HostTraceOutputTL(appErrorClass, "HandleTask:FAI Waypoint Achieved!");
									} else {
										inturnsector++;
//										HostTraceOutputTL(appErrorClass, "In Turn Sector Num:%s",DblToStr(inturnsector,0));
									}
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
								} else {
									inturnsector = 0;
								}
								break;
							case CYLINDER:
//								HostTraceOutputTL(appErrorClass, "HandleTask:Cylinder Waypoint");
								HandleTaskAutoZoom(tgtrange, data.config.turncircrad, false);
								if (InSectorTest(0, 0, 0, data.config.turncircrad, poibearing, poirange)) {
									if (inturnsector >= LOGGEDPTS) {
										HandleTurn();
										// Reset the Auto Task Zoom
										HandleTaskAutoZoom(0.0, 0.0, true);
										inturnsector = 0;
//										HostTraceOutputTL(appErrorClass, "HandleTask:Cylinder Waypoint Achieved!");
									} else {
										inturnsector++;
//										HostTraceOutputTL(appErrorClass, "In Turn Sector Num:%s",DblToStr(inturnsector,0));
									}
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
								} else {
									inturnsector = 0;
								}
								break;
							case BOTH:
//								HostTraceOutputTL(appErrorClass, "HandleTask:Both FAI and Cylinder Waypoint");
								if (data.config.turnfairad > data.config.turncircrad) {
									HandleTaskAutoZoom(tgtrange, data.config.turnfairad, false);
								} else {
									HandleTaskAutoZoom(tgtrange, data.config.turncircrad, false);
								}

								// This converts from True to Magnetic for the comparison
								sect1 = nice_brg(data.task.sectbear1[activetskway] + data.input.deviation.value);
								sect2 = nice_brg(data.task.sectbear2[activetskway] + data.input.deviation.value);
								// Since poibearing is in magnetic, should pass all as magnetic
								if ((InSectorTest(sect1, sect2, 0, data.config.turnfairad, poibearing, poirange)) ||
								    (InSectorTest(0, 0, 0, data.config.turncircrad, poibearing, poirange))) {
									if (inturnsector >= LOGGEDPTS) {
										HandleTurn();
										// Reset the Auto Task Zoom
										HandleTaskAutoZoom(0.0, 0.0, true);
										inturnsector = 0;
//										HostTraceOutputTL(appErrorClass, "HandleTask:Both Waypoint Achieved!");
									} else {
										inturnsector++;
//										HostTraceOutputTL(appErrorClass, "In Turn Sector Num:%s",DblToStr(inturnsector,0));
									}
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//									HostTraceOutputTL(appErrorClass, "In Turn Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
								} else {
									inturnsector = 0;
								}
								break;
							default:
								break;
						}
					}

//************************************* Always Check for inside sector of AREA waypoint
					// checking previous waypoint as activetskway is incremented on entry
					z = activetskway-1;
					while (data.task.waypttypes[z] & CONTROL) z--;
					if ((activetskway > 0) && (data.task.waypttypes[z] & AREA)
						&& ((z > lastareasector) || ((data.task.waypttypes[z] & AREAEXIT) == 0))) {

						// update range and bearing to area waypoint
						LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[z], data.task.wayptlons[z], &poirange, &poibearing);
						poibearing = nice_brg(poibearing + data.input.deviation.value);

						// calculate distance to target in area
						LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[z], data.task.targetlons[z], &data.activetask.AATtgtdist);

						// This converts from True to Magnetic for the comparison
						sect1 = nice_brg(data.task.sectbear1[z] + data.input.deviation.value);
						sect2 = nice_brg(data.task.sectbear2[z] + data.input.deviation.value);

						// Since poibearing is in magnetic, should pass all as magnetic
						if (InSectorTest(sect1, sect2, data.task.arearadii2[z], data.task.arearadii[z], poibearing, poirange)) {
//							HostTraceOutputTL(appErrorClass, "in AREA : %s", data.task.wayptnames[z]);

							if (inareasector == -1) {
								// first entry into area
								// calculate max and min distance points in area
								CalcAreaMax(&data.task, z, &data.input.areamaxlat, &data.input.areamaxlon, false);
								CalcAreaMin(&data.task, z, &data.input.areaminlat, &data.input.areaminlon, false);

								// set AATmode to manual turn on
								if (data.config.AATmode & AAT_MTURN) {
//									HostTraceOutputTL(appErrorClass, "AREA : (auto) Manual Turn On %s", DblToStr(inareasector,0));
									data.config.AATmode = AAT_MTURN_ON;
									// set task distances to target
									data.task.distlats[z] = data.task.targetlats[z];
									data.task.distlons[z] = data.task.targetlons[z];
								}
							}

							// calculate distance from previous point to position in area, skipping control points
    							x = z-1;
							while (data.task.waypttypes[x] & CONTROL) x--;
							LatLonToRange(data.task.distlats[x], data.task.distlons[x], data.input.gpslatdbl, data.input.gpslngdbl, &rng1);

							// calculate distance from position in area to next point, skipping control points
							y = activetskway;
							while (data.task.waypttypes[y] & CONTROL) y++;
							LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, data.task.distlats[y], data.task.distlons[y], &rng2);

							// update if task distance increased
							if ((rng1 + rng2) > data.activetask.maxareadist[z]) {
								data.activetask.maxareadist[z] = rng1 + rng2;
								data.activetask.arearng1 = rng1;
								data.activetask.arearng2 = rng2;
								// update max distance point in sector
								data.activetask.distlats[z] = data.input.gpslatdbl;
								data.activetask.distlons[z] = data.input.gpslngdbl;

								// adjust for cylinder radii if required
								distadj = 0.0;
								if (data.task.rulesactive && !data.task.inclcyldist) {
									// prev waypoint
									if ((x == TAKEOFFSET) && (data.config.starttype == CYLINDER)) {
										distadj += data.config.startrad;
									} else if ((x != TAKEOFFSET) && ((data.task.waypttypes[x] & AREA) == 0) && ((data.config.turntype == CYLINDER) || (data.config.turntype == BOTH))) {
										distadj += data.config.turncircrad;
									}
									// next waypoint
									if ((y == LANDOFFSET-1) && (data.config.finishtype == CYLINDER)) {
										distadj += data.config.startrad;
									} else if ((y != LANDOFFSET-1) && ((data.task.waypttypes[y] & AREA) == 0) && ((data.config.turntype == CYLINDER) || (data.config.turntype == BOTH))) {
										distadj += data.config.turncircrad;
									}
								}
//								HostTraceOutputT(appErrorClass, "Area %s", print_distance2(data.activetask.arearng1+data.activetask.arearng2-distadj,1));
//								HostTraceOutputTL(appErrorClass, " - Dist %s", print_distance2(data.task.distances[z]+data.task.distances[y],1));

								// check if maximum distance is greater than target distance and manual turn mode is on
								if (!tgtchg && (data.config.AATmode == AAT_MTURN_ON) && (data.activetask.arearng1+data.activetask.arearng2-distadj > data.task.distances[z]+data.task.distances[y])) {
//									HostTraceOutputTL(appErrorClass, "Max > Target");
									// automatically mode MTP towards max distance point by 5%
									data.task.targetlats[z] = data.input.gpslatdbl + 0.05 * (data.input.areamaxlat - data.input.gpslatdbl);
									data.task.targetlons[z] = data.input.gpslngdbl + 0.05 * (data.input.areamaxlon - data.input.gpslngdbl);;
								}
								tgtchg = false;

								if (data.config.AATmode == AAT_MTURN_ON) {
									// set distances to target position
//									HostTraceOutputTL(appErrorClass, "Target");
									data.task.distlats[z] = data.task.targetlats[z];
									data.task.distlons[z] = data.task.targetlons[z];
								} else {
									// update task distance is not waiting for manual turn
//									HostTraceOutputTL(appErrorClass, "Position");
									data.task.distlats[z] = data.input.gpslatdbl;
									data.task.distlons[z] = data.input.gpslngdbl;
								}

							} else if (tgtchg) {
								// check for target change in AAT
							 	if (data.config.AATmode == AAT_MTURN_ON) {
									// target changed but no increase in scoring distance
//									HostTraceOutputTL(appErrorClass, "Target");
									data.task.distlats[z] = data.task.targetlats[z];
									data.task.distlons[z] = data.task.targetlons[z];
								}
								tgtchg = false;
							}

							if ((data.task.waypttypes[z] & AREAEXIT) && (data.config.AATmode & AAT_MTURN)) {
								// set distances to closest edge of area
//								HostTraceOutputTL(appErrorClass, "Update exit area target %s", DblToStr(data.config.AATmode,0));
								LatLonToBearing(data.task.wayptlats[z], data.task.wayptlons[z], data.input.gpslatdbl, data.input.gpslngdbl, &tempbrg);
								RangeBearingToLatLon(data.task.wayptlats[z], data.task.wayptlons[z], data.task.arearadii[z], tempbrg, &data.task.targetlats[z], &data.task.targetlons[z]);
								data.task.distlats[z] = data.task.targetlats[z];
								data.task.distlons[z] = data.task.targetlons[z];
							}

							// update centre point when in sector
							if ((FrmGetActiveFormID() == form_waypoint_sector) && (selectedTaskWayIdx == z)) {
								// update center to max distance point
								//sectorlat = data.task.distlats[z];
								//sectorlon = data.task.distlons[z];

								// update center to target point
								sectorlat = data.task.targetlats[z];
								sectorlon = data.task.targetlons[z];

								// update center to current position
								//sectorlat = data.input.gpslatdbl;
								//sectorlon = data.input.gpslngdbl;

								// update center to waypoint
								//sectorlat = data.task.wayptlats[z];
								//sectorlon = data.task.wayptlons[z];
							}

							// flag as in sector
							inareasector = z;

						} else {
							// not in area
							if (inareasector > -1) {
								// just left area update last sector for AREAEXIT types
								if (data.task.waypttypes[z] & AREAEXIT) lastareasector = z;
							}
							// reset AATmode
							if (data.config.AATmode & AAT_MTURN) {
								data.config.AATmode = AAT_MTURN;
								if (inareasector > -1) {
//									HostTraceOutputTL(appErrorClass, "Exited area : Manual Turn Off %s", data.task.wayptnames[inareasector]);
									// update task distances achieved in area
									data.task.distlats[z] = data.activetask.distlats[z];
									data.task.distlons[z] = data.activetask.distlons[z];
									PlayTurnSound();
								}
							}
							inareasector = -1;
						}

					} else {
						// not area waypoint
						if (inareasector > -1) {
							// just left area update last sector for AREAEXIT types
							if (data.task.waypttypes[z] & AREAEXIT) lastareasector = z;
						}
						// reset AATmode just in case
						if (data.config.AATmode & AAT_MTURN) {
							data.config.AATmode = AAT_MTURN;
//							HostTraceOutputTL(appErrorClass, "NOT AREA : Manual Turn Off %s", DblToStr(inareasector,0));
							if (inareasector > -1) {
								// update task distances achieved in area
//								HostTraceOutputTL(appErrorClass, "NOT AREA : Copy active dists -> task dists");
								data.task.distlats[inareasector] = data.activetask.distlats[inareasector];
								data.task.distlons[inareasector] = data.activetask.distlons[inareasector];
								PlayTurnSound();
							}
						}
						inareasector = -1;
					}
				}
				break;
//***************************** End of TSKNORMAL

			case TSKNEW:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKNEW");
				tasknotfinished = true;
				StrCopy(data.flight.tskstartutc, "NT");
				StrCopy(data.flight.tskstartdtg, "NT");
				StrCopy(data.flight.tskstoputc, "NT");
				StrCopy(data.flight.tskstopdtg, "NT");
				data.activetask.tskstartsecs = 0;
				data.activetask.tskstopsecs = 0;
				data.flight.startheight = 0.0;
				data.flight.tskdist = 0.0;
				data.flight.tskspeed = 0.0;
				data.flight.tskairspeed = 0.0;
				if (inflight && (activefltindex >= 0)) {
					OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
					data.input.logfltupd = cursecs;
				}
				activetskway = TAKEOFFSET;
				activectlpts = 0;
				selectedTaskWayIdx = TAKEOFFSET;
				HandleTaskAutoZoom(0.0, 0.0, true);
//				HostTraceOutputTL(appErrorClass, "TSKNEW-selectedTaskWayIdx:|%hd|", selectedTaskWayIdx);
				break;
			case TSKREACTIVATE:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKREACTIVATE");
				tasknotfinished = true;
				if (activetskway < TAKEOFFSET) {
					activetskway = TAKEOFFSET;
					activectlpts = 0;
				}
				selectedTaskWayIdx = activetskway;
				HandleTaskAutoZoom(0.0, 0.0, true);
//				HostTraceOutputTL(appErrorClass, "TSKREACTIVATE-selectedTaskWayIdx:|%hd|", selectedTaskWayIdx);
				break;
			case TSKDEACTIVATE:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKDEACTIVATE");
				// Deactivating Active Task
				if (tasknotfinished && (data.task.numwaypts > 0)) {
					StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
					StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
					data.activetask.tskstopsecs = utcsecs;
					data.flight.tskdist = CalcCompletedTskDist();
					data.flight.tskspeed = CalcCompletedTskSpeed();
					data.flight.flttask.ttldist = data.task.ttldist;
					if (activefltindex >= 0) {
						OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
                                                data.input.logfltupd = cursecs;
					}
				}
				tasknotfinished = false;
				activetskway = -1;
				glidingtoheight = ELEVATION;
				HandleTaskAutoZoom(0.0, 0.0, true);
				// clear in use waypoint
//				data.input.distance_to_destination.valid = NOT_VALID;
//				data.input.distance_to_destination.value = 0.0;
//				data.input.bearing_to_destination.valid = NOT_VALID;
//				data.input.bearing_to_destination.value = 000.0;
//				data.input.destination_name[0] = 0;
//				data.input.destination_elev = 0.0;
//				data.input.destination_valid = false;
//				return; //Don't want to recalculate anything
				break;
			case TSKREMWAY:
//				HostTraceOutputTL(appErrorClass, "HandleTask:TSKREMWAY");
//				HostTraceOutputTL(appErrorClass, "     TSKREMWAY:activetskway =|%hd|", activetskway);
//				HostTraceOutputTL(appErrorClass, "     TSKREMWAY:data.task.numwaypts =|%hd|", data.task.numwaypts);
				if (activetskway > (Int16)data.task.numwaypts-1) {
					activetskway = data.task.numwaypts-1;
				}
				return; //Don't want to recalculate anything
				break;
			default:
//				HostTraceOutputTL(appErrorClass, "HandleTask:default");
				break;
		}

//************* TSKNONE
		x = activetskway;
		glidingtoheight = ELEVATION;
		if (tasknotfinished && !taskonhold && (activetskway >= 0)) {
			if (data.config.AATmode == AAT_MTURN_ON) {
				// update range / bearing to target in current area
//				HostTraceOutputTL(appErrorClass, "to area target");
				data.inuseWaypoint.lat = data.task.targetlats[inareasector];
				data.inuseWaypoint.lon = data.task.targetlons[inareasector];
				x = inareasector;
			} else if (!data.config.accuratedistcalc) {
				// update range / bearing to inuseWaypoint
//				HostTraceOutputTL(appErrorClass, "to inusewaypoint");
				data.inuseWaypoint.lat = data.task.targetlats[activetskway];
				data.inuseWaypoint.lon = data.task.targetlons[activetskway];
			} else {
				// update range / bearing to current target point
//				HostTraceOutputTL(appErrorClass, "to target");
				data.inuseWaypoint.lat = data.activetask.tasktgtlat;
				data.inuseWaypoint.lon = data.activetask.tasktgtlon;
			}

			// set destination elevaton
			if (data.task.rulesactive && data.config.fgtostartalt && (x == TAKEOFFSET) && (data.task.maxstartheight > 0.0)) {
				// check if gliding to max start height
				glidingtoheight  = STARTHEIGHT;
				data.inuseWaypoint.elevation =  data.activetask.startheight;
			} else if (data.task.rulesactive && (data.task.finishheight > 0.0) && (x == LANDOFFSET-1)) {
				// check if gliging to min finish height
				glidingtoheight = FINISHHEIGHT;
				data.inuseWaypoint.elevation = data.activetask.finishheight;
			} else if ((data.task.waypttypes[x] & AREA)
					&& (LatLonToRange2(data.task.targetlats[x],data.task.targetlons[x],data.task.wayptlats[x],data.task.wayptlons[x]) > 0.25)) {
				// gliding to area target elevation
				data.inuseWaypoint.elevation = GetTerrainElev(data.inuseWaypoint.lat, data.inuseWaypoint.lon);
				if (!terrainvalid) data.inuseWaypoint.elevation = data.task.elevations[x];
			} else {
				// gliding to normal waypoint elevation
				data.inuseWaypoint.elevation = data.task.elevations[x];
			}
			data.input.destination_elev = data.inuseWaypoint.elevation;

			// update other inuseWaypoint data
			data.input.destination_aalt = 0.0;
			data.inuseWaypoint.type = data.task.waypttypes[x];
			StrCopy(data.inuseWaypoint.name, data.task.wayptnames[x]);
			StrCopy(data.inuseWaypoint.rmks, data.task.remarks[x]);
			data.inuseWaypoint.arearadial1 = data.task.sectbear1[x];
			data.inuseWaypoint.arearadial2 = data.task.sectbear2[x];
			data.inuseWaypoint.arearadius = data.task.arearadii[x];
			data.inuseWaypoint.arearadius2 = data.task.arearadii2[x];
			data.input.destination_valid = true;
		}
		InuseWaypointCalcEvent();

		if ((!taskonhold && (x >= 0)) && tasknotfinished) {
			// update total task distance
			CalcTaskDists(&data.task, false, false, false);

			// update FGA altitudes
			data.activetask.FGAalt = CalcRemainingTskFGA(&data.task, x, data.input.inusealt, MCCurVal);

			// update task stop and TOT seconds
			if (data.activetask.tskstartsecs > 0) {
				StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
				StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
				data.activetask.tskstopsecs = utcsecs;
			}
			data.activetask.TOTsecs = CalcTOT(x, MCCurVal) + data.activetask.tskstopsecs - data.activetask.tskstartsecs;
			if (data.activetask.TOTsecs > 86399) data.activetask.TOTsecs = 86399; // 24 hours

			// Calculate the distance completed and speed
			data.flight.tskdist = CalcCompletedTskDist();
			data.flight.tskspeed = CalcCompletedTskSpeed();
		}

		// adjust task distance and time for manual wayoint change
		if (manchgwpt || manualstart) {
			mantskdist = CalcCompletedTskDist();
			manstartsecs = utcsecs;
			manualstart = false;
			manchgwpt = false;
		}

		// check for min task time or task on hold
		if (taskonhold) {
			StrCopy(timeinfo, "On Hold");
		} else if (data.task.rulesactive && (data.task.mintasktime > 0) && (StrLen(timeinfo) == 0)) {
			if ((data.flight.tskspeed > 0.0) || (data.input.Vxc > 0.0)) {
				// TOT early or late
				StrCopy(timeinfo, "TOT");
				StrCat(timeinfo, CalcTOTvar(data.activetask.TOTsecs/60, data.task.mintasktime));
			} else {
				// no valid speed
				StrCopy(timeinfo, "TOT XX");
			}
		}
	}

//	HostTraceOutputTL(appErrorClass, "Handle Task End");
	return;
}

Int16 FindTaskRecordByName(Char* NameString, MemHandle *output_hand, MemPtr *output_ptr)
{
	Int16 x = 0;
	Int16 nrecs;
	TaskData *tskdata;
	MemHandle tskhand;
	Int16 retval=TSKNOTFOUND;

	nrecs = OpenDBCountRecords(task_db);

	tskhand = MemHandleNew(sizeof(TaskData));
	tskdata = MemHandleLock(tskhand);
	for (x=0; x<nrecs; x++) {
		OpenDBQueryRecord(task_db, x, output_hand, output_ptr);
		MemMove(tskdata, *output_ptr, sizeof(TaskData));
		MemHandleUnlock(*output_hand);

		if (StrCompare(NameString, tskdata->name) == 0) {
			retval = x;
			x = nrecs;
		}
	}
	MemHandleUnlock(tskhand);
	MemHandleFree(tskhand);
	return(retval);
}

// Doesn't matter if bearings are true or magnetic as long as they match
// However, curbrg is NORMALLY magnetic so left and right should be as well
Boolean InSectorTest(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng)
{
	Boolean withinbrg = false;
	Boolean withinrng = false;
	double recipbrg = 0.0;

//	HostTraceOutputTL(appErrorClass, "IST:leftbrg =|%s|", DblToStr(leftbrg, 1));
//	HostTraceOutputTL(appErrorClass, "IST:rightbrg =|%s|", DblToStr(rightbrg, 1));

	// special case to avoid rounding problem
	if ((currng < 0.001) && (inrng < 0.001)) {
		return(true);
	}

	// Determine if currently within the defined range limits
	if (currng >= inrng && currng <= outrng) {
		withinrng = true;
//		HostTraceOutputTL(appErrorClass, "IST:withinrng =|%hd|", (Int16)withinrng);
	} else {
		return(false);
	}

	if (leftbrg != rightbrg) {
		recipbrg = RecipCse(curbrg);
//		HostTraceOutputTL(appErrorClass, "IST:recipbrg =|%s|", DblToStr(recipbrg, 1));

		if (leftbrg > rightbrg) {
			rightbrg += 360.0;
		}
		if ((recipbrg >= leftbrg) && (recipbrg <= rightbrg)) {
			withinbrg = true;
		} else if ((recipbrg+360.0 >= leftbrg) && (recipbrg+360.0 <= rightbrg)) {
			withinbrg = true;
		}
//
	} else {
		withinbrg = true;
//		HostTraceOutputTL(appErrorClass, "IST:withinbrg4 =|%hd|", (Int16)withinbrg);
	}

	return(withinbrg);
}

// Doesn't matter whether bearings are true or magnectic
// as long as they all match.
// However, curbrg is normally magnetic
Boolean ProcessStart(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng)
{
	Boolean curstat = false;
	Boolean nxtstat = false;
	static Boolean withinSector = false;
	Boolean completed = false;
	static Int8 crossedstartline = 0;
	static Int8 instartsector = 0;

	// Since curbrg is magnetic, should pass all as magnetic
	curstat = InSectorTest(leftbrg, rightbrg, inrng, outrng, curbrg, currng);
//	HostTraceOutputTL(appErrorClass, "PSF:curstat =|%hd|", (Int16)curstat);
	if (!curstat) {
		// Since curbrg is magnetic, should pass all as magnetic
		nxtstat = InSectorTest(rightbrg, leftbrg, inrng, outrng, curbrg, currng);
//		HostTraceOutputTL(appErrorClass, "PSF:nxtstat =|%hd|", (Int16)nxtstat);
		instartsector = 0;
		if (!nxtstat) {
			crossedstartline = 0;
		}
	} else {
		crossedstartline = 0;
	}
	if (!curstat && !nxtstat) {
		withinSector = false;
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (curstat && !withinSector) {
		if (instartsector >= LOGGEDPTS) {
			withinSector = true;
			instartsector = 0;
//			HostTraceOutputTL(appErrorClass, "In Start Line Sector!");
		} else {
			instartsector++;
//			HostTraceOutputTL(appErrorClass, "In Start Line Sector Num:%s",DblToStr(instartsector,0));
		}
//		HostTraceOutputTL(appErrorClass, "In Start Line Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "In Start Line Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (nxtstat && withinSector) {
		if (crossedstartline >= LOGGEDPTS) {
			withinSector = false;
			completed = true;
			crossedstartline = 0;
//			HostTraceOutputTL(appErrorClass, "Crossed Start Line!");
		} else {
			crossedstartline++;
//			HostTraceOutputTL(appErrorClass, "Crossed Start Line Num:%s",DblToStr(crossedstartline,0));
		}
//		HostTraceOutputTL(appErrorClass, "Crossed Start Line Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "Crossed Start Line Lon:%s",DblToStr(data.input.gpslngdbl,5));
	}
//	HostTraceOutputTL(appErrorClass, "PSF:completed =|%hd|", (Int16)completed);
//	HostTraceOutputTL(appErrorClass, "PSF:================================");
	return(completed);
}

Boolean ProcessArcStart(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng)
{
	Boolean curstat = false;
	Boolean nxtstat = false;
	static Boolean withinSector = false;
	Boolean completed = false;
	static Int8 crossedarcline = 0;
	static Int8 inarcsector = 0;

	// Since curbrg is magnetic, should pass all as magnetic
	curstat = InSectorTest(leftbrg, rightbrg, outrng, outrng*2, curbrg, currng);
//	HostTraceOutputTL(appErrorClass, "PSF:curstat =|%hd|", (Int16)curstat);
	if (!curstat) {
		// Since curbrg is magnetic, should pass all as magnetic
		nxtstat = InSectorTest(leftbrg, rightbrg, inrng, outrng, curbrg, currng);
//		HostTraceOutputTL(appErrorClass, "PSF:nxtstat =|%hd|", (Int16)nxtstat);
		inarcsector = 0;
		if (!nxtstat) {
			crossedarcline = 0;
		}
	} else {
		crossedarcline = 0;
	}
	if (!curstat && !nxtstat) {
		withinSector = false;
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (curstat && !withinSector) {
		if (inarcsector >= LOGGEDPTS) {
			withinSector = true;
			inarcsector = 0;
//			HostTraceOutputTL(appErrorClass, "In Arc Sector!");
		} else {
			inarcsector++;
//			HostTraceOutputTL(appErrorClass, "In Arc Sector Num:%s",DblToStr(inarcsector,0));
		}
//		HostTraceOutputTL(appErrorClass, "In Arc Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "In Arc Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (nxtstat && withinSector) {
		if (crossedarcline >= LOGGEDPTS) {
			withinSector = false;
			completed = true;
			crossedarcline = 0;
//			HostTraceOutputTL(appErrorClass, "Crossed Arc Line!");
		} else {
			crossedarcline++;
//			HostTraceOutputTL(appErrorClass, "Crossed Arc Line Num:%s",DblToStr(crossedarcline,0));
		}
//		HostTraceOutputTL(appErrorClass, "Crossed Arc Line Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "Crossed Arc Line Lon:%s",DblToStr(data.input.gpslngdbl,5));
	}
//	HostTraceOutputTL(appErrorClass, "PSF:completed =|%hd|", (Int16)completed);
//	HostTraceOutputTL(appErrorClass, "PSF:================================");
	return(completed);
}

Boolean ProcessFinish(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng)
{
	Boolean curstat = false;
	Boolean nxtstat = false;
	static Boolean withinSector = false;
	Boolean completed = false;
	static Int8 crossedfinishline = 0;
	static Int8 infinishsector = 0;

	// Since curbrg is magnetic, should pass all as magnetic
	curstat = InSectorTest(leftbrg, rightbrg, inrng, outrng, curbrg, currng);
//	HostTraceOutputTL(appErrorClass, "PSF:curstat =|%hd|", (Int16)curstat);
	if (!curstat) {
		// Since curbrg is magnetic, should pass all as magnetic
		nxtstat = InSectorTest(rightbrg, leftbrg, inrng, outrng, curbrg, currng);
//		HostTraceOutputTL(appErrorClass, "PSF:nxtstat =|%hd|", (Int16)nxtstat);
		infinishsector = 0;
		if (!nxtstat) {
			crossedfinishline = 0;
		}
	} else {
		crossedfinishline = 0;
	}
	if (!curstat && !nxtstat) {
		withinSector = false;
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (curstat && !withinSector) {
		if (infinishsector >= LOGGEDPTS) {
			withinSector = true;
			infinishsector = 0;
//			HostTraceOutputTL(appErrorClass, "In Finish Line Sector!");
		} else {
			infinishsector++;
//			HostTraceOutputTL(appErrorClass, "In Finish Line Sector Num:%s",DblToStr(infinishsector,0));
		}
//		HostTraceOutputTL(appErrorClass, "In Finish Line Sector Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "In Finish Line Sector Lon:%s",DblToStr(data.input.gpslngdbl,5));
//		HostTraceOutputTL(appErrorClass, "PSF:withinSector =|%hd|", (Int16)withinSector);
	} else if (nxtstat && withinSector) {
		if (crossedfinishline >= LOGGEDPTS) {
			withinSector = false;
			completed = true;
			crossedfinishline = 0;
//			HostTraceOutputTL(appErrorClass, "Crossed Finish Line!");
		} else {
			crossedfinishline++;
//			HostTraceOutputTL(appErrorClass, "Crossed Finish Line Num:%s",DblToStr(crossedfinishline,0));
		}
//		HostTraceOutputTL(appErrorClass, "Crossed Finish Line Lat:%s",DblToStr(data.input.gpslatdbl,5));
//		HostTraceOutputTL(appErrorClass, "Crossed Finish Line Lon:%s",DblToStr(data.input.gpslngdbl,5));
	}
//	HostTraceOutputTL(appErrorClass, "PSF:completed =|%hd|", (Int16)completed);
//	HostTraceOutputTL(appErrorClass, "PSF:================================");
	return(completed);
}

void CalcAreaMax(TaskData *task, UInt16 wptidx, double *maxlat, double *maxlon, Boolean forcecalc)
{	 // Calculate lat/lon for area maximum distance point

	double temprng, tempbrg, rng1, brg1;
	static Int16 lastidx = -1;
	UInt16 x, y;
	double dist1, dist2, maxdist;
	double templat, templon;

//	HostTraceOutputTL(appErrorClass, "-----");
	if (forcecalc) lastidx = -1;

	if ((Int16)wptidx != lastidx) { // do calculation only for a different area
//		HostTraceOutputTL(appErrorClass, "Calc Area Max - idx %s", DblToStr(wptidx,0));
		lastidx = wptidx;

		// find max distance range and bearing
		if (task->sectbear1[wptidx] == task->sectbear2[wptidx]) {
			// area is a circle so max is opposite min
			tempbrg = RecipCse(CalcMinDistBearing(task, wptidx, true));
			temprng = task->arearadii[wptidx];

		} else {

			// check if within sector
			tempbrg = RecipCse(CalcMinDistBearing(task, wptidx, true));
			if (InSectorTest(task->sectbear1[wptidx], task->sectbear2[wptidx], 0, 2, RecipCse(tempbrg), 1)) {
//				HostTraceOutputTL(appErrorClass, "In Sector Max");
				// within sector so radius is max
				temprng = task->arearadii[wptidx];

			} else {
				// check 6 points left,mid,right of sector and min,max radius to find max distance

				// find non-control points
				y = wptidx-1;
				while (task->waypttypes[y] & CONTROL) y--;
				x = wptidx+1;
				while (task->waypttypes[x] & CONTROL) x++;

				// left + max
				brg1 = task->sectbear1[wptidx];
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				maxdist = dist1 + dist2;
				tempbrg = brg1;
				temprng = rng1;

				// right + max
				brg1 = task->sectbear2[wptidx];
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 > maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// left + min
				brg1 = task->sectbear1[wptidx];
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 > maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// right + min
				brg1 = task->sectbear2[wptidx];
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 > maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// mid + max
				brg1 = CalcMinDistBearing(task, wptidx, false);
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 > maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// mid + min
				brg1 = CalcMinDistBearing(task, wptidx, false);
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 > maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}
			}
		}

		// calculate max distance point (bisector of area sector)
		if (temprng < 0.001) temprng = 0.001;
		RangeBearingToLatLon(task->wayptlats[wptidx], task->wayptlons[wptidx], temprng, tempbrg, maxlat, maxlon);
	}
}

void CalcAreaMin(TaskData *task, UInt16 wptidx, double *minlat, double *minlon, Boolean forcecalc)
{	// Calculate lat/lon for area minimum distance point

	double temprng, tempbrg, rng1, brg1;
	static Int16 lastidx = -1;
	UInt16 x,y;
	double dist1, dist2, maxdist;
	double templat, templon;

//	HostTraceOutputTL(appErrorClass, "-----");
	if (forcecalc) lastidx = -1;

	if ((Int16)wptidx != lastidx) { // do calculation only for a different area
//		HostTraceOutputTL(appErrorClass, "Calc Area Min - idx %s", DblToStr(wptidx,0));
		lastidx = wptidx;

		// find prev and next non-control points
		y = wptidx-1;
		while (task->waypttypes[y] & CONTROL) y--;
		x = wptidx+1;
		while (task->waypttypes[x] & CONTROL) x++;

		// find min distance range and bearing
		if (task->sectbear1[wptidx] == task->sectbear2[wptidx]) {
			// area is a circle
			temprng = Sqrt(dist2seg(task->wayptlats[wptidx], task->wayptlons[wptidx], task->distlats[y], task->distlons[y], task->distlats[x], task->distlons[x], &templat, &templon));
			if (temprng > task->arearadii[wptidx]) temprng = task->arearadii[wptidx];
			tempbrg = CalcMinDistBearing(task, wptidx, true);

		} else {
			// area is a sector

			// check if within sector
			tempbrg = CalcMinDistBearing(task, wptidx, true);
			if (InSectorTest(task->sectbear1[wptidx], task->sectbear2[wptidx], 0, 2, RecipCse(tempbrg), 1)) {
//				HostTraceOutputTL(appErrorClass, "In Sector Min");
				// bisector
				temprng = Sqrt(dist2seg(task->wayptlats[wptidx], task->wayptlons[wptidx], task->distlats[y], task->distlons[y], task->distlats[x], task->distlons[x], &templat, &templon));
				if (temprng > task->arearadii[wptidx]) temprng = task->arearadii[wptidx];
				if (temprng < task->arearadii2[wptidx]) temprng = task->arearadii2[wptidx];

			} else {
				// check 6 points left,mid,right of sector and min,max radius
//				HostTraceOutputTL(appErrorClass, "Checking 6 points");

				// left + max
				brg1 = task->sectbear1[wptidx];
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				maxdist = dist1 + dist2;
				tempbrg = brg1;
				temprng = rng1;

				// right + max
				brg1 = task->sectbear2[wptidx];
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 < maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// left + min
				brg1 = task->sectbear1[wptidx];
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 < maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// right + min
				brg1 = task->sectbear2[wptidx];
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 < maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// mid + max
				brg1 = CalcMinDistBearing(task, wptidx, false);
				rng1 = task->arearadii[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 < maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}

				// mid + min
				brg1 = CalcMinDistBearing(task, wptidx, false);
				rng1 = task->arearadii2[wptidx];
				RangeBearingToLatLonLinearInterp(task->wayptlats[wptidx], task->wayptlons[wptidx], rng1, brg1, &templat, &templon);
				LatLonToRange(task->distlats[y], task->distlons[y], templat, templon, &dist1);
				LatLonToRange(task->distlats[x], task->distlons[x], templat, templon, &dist2);
				if (dist1 + dist2 < maxdist) {
					maxdist = dist1 + dist2;
					tempbrg = brg1;
					temprng = rng1;
				}
			}
		}

		// calculate min distance point (bisector of area sector)
		if (temprng < 0.001) temprng = 0.001;
		RangeBearingToLatLon(task->wayptlats[wptidx], task->wayptlons[wptidx], temprng, tempbrg, minlat, minlon);
	}
}

double CalcMinDistBearing(TaskData *task, UInt16 wptidx, Boolean forcecircle)
{
	double brg1, brg2;
	double tempbrg;
	double minbrg;
	Int16 x;

	if ((forcecircle) || (task->sectbear1[wptidx] == task->sectbear2[wptidx])) {
		// area is a circle

		// find previous non-control point
		x = wptidx-1;
		while (task->waypttypes[x] & CONTROL) x--;
		// get the bearing
		LatLonToBearing(task->distlats[x], task->distlons[x], task->wayptlats[wptidx], task->wayptlons[wptidx], &brg1);

		// find next non-control point
		x = wptidx+1;
		while (task->waypttypes[x] & CONTROL) x++;
		// get the bearing
		LatLonToBearing(task->wayptlats[wptidx], task->wayptlons[wptidx], task->distlats[x], task->distlons[x], &brg2);

		// calculate leg bisector bearing
		tempbrg = nice_brg((brg1 + 180.0 + brg2) / 2.0);

		// find correct side for min point bearing
		if (brg1 > brg2) {
			minbrg = RecipCse(tempbrg);
		} else {
			minbrg = tempbrg;
		}

	} else {
		// area is pie shaped, pick the bisector of the pie sector
		brg1 = task->sectbear1[wptidx];
		brg2 = task->sectbear2[wptidx];
		if (brg2 < brg1) brg2 += 360.0;
		minbrg = nice_brg((brg1 + brg2) / 2.0);
	}

	return(minbrg);
}

void CalcBisectors(TaskData *task) {
	Int16 x = 0;
	double nxt, diff, angle;
	Boolean opposite = false;
	static double prv;
	UInt16 wptidx;

	// find first non CONTROL point
	wptidx = 1;
	while (task->waypttypes[wptidx] & CONTROL) wptidx++;
	LatLonToBearing(task->wayptlats[wptidx], task->wayptlons[wptidx], task->wayptlats[0], task->wayptlons[0], &prv);

	for (x=1; x<(Int16)task->numwaypts-1; x++) {
		// check for control points
		if ((task->waypttypes[x] & CONTROL) == 0) {
			// Normal Turnpoint
	 		opposite = false;
//			HostTraceOutputTL(appErrorClass, "Waypt %s", task->wayptnames[x]);

	 		// find next non CONTROL point
 			wptidx = x+1;
			while (task->waypttypes[wptidx] & CONTROL) wptidx++;
			LatLonToBearing(task->wayptlats[x], task->wayptlons[x], task->wayptlats[wptidx], task->wayptlons[wptidx], &nxt);

//			HostTraceOutputTL(appErrorClass, "Waypt %s", task->wayptnames[x]);
//			HostTraceOutputTL(appErrorClass, "From =|%s|", DblToStr(x, 0));
//			HostTraceOutputTL(appErrorClass, "To   =|%s|", DblToStr(wptidx, 0));
//			HostTraceOutputTL(appErrorClass, "prv =|%s|", DblToStr(prv, 1));
//			HostTraceOutputTL(appErrorClass, "nxt =|%s|", DblToStr(nxt, 1));
			if (prv > nxt) {
				diff = prv - nxt;
			} else {
				diff = nxt - prv;
			}

			if (diff < 180.0) {
				diff = 360.0 - diff;
				opposite = true;
			}

			diff = diff/2.0;
			if (prv > nxt) {
				if (opposite) {
					angle = nice_brg(nxt - diff);
				} else {
					angle = nice_brg(diff + nxt);
				}
			} else {
				if (opposite) {
					angle = nice_brg(prv - diff);
				} else {
					angle = nice_brg(diff + prv);
				}
			}
			// check for area turnpoint
			if ((task->waypttypes[x] & AREA) == 0) {
//				HostTraceOutputTL(appErrorClass, "Angle =|%s|", DblToStr(angle, 1));
				// Left and Right Bearings Stored As True
//				HostTraceOutputTL(appErrorClass, "angle =|%s|", DblToStr(angle, 1));
				task->sectbear1[x] = (UInt16)nice_brg((angle - 45.0));
//				HostTraceOutputTL(appErrorClass, "sectbear1 =|%hu|", task->sectbear1[x]);
				task->sectbear2[x] = (UInt16)nice_brg((angle + 45.0));
//				HostTraceOutputTL(appErrorClass, "sectbear2 =|%hu|", task->sectbear2[x]);
			}
			prv = RecipCse(nxt);
		} else {
//			HostTraceOutputTL(appErrorClass, "Control %s", task->wayptnames[x]);
			// find next non control point to get task leg bearing info
 			wptidx = x+1;
			while (task->waypttypes[wptidx] & CONTROL) wptidx++;
			// calc perpendicular line for control points sector
			angle = task->bearings[wptidx];
			task->arearadii[x] = 999.9; // huge radius for perpendicular line
			task->sectbear1[x] = (UInt16)nice_brg((angle - 90.0));
			task->sectbear2[x] = (UInt16)nice_brg((angle + 90.0));
//			HostTraceOutputTL(appErrorClass, "sectbear1 =|%hu|", task->sectbear1[x]);
//			HostTraceOutputTL(appErrorClass, "sectbear2 =|%hu|", task->sectbear2[x]);
		}
	}
//	HostTraceOutputTL(appErrorClass, "--------------------------------");
	return;
}

void CalcStartFinishDirs(TaskData *task)
{
	Int16 TAKEOFFSET=0, LANDOFFSET=0;
	UInt16 wptidx;
	
	if (task->hastakeoff) {
		TAKEOFFSET = 1;
	}

	if (task->haslanding) {
		LANDOFFSET = task->numwaypts - 1;
	} else {
		LANDOFFSET = task->numwaypts;
	}

	if (task->numwaypts > 1) {
		if (data.config.startdirauto) {
			// Calculate the start direction from the direction of the first course
			// Returns bearing in true
			wptidx = TAKEOFFSET+1;
			// find first non-control waypoint after start, and with distance > zero
			while ((task->waypttypes[wptidx] & CONTROL) || (task->distances[wptidx] <= 0.01)) wptidx++;
			LatLonToBearing(task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], task->wayptlats[wptidx], task->wayptlons[wptidx], &data.config.startdir);
			// copy to waypoint radials
			if (data.config.starttype == FAI) {
				task->sectbear1[TAKEOFFSET] = nice_brg(data.config.startdir-45.0);
				task->sectbear2[TAKEOFFSET] = nice_brg(data.config.startdir+45.0);
			} else if ((data.config.starttype == TSKLINE) || (data.config.starttype == ARC)) {
				task->sectbear1[TAKEOFFSET] = nice_brg(data.config.startdir-90.0);
				task->sectbear2[TAKEOFFSET] = nice_brg(data.config.startdir+90.0);
			} else {
				task->sectbear1[TAKEOFFSET] = 0;
				task->sectbear2[TAKEOFFSET] = 0;
			}
		}

		if (data.config.finishdirauto) {
			// Calculate the finish direction from the direction of the last course
			// Returns bearing in true
			wptidx = LANDOFFSET-2;
			// find first non-control waypoint before finish, and with distance > zero
			while (task->waypttypes[wptidx] & CONTROL) wptidx--;
			LatLonToBearing(task->wayptlats[wptidx], task->wayptlons[wptidx], task->wayptlats[LANDOFFSET-1], task->wayptlons[LANDOFFSET-1], &data.config.finishdir);
			// copy to waypoint radials
			if (data.config.finishtype == FAI) {
				task->sectbear1[LANDOFFSET-1] = nice_brg(data.config.finishdir-45.0);
				task->sectbear2[LANDOFFSET-1] = nice_brg(data.config.finishdir+45.0);
			} else if ((data.config.finishtype == TSKLINE)  || (data.config.finishtype == ARC)) {
				task->sectbear1[LANDOFFSET-1] = nice_brg(data.config.finishdir-90.0);
				task->sectbear2[LANDOFFSET-1] = nice_brg(data.config.finishdir+90.0);
			} else {
				task->sectbear1[LANDOFFSET-1] = 0;
				task->sectbear2[LANDOFFSET-1] = 0;
			}
		}

		// calculate ARC start radii and sectors, in case needed
		wptidx = TAKEOFFSET+1;
		while (task->waypttypes[wptidx] & CONTROL) wptidx++;
		data.input.startlat = task->wayptlats[wptidx];
		data.input.startlon = task->wayptlons[wptidx];
		LatLonToRange(data.input.startlat, data.input.startlon, task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], &data.input.startrad);
		data.input.startsect = (180.0 * data.config.startrad) / (PI * data.input.startrad);
		if (data.input.startsect > 90.0) data.input.startsect = 90.0;

/*		// calculate ARC finish radii and sectors, in case needed
		wptidx = LANDOFFSET-2;
		while (task->waypttypes[wptidx] & CONTROL) wptidx--;
		data.input.finishlat = task->wayptlats[wptidx];
		data.input.finishlon = task->wayptlons[wptidx];
		LatLonToRange(data.input.finishlat, data.input.finishlon, task->wayptlats[LANDOFFSET-1], task->wayptlons[LANDOFFSET-1], &data.input.finishrad);
		data.input.finishsect = (180.0 * data.config.finishrad) / (PI * data.input.finishrad);
		if (data.input.finishsect > 180.0) data.input.finishsect = 180.0;
*/		
		// calculate start line / arc / FAI end points
		switch (data.config.starttype) {
			case TSKLINE:
				RangeBearingToLatLon(task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], data.config.startrad, data.config.startdir + 90.0, &data.input.startendlat1, &data.input.startendlon1);
				RangeBearingToLatLon(task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], data.config.startrad, data.config.startdir - 90.0, &data.input.startendlat2, &data.input.startendlon2);
				break;
			case FAI:
				RangeBearingToLatLon(task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], data.config.startrad, data.config.startdir + 135.0, &data.input.startendlat1, &data.input.startendlon1);
				RangeBearingToLatLon(task->wayptlats[TAKEOFFSET], task->wayptlons[TAKEOFFSET], data.config.startrad, data.config.startdir - 135.0, &data.input.startendlat2, &data.input.startendlon2);
				break;
			case ARC:
				RangeBearingToLatLon(data.input.startlat, data.input.startlon, data.input.startrad, data.config.startdir + data.input.startsect + 180, &data.input.startendlat1, &data.input.startendlon1);
				RangeBearingToLatLon(data.input.startlat, data.input.startlon, data.input.startrad, data.config.startdir - data.input.startsect + 180, &data.input.startendlat2, &data.input.startendlon2);
				break;
			default:
				data.input.startendlat1 = task->wayptlats[TAKEOFFSET];
				data.input.startendlon1 = task->wayptlons[TAKEOFFSET];
				data.input.startendlat2 = task->wayptlats[TAKEOFFSET];
				data.input.startendlon2 = task->wayptlons[TAKEOFFSET];
				break;
		}
		// calculate start line / arc / FAI end points
		switch (data.config.finishtype) {
			case TSKLINE:
				RangeBearingToLatLon(task->wayptlats[LANDOFFSET-1], task->wayptlons[LANDOFFSET-1], data.config.finishrad, data.config.finishdir + 90.0, &data.input.finishendlat1, &data.input.finishendlon1);
				RangeBearingToLatLon(task->wayptlats[LANDOFFSET-1], task->wayptlons[LANDOFFSET-1], data.config.finishrad, data.config.finishdir - 90.0, &data.input.finishendlat2, &data.input.finishendlon2);
				break;
			default:
				data.input.finishendlat1 = task->wayptlats[LANDOFFSET-1];
				data.input.finishendlon1 = task->wayptlons[LANDOFFSET-1];
				data.input.finishendlat2 = task->wayptlats[LANDOFFSET-1];
				data.input.finishendlon2 = task->wayptlons[LANDOFFSET-1];
				break;
		}	
//		HostTraceOutputTL(appErrorClass, "start end lat1 %s", DblToStr(data.input.startendlat1,4));
//		HostTraceOutputTL(appErrorClass, "start end lon1 %s", DblToStr(data.input.startendlon1,4));
//		HostTraceOutputTL(appErrorClass, "start end lat2 %s", DblToStr(data.input.startendlat2,4));
//		HostTraceOutputTL(appErrorClass, "start end lon2 %s", DblToStr(data.input.startendlon2,4));
	}
	return;
}

void task_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  tskidx;
	Int16  fieldadd = 0;
	UInt16 strnglen;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	Char tempchar2[10];
	Char *tempcharptr=NULL;
	static Boolean skip = false;
	static Boolean begintask = true;
	static Int16 wayindex = 0;
	Boolean retval;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		begintask = true;
		wayindex = 0;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		return;
	}
//	HostTraceOutputTL(appErrorClass, " serinp|%s|", serinp);
//	HostTraceOutputTL(appErrorClass, " length|%lu|", length);
	StrCopy(rxtype, "Tasks");

	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}


		if (next >= PARSELEN) {
			/* Parsing error start over */
//			HostTraceOutputTL(appErrorClass, "Parsing error-setting next = 0");
			lineoverflow = true;
			next=0;
		}

		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {
				GetField(buf, 0, tempchar);

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else if (StrCompare(tempchar, "TS") == 0) {
//					HostTraceOutputTL(appErrorClass, "TS");
					MemSet(edittsk, sizeof(TaskData), 0);
					GetField(buf, 1, tempchar);
					StrNCopy(edittsk->name, trim(NoComma(tempchar," "),' ',true),12);
					GetField(buf, 2, tempchar);
					edittsk->numwaypts = (UInt16)StrAToI(tempchar);
					GetField(buf, 3, tempchar);
					if (StrChr(tempchar, 'T')) {
						edittsk->hastakeoff = true;
					} else {
						edittsk->hastakeoff = false;
					}
					if (StrChr(tempchar, 'L')) {
						edittsk->haslanding = true;
					} else {
						edittsk->haslanding = false;
					}
					begintask = false;
					wayindex = 0;

				} else if ((StrCompare(tempchar, "TR") == 0) && (!begintask)) {
					// getting the rules active setting
					GetField(buf, 1, tempchar);
					edittsk->rulesactive = (Boolean)StrAToI(tempchar);

					// getting the max start height
					GetField(buf, 2, tempchar);
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
						if (StrChr(tempchar, 'F')) {
							tempchar[strnglen-1] = '\0';
							edittsk->maxstartheight = StrToDbl(tempchar);
						} else {
							tempchar[strnglen-1] = '\0';
							edittsk->maxstartheight = StrToDbl(tempchar)/ALTMETCONST;
						}
					} else {
						edittsk->maxstartheight = 0.0;
					}

					// getting the min time below start height
					GetField(buf, 3, tempchar);
					edittsk->timebelowstart = (Int32)StrAToI(tempchar);

					// getting the min task time
					GetField(buf, 4, tempchar);
					edittsk->mintasktime = (Int32)StrAToI(tempchar);

					// getting the finish height
					GetField(buf, 5, tempchar);
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
						if (StrChr(tempchar, 'F')) {
							tempchar[strnglen-1] = '\0';
							edittsk->finishheight = StrToDbl(tempchar);
						} else {
							tempchar[strnglen-1] = '\0';
							edittsk->finishheight = StrToDbl(tempchar)/ALTMETCONST;
						}
					} else {
						edittsk->finishheight = 0.0;
					}

					// getting the final glide to 1000m below start height setting
					GetField(buf, 6, tempchar);
					edittsk->fgto1000m = (Boolean)StrAToI(tempchar);

					// getting the start altitude reference setting
					GetField(buf, 7, tempchar);
					edittsk->startaltref = (Int8)StrAToI(tempchar);

					// getting the finish altitude reference setting
					GetField(buf, 8, tempchar);
					edittsk->finishaltref = (Int8)StrAToI(tempchar);

					// getting the start local time setting
					GetField(buf, 9, tempchar);
					edittsk->startlocaltime = (Int32)StrAToI(tempchar);

					// getting the start on entry setting
					GetField(buf, 10, tempchar);
					edittsk->startonentry = (Boolean)StrAToI(tempchar);

					// getting the warn before start setting
					GetField(buf, 11, tempchar);
					edittsk->warnbeforestart = (Boolean)StrAToI(tempchar);

					// getting the include cylinder distance setting
					GetField(buf, 12, tempchar);
					edittsk->inclcyldist = (Boolean)StrAToI(tempchar);

					// getting the start warn height
					GetField(buf, 13, tempchar);
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
						if (StrChr(tempchar, 'F')) {
							tempchar[strnglen-1] = '\0';
							edittsk->startwarnheight = StrToDbl(tempchar);
						} else {
							tempchar[strnglen-1] = '\0';
							edittsk->startwarnheight = StrToDbl(tempchar)/ALTMETCONST;
						}
					} else {
						edittsk->startwarnheight = 0.0;
					}

				} else if ((StrCompare(tempchar, "TW") == 0) && (!begintask)) {
//					HostTraceOutputTL(appErrorClass, "TW");

					// Getting waypoint latitude
					GetField(buf, 1, tempchar);
					if ((tempcharptr = StrChr(tempchar, ':')) != NULL) {
						if (StrChr(tempcharptr+1, ':') != NULL) {
							edittsk->wayptlats[wayindex] = DegMinSecColonStringToLatLon(tempchar);
						} else {
							edittsk->wayptlats[wayindex] = DegMinColonStringToLatLon(tempchar);
						}
					} else {
						edittsk->wayptlats[wayindex] = DegMinColonStringToLatLon(tempchar);
					}

					//	Getting waypoint longitude
					GetField(buf, 2, tempchar);
					if ((tempcharptr = StrChr(tempchar, ':')) != NULL) {
						if (StrChr(tempcharptr+1, ':') != NULL) {
							edittsk->wayptlons[wayindex] = DegMinSecColonStringToLatLon(tempchar);
						} else {
							edittsk->wayptlons[wayindex] = DegMinColonStringToLatLon(tempchar);
						}
					} else {
						edittsk->wayptlons[wayindex] = DegMinColonStringToLatLon(tempchar);
					}

					// Getting the waypoint type if it exists
					GetField(buf, 3, tempchar);
					if (!StrChr(tempchar, 'F') && !StrChr(tempchar, 'M')) {
						edittsk->waypttypes[wayindex] = (Int16)StrAToI(tempchar);
						fieldadd = 1;
					} else {
						// If not found default to a generic TURNpoint
						edittsk->waypttypes[wayindex] = 2;
					}

					//	Getting waypoint elevation
					if (fieldadd==1) {
						GetField(buf, 3+fieldadd, tempchar);
					}
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
//						HostTraceOutputTL(appErrorClass, "strnglen:|%hu|", strnglen);
						if (StrChr(tempchar, 'F')) {
							tempchar[strnglen-1] = '\0';
							edittsk->elevations[wayindex] = StrToDbl(tempchar);
						} else {
							tempchar[strnglen-1] = '\0';
							edittsk->elevations[wayindex] = StrToDbl(tempchar)/ALTMETCONST;
						}
					} else {
						edittsk->elevations[wayindex] = 0.0;
					}

					//	Getting waypoint name
					GetField(buf, 4+fieldadd, tempchar);
					StrNCopy(edittsk->wayptnames[wayindex], trim(NoComma(tempchar," "), ' ', true),12);
					edittsk->wayptidxs[wayindex] = FindWayptRecordByName(edittsk->wayptnames[wayindex]);

					//	Getting waypoint remarks
					GetField(buf, 5+fieldadd, tempchar);
					StrNCopy(edittsk->remarks[wayindex], trim(tempchar, ' ', true),12);

					//	Getting the area turnpoint info if it is available
					retval = GetField(buf, 6+fieldadd, tempchar);
					if (retval) {
						MemSet(&tempchar2, sizeof(tempchar2), 0);
//						HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);
						StrNCopy(tempchar2, tempchar, 6);
//						HostTraceOutputTL(appErrorClass, "tempchar2:|%s|", tempchar2);
						edittsk->arearadii[wayindex] = StrToDbl(tempchar2)/1000.0;

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[6];
//						HostTraceOutputTL(appErrorClass, "tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 6);
//						HostTraceOutputTL(appErrorClass, "tempchar2:|%s|", tempchar2);
						edittsk->arearadii2[wayindex] = StrToDbl(tempchar2)/1000.0;

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[12];
//						HostTraceOutputTL(appErrorClass, "1-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
//						HostTraceOutputTL(appErrorClass, "1-tempchar2:|%s|", tempchar2);
						edittsk->sectbear1[wayindex] = (UInt16)StrAToI(tempchar2);
//						HostTraceOutputTL(appErrorClass, "1-waypoint.arearadial1:|%hd|", waypoint.arearadial1);

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[15];
//						HostTraceOutputTL(appErrorClass, "2-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
//						HostTraceOutputTL(appErrorClass, "2-tempchar2:|%s|", tempchar2);
						edittsk->sectbear2[wayindex] = (UInt16)StrAToI(tempchar2);
//						HostTraceOutputTL(appErrorClass, "2-waypoint.arearadial2:|%hd|", waypoint.arearadial2);
					} else {
//						HostTraceOutputTL(appErrorClass, "arearadius not found, setting to 0.0");
						edittsk->arearadii[wayindex] = 0.0;
						edittsk->arearadii2[wayindex] = 0.0;
						edittsk->sectbear1[wayindex] = 0;
						edittsk->sectbear2[wayindex] = 0;
					}

					// set up target lat/lon for area waypoint
					if ((edittsk->wayptlats[wayindex] != INVALID_LAT_LON) && (edittsk->wayptlons[wayindex] != INVALID_LAT_LON)) {
						wayindex++;
					}

				} else if (StrCompare(tempchar, "TE") == 0 && (!begintask) && wayindex != 0) {
//					HostTraceOutputTL(appErrorClass, "TE");
					// Only add the task if the number of waypoints parsed from the TS line
					// matches the number of waypoints parsed
					if ((Int16)edittsk->numwaypts == wayindex) {
						edittsk->aataimtype = AATctr;
						CalcTaskDists(edittsk, true, true, true);
						CalcBisectors(edittsk);
						CalcStartFinishDirs(edittsk);
						tskidx = OpenDBAddRecord(task_db, sizeof(TaskData), edittsk, false);
						rxrecs = tskidx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(tskidx+1,0));
						}
					}
					begintask = true;
					wayindex = 0;
				}
			}
			next=0;
		}
		skip = false;
	}
	return;
}

void OutputTasks()
{
	Char output_char[PARSELEN];
	Char *charP=NULL;
	Char tempchar[30];
	Int16 taskindex=0;
	Int16 wayptindex=0;
	Int16 ntasks;
	MemHandle output_hand;
	MemPtr output_ptr;
	Char dtgstr[15];
	DateType date;
	TaskData *taskdata;

	AllocMem((void *)&taskdata, sizeof(TaskData));

	ntasks = OpenDBCountRecords(task_db);

	if (ntasks > 0) {
		DateSecondsToDate(TimGetSeconds(), &date);
		DateToAscii(date.month, date.day, date.year+1904, dfDMYLong, dtgstr);

		StrCopy(output_char, "** -------------------------------------------------------------");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "**      SOARINGPILOT Version ");
		StrCat(output_char, device.appVersion);
		StrCat(output_char, " Tasks");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "**      Date: ");
		StrCat(output_char, dtgstr);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "** -------------------------------------------------------------");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		// Start at task #1. Don't want to output the active task
		for (taskindex=1; taskindex<ntasks; taskindex++) {
			if ((taskindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, ntasks-1, taskindex, "Tasks");

			OpenDBQueryRecord(task_db, taskindex, &output_hand, &output_ptr);
			MemMove(taskdata, output_ptr, sizeof(TaskData));
			MemHandleUnlock(output_hand);

			StrCopy(output_char, "TS,");
			while ((charP=StrChr(taskdata->name, ','))) {
				*charP = ' ';
			}
			StrCat(output_char, taskdata->name);
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->numwaypts));
			if (taskdata->hastakeoff || taskdata->haslanding) {
				StrCat(output_char, ",");
			}
			if (taskdata->hastakeoff) StrCat(output_char, "T");
			if (taskdata->haslanding) StrCat(output_char, "L");
			StrCatEOL(output_char, data.config.xfertype);
			TxData(output_char, data.config.xfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);

			// task rules
			StrCopy(output_char, "TR");
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->rulesactive));
			StrCat(output_char, ",");
			//StrCat(output_char, DblToStr(taskdata->maxstartheight,2));
			StrCat(output_char,StrIToA(tempchar, (Int32)pround(taskdata->maxstartheight, 0)));
			StrCat(output_char, "F");
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, taskdata->timebelowstart));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, taskdata->mintasktime));
			StrCat(output_char, ",");
			//StrCat(output_char, DblToStr(taskdata->finishheight,2));
			StrCat(output_char,StrIToA(tempchar, (Int32)pround(taskdata->finishheight, 0)));
			StrCat(output_char, "F");
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->fgto1000m));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->startaltref));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->finishaltref));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->startlocaltime));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->startonentry));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->warnbeforestart));
			StrCat(output_char, ",");
			StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->inclcyldist));
			StrCat(output_char, ",");
			StrCat(output_char,StrIToA(tempchar, (Int32)pround(taskdata->startwarnheight, 0)));
			StrCat(output_char, "F");

			StrCatEOL(output_char, data.config.xfertype);
			TxData(output_char, data.config.xfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);

			for (wayptindex=0; wayptindex<(Int16)taskdata->numwaypts; wayptindex++) {
				StrCopy(output_char, "TW,");

				LLToStringDMS(taskdata->wayptlats[wayptindex], tempchar, ISLAT);
				StrCat(output_char, tempchar);
				StrCat(output_char, ",");

				LLToStringDMS(taskdata->wayptlons[wayptindex], tempchar, ISLON);
				StrCat(output_char, tempchar);
				StrCat(output_char, ",");

				StrCat(output_char, StrIToA(tempchar, (Int32)taskdata->waypttypes[wayptindex]));
				StrCat(output_char, ",");

				StrIToA(tempchar, (Int32)pround(taskdata->elevations[wayptindex], 0));
				StrCat(output_char, tempchar);
				StrCat(output_char, "F");
				StrCat(output_char, ",");

				while ((charP=StrChr(taskdata->wayptnames[wayptindex], ','))) {
					*charP = ' ';
				}
				StrCat(output_char, taskdata->wayptnames[wayptindex]);
				StrCat(output_char, ",");

				while ((charP=StrChr(taskdata->remarks[wayptindex], ','))) {
					*charP = ' ';
				}
				StrCat(output_char, taskdata->remarks[wayptindex]);
				// Output the AREA turnpoint information
				if (taskdata->waypttypes[wayptindex] & AREA) {
					StrCat(output_char, ",");
					StrCat(output_char, leftpad(DblToStr2(taskdata->arearadii[wayptindex], 3, false), '0', 6));
					StrCat(output_char, leftpad(DblToStr2(taskdata->arearadii2[wayptindex], 3, false), '0', 6));
					StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)taskdata->sectbear1[wayptindex]), '0', 3));
					StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)taskdata->sectbear2[wayptindex]), '0', 3));
				}
				StrCatEOL(output_char, data.config.xfertype);
				TxData(output_char, data.config.xfertype);
//				HostTraceOutputTL(appErrorClass, "%s", output_char);
			}
			StrCopy(output_char, "TE");
			StrCatEOL(output_char, data.config.xfertype);
			TxData(output_char, data.config.xfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);
		}
	}

	FreeMem((void *)&taskdata);
	return;
}

Int8 CheckForTooFew() {
	Int8 ReturnResp = TFOKUP;

//	HostTraceOutputTL(appErrorClass, "closeupdate:taskIndex=|%hd|", taskIndex);
	if (FrmGetActiveFormID() == form_set_task) {
		StrCopy(tsk->name, NoComma(field_get_str(form_set_task_name)," "));
		OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
	}
	ReturnResp = TFOKUP;

	return(ReturnResp);
}

double CalcCompletedTskDist()
{
	double curtskdist = 0.0;

	// use new logic from CalcRemainingTskFGA
	curtskdist = data.task.ttldist - data.activetask.FGAdist;

	return(curtskdist);
}

double CalcCompletedTskSpeed()
{
	double curtskspeed = 0.0;

//	Calculate Task Speed
	if ((data.activetask.tskstopsecs > data.activetask.tskstartsecs) && (data.activetask.tskstopsecs > manstartsecs)) {

		// check for manual next waypoint
		if ((mantskdist > 0.0) || (manstartsecs > 0)) {
			// adjust speed since manual adjustment
			if (data.flight.tskdist > mantskdist) {
				curtskspeed = (data.flight.tskdist - mantskdist) / ((double)(data.activetask.tskstopsecs - manstartsecs)/3600.0);;
			}
		} else {
			// no manual change to waypoint
			curtskspeed = data.flight.tskdist / ((double)(data.activetask.tskstopsecs - data.activetask.tskstartsecs)/3600.0);
		}
//		HostTraceOutputTL(appErrorClass, "CalcCompletedTskSpeed:|%s|", print_horizontal_speed2(curtskspeed,1));
	}
	
	return(curtskspeed);
}

double CalcRemainingTskFGA(TaskData *tsk, Int16 wptidx, double startalt, double MCVal)
{
	double totalaltloss = 0.0;
	double remaltloss = 0.0;
	double secondaltloss = 0.0;
	Int16 TAKEOFFSET, LANDOFFSET;
	double poiBrg, poiRng;
	Int16 x,y;
	double firstaltloss = 0.0;
	double WaySTF;
	double disttogo = 0.0;

//	HostTraceOutputTL(appErrorClass, "Inside CalcRemainingTskFGA");
	if (tsk->hastakeoff) {
		TAKEOFFSET = 1;
	} else {
		TAKEOFFSET = 0;
	}
	if (tsk->haslanding) {
		LANDOFFSET = tsk->numwaypts - 1;
	} else {
		LANDOFFSET = tsk->numwaypts;
	}

	// calculate distance to next waypoint (skipping control points)
	y = wptidx;
	while (tsk->waypttypes[y] & CONTROL) y++;
	if (data.config.accuratedistcalc) {// && (data.config.earthmodel != EMSPHERE)) {
		// accurate range calculation
		if (wptidx == activetskway) {
			LatLonToRangeBearingEll(data.input.gpslatdbl, data.input.gpslngdbl, data.activetask.tasktgtlat, data.activetask.tasktgtlon, &poiRng, &poiBrg);
		} else {
			LatLonToRangeBearingEll(data.input.gpslatdbl, data.input.gpslngdbl, tsk->targetlats[y], tsk->targetlons[y], &poiRng, &poiBrg);
		}
	} else {
		// faster approximate range calculation
		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, tsk->targetlats[y], tsk->targetlons[y], &poiRng, &poiBrg);
	}

	// find distance to current waypoint (wptidx)
	disttogo = poiRng;
	if (((tsk->waypttypes[y] & AREA) == 0) && (!tsk->rulesactive || !tsk->inclcyldist)) {
		// adjust for cylinder distance if required
		if ((wptidx <= TAKEOFFSET) && (data.config.starttype == CYLINDER)) {
			disttogo -= data.config.startrad;
		} else if ((wptidx == LANDOFFSET-1) && (data.config.finishtype == CYLINDER)) {
			disttogo -= data.config.finishrad;
		} else if ((wptidx > TAKEOFFSET) && (wptidx < LANDOFFSET-1) && ((data.config.turntype == CYLINDER) || (data.config.turntype == BOTH))) {
			disttogo -= data.config.turncircrad;
		}
	}
	if (disttogo > tsk->distances[y]) disttogo = tsk->distances[y];

	// calculate altitude required
	CalcSTFSpdAlt2(MCVal, poiRng, poiBrg, &WaySTF, &firstaltloss);
	data.activetask.alts[y] = firstaltloss;
	data.activetask.stfs[y] = WaySTF;
	data.activetask.hwinds[y] = calcstfhwind;
//	HostTraceOutputTL(appErrorClass, "    firstaltloss-|%s|", DblToStr(firstaltloss, 1));

	if (y < LANDOFFSET) {
		remaltloss = data.activetask.finishheight;
//		HostTraceOutputTL(appErrorClass, "  finish elev-|%s|", print_altitude(remaltloss));
		for (x=LANDOFFSET-1; x > y; x--) {
			CalcSTFSpdAlt2(MCVal, tsk->distances[x], tsk->bearings[x], &WaySTF, &secondaltloss);
			remaltloss += secondaltloss;
			data.activetask.alts[x] = secondaltloss;
			data.activetask.stfs[x] = WaySTF;
			data.activetask.hwinds[x] = calcstfhwind;

			disttogo += tsk->distances[x];
//			HostTraceOutputTL(appErrorClass, "x-|%hu|", x);
//			HostTraceOutputTL(appErrorClass, "  tsk->wayptnames[x]-|%s|", tsk->wayptnames[x]);
//			HostTraceOutputTL(appErrorClass, "  tsk->distances[x] -|%s|", DblToStr(tsk->distances[x], 1));
//			HostTraceOutputTL(appErrorClass, "  secondaltloss-|%s|", DblToStr(secondaltloss, 1));
//			HostTraceOutputTL(appErrorClass, "  remaltloss-|%s|", DblToStr(remaltloss, 1));
			// Step #4
			if (remaltloss < tsk->elevations[x-1]) {
				remaltloss = tsk->elevations[x-1];
			}
//			HostTraceOutputTL(appErrorClass, "--------------------------");
		}

		totalaltloss = remaltloss + firstaltloss;

		// Step #2 & #6 happen in ConvertAltType for a REQALT
		totalaltloss = ConvertAltType(0, startalt, true, REQALT, (firstaltloss+remaltloss));

//		HostTraceOutputTL(appErrorClass, "totalaltloss1-|%s|", DblToStr(totalaltloss, 1));
		// If in Required Altitude mode, add on the wayptData altitude value to get the
		// total required altitude around the turnpoint.  If in Arrival or Delta Altitude mode,
		// the value returned from the second CalcSTFSpdAlt call is the actual FGA altitude.
		// If the current waypoint/turnpoint is or matches the Home/Finish waypoint's Lat/Lon,
		// fgatpalt2 will simply be the wayptData.alt value.
		if (data.config.alttype != REQALT) {
			totalaltloss = ConvertAltType(0, startalt, true, data.config.alttype, (firstaltloss+remaltloss));
//			HostTraceOutputTL(appErrorClass, "totalaltloss2-|%s|", DblToStr(totalaltloss, 1));
		}
	}
//	HostTraceOutputTL(appErrorClass, "totalaltloss-|%s|", DblToStr(totalaltloss, 1));
	// update task distance to go
	if (disttogo > data.task.ttldist) disttogo = data.task.ttldist;
	if (disttogo < 0.0) disttogo = 0.0;
	data.activetask.FGAdist = disttogo;

//	HostTraceOutputTL(appErrorClass, "==========================");
	return(totalaltloss);
}

void HandleTaskAutoZoom(double range, double radius, Boolean reset)
{
	// do until no more changes - immediate zoom in
	// with protection from looping!
	Int8 ct = 0;

	while (HandleTaskAutoZoom2(range, radius, reset) && (ct < MAXMAPSCALEIDX)) {
//		HostTraceOutputTL(appErrorClass, "ct %s", DblToStr(ct,0));
		ct++;
	}
	return;
}

// new method - based on range to target point
Boolean HandleTaskAutoZoom2(double range, double radius, Boolean reset)
{
	static double savemapscale = 0.0;
	static Int8 tempscale = 100;
	Boolean zoomchg = false;
	
	zoommapscale = 999.9;
	if (data.logger.thermal == CRUISE && data.config.tskzoom) {
//		HostTraceOutputTL(appErrorClass, "Auto Zoom Range %s", print_distance2(range,1));
//		HostTraceOutputTL(appErrorClass, "Manual Zoom Change %s", DblToStr(manualzoomchg,0));

		// reset auto zoom, revert to saved mapscale
		if (reset) {
//			HostTraceOutputTL(appErrorClass, "Zoom Reset");
			if (savemapscale != 0.0) {
				curmapscale = savemapscale;
				actualmapscale = curmapscale * data.input.disconst;
			}
			savemapscale = 0.0;
			tempscale = 100;
			manualzoomchg = false;
			data.input.logpollint = data.config.slowlogint;
			return(false);

		} else if (!manualzoomchg) {

			// zoom out if target going off screen
			if (savemapscale != 0.0) {
				if (range > (curmapscale / data.input.disconst * ((tempscale > 2) ? 1.00 : 0.67))) {
					tempscale = FindNextMapScale(&actualmapscale, false);
					zoommapscale = actualmapscale / data.input.disconst;
					zoomchg = true;
//					HostTraceOutputTL(appErrorClass, "Zoom Out");
				}
			}

			// zoom in if target is half way down the screen
			if (tempscale > (data.config.disunits==METRIC?3:2) ) {
				if (range < (curmapscale / data.input.disconst * ((tempscale > 2) ? 0.50 : 0.33))) {
					tempscale = FindNextMapScale(&actualmapscale, true);
					zoommapscale = actualmapscale / data.input.disconst;
					zoomchg = true;
//					HostTraceOutputTL(appErrorClass, "Zoom In");
				}
			}

			// change zoom
			if (zoomchg) {
				if (zoommapscale < curmapscale) {
					// zoom in
					if (savemapscale == 0.0) {
						// save map scale before first zoom
						savemapscale = curmapscale;
						tempscale = data.config.mapscaleidx;
					}
					curmapscale = zoommapscale;
					actualmapscale = curmapscale * data.input.disconst;
//					HostTraceOutputTL(appErrorClass, "Zoom In  : %s", DblToStr(tempscale,0));
				} else 	if (zoommapscale < savemapscale) {
					// zoom out
					curmapscale = zoommapscale;
					actualmapscale = curmapscale * data.input.disconst;
//					HostTraceOutputTL(appErrorClass, "Zoom Out : %s", DblToStr(tempscale,0));

				}
				if (zoommapscale >= savemapscale) {
					// zoomed out to original scale
					manualzoomchg = false;
					curmapscale = savemapscale;
					actualmapscale = curmapscale * data.input.disconst;
					savemapscale = 0.0;
					tempscale = 100;
//					HostTraceOutputTL(appErrorClass, "Zoom Out Reset");
				}
			}
		}
//		HostTraceOutputTL(appErrorClass, "Zoom act %s", DblToStr(curmapscale,1));

//		// Change the logger interval to fast if getting close to the task point
		if (range > radius + 0.5) {
			// more than 0.5nm away, so set to slow logging
			data.input.logpollint = data.config.slowlogint;
		} else {
			data.input.logpollint = data.config.fastlogint;
		}
	}

	return(zoomchg);
}


void task_start_alert(Boolean restart)
{
	Char tempchar[100];
	Char tempchar1[10];
	DateTimeType strtdt;
	UInt32 startsecs;
	Int16 nextwpt;
	
	// automatically start task first time
	if (!restart) {
		starttask = true;
		return;
	}

 	// check if re-starts are allowed
	if (restart && !data.config.tskrestarts) return;

	// check for invalid start
	if (abovemaxstartheight || starttooearly) {
		// allow normal invalid start dialog to handle restart
		starttask = true;
		return;
	}

	// check for already open re-start dialog
	if ((suaalert->alerttype < TASKSTART_ALERT) || (data.application.form_id != form_genalert)) {
//		HostTraceOutputTL(appErrorClass, "Playing NO GPS Sound");
		PlayNoGPSSound();
		// check for control points
		nextwpt = (data.task.hastakeoff?1:0)+1;
		while (data.task.waypttypes[nextwpt] & CONTROL) nextwpt++;
		// record time warned
		if (!data.config.autostart && (activetskway > nextwpt)) {
			// never auto dismiss if 1st waypoint reached
			warning_time = (0xffffffff - data.config.autodismisstime - 1);
			allowgenalerttap = false;
		} else {
			// set warned time as normal
			warning_time = cursecs;
			allowgenalerttap = true;
		}
	}

	// set up alert window data
	if (restart) {
		StrCopy(suaalert->title, "Task Re-Start");
		StrCopy(tempchar, "\n   Re-Start the Task?\n\n   ");
	} else {
		StrCopy(suaalert->title, "Task Start");
		StrCopy(tempchar, "\n   Start the Task?\n\n   ");
	}

	// time
	StrCat(tempchar, "Start Time:       ");
	StringToDateAndTime(start_dtg, start_utc, &strtdt);
	startsecs = TimDateTimeToSeconds(&strtdt);
	SecondsToDateOrTimeString(startsecs, tempchar1, 1, (Int32)data.config.timezone);
	StrCat(tempchar, tempchar1);
	StrCat(tempchar, "\n   ");
	// height
	StrCat(tempchar, "Start Height:   ");
	StrCat(tempchar, print_altitude(start_height));
	StrCat(tempchar, " ");
	StrCat(tempchar, data.input.alttext);
	StrCopy(suaalert->displaytext, tempchar);

	// alert data fields
	suaalert->alerttype = TASKSTART_ALERT;
	suaalert->priority = RESTARTTASK;
	suaalert->alertidx = -1;
	suaalert->numbtns = 2;
	StrCopy(suaalert->btn0text,"Yes");
	StrCopy(suaalert->btn1text,"No");

	// clear last response
	suaalertret->valid = false;
	suaalertret->btnselected = -1;

	// popup alert window
	HandleWaitDialogWin(1);
	return;
}

void task_invalid_start_alert(Boolean overstartheight, Boolean earlystart, Boolean invalidstart, Boolean gotostart)
{
	Char tempchar[150];
	Char tempchar2[10];
	Int16 TAKEOFFSET = 0;

	// check for already open re-start dialog
	if ((suaalert->alerttype < TASKSTART_ALERT) || (data.application.form_id != form_genalert)) {
//		HostTraceOutputTL(appErrorClass, "Playing NO GPS Sound");
		PlayNoGPSSound();
		// log time warned
		warning_time = cursecs;
	}

	//  don't repeat goto start wanring
	if (gotostart && (cursecs < warnedgotostart)) return;

	// set up alert window data
	if (invalidstart) {
		StrCopy(suaalert->title, "Invalid Task Start");
	} else if (gotostart) {
		StrCopy(suaalert->title, "Goto Start Warning");
	} else {
		StrCopy(suaalert->title, "Start Height Warning");
	}
	if (data.task.hastakeoff) TAKEOFFSET = 1;

	if (overstartheight) {
//		HostTraceOutputTL(appErrorClass, "Above Start Height");
		StrCopy(tempchar, "\n   Above Start Height of\n\n   ");
		if (data.task.startaltref == AGL) {
			StrCat(tempchar, print_altitude(data.activetask.startheight - data.task.elevations[TAKEOFFSET]));
		} else {
			StrCat(tempchar, print_altitude(data.activetask.startheight));
		}
		StrCat(tempchar, " ");
		StrCat(tempchar, data.input.alttext);
		StrCat(tempchar, " ");
		switch (data.task.startaltref) {
			case MSL:
				StrCat(tempchar, "MSL");
				break;
			case AGL:
				StrCat(tempchar, "above start");
				break;
			default:
				break;
		}
		startheightwarned2 = true;
		startheightwarned1 = true;
	} else if (earlystart) {
		if (gotostart) {
//			HostTraceOutputTL(appErrorClass, "Goto Start");
			StrCopy(tempchar, "\n   ! GOTO START !\n\n   ");
		} else {
//			HostTraceOutputTL(appErrorClass, "Early Start");
			StrCopy(tempchar, "\n   Start Too Early\n\n   ");
		}
		if (gotostart) {
			StrCat(tempchar, "Start time           ");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(tsk->startlocaltime/60,0));
			StrCat(tempchar, Right(tempchar2,2));
			StrCat(tempchar, ":");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(tsk->startlocaltime%60,0));
			StrCat(tempchar, Right(tempchar2,2));
			StrCat(tempchar, "\n");

			StrCat(tempchar, "   Distance               ");
			StrCat(tempchar, print_distance2(startrange, 1));
			StrCat(tempchar, " ");
			StrCat(tempchar, data.input.distext);
			StrCat(tempchar, "\n");

			StrCat(tempchar, "   Speed to Fly       ");
			if ((speedtostart < 0.0) || (speedtostart > 150.0)) {
				StrCat(tempchar, "Max");
			} else {
				StrCat(tempchar, print_horizontal_speed2(speedtostart, 0));
				StrCat(tempchar, " ");
				StrCat(tempchar, data.input.spdtext);
			}
		} else {
			StrCat(tempchar, "Earliest start   ");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(tsk->startlocaltime/60,0));
			StrCat(tempchar, Right(tempchar2,2));
			StrCat(tempchar, ":");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(tsk->startlocaltime%60,0));
			StrCat(tempchar, Right(tempchar2,2));
			StrCat(tempchar, "\n");

			StrCat(tempchar, "   Local Time           ");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(((Int32)(cursecs%86400)/3600)%24,0));
			StrCat(tempchar, Right(tempchar2,2));
			StrCat(tempchar, ":");
			StrCopy(tempchar2, "00");
			StrCat(tempchar2, DblToStr(((Int32)(cursecs%86400)/60)%60,0));
			StrCat(tempchar, Right(tempchar2,2));
		}
	} else if (invalidstart) {
		StrCopy(tempchar, "\n   Not Below Start Height\n   for  ");
		StrCat(tempchar, DblToStr(data.task.timebelowstart,0));
		StrCat(tempchar, "  secs\n\n   ");
		StrCat(tempchar, DblToStr(data.task.timebelowstart - (cursecs - abovestartheighttime) ,0));
		StrCat(tempchar, "  secs remain");
	} else {
		StrCopy(tempchar, "\n   Less than  ");
		StrCat(tempchar, print_altitude(data.task.startwarnheight));
		StrCat(tempchar, " ");
		StrCat(tempchar, data.input.alttext);
		StrCat(tempchar, "\n\n   Below Start Height");
		startheightwarned1 = true;
	}
	StrCopy(suaalert->displaytext, tempchar);

	// alert data fields
	suaalert->alerttype = TASKSTART_ALERT;
	if (gotostart) {
		suaalert->priority = GOTOSTART;
	} else {
		suaalert->priority = INVALIDSTART;
	}
	suaalert->alertidx = -1;
	if (invalidstart) {
		suaalert->numbtns = 2;
		StrCopy(suaalert->btn0text,"Start");
		StrCopy(suaalert->btn1text,"Cancel");
	} else {
		suaalert->numbtns = 1;
		StrCopy(suaalert->btn0text,"OK");
	}

	// clear response
	suaalertret->valid = false;
	suaalertret->btnselected = -1;

	// popup alert window
	allowgenalerttap = true;
	HandleWaitDialogWin(1);
}

// copy waypoint edit routine to end a waypoint in a task after it is entered in the task
Boolean form_task_waypt_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormType *pfrm = FrmGetActiveForm();
	Char tempchar[30];
	Int16 tskidx = taskIndex;
	Int16 wptidx = selectedTaskWayIdx;
	Boolean arearadial1mag = false;
	Boolean arearadial2mag = false;
	static Boolean isctlpt;
	static Boolean gotosector = false;
	static Int16 TAKEOFFSET;
	static Int16 LANDOFFSET;
	Int8 turntype;
	double radius;
	UInt16 waypttype = 0;
	
	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:

//			HostTraceOutputTL(appErrorClass, "-------------");
//			HostTraceOutputTL(appErrorClass, "Task Idx %s",DblToStr(tskidx,0));
//			HostTraceOutputTL(appErrorClass, "Task Way %s",DblToStr(selectedTaskWayIdx,0));

			FrmDrawForm(pfrm);

			// set title
			if (settaskreadonly) {
				frm_set_title("Task Waypoint");
			} else if (taskIndex == 0) {
				frm_set_title("Active Task Waypoint Edit");
			} else {
				frm_set_title("Task Waypoint Edit");
			}

			TAKEOFFSET = 0;
			if (edittsk->hastakeoff) TAKEOFFSET++;
			LANDOFFSET = edittsk->numwaypts-1;
			if (edittsk->haslanding) LANDOFFSET--;

			if (settaskreadonly) {
				ctl_set_label(form_task_waypt_save, "Back");
				ctl_set_visible(form_task_waypt_quit, false);
//				ctl_set_visible(form_task_waypt_delete, false);
			}

			// load data back into fields from temp waypoint used for view sector
			if (gotosector) {
//				HostTraceOutputTL(appErrorClass, "Enter Task Waypt - Load from Temp Waypt");
				gotosector = false;
				MemMove(edittsk, temptsk, sizeof(TaskData));
			}

			field_set_value(form_task_waypt_taskname, edittsk->name);
			field_set_value(form_task_waypt_name, edittsk->wayptnames[wptidx]);
			field_set_value(form_task_waypt_distance, print_distance2(edittsk->distances[wptidx],1));
			field_set_value(form_task_waypt_elev, print_altitude(edittsk->elevations[wptidx]));
			field_set_value(form_task_waypt_bearing, print_direction2(edittsk->bearings[wptidx]));
			WinDrawChars(data.input.distext, 2, 69, 39);
			WinDrawChars(data.input.alttext, 2, 147, 27);

			// set turnpoint type
			ctl_set_enable(form_task_waypt_typeS, false);
			ctl_set_enable(form_task_waypt_typeT, false);
			ctl_set_enable(form_task_waypt_typeF, false);
			waypttype |= AREA;
			waypttype |= AIRPORT;
			waypttype |= LAND;
			waypttype |= AIRLAND;
			waypttype |= CONTROL;
			waypttype |= AREAEXIT;
			edittsk->waypttypes[wptidx] = edittsk->waypttypes[wptidx] & waypttype;
			if (edittsk->waypttypes[wptidx] & CONTROL) {
				isctlpt = true;
				WinDrawChars("Control", 7, 125, 140);
				WinDrawChars("Point", 5, 125, 149);
				ctl_set_enable(form_task_waypt_typeAR, false); // disable AREA button
				ctl_set_visible(form_task_waypt_sector, false); // sector button not visible
			} else {
				isctlpt = false;
				if (settaskreadonly) {
					ctl_set_visible(form_task_waypt_sector, (edittsk->waypttypes[wptidx] & AREA) != 0);
				} else {
					ctl_set_visible(form_task_waypt_sector, true);
				}
				if (wptidx == TAKEOFFSET) edittsk->waypttypes[wptidx] |= START;
				if (wptidx == LANDOFFSET) edittsk->waypttypes[wptidx] |= FINISH;
				if ((wptidx > TAKEOFFSET) && (wptidx < LANDOFFSET) && ((edittsk->waypttypes[wptidx] & AREA) == 0)) {
					edittsk->waypttypes[wptidx] |= TURN;
				}
			}
			if (edittsk->waypttypes[wptidx] & AIRPORT) ctl_set_value(form_task_waypt_typeA, true);
			if (edittsk->waypttypes[wptidx] & TURN) ctl_set_value(form_task_waypt_typeT, true);
			if (edittsk->waypttypes[wptidx] & LAND)	ctl_set_value(form_task_waypt_typeL, true);
			if (edittsk->waypttypes[wptidx] & START) ctl_set_value(form_task_waypt_typeS, true);
			if (edittsk->waypttypes[wptidx] & FINISH) ctl_set_value(form_task_waypt_typeF, true);
//			if (edittsk->waypttypes[wptidx] & MARK) ctl_set_value(form_task_waypt_typeM, true);
//			if (edittsk->waypttypes[wptidx] & THRML) ctl_set_value(form_task_waypt_typeTH, true);
//			if (edittsk->waypttypes[wptidx] & HOME) ctl_set_value(form_task_waypt_typeH, true);
//			ctl_set_enable(form_task_waypt_typeH, false); // disable HOME button
//			if (edittsk->waypttypes[wptidx] & AIRLAND) ctl_set_value(form_task_waypt_typeAL, true);
//			ctl_set_enable(form_task_waypt_typeRF, false); // disable REFWPT button
//			ctl_set_enable(form_task_waypt_typeTH, false); // disable THRML button

			// sector type
			if (edittsk->waypttypes[wptidx] & AREA) {
				ctl_set_value(form_task_waypt_typeAR, true);
				if (edittsk->waypttypes[wptidx] & AREAEXIT) {
					field_set_value(form_task_waypt_turntype, "EXIT AREA");
				} else {
					field_set_value(form_task_waypt_turntype, "AREA");
				}
				field_set_enable(form_task_waypt_arearadial1, true);
				ctl_set_enable(form_task_waypt_arearadial1m, true);
				ctl_set_enable(form_task_waypt_arearadial1t, true);
				if (arearadial1mag) {
					ctl_set_value(form_task_waypt_arearadial1m, true);
					ctl_set_value(form_task_waypt_arearadial1t, false);
					field_set_value(form_task_waypt_arearadial1,print_direction2(nice_brg(((double)edittsk->sectbear1[wptidx])+data.input.deviation.value)));
				} else {
					ctl_set_value(form_task_waypt_arearadial1m, false);
					ctl_set_value(form_task_waypt_arearadial1t, true);
					field_set_value(form_task_waypt_arearadial1,print_direction2((double)edittsk->sectbear1[wptidx]));
				}
				field_set_enable(form_task_waypt_arearadial2, true);
				ctl_set_enable(form_task_waypt_arearadial2m, true);
				ctl_set_enable(form_task_waypt_arearadial2t, true);
				if (arearadial2mag) {
					ctl_set_value(form_task_waypt_arearadial2m, true);
					ctl_set_value(form_task_waypt_arearadial2t, false);
					field_set_value(form_task_waypt_arearadial2,print_direction2(nice_brg(((double)edittsk->sectbear2[wptidx])+data.input.deviation.value)));
				} else {
					ctl_set_value(form_task_waypt_arearadial2m, false);
					ctl_set_value(form_task_waypt_arearadial2t, true);
					field_set_value(form_task_waypt_arearadial2,print_direction2((double)edittsk->sectbear2[wptidx]));
				}
				field_set_enable(form_task_waypt_arearadius, true);
				field_set_value(form_task_waypt_arearadius, print_distance2(edittsk->arearadii[wptidx],2));
				field_set_enable(form_task_waypt_arearadius2, true);
				field_set_value(form_task_waypt_arearadius2, print_distance2(edittsk->arearadii2[wptidx],2));
			} else {
				// display radii and bearings
				if (edittsk->waypttypes[wptidx] & CONTROL) {
					// control point
					field_set_value(form_task_waypt_turntype, "Control");
					field_set_value(form_task_waypt_arearadial2, "N/A");
					field_set_value(form_task_waypt_arearadial1, "N/A");
					field_set_value(form_task_waypt_arearadius, "N/A");
					field_set_value(form_task_waypt_arearadius2, "N/A");
				} else if ((wptidx < TAKEOFFSET) || (wptidx > LANDOFFSET)) {
					// takeoff or landing point
					if (wptidx < TAKEOFFSET) {
						field_set_value(form_task_waypt_turntype, "Takeoff");
					} else {
						field_set_value(form_task_waypt_turntype, "Landing");
					}
					field_set_value(form_task_waypt_arearadial2, "N/A");
					field_set_value(form_task_waypt_arearadial1, "N/A");
					field_set_value(form_task_waypt_arearadius, "N/A");
					field_set_value(form_task_waypt_arearadius2, "N/A");
				} else {
					// turnpoint type
					if (wptidx == TAKEOFFSET) {
						turntype = data.config.starttype;
						radius = data.config.startrad;
					} else if (wptidx == LANDOFFSET) {
						turntype = data.config.finishtype;
						radius = data.config.finishrad;
					} else {
						turntype = data.config.turntype;
						if (turntype == CYLINDER) {
							radius = data.config.turncircrad;
						} else {
							radius = data.config.turnfairad;
						}
					}
					if (turntype != CYLINDER) {
						// FAI sector so show +/- 90 deg bearings
						if (arearadial1mag) {
							ctl_set_value(form_task_waypt_arearadial1m, true);
							ctl_set_value(form_task_waypt_arearadial1t, false);
							field_set_value(form_task_waypt_arearadial1,print_direction2(nice_brg(((double)edittsk->sectbear1[wptidx])+data.input.deviation.value)));
						} else {
							ctl_set_value(form_task_waypt_arearadial1m, false);
							ctl_set_value(form_task_waypt_arearadial1t, true);
							field_set_value(form_task_waypt_arearadial1,print_direction2((double)edittsk->sectbear1[wptidx]));
						}
						field_set_enable(form_task_waypt_arearadial1, false);
						if (arearadial2mag) {
							ctl_set_value(form_task_waypt_arearadial2m, true);
							ctl_set_value(form_task_waypt_arearadial2t, false);
							field_set_value(form_task_waypt_arearadial2,print_direction2(nice_brg(((double)edittsk->sectbear2[wptidx])+data.input.deviation.value)));
						} else {
							ctl_set_value(form_task_waypt_arearadial2m, false);
							ctl_set_value(form_task_waypt_arearadial2t, true);
							field_set_value(form_task_waypt_arearadial2,print_direction2((double)edittsk->sectbear2[wptidx]));
						}
						field_set_enable(form_task_waypt_arearadial2, false);
						field_set_value(form_task_waypt_arearadius, print_distance2(radius,2));
						if (turntype == BOTH) {
							field_set_value(form_task_waypt_turntype, "FAI / Cyl");
							field_set_value(form_task_waypt_arearadius2, print_distance2(data.config.turncircrad,2));
						} else {
							if (turntype == FAI) {
								field_set_value(form_task_waypt_turntype, "FAI");
							} else if (turntype == ARC) {
								field_set_value(form_task_waypt_turntype, "Arc");
							} else if (turntype == TSKLINE) {
								field_set_value(form_task_waypt_turntype, "Line");
							}
							field_set_value(form_task_waypt_arearadius2, "N/A");
						}
					} else {
						// cylinder
						field_set_value(form_task_waypt_turntype, "Cylinder");
						field_set_value(form_task_waypt_arearadial2, "N/A");
						field_set_value(form_task_waypt_arearadial1, "N/A");
						field_set_value(form_task_waypt_arearadius, print_distance2(radius,2));
						field_set_value(form_task_waypt_arearadius2, "N/A");
					}
				}
				field_set_enable(form_task_waypt_arearadius, false);
				field_set_enable(form_task_waypt_arearadius2, false);
				ctl_set_enable(form_task_waypt_arearadial1m, true);
				ctl_set_value(form_task_waypt_arearadial1m, false);
				ctl_set_enable(form_task_waypt_arearadial1m, false);
				ctl_set_enable(form_task_waypt_arearadial1t, true);
				ctl_set_value(form_task_waypt_arearadial1t, false);
				ctl_set_enable(form_task_waypt_arearadial1t, false);
				ctl_set_enable(form_task_waypt_arearadial2m, true);
				ctl_set_value(form_task_waypt_arearadial2m, false);
				ctl_set_enable(form_task_waypt_arearadial2m, false);
				ctl_set_enable(form_task_waypt_arearadial2t, true);
				ctl_set_value(form_task_waypt_arearadial2t, false);
				ctl_set_enable(form_task_waypt_arearadial2t, false);
			}

			// display lat / lon
			field_set_value(form_task_waypt_remark, edittsk->remarks[wptidx]);
			switch (data.config.llfmt) {
				case LLUTM:
					frm_set_label(form_task_waypt_latlbl, "Est:");
					frm_set_label(form_task_waypt_lonlbl, "Nth:");
					ctl_set_value(form_task_waypt_llutm, true);
					ctl_set_value(form_task_waypt_lldm, false);
					ctl_set_value(form_task_waypt_lldms, false);
					ctl_set_visible(form_task_waypt_utmzone, true);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, EASTING);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, NORTHING);
					field_set_value(form_task_waypt_lon, tempchar);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, ZONE);
					field_set_value(form_task_waypt_utmzone, tempchar);
					break;
				case LLDM:
					frm_set_label(form_task_waypt_latlbl, "Lat:");
					frm_set_label(form_task_waypt_lonlbl, "Lon:");
					ctl_set_value(form_task_waypt_llutm, false);
					ctl_set_value(form_task_waypt_lldm, true);
					ctl_set_value(form_task_waypt_lldms, false);
					ctl_set_visible(form_task_waypt_utmzone, false);
					LLToStringDM(edittsk->wayptlats[wptidx], tempchar, ISLAT, true, true, 3);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringDM(edittsk->wayptlons[wptidx], tempchar, ISLON, true, true, 3);
					field_set_value(form_task_waypt_lon, tempchar);
					break;
				case LLDMS:
				default:
					frm_set_label(form_task_waypt_latlbl, "Lat:");
					frm_set_label(form_task_waypt_lonlbl, "Lon:");
					ctl_set_value(form_task_waypt_llutm, false);
					ctl_set_value(form_task_waypt_lldm, false);
					ctl_set_value(form_task_waypt_lldms, true);
					ctl_set_visible(form_task_waypt_utmzone,false);
					LLToStringDMS(edittsk->wayptlats[wptidx], tempchar, ISLAT);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringDMS(edittsk->wayptlons[wptidx], tempchar, ISLON);
					field_set_value(form_task_waypt_lon, tempchar);
					break;
			}

			// only display OLC turn button if inflight towards a turnpoint
			if (inflight && !(edittsk->waypttypes[wptidx] & AREA)) {
				if ((wptidx < edittsk->numwaypts-1) && (wptidx == activetskway)) ctl_set_visible(form_task_waypt_OLCturn, true);
			}
			WinDrawChars(data.input.distext, 2, 55, 91);
			WinDrawChars(data.input.distext, 2, 134, 91);
			handled=true;
			break;

		case fldEnterEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_task_waypt_turntype:
					if (edittsk->waypttypes[wptidx] & AREA) {
						if (edittsk->waypttypes[wptidx] & AREAEXIT) {
							edittsk->waypttypes[wptidx] ^= AREAEXIT;
							field_set_value(form_task_waypt_turntype, "AREA");
						} else {
							edittsk->waypttypes[wptidx] |= AREAEXIT;
							field_set_value(form_task_waypt_turntype, "EXIT AREA");
							field_set_value(form_task_waypt_arearadial1, print_direction2(0.0));
							field_set_value(form_task_waypt_arearadial2, print_direction2(0.0));
							CalcAreaMax(edittsk, wptidx, &edittsk->targetlats[wptidx], &edittsk->targetlons[wptidx], true);
						}
					}
					handled=true;
				break;
				default:
				        break;
			}
			break;

		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
//				case form_task_waypt_delete:
//					refresh_task_details(TASKREM);
//					dispactive = false; // go to edited task not active task
//					FrmGotoForm( form_set_task );
//					break;
				case form_task_waypt_sector:
					if ((taskIndex == 0) && (data.task.numwaypts > 0)) {	// && inflight) {
						// editting in flight the active and activated task
						activetasksector = true;
					} else {
						activetasksector = false;
					}
//					HostTraceOutputTL(appErrorClass, "Save to Temp Waypt");
					gotosector = true;
					MemMove(temptsk, edittsk, sizeof(TaskData));
					if (save_task_waypt_fields(temptsk, wptidx, tskidx, isctlpt, false)) {
						tsktoedit = temptsk;
						FrmGotoForm(form_waypoint_sector);
					}
					break;
				case form_task_waypt_save:
					if (settaskreadonly) {
						dispactive = false; // go to edited task not active task
						FrmGotoForm( form_set_task );
					} else if (save_task_waypt_fields(edittsk, wptidx, tskidx, isctlpt, true)) {
						dispactive = false; // go to edited task not active task
						FrmGotoForm( form_set_task );
					}
					break;
				case form_task_waypt_quit:
					dispactive = false; // go to edited task not active task
					FrmGotoForm( form_set_task );
					break;
				case form_task_waypt_OLCturn:
					// set TP to current location for OLC rules
					StrCopy(tempchar, "OLC");
					StrCat(tempchar, DblToStr(wptidx,0));
					field_set_value(form_task_waypt_name, tempchar);
					field_set_value(form_task_waypt_remark, "");
					field_set_value(form_task_waypt_elev, "0");
					switch (data.config.llfmt) {
						case LLUTM:
							LLToStringUTM(data.input.gpslatdbl, data.input.gpslngdbl, tempchar, EASTING);
							field_set_value(form_task_waypt_lat, tempchar);
							LLToStringUTM(data.input.gpslatdbl, data.input.gpslngdbl, tempchar, NORTHING);
							field_set_value(form_task_waypt_lon, tempchar);
							LLToStringUTM(data.input.gpslatdbl, data.input.gpslngdbl, tempchar, ZONE);
							field_set_value(form_task_waypt_utmzone, tempchar);
							break;
						case LLDM:
							LLToStringDM(data.input.gpslatdbl, tempchar, ISLAT, true, true, 3);
							field_set_value(form_task_waypt_lat, tempchar);
							LLToStringDM(data.input.gpslngdbl, tempchar, ISLON, true, true, 3);
							field_set_value(form_task_waypt_lon, tempchar);
							break;
						case LLDMS:
						default:
							LLToStringDMS(data.input.gpslatdbl, tempchar, ISLAT);
							field_set_value(form_task_waypt_lat, tempchar);
							LLToStringDMS(data.input.gpslngdbl, tempchar, ISLON);
							field_set_value(form_task_waypt_lon, tempchar);
							break;
					}
					// save changes
					if (save_task_waypt_fields(edittsk, wptidx, tskidx, isctlpt, true)) {
						// move activetskway to the requested waypoint
						requestedtskidx = wptidx+1;
						question->type = QturnOLC;
						handled = true;
						FrmPopupForm(form_question);
					}
					break;
				case form_task_waypt_lldm:
					frm_set_label(form_task_waypt_latlbl, "Lat:");
					frm_set_label(form_task_waypt_lonlbl, "Lon:");
					ctl_set_visible(form_task_waypt_utmzone, false);
					LLToStringDM(edittsk->wayptlats[wptidx], tempchar, ISLAT, true, true, 3);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringDM(edittsk->wayptlons[wptidx], tempchar, ISLON, true, true, 3);
					field_set_value(form_task_waypt_lon, tempchar);
					data.config.llfmt = LLDM;
					break;
				case form_task_waypt_lldms:
					frm_set_label(form_task_waypt_latlbl, "Lat:");
					frm_set_label(form_task_waypt_lonlbl, "Lon:");
					ctl_set_visible(form_task_waypt_utmzone, false);
					LLToStringDMS(edittsk->wayptlats[wptidx], tempchar, ISLAT);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringDMS(edittsk->wayptlons[wptidx], tempchar, ISLON);
					field_set_value(form_task_waypt_lon, tempchar);
					data.config.llfmt = LLDMS;
					break;
				case form_task_waypt_llutm:
					frm_set_label(form_task_waypt_latlbl, "Est:");
					frm_set_label(form_task_waypt_lonlbl, "Nth:");
					ctl_set_visible(form_task_waypt_utmzone, true);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, EASTING);
					field_set_value(form_task_waypt_lat, tempchar);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, NORTHING);
					field_set_value(form_task_waypt_lon, tempchar);
					LLToStringUTM(edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], tempchar, ZONE);
					field_set_value(form_task_waypt_utmzone, tempchar);
					data.config.llfmt = LLUTM;
					break;
				case form_task_waypt_typeAR:
					if (ctl_get_value(form_task_waypt_typeAR)) {
						field_set_value(form_task_waypt_turntype, "AREA");
						if ((edittsk->waypttypes[wptidx] & AREA) == 0) {
							// zero sectors and min radius to default to circle area
							edittsk->sectbear1[wptidx] = 0;
							edittsk->sectbear2[wptidx] = 0;
							edittsk->arearadii[wptidx] = 0.0;
							edittsk->arearadii2[wptidx] = 0.0;
							edittsk->waypttypes[wptidx] |= AREA;
						}
						field_set_enable(form_task_waypt_arearadial1, true);
						ctl_set_enable(form_task_waypt_arearadial1m, true);
						ctl_set_enable(form_task_waypt_arearadial1t, true);
						if (arearadial1mag) {
							ctl_set_value(form_task_waypt_arearadial1m, true);
							ctl_set_value(form_task_waypt_arearadial1t, false);
							field_set_value(form_task_waypt_arearadial1,print_direction2(nice_brg(((double)edittsk->sectbear1[wptidx])+data.input.deviation.value)));
						} else {
							ctl_set_value(form_task_waypt_arearadial1m, false);
							ctl_set_value(form_task_waypt_arearadial1t, true);
							field_set_value(form_task_waypt_arearadial1,print_direction2((double)edittsk->sectbear1[wptidx]));
						}
						field_set_enable(form_task_waypt_arearadial2, true);
						ctl_set_enable(form_task_waypt_arearadial2m, true);
						ctl_set_enable(form_task_waypt_arearadial2t, true);
						if (arearadial2mag) {
							ctl_set_value(form_task_waypt_arearadial2m, true);
							ctl_set_value(form_task_waypt_arearadial2t, false);
							field_set_value(form_task_waypt_arearadial2,print_direction2(nice_brg(((double)edittsk->sectbear2[wptidx])+data.input.deviation.value)));
						} else {
							ctl_set_value(form_task_waypt_arearadial2m, false);
							ctl_set_value(form_task_waypt_arearadial2t, true);
							field_set_value(form_task_waypt_arearadial2,print_direction2((double)edittsk->sectbear2[wptidx]));
						}
						field_set_enable(form_task_waypt_arearadius, true);
						field_set_value(form_task_waypt_arearadius, print_distance2(edittsk->arearadii[wptidx],2));
						field_set_enable(form_task_waypt_arearadius2, true);
						field_set_value(form_task_waypt_arearadius2, print_distance2(edittsk->arearadii2[wptidx],2));
						if (edittsk->arearadii[wptidx] == 0 ) {
							FrmSetFocus(pfrm,  FrmGetObjectIndex(pfrm, form_task_waypt_arearadius));
						}
						// When turning on the AREA attribute, set the AAT aim type to AATusr
						// if it isn't set to AATctr.
						if (edittsk->aataimtype != AATctr) {
							edittsk->aataimtype = AATusr;
						}
						CalcWptDists(edittsk, wptidx, true, true);
					} else {
						// display radii and bearings
						if (edittsk->waypttypes[wptidx] & CONTROL) {
							// control point
							field_set_value(form_task_waypt_turntype, "Control");
							field_set_value(form_task_waypt_arearadial2, "N/A");
							field_set_value(form_task_waypt_arearadial1, "N/A");
							field_set_value(form_task_waypt_arearadius, "N/A");
							field_set_value(form_task_waypt_arearadius2, "N/A");
						} else if ((wptidx < TAKEOFFSET) || (wptidx > LANDOFFSET)) {
							// takeoff or landing point
							if (wptidx < TAKEOFFSET) {
								field_set_value(form_task_waypt_turntype, "Takeoff");
							} else {
								field_set_value(form_task_waypt_turntype, "Landing");
							}
							field_set_value(form_task_waypt_arearadial2, "N/A");
							field_set_value(form_task_waypt_arearadial1, "N/A");
							field_set_value(form_task_waypt_arearadius, "N/A");
							field_set_value(form_task_waypt_arearadius2, "N/A");
						} else {
							if (edittsk->waypttypes[wptidx] & AREA) edittsk->waypttypes[wptidx] = edittsk->waypttypes[wptidx] ^ AREA;
							CalcWptDists(edittsk, wptidx, true, true);
							CalcBisectors(edittsk);
							// turnpoint type
							if (wptidx == TAKEOFFSET) {
								turntype = data.config.starttype;
								radius = data.config.startrad;
							} else if (wptidx == LANDOFFSET) {
								turntype = data.config.finishtype;
								radius = data.config.finishrad;
							} else {
								turntype = data.config.turntype;
								if (turntype == CYLINDER) {
									radius = data.config.turncircrad;
								} else {
									radius = data.config.turnfairad;
								}
							}
							if (turntype != CYLINDER) {
								// FAI sector so show +/- 90 deg bearings
								if (arearadial1mag) {
									ctl_set_value(form_task_waypt_arearadial1m, true);
									ctl_set_value(form_task_waypt_arearadial1t, false);
									field_set_value(form_task_waypt_arearadial1,print_direction2(nice_brg(((double)edittsk->sectbear1[wptidx])+data.input.deviation.value)));
								} else {
									ctl_set_value(form_task_waypt_arearadial1m, false);
									ctl_set_value(form_task_waypt_arearadial1t, true);
									field_set_value(form_task_waypt_arearadial1,print_direction2((double)edittsk->sectbear1[wptidx]));
								}
								field_set_enable(form_task_waypt_arearadial1, false);
								if (arearadial2mag) {
									ctl_set_value(form_task_waypt_arearadial2m, true);
									ctl_set_value(form_task_waypt_arearadial2t, false);
									field_set_value(form_task_waypt_arearadial2,print_direction2(nice_brg(((double)edittsk->sectbear2[wptidx])+data.input.deviation.value)));
								} else {
									ctl_set_value(form_task_waypt_arearadial2m, false);
									ctl_set_value(form_task_waypt_arearadial2t, true);
									field_set_value(form_task_waypt_arearadial2,print_direction2((double)edittsk->sectbear2[wptidx]));
								}
								field_set_enable(form_task_waypt_arearadial2, false);
								field_set_value(form_task_waypt_arearadius, print_distance2(radius,2));
								if (turntype == BOTH) {
									field_set_value(form_task_waypt_turntype, "FAI / Cyl");
									field_set_value(form_task_waypt_arearadius2, print_distance2(data.config.turncircrad,2));
								} else {
									if (turntype == FAI) {
										field_set_value(form_task_waypt_turntype, "FAI");
									} else if (turntype == ARC) {
										field_set_value(form_task_waypt_turntype, "Arc");
									} else if (turntype == TSKLINE) {
										field_set_value(form_task_waypt_turntype, "Line");
									}
									field_set_value(form_task_waypt_arearadius2, "N/A");
								}
							} else {
								// cylinder
								field_set_value(form_task_waypt_turntype, "Cylinder");
								field_set_value(form_task_waypt_arearadial2, "N/A");
								field_set_value(form_task_waypt_arearadial1, "N/A");
								field_set_value(form_task_waypt_arearadius, print_distance2(radius,2));
								field_set_value(form_task_waypt_arearadius2, "N/A");
							}
						}
						field_set_enable(form_task_waypt_arearadius, false);
						field_set_enable(form_task_waypt_arearadius2, false);
						ctl_set_enable(form_task_waypt_arearadial1m, true);
						ctl_set_value(form_task_waypt_arearadial1m, false);
						ctl_set_enable(form_task_waypt_arearadial1m, false);
						ctl_set_enable(form_task_waypt_arearadial1t, true);
						ctl_set_value(form_task_waypt_arearadial1t, false);
						ctl_set_enable(form_task_waypt_arearadial1t, false);
						ctl_set_enable(form_task_waypt_arearadial2m, true);
						ctl_set_value(form_task_waypt_arearadial2m, false);
						ctl_set_enable(form_task_waypt_arearadial2m, false);
						ctl_set_enable(form_task_waypt_arearadial2t, true);
						ctl_set_value(form_task_waypt_arearadial2t, false);
						ctl_set_enable(form_task_waypt_arearadial2t, false);
					}
					break;
				default:
					break;
			}
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled = false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "Leaving Task Waypt");
			handled=false;
			break;
		default:
			break;
	}
	return handled;
}

Boolean save_task_waypt_fields(TaskData *edittsk, Int16 wptidx, Int16 tskidx, Boolean isctlpt, Boolean writetoDB)
{
	Boolean arearadial1mag;
	Boolean arearadial2mag;
	Char *str;
	double temprng, tempbrg;
	Char turnpointname[15];
	double UTMn, UTMe;
	Char UTMz[5];
	
//	HostTraceOutputTL(appErrorClass, "Saving Task Waypoint");

	edittsk->wayptidxs[wptidx] = FindWayptRecordByName(edittsk->wayptnames[wptidx]);

	edittsk->elevations[wptidx] = field_get_double(form_task_waypt_elev)/data.input.altconst;

	if (data.config.llfmt == LLUTM) {
		UTMe = field_get_double(form_task_waypt_lat);
		UTMn = field_get_double(form_task_waypt_lon);
		StrCopy(UTMz, field_get_str(form_task_waypt_utmzone));
		UTMtoLL(UTMWGS84, UTMn, UTMe, UTMz, &edittsk->wayptlats[wptidx],  &edittsk->wayptlons[wptidx]);
	} else {
		str = field_get_str(form_task_waypt_lat);
		if (str){
			if (data.config.llfmt == LLDM) {
				edittsk->wayptlats[wptidx] = DegMinColonStringToLatLon(str);
			} else {
				edittsk->wayptlats[wptidx] = DegMinSecColonStringToLatLon(str);
			}
			if ((edittsk->wayptlats[wptidx] == 0.0) && (data.input.homeLat < 0.0)) edittsk->wayptlats[wptidx] = -0.0000001;
		}
		str = field_get_str(form_task_waypt_lon);
		if (str) {
			if (data.config.llfmt == LLDM) {
				edittsk->wayptlons[wptidx] = DegMinColonStringToLatLon(str);
			} else {
				edittsk->wayptlons[wptidx] = DegMinSecColonStringToLatLon(str);
			}
			if ((edittsk->wayptlons[wptidx] == 0.0) && (data.input.homeLon < 0.0)) edittsk->wayptlons[wptidx] = -0.0000001;
		}
	}
	
	if (ctl_get_value(form_task_waypt_typeAR) && (edittsk->waypttypes[wptidx] & AREAEXIT)) {
		edittsk->waypttypes[wptidx] = AREAEXIT;
	} else {
		edittsk->waypttypes[wptidx] = NULL;
	}
	if (isctlpt) edittsk->waypttypes[wptidx] |= CONTROL;
	if (ctl_get_value(form_task_waypt_typeA)) edittsk->waypttypes[wptidx]|= AIRPORT;
	if (ctl_get_value(form_task_waypt_typeT)) edittsk->waypttypes[wptidx]|= TURN;
	if (ctl_get_value(form_task_waypt_typeL)) edittsk->waypttypes[wptidx]|= LAND;
	if (ctl_get_value(form_task_waypt_typeS)) edittsk->waypttypes[wptidx]|= START;
	if (ctl_get_value(form_task_waypt_typeF)) edittsk->waypttypes[wptidx]|= FINISH;
//	if (ctl_get_value(form_task_waypt_typeM)) edittsk->waypttypes[wptidx]|= MARK;
//	if (ctl_get_value(form_task_waypt_typeTH)) edittsk->waypttypes[wptidx]|= THRML;
//	if (ctl_get_value(form_task_waypt_typeH))  edittsk->waypttypes[wptidx]|= HOME;
//	if (ctl_get_value(form_task_waypt_typeRF)) edittsk->waypttypes[wptidx]|= REFWPT;
	if ((edittsk->waypttypes[wptidx] & AIRPORT) || (edittsk->waypttypes[wptidx] & LAND)) edittsk->waypttypes[wptidx] |= AIRLAND;

	if (ctl_get_value(form_task_waypt_typeAR)) {
		edittsk->waypttypes[wptidx]|= AREA;
		edittsk->arearadii[wptidx] = Fabs(field_get_double(form_task_waypt_arearadius)/data.input.disconst);
		edittsk->arearadii2[wptidx] = Fabs(field_get_double(form_task_waypt_arearadius2)/data.input.disconst);
		arearadial1mag = ctl_get_value(form_task_waypt_arearadial1m);
		arearadial2mag = ctl_get_value(form_task_waypt_arearadial2m);
		if (edittsk->waypttypes[wptidx] & AREAEXIT) {
			edittsk->sectbear1[wptidx] = 0.0;
			edittsk->sectbear2[wptidx] = 0.0;
			CalcAreaMax(edittsk, wptidx, &edittsk->targetlats[wptidx], &edittsk->targetlons[wptidx], true);
		} else {
			edittsk->sectbear1[wptidx] = (UInt16)nice_brg(field_get_long(form_task_waypt_arearadial1));
			edittsk->sectbear2[wptidx] = (UInt16)nice_brg(field_get_long(form_task_waypt_arearadial2));
		}
		// If set to Magnetic, convert entry from Magnetic to True for storage
		if (arearadial1mag) {
			edittsk->sectbear1[wptidx] = (UInt16)nice_brg(edittsk->sectbear1[wptidx] - data.input.deviation.value);
		}
		if (arearadial2mag) {
			edittsk->sectbear2[wptidx] = (UInt16)nice_brg(edittsk->sectbear2[wptidx] - data.input.deviation.value);
		}
		// Check target point remains in area, if not reset targets and distances
		LatLonToRangeBearingEll(edittsk->targetlats[wptidx], edittsk->targetlons[wptidx], edittsk->wayptlats[wptidx], edittsk->wayptlons[wptidx], &temprng, &tempbrg);
		if (edittsk->sectbear1[wptidx] == edittsk->sectbear2[wptidx]) {
			// area is a circle
			if (!InSectorTest(edittsk->sectbear1[wptidx], edittsk->sectbear2[wptidx], edittsk->arearadii2[wptidx]*0.999, edittsk->arearadii[wptidx]*1.001, tempbrg, temprng)) {
				CalcWptDists(edittsk, wptidx, true, true);
			} else {
				CalcWptDists(edittsk, wptidx, false, false);
			}
		} else {
			// area is a sector
			if (!InSectorTest(nice_brg(edittsk->sectbear1[wptidx] + data.input.deviation.value - 1), nice_brg(edittsk->sectbear2[wptidx] + data.input.deviation.value + 1), edittsk->arearadii2[wptidx]*0.999, edittsk->arearadii[wptidx]*1.001, tempbrg, temprng)) {
				CalcWptDists(edittsk, wptidx, true, true);
			} else {
				CalcWptDists(edittsk, wptidx, false, false);
			}
		}
	} else {
		edittsk->sectbear1[wptidx] = 0;
		edittsk->sectbear2[wptidx] = 0;
		edittsk->arearadii[wptidx] = 0.0;
		edittsk->arearadii2[wptidx] = 0.0;
		CalcWptDists(edittsk, wptidx, true, true);
	}

	StrCopy(edittsk->remarks[wptidx], NoComma(field_get_str(form_task_waypt_remark)," "));

	// do error checks

	// check for empty name
	StrCopy(turnpointname, field_get_str(form_task_waypt_name));
	if (StrLen(trim(NoComma(turnpointname, " "), ' ', true)) == 0) {
		warning->type = Wgeneric;
		StrCopy(warning->line1, "Turnpoint name");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);
		return(false);
	}
	StrCopy(edittsk->wayptnames[wptidx], trim(NoComma(turnpointname, " "), ' ', true));

	if (edittsk->waypttypes[wptidx] & AREA) {
		// check for non-zero max-R
		if (edittsk->arearadii[wptidx] == 0) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius cannot be zero");
			StrCopy(warning->line2, "for an AREA Waypoint");
			FrmPopupForm(form_warning);
			return(false);
		}
		// check for max R > min R
		if (edittsk->arearadii2[wptidx] >= edittsk->arearadii[wptidx]) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Max Radius must be bigger");
			StrCopy(warning->line2, "than Min Radius");
			FrmPopupForm(form_warning);
			return(false);
		}
	}
	
	// update distances and bearings
	CalcTaskDists(edittsk, false, false, false);
	CalcBisectors(edittsk);
	CalcStartFinishDirs(edittsk);

	// save updated task
	if (writetoDB) {
		if (taskIndex == 0) set_task_changed = true;
		OpenDBUpdateRecord(task_db, sizeof(TaskData), edittsk, tskidx);
	}

	return(true);
}

void refresh_task_list(Int16 scr)
{
	#define FIELDLEN 16

	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3;
	Int16 x,y;
	MemHandle task_hand;
	MemPtr task_ptr;
	static Char **items = NULL;
	static Char **dist = NULL;
	static Char **numwp = NULL;
	Int16 nrecs;
	Int16 start;
	Int16 end;
	static Int16 prevNumRecs = 0;
	Char pageString[20];
	Char tmpString[20];

	// free up each of the previous strings and then free up
	// the array of pointers itself.
	for (x = 0; x < prevNumRecs; x++) {
		MemPtrFree(items[x]);
		MemPtrFree(dist[x]);
		MemPtrFree(numwp[x]);
	}
	if (items) {
		MemPtrFree(items);
		items = NULL;
	}
	if (dist) {
		MemPtrFree(dist);
		dist = NULL;
	}
	if (numwp) {
		MemPtrFree(numwp);
		numwp = NULL;
	}

	if (scr != 9999) {
		// get the number of tasks in the database
		numOfTasks = DmNumRecords(task_db);

		// get the List pointer
		lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list));
		lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list2));
		lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list3));

		// lets compute the "page" of tasks we're currently looking at.
		if (scr > 0) {
			if (((currentTaskListPage + 1) * 7) < numOfTasks) {
				// If there are more tasks to display, move down one page
				currentTaskListPage++;
			} else {
				// If at the bottom, wrap to the first page
				currentTaskListPage = 0;
			}
		} else if (scr < 0)  {
			if (currentTaskListPage > 0) {
				// If not on the first page of tasks, move up one page
				currentTaskListPage--;
			} else {
				// If at the top, wrap to the last page
				if (numOfTasks == 0) {
					currentTaskListPage = 0;
				} else if (Fmod((double)numOfTasks,7.0) == 0.0) {
					currentTaskListPage = (Int16)(numOfTasks/7) - 1;
				} else {
					currentTaskListPage = (Int16)(numOfTasks/7);
				}
			}
		}

		// given the current "page", compute the starting
		//  and ending index and the number of records.
		start = currentTaskListPage * 7;
		end = ((start + 7) > numOfTasks) ? numOfTasks : (start + 7);
		nrecs = end - start;

		if (nrecs > 0) {

			// we got at least one record so allocate enough
			//  memory to hold nrecs pointers-to-pointers
			items = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			dist = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			numwp = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// loop through each task record
			for (x = 0, y = start; y < end; x++, y++) {
				// assign each of the nrecs pointers-to-pointers
				// the address of a newly allocated 25 character array,
				// retrieve the task name associated with that record,
				// and copy that name into the array.
				OpenDBQueryRecord(task_db, y, &task_hand, &task_ptr);
				MemMove(tsk,task_ptr,sizeof(TaskData));
				MemHandleUnlock(task_hand);
				items[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				dist[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				numwp[x] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
				MemSet(items[x],FIELDLEN,0);
				MemSet(dist[x],FIELDLEN,0);
				MemSet(numwp[x],FIELDLEN,0);

				// Calculate total task distance
				CalcTaskDists(tsk, false, false, false);

				// task names
				StrNCopy(items[x], NoComma(tsk->name," "), 13);

				// distances
				if (tsk->numwaypts > 0) {
					if (tsk->ttldist*data.input.disconst < 10000.0) {
						StrCopy(dist[x], DblToStr(tsk->ttldist*data.input.disconst, 1));
						StrCopy(dist[x], leftpad(dist[x], '\031', 6));
					} else if (tsk->ttldist*data.input.disconst < 100000.0) {
						StrCopy(dist[x], DblToStr(tsk->ttldist*data.input.disconst, 0));
					} else {
						StrCopy(dist[x], "99999");
					}
				} else {
					StrCopy(dist[x], DblToStr(0.0, 1));
					StrCopy(dist[x], leftpad(dist[x], '\031', 6));
				}

				// number of waypoints
				StrCopy(numwp[x], StrIToA(tmpString, tsk->numwaypts));
				StrCopy(numwp[x], leftpad(numwp[x], '\031', 2));
			}

			// reform the list
			LstSetListChoices(lst, items, nrecs);
			LstSetListChoices(lst2, dist, nrecs);
			LstSetListChoices(lst3, numwp, nrecs);

		} else {
			items = (char **) MemPtrNew(1 * (sizeof(char *)));
			dist = (char **) MemPtrNew(1 * (sizeof(char *)));
			numwp = (char **) MemPtrNew(1 * (sizeof(char *)));
			prevNumRecs = 1;
			items[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			dist[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			numwp[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			MemSet(items[0],FIELDLEN,0);
			MemSet(dist[0],FIELDLEN,0);
			MemSet(numwp[0],FIELDLEN,0);
			StrNCopy(items[0], "No Tasks", 9);
			LstSetListChoices(lst, items, 1);
			LstSetListChoices(lst2, dist, 1);
			LstSetListChoices(lst3, numwp, 1);
			LstSetSelection(lst, 0);
			LstSetSelection(lst2, 0);
			LstSetSelection(lst3, 0);
		}

		// create the "Page: # of #" string
		MemSet(pageString,20, 0);
		if (numOfTasks == 0) {
			StrCopy(pageString,StrIToA(tmpString,(currentTaskListPage)));
		} else {
			StrCopy(pageString,StrIToA(tmpString,(currentTaskListPage+1)));
		}
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tmpString,(numOfTasks % 7) ? (((int)(numOfTasks/7)) + 1) : (int)(numOfTasks/7)));
		field_set_value(form_list_task_page, pageString);
		field_set_value(form_list_task_nrecs, StrIToA(tmpString,numOfTasks));

		// Redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
			LstDrawList(lst2);
			LstDrawList(lst3);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_task_list));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_task_list2));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_task_list3));
		}

		// if the currently selected waypoint is on the currently
		// displayed page, then darken it as if it were selected.  If not then
		// de-select everything.
//		HostTraceOutputTL(appErrorClass, "Task Idx %s", DblToStr(taskIndex,0));
//		HostTraceOutputTL(appErrorClass, "Task Page %s", DblToStr(currentTaskListPage,0));
		if ((taskIndex >= (currentTaskListPage*7)) && (taskIndex < ((currentTaskListPage*7) + 7))) {
			LstSetSelection(lst,  taskIndex % 7);
			LstSetSelection(lst2, taskIndex % 7);
			LstSetSelection(lst3, taskIndex % 7);
//			OpenDBQueryRecord(task_db, taskIndex, &task_hand, &task_ptr);
		} else {
			LstSetSelection(lst, -1);
			LstSetSelection(lst2, -1);
			LstSetSelection(lst3, -1);
		}

		// draw the horizontal lines
		DrawHorizListLines(7, 44, 14);
	}

}

Boolean form_list_task_event_handler(EventPtr event)
{
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3;
	Int16 selectedIdx = 0;
	Boolean handled=false;
	Char tempchar[25];
	Int16 i, tskidx;
	static Int16 savetskidx;
	MemHandle task_hand;
	MemPtr task_ptr;
	double maxlat = -999.9;
	double minlat = 999.9;
	double maxlon = -999.9;
	double minlon = 999.9;
	double maxrad = max(data.config.turnfairad,data.config.turncircrad);
	
	// get the List pointer
	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list));
	lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list2));
	lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_task_list3));

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:

			FrmDrawForm(pfrm);
			savetskidx = -1;

			StrCopy(tempchar, "Task List - ");
			switch (tasksortType) {
				case SortByNameA:
					StrCat(tempchar, "Name A");
					WinDrawLine(4,26,31,26);
					break;
				case SortByNameD:
					StrCat(tempchar, "Name D");
					WinDrawLine(4,26,31,26);
					break;
				case SortByDistance:
					StrCat(tempchar, "Dist A");
					WinDrawLine(45,26,64,26);
					break;
				case SortByDistanceD:
					StrCat(tempchar, "Dist D");
					WinDrawLine(45,26,64,26);
					break;
				default:
					break;
			}
			frm_set_title(tempchar);
			DmQuickSort(task_db, (DmComparF*)task_comparison, tasksortType);

			// only show view button if not in flight
			if (!inflight) ctl_set_visible(form_list_task_viewbtn, true);

			refresh_task_list(0);
			handled=true;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_list_task_name:
				case form_list_task_dist:
					ctl_set_visible(form_list_task_name, true);
					ctl_set_visible(form_list_task_dist, true);
					if (event->data.ctlEnter.controlID == form_list_task_name) {
						if (tasksortType == SortByNameA) {
							tasksortType = SortByNameD;
						} else {
							tasksortType = SortByNameA;
						}
						WinDrawLine(4,26,31,26);
					} else {
						if (tasksortType == SortByDistance) {
							tasksortType = SortByDistanceD;
						} else {
							tasksortType = SortByDistance;
						}
						WinDrawLine(45,26,64,26);
					}
					StrCopy(tempchar, "Task List - ");
					switch (tasksortType) {
						case SortByNameA:
							StrCat(tempchar, "Name A");
							break;
						case SortByNameD:
							StrCat(tempchar, "Name D");
							break;
						case SortByDistance:
							StrCat(tempchar, "Dist A");
							break;
						case SortByDistanceD:
							StrCat(tempchar, "Dist D");
							break;
						default:
							break;
					}
					frm_set_title(tempchar);
					DmQuickSort(task_db, (DmComparF*)task_comparison, tasksortType);
					taskIndex = 0;
					currentTaskListPage = 0;
					refresh_task_list(0);
					break;
				case form_list_task_viewbtn:
					// calculate centre and zoom level to view whole task on map
					OpenDBQueryRecord(task_db, taskIndex, &task_hand, &task_ptr);
					MemMove(edittsk,task_ptr,sizeof(TaskData));
					MemHandleUnlock(task_hand);
					// find center of lat/lon
					for (i=0; i<edittsk->numwaypts; i++) {
						if (edittsk->wayptlats[i] > maxlat) maxlat = edittsk->wayptlats[i];
						if (edittsk->wayptlats[i] < minlat) minlat = edittsk->wayptlats[i];
						if (edittsk->wayptlons[i] > maxlon) maxlon = edittsk->wayptlons[i];
						if (edittsk->wayptlons[i] < minlon) minlon = edittsk->wayptlons[i];
						if (edittsk->arearadii[i] > maxrad) maxrad = edittsk->arearadii[i];
					}
					taskctrlat = minlat + (HEIGHT_BASE-GLIDERY_BASE)*(maxlat - minlat)/HEIGHT_BASE;
					taskctrlon = minlon + GLIDERX_BASE*(maxlon - minlon)/WIDTH_BASE;
					data.input.gpslatdbl = taskctrlat;
					data.input.gpslngdbl = taskctrlon;
					// find scale
					logmapscale = curmapscale;
					logmapscaleidx = data.config.mapscaleidx;
					logmaporient = data.input.curmaporient;
					taskmapscale = ((max((maxlat-minlat),(maxlon-minlon)*data.input.coslat))*60.0 + 2.0*maxrad) * 1.02;
					curmapscale = taskmapscale;
					actualmapscale = curmapscale*data.input.disconst;
					// calculate start / finsih directions
					CalcStartFinishDirs(edittsk);
					// goto to map in pan mode
					draw_log = true;
					draw_task = true;
					taskpreview = false;
					FrmGotoForm(form_moving_map);
					break;
				case form_list_task_editbtn:
					origform2 = form_list_task;
					if (taskIndex == 0) {
						dispactive = true;
					} else {
						dispactive = false;
						selectedTaskWayIdx = 0;
					}
					settaskreadonly = false;
					FrmGotoForm(form_set_task);
					break;
				case form_list_task_newbtn:
					refresh_task_details(TASKNEW);
					currentTaskListPage = (Int16)(taskIndex/7);
					refresh_task_list(0);
					break;
				case form_list_task_deletebtn:
					refresh_task_details(TASKDEL);
					currentTaskListPage = (Int16)(taskIndex/7);
					refresh_task_list(0);
					break;
				case form_list_task_copybtn:
					refresh_task_details(TASKCOPY);
					currentTaskListPage = (Int16)(taskIndex/7);
					refresh_task_list(0);
					break;
				case form_list_task_actvbtn:
					FrmGotoForm(defaultscreen);
					refresh_task_details(TASKACTV);
					currentTaskListPage = 0;
					break;
				default:
					break;
			}
			handled=true;
			break;
		case lstSelectEvent:
		{
			switch (event->data.lstSelect.listID) {
				case form_list_task_list:
					PlayKeySound();
					selectedIdx = LstGetSelection(lst);
					tskidx = selectedIdx + (currentTaskPage * 7);
					if (tskidx == savetskidx) {
						// go to task edit
						if (selectedIdx != -1) {
							if (taskIndex == 0) {
								dispactive = true;
							} else {
								selectedTaskWayIdx = 0;
								dispactive = false;
							}
							origform2 = form_list_task;
							settaskreadonly = false;
							FrmGotoForm(form_set_task);
						}
						savetskidx = -1; // forces double click again
					} else {
						savetskidx = tskidx;
					}
					break;
				case form_list_task_list2:
					PlayKeySound();
					selectedIdx = LstGetSelection(lst2);
					break;
				case form_list_task_list3:
					PlayKeySound();
					selectedIdx = LstGetSelection(lst3);
					break;
				default:
					break;
			}
			LstSetSelection(lst, selectedIdx);
			LstSetSelection(lst2, selectedIdx);
			LstSetSelection(lst3, selectedIdx);
			DrawHorizListLines(7, 44, 14);
			taskIndex = selectedIdx + (currentTaskListPage * 7);
//			HostTraceOutputTL(appErrorClass, "Task Idx %s", DblToStr(taskIndex,0));
			handled=true;
			break;
		}
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Task List menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Task List menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case frmCloseEvent:
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

// comparison function supplied to DmQuickSort
Int16 task_comparison(TaskData* rec1, TaskData* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h)
{
	//Not sure it should be initialized to zero
	Int16 retval=0;

	// special code to keep Active Task at the top of the list
	if (StrCompare(rec1->name, "Active Task") == 0) return (-1);
	if (StrCompare(rec2->name, "Active Task") == 0) return (1);

	switch ( order ) {
		case SortByNameA:
			// David Lane - compare the names of the two waypoints in ascending order
			retval = (StrCompare(rec1->name, rec2->name));
			break;
		case SortByNameD:
			// David Lane - compare the names of the two waypoints in descending order
			retval = (StrCompare(rec2->name, rec1->name));
			break;
		case SortByDistance:
		{
			if (rec1->ttldist == rec2->ttldist) {
				retval = 0;
			} else if (rec1->ttldist < rec2->ttldist) {
				retval = -1;
			} else {
				retval = 1;
			}
			break;
		}
		case SortByDistanceD:
		{
			if (rec1->ttldist == rec2->ttldist) {
				retval = 0;
			} else if (rec1->ttldist > rec2->ttldist) {
				retval = -1;
			} else {
				retval = 1;
			}
			break;
		}
	}
	return (retval);
}

void MoveTaskWaypt(TaskData *tsk, Int16 dest, Int16 src)
{
	StrCopy(tsk->wayptnames[dest], tsk->wayptnames[src]);
	StrCopy(tsk->remarks[dest], tsk->remarks[src]);
	tsk->wayptidxs[dest] = tsk->wayptidxs[src];
	tsk->wayptlats[dest] = tsk->wayptlats[src];
	tsk->wayptlons[dest] = tsk->wayptlons[src];
	tsk->targetlats[dest] = tsk->targetlats[src];
	tsk->targetlons[dest] = tsk->targetlons[src];
	if (tsk == &data.task) {
		data.activetask.maxareadist[dest] = data.activetask.maxareadist[src];
	}
	tsk->distlats[dest] = tsk->distlats[src];
	tsk->distlons[dest] = tsk->distlons[src];
	tsk->elevations[dest] = tsk->elevations[src];
	tsk->sectbear1[dest] = tsk->sectbear1[src];
	tsk->sectbear2[dest] = tsk->sectbear2[src];
	tsk->arearadii[dest] = tsk->arearadii[src];
	tsk->arearadii2[dest] = tsk->arearadii2[src];
	tsk->waypttypes[dest] = tsk->waypttypes[src];
	tsk->distances[dest] = tsk->distances[src];
	tsk->bearings[dest] = tsk->bearings[src];
	tsk->terrainidx[dest] = tsk->terrainidx[src];
}

Boolean form_waypoint_sector_event_handler(EventPtr event)
{
	Boolean handled=false;
	double templat, templon, temprng, tempbrg;
	Int16 TAKEOFFSET=0, LANDOFFSET=0;
	FormType *pfrm = FrmGetActiveForm();
//	Char tempchar[30];
	RectangleType rectP;
	static RectangleType saverectP;
	static Boolean validpendown = false;
	Coord arrowX=0, arrowY=0;
	double sect1, sect2;
	double truecse, xfactor, yfactor;
	Int16 y;
	static Boolean origmanualzoomchg, targetmoved;
	
#define zoomfactor 3.0
#define buttonsize 30
#define movefactor 0.1

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);

//			HostTraceOutputTL(appErrorClass, "Waypoint %s", tsktoedit->wayptnames[selectedTaskWayIdx]);
//			HostTraceOutputTL(appErrorClass, "Waypt index %hd", selectedTaskWayIdx);
//			HostTraceOutputTL(appErrorClass, "Task index %hd", taskIndex);

			if (device.DIACapable) {
				//----------------------------------------------------------
				//Resize the form.
				//----------------------------------------------------------
				rectP.topLeft.x = 0;
				rectP.topLeft.y = 0;
				rectP.extent.x = 320;
				rectP.extent.y = 480;
				WinSetWindowBounds( FrmGetWindowHandle( pfrm ), &rectP );

				//------------------------------------------------------
				//Set this forms's constraints.
				//------------------------------------------------------
				WinSetConstraintsSize( FrmGetWindowHandle(pfrm), 160, 160, 240, 160, 160, 160 );

				//------------------------------------------------------
				//Set the form's dynamic Input Area Policy.
				//------------------------------------------------------
				FrmSetDIAPolicyAttr( pfrm, frmDIAPolicyCustom );

				//------------------------------------------------------
				//Enable the input trigger.
				//------------------------------------------------------
				PINSetInputTriggerState( pinInputTriggerDisabled );

				//------------------------------------------------------
				//If your application wanted to close the input area to
				//have as much space available as possible, you would
				//use the following call.
				//------------------------------------------------------
				PINSetInputAreaState( pinInputAreaClosed );
			}

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			// check for takeoff and landing points
			if (tsktoedit->hastakeoff) {
				TAKEOFFSET = 1;
			} else {
				TAKEOFFSET = 0;
			}
			if (tsktoedit->haslanding) {
				LANDOFFSET = tsktoedit->numwaypts - 2;
			} else {
				LANDOFFSET = tsktoedit->numwaypts - 1;
			}
//			HostTraceOutputTL(appErrorClass, "T/O %hd", TAKEOFFSET);
//			HostTraceOutputTL(appErrorClass, "Land %hd", LANDOFFSET);

			// set up centre point and scale
			if (tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) {
//				HostTraceOutputTL(appErrorClass, "Area");
				CalcAreaMin(tsktoedit, selectedTaskWayIdx, &data.input.areaminlat, &data.input.areaminlon, true);
				CalcAreaMax(tsktoedit, selectedTaskWayIdx, &data.input.areamaxlat, &data.input.areamaxlon, true);
				if (tsktoedit->sectbear1[selectedTaskWayIdx] == tsktoedit->sectbear2[selectedTaskWayIdx]) {
					// circle area
					// centre on max penetration point (will be same as target point until entering sector)
					sectorlat = tsktoedit->distlats[selectedTaskWayIdx];
					sectorlon = tsktoedit->distlons[selectedTaskWayIdx];
					sectorscale = tsktoedit->arearadii[selectedTaskWayIdx];
				} else {
					// sector area find centre
					if (tsktoedit->sectbear2[selectedTaskWayIdx] < tsktoedit->sectbear1[selectedTaskWayIdx]) {
						tempbrg = (tsktoedit->sectbear1[selectedTaskWayIdx] + tsktoedit->sectbear2[selectedTaskWayIdx]+360.0)/2.0;
					} else {
						tempbrg = (tsktoedit->sectbear1[selectedTaskWayIdx] + tsktoedit->sectbear2[selectedTaskWayIdx])/2.0;
					}
					temprng = (tsktoedit->arearadii[selectedTaskWayIdx] + tsktoedit->arearadii2[selectedTaskWayIdx])/2.0;
					RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], temprng, tempbrg, &sectorlat, &sectorlon);
					RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], tsktoedit->arearadii[selectedTaskWayIdx], tsktoedit->sectbear1[selectedTaskWayIdx], &templat, &templon);
					// set scale to fit whole sector
					LatLonToRange(sectorlat, sectorlon, templat, templon, &sectorscale);
					// centre on max penetration point (will be same as target point until entering sector)
					sectorlat = tsktoedit->distlats[selectedTaskWayIdx];
					sectorlon = tsktoedit->distlons[selectedTaskWayIdx];
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "Normal");
				// normal start, finish or turn
				sectorlat = tsktoedit->wayptlats[selectedTaskWayIdx];
				sectorlon = tsktoedit->wayptlons[selectedTaskWayIdx];
				if (selectedTaskWayIdx == TAKEOFFSET) {
					sectorscale = data.config.startrad;
//					HostTraceOutputTL(appErrorClass, "Start");
				} else if (selectedTaskWayIdx == LANDOFFSET) {
					sectorscale = data.config.finishrad;
//					HostTraceOutputTL(appErrorClass, "Finish");
				} else if (data.config.turntype == CYLINDER) {
					sectorscale = data.config.turncircrad;
//					HostTraceOutputTL(appErrorClass, "Circle");
				} else {
					sectorscale = data.config.turnfairad;
//					HostTraceOutputTL(appErrorClass, "FAI");
				}
			}
			sectorscale = Ceil(sectorscale*zoomfactor);

			// find nearest scale
//			temprng = sectorscale * data.input.disconst;
//			FindNextMapScale(&temprng, false);
//			sectorscale = temprng / data.input.disconst;
//			HostTraceOutputTL(appErrorClass, "Scale %s",DblToStr(sectorscale,1));

			// calculate task
			CalcTaskDists(tsktoedit, false, false, false);
			CalcBisectors(tsktoedit);
			CalcStartFinishDirs(tsktoedit);

			// draw zoomed sector and information (keeping title bar)
			if (device.DIACapable) {
				TOff = 164;

				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_tasklbl)), 1, TOff);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_task)), 19, TOff-2);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_totlbl)), 73, TOff);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_tot)), 94, TOff-2);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_early)), 141, TOff-2);

				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_wptetalbl)), 1, TOff+20);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_wptetelbl)), 1, TOff+20);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_wpteta)), 20, TOff+18);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_speedlbl)), 81, TOff+20);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_speed)), 108, TOff+18);
			}

			// set up settings for zoomed waypoint sector
			origmanualzoomchg = manualzoomchg;
			manualzoomchg = true;
			// This is used to set which map orientation mode is being used
			// depending on whether it is in THERMAL or CRUISE mode
			if (activetasksector && (selectedTaskWayIdx <= activetskway)) {
				if (mapmode == THERMAL) {
					data.input.curmaporient = data.config.thmaporient;
				} else {
					data.config.sectormaporient = data.config.maporient;
					data.input.curmaporient = data.config.maporient;
				}
			} else {
				// otherwise north up by default
				data.config.sectormaporient = NORTHUP;
				data.input.curmaporient = NORTHUP;
			}
			targetmoved = false;
			UpdateMap2(false, sectorlat, sectorlon, sectorscale, false, tsktoedit, false);

			handled=true;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "waypoint sector-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "winExitEvent");
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			menuopen = true;
			goingtomenu = true;
//			if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
//				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
//				// active task has changes, declare changes?
//				question->type = Qacttaskchg;
//				FrmPopupForm(form_question);
//			}
			handled = true;
			break;
		case frmCloseEvent:
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			if (device.DIACapable) {
				PINSetInputAreaState( pinInputAreaOpen );
			}
//			HostTraceOutputTL(appErrorClass, "Waypt Sector - win close event");
			// restore settings
			//WinResetClip();
			manualzoomchg = origmanualzoomchg;;
			data.input.curmaporient = data.config.maporient;
			if (activetasksector) {
				// copy changes to active task waypoint to task DB record 0
				if (taskIndex == 0) OpenDBUpdateRecord(task_db, sizeof(TaskData), tsktoedit, taskIndex);
//				if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
//					// active task has changes, declare changes?
//					question->type = Qacttaskchg;
//					FrmPopupForm(form_question);
//				}
 				// load terrain on task
				tskoffterrain = !loadtskterrain(tsktoedit);
			} else if (!gototskwpt) {
				// restore edittsk prior to changes
				MemMove(temptsk, edittsk, sizeof(TaskData));
			}
			gototskwpt = false;
			goingtomenu = false;
			break;
		case nilEvent:
			if (targetmoved) {
				// load terrain on task if target point has changed in  active task
				if (activetasksector) tskoffterrain = !loadtskterrain(tsktoedit);
				targetmoved = false;
			}
			break;
		case ctlSelectEvent:
			PlayKeySound();
//			if (taskIndex == 0) set_task_changed = true;
			switch (event->data.ctlEnter.controlID) {
				case form_waypoint_sector_max:
//					HostTraceOutputTL(appErrorClass, "Max");
					CalcAreaMax(tsktoedit, selectedTaskWayIdx, &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx], true);
					// update sector lat/lon to re-centre map
					sectorlat = tsktoedit->targetlats[selectedTaskWayIdx];
					sectorlon = tsktoedit->targetlons[selectedTaskWayIdx];
					// update distances
					if (updatesectordists) {
						tsktoedit->distlats[selectedTaskWayIdx] = tsktoedit->targetlats[selectedTaskWayIdx];
						tsktoedit->distlons[selectedTaskWayIdx] = tsktoedit->targetlons[selectedTaskWayIdx];
					}
					tsktoedit->aataimtype = AATusr;
					updatemap = true;
   					targetmoved = true;
					break;
				case form_waypoint_sector_min:
//					HostTraceOutputTL(appErrorClass, "Min");
					CalcAreaMin(tsktoedit, selectedTaskWayIdx, &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx], true);
					// update sector lat/lon to re-centre map
					sectorlat = tsktoedit->targetlats[selectedTaskWayIdx];
					sectorlon = tsktoedit->targetlons[selectedTaskWayIdx];
					// update distances
					if (updatesectordists) {
						tsktoedit->distlats[selectedTaskWayIdx] = tsktoedit->targetlats[selectedTaskWayIdx];
						tsktoedit->distlons[selectedTaskWayIdx] = tsktoedit->targetlons[selectedTaskWayIdx];
					}
					tsktoedit->aataimtype = AATusr;
					updatemap = true;
					targetmoved = true;
					break;
				case form_waypoint_sector_center:
//					HostTraceOutputTL(appErrorClass, "Center");
					if (selectedTaskWayIdx == inareasector) {
						// in flight and in active area to reset target to glider position
						tsktoedit->targetlats[selectedTaskWayIdx] = data.input.gpslatdbl;
						tsktoedit->targetlons[selectedTaskWayIdx] = data.input.gpslngdbl;
					} else {
						// reset target to centre of area
						CalcWptDists(tsktoedit, selectedTaskWayIdx, false, true);
					}
					// update sector lat/lon to re-centre map
					sectorlat = tsktoedit->targetlats[selectedTaskWayIdx];
					sectorlon = tsktoedit->targetlons[selectedTaskWayIdx];
					// update distances
					if (updatesectordists) {
						tsktoedit->distlats[selectedTaskWayIdx] = tsktoedit->targetlats[selectedTaskWayIdx];
						tsktoedit->distlons[selectedTaskWayIdx] = tsktoedit->targetlons[selectedTaskWayIdx];
					}
					tsktoedit->aataimtype = AATusr;
					updatemap = true;
 					targetmoved = true;
					break;
				default:
					break;
			}
			if (updatemap && (inareasector > -1)) {
				// re-calculate max distance due to target change in next area
				if (selectedTaskWayIdx == inareasector) {
//					HostTraceOutputTL(appErrorClass, "Target Changed");
					tgtchg = true;
				}
				y = activetskway;
				while (data.task.waypttypes[y] & CONTROL) y++;
				if (selectedTaskWayIdx == y) {
					LatLonToRange(data.activetask.distlats[inareasector], data.activetask.distlons[inareasector], data.task.distlats[y], data.task.distlons[y], &data.activetask.arearng2);
//					HostTraceOutputTL(appErrorClass, "Re-calc Area Max Dist - Ctl");
					if ((data.activetask.arearng1 + data.activetask.arearng2) > data.activetask.maxareadist[inareasector]) {
						data.activetask.maxareadist[inareasector] = data.activetask.arearng1 + data.activetask.arearng2;
					}
				}
				HandleTask(TSKNONE);
			}
			handled=true;
			break;
		case penDownEvent:
			validpendown = false;

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(device.HiDensityScrPresent);
				event->screenX = WinScaleCoord (event->screenX, true);
				event->screenY = WinScaleCoord (event->screenY, true);
			}

			// open menu by tapping in title bar
			arrowX = WIDTH_MIN;
			arrowY = HEIGHT_MIN;
			RctSetRectangle (&rectP, arrowX, arrowY, SCREEN.WIDTH, (SCREEN.SRES*12));
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
				RctSetRectangle (&saverectP, 0, 0, 0, 0);
				validpendown = true;
				handled=true;
			}

			if (tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) {
				// Centre Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = (SCREEN.HEIGHT-(SCREEN.SRES*43))/2-(SCREEN.SRES*(buttonsize*0.75-22));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize*2));
				} else {
					arrowY = (SCREEN.HEIGHT-(SCREEN.SRES*43))/2-(SCREEN.SRES*(buttonsize*0.75-43));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize*1.5));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Centre
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
				// Move Left Button
				arrowX = WIDTH_MIN+(SCREEN.SRES*1);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(22+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2.5));
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(43+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Left
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
				// Move Right Button
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(22+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2.5));
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(43+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Right
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
				// Move Up Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*11);
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*43);
				}
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Up
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
				// Move Down Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*buttonsize);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Down
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			// Invert the Scale Selection Area
			arrowX = SCREEN.WIDTH-(SCREEN.SRES*24);
			arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22);
			RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*24), (SCREEN.SRES*22));
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
				WinInvertRectangle (&rectP, 0);
				validpendown = true;
				saverectP = rectP;
				handled=true;
			}

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			break;

		case penUpEvent:
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(device.HiDensityScrPresent);
				event->screenX = WinScaleCoord (event->screenX, true);
				event->screenY = WinScaleCoord (event->screenY, true);
//				HostTraceOutputTL(appErrorClass, "screenX=|%hd|", event->screenX);
//				HostTraceOutputTL(appErrorClass, "screenY=|%hd|", event->screenY);
			}

			// un-invert saved rectangle
			if (validpendown) {
				WinInvertRectangle(&saverectP, 0);
			}

			// open menu by tapping in title bar
			arrowX = WIDTH_MIN;
			arrowY = HEIGHT_MIN;
			RctSetRectangle (&rectP, arrowX, arrowY, SCREEN.WIDTH, (SCREEN.SRES*12));
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
				}
				EvtEnqueueKey (menuChr, 0, commandKeyMask);
				handled=true;
			}

			if ((tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) && (validpendown)){
				// find bearing given map orientation
				if (data.input.curmaporient == TRACKUP) {
					truecse = data.input.true_track.value;
				} else if ((data.input.curmaporient == COURSEUP) && (data.input.bearing_to_destination.valid==VALID)) {
					truecse = nice_brg(data	.input.bearing_to_destination.value - data.input.deviation.value);
				} else {
					truecse = nice_brg(0.0 - data.input.deviation.value);
				}
				yfactor = (sectorscale/60.0*movefactor) * Cos(DegreesToRadians(truecse));
				xfactor = (sectorscale/60.0*movefactor) * Sin(DegreesToRadians(truecse));

				// Centre Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = (SCREEN.HEIGHT-(SCREEN.SRES*43))/2-(SCREEN.SRES*(buttonsize*0.75-22));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize*2));
				} else {
					arrowY = (SCREEN.HEIGHT-(SCREEN.SRES*43))/2-(SCREEN.SRES*(buttonsize*0.75-43));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize*1.5));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "Center");
					PlayKeySound();
					//if (selectedTaskWayIdx == inareasector) {
					//	// in flight and in active area to reset target to glider position
					//	tsktoedit->targetlats[selectedTaskWayIdx] = data.input.gpslatdbl;
					//	tsktoedit->targetlons[selectedTaskWayIdx] = data.input.gpslngdbl;
					//} else {
						// reset target to centre of area
						CalcWptDists(tsktoedit, selectedTaskWayIdx, false, true);
					//}
					tsktoedit->aataimtype = AATusr;
					updatemap = true;
					targetmoved = true;
					handled=true;
				}

				// Move Left Button
				arrowX = WIDTH_MIN+(SCREEN.SRES*1);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(22+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2.5));
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(43+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "Left");
					PlayKeySound();
					// calc new position
					templat = tsktoedit->targetlats[selectedTaskWayIdx] + xfactor * data.input.coslat;
					templon = tsktoedit->targetlons[selectedTaskWayIdx] - yfactor;
					// Check new point is still in the area
					LatLonToRangeBearing(templat, templon, tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], &temprng, &tempbrg);
					// This converts from True to Magnetic for the comparison
					sect1 = nice_brg(tsktoedit->sectbear1[selectedTaskWayIdx] + data.input.deviation.value);
					sect2 = nice_brg(tsktoedit->sectbear2[selectedTaskWayIdx] + data.input.deviation.value);
					// Since poibearing is in magnetic, should pass all as magnetic
					if (InSectorTest(sect1, sect2, tsktoedit->arearadii2[selectedTaskWayIdx], tsktoedit->arearadii[selectedTaskWayIdx], tempbrg, temprng)) {
						// still in area
						tsktoedit->targetlats[selectedTaskWayIdx] = templat;
						tsktoedit->targetlons[selectedTaskWayIdx] = templon;
					} else if (InSectorTest(sect1, sect2, 0, 2, tempbrg, 1)) {
						// edge of area
						if (temprng > tsktoedit->arearadii[selectedTaskWayIdx]) {
							temprng = tsktoedit->arearadii[selectedTaskWayIdx];
						} else {
							temprng = tsktoedit->arearadii2[selectedTaskWayIdx];
						}
						RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], temprng, nice_brg(tempbrg+180), &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx]);
					}
					tsktoedit->aataimtype = AATusr;
//					if (taskIndex == 0) set_task_changed = true;
					updatemap = true;
					targetmoved = true;
					handled=true;
				}

				// Move Right Button
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(22+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2.5));
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*(43+58-buttonsize));
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize), (SCREEN.SRES*buttonsize*2));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "Right");
					PlayKeySound();
					// calc new position
					templat = tsktoedit->targetlats[selectedTaskWayIdx] - xfactor * data.input.coslat;
					templon = tsktoedit->targetlons[selectedTaskWayIdx] + yfactor;
					// Check new point is still in the area
					LatLonToRangeBearing(templat, templon, tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], &temprng, &tempbrg);
					// This converts from True to Magnetic for the comparison
					sect1 = nice_brg(tsktoedit->sectbear1[selectedTaskWayIdx] + data.input.deviation.value);
					sect2 = nice_brg(tsktoedit->sectbear2[selectedTaskWayIdx] + data.input.deviation.value);
					// Since poibearing is in magnetic, should pass all as magnetic
					if (InSectorTest(sect1, sect2, tsktoedit->arearadii2[selectedTaskWayIdx], tsktoedit->arearadii[selectedTaskWayIdx], tempbrg, temprng)) {
						// still in area
						tsktoedit->targetlats[selectedTaskWayIdx] = templat;
						tsktoedit->targetlons[selectedTaskWayIdx] = templon;
					} else if (InSectorTest(sect1, sect2, 0, 2, tempbrg, 1)) {
						// edge of area
						if (temprng > tsktoedit->arearadii[selectedTaskWayIdx]) {
							temprng = tsktoedit->arearadii[selectedTaskWayIdx];
						} else {
							temprng = tsktoedit->arearadii2[selectedTaskWayIdx];
						}
						RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], temprng, nice_brg(tempbrg+180), &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx]);
					}
					tsktoedit->aataimtype = AATusr;
//					if (taskIndex == 0) set_task_changed = true;
					updatemap = true;
					targetmoved = true;
					handled=true;
				}

				// Move Up Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				if (device.DIACapable) {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*11);
				} else {
					arrowY = HEIGHT_MIN+(SCREEN.SRES*43);
				}
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "Up");
					PlayKeySound();
					// calc new position
					templat = tsktoedit->targetlats[selectedTaskWayIdx] + yfactor * data.input.coslat;
					templon = tsktoedit->targetlons[selectedTaskWayIdx] + xfactor;
					// Check new point is still in the area
					LatLonToRangeBearing(templat, templon, tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], &temprng, &tempbrg);
					// This converts from True to Magnetic for the comparison
					sect1 = nice_brg(tsktoedit->sectbear1[selectedTaskWayIdx] + data.input.deviation.value);
					sect2 = nice_brg(tsktoedit->sectbear2[selectedTaskWayIdx] + data.input.deviation.value);
					// Since poibearing is in magnetic, should pass all as magnetic
					if (InSectorTest(sect1, sect2, tsktoedit->arearadii2[selectedTaskWayIdx], tsktoedit->arearadii[selectedTaskWayIdx], tempbrg, temprng)) {
						// still in area
						tsktoedit->targetlats[selectedTaskWayIdx] = templat;
						tsktoedit->targetlons[selectedTaskWayIdx] = templon;
					} else if (InSectorTest(sect1, sect2, 0, 2, tempbrg, 1)) {
						// edge of area
						if (temprng > tsktoedit->arearadii[selectedTaskWayIdx]) {
							temprng = tsktoedit->arearadii[selectedTaskWayIdx];
						} else {
							temprng = tsktoedit->arearadii2[selectedTaskWayIdx];
						}
						RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], temprng, nice_brg(tempbrg+180), &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx]);
					}
					tsktoedit->aataimtype = AATusr;
//					if (taskIndex == 0) set_task_changed = true;
					updatemap = true;
					targetmoved = true;
					handled=true;
				}

				// Move Down Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*buttonsize);
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*buttonsize);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*buttonsize*2), (SCREEN.SRES*buttonsize));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "Down");
					PlayKeySound();
					// calc new position
					templat = tsktoedit->targetlats[selectedTaskWayIdx] - yfactor * data.input.coslat;
					templon = tsktoedit->targetlons[selectedTaskWayIdx] - xfactor;
					// Check new point is still in the area
					LatLonToRangeBearing(templat, templon, tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], &temprng, &tempbrg);
					// This converts from True to Magnetic for the comparison
					sect1 = nice_brg(tsktoedit->sectbear1[selectedTaskWayIdx] + data.input.deviation.value);
					sect2 = nice_brg(tsktoedit->sectbear2[selectedTaskWayIdx] + data.input.deviation.value);
					// Since poibearing is in magnetic, should pass all as magnetic
					if (InSectorTest(sect1, sect2, tsktoedit->arearadii2[selectedTaskWayIdx], tsktoedit->arearadii[selectedTaskWayIdx], tempbrg, temprng)) {
						// still in area
						tsktoedit->targetlats[selectedTaskWayIdx] = templat;
						tsktoedit->targetlons[selectedTaskWayIdx] = templon;
					} else if (InSectorTest(sect1, sect2, 0, 2, tempbrg, 1)) {
						// edge of area
						if (temprng > tsktoedit->arearadii[selectedTaskWayIdx]) {
							temprng = tsktoedit->arearadii[selectedTaskWayIdx];
						} else {
							temprng = tsktoedit->arearadii2[selectedTaskWayIdx];
						}
						RangeBearingToLatLonLinearInterp(tsktoedit->wayptlats[selectedTaskWayIdx], tsktoedit->wayptlons[selectedTaskWayIdx], temprng, nice_brg(tempbrg+180), &tsktoedit->targetlats[selectedTaskWayIdx], &tsktoedit->targetlons[selectedTaskWayIdx]);
					}
					tsktoedit->aataimtype = AATusr;
//					if (taskIndex == 0) set_task_changed = true;
					updatemap = true;
					targetmoved = true;
					handled=true;
				}

				// update sector lat/lon to re-centre map
				sectorlat = tsktoedit->targetlats[selectedTaskWayIdx];
				sectorlon = tsktoedit->targetlons[selectedTaskWayIdx];

				// update distances
				if (updatesectordists) {
					tsktoedit->distlats[selectedTaskWayIdx] = tsktoedit->targetlats[selectedTaskWayIdx];
					tsktoedit->distlons[selectedTaskWayIdx] = tsktoedit->targetlons[selectedTaskWayIdx];
				}
			}

			// Invert the Scale Selection Area
			// to change the Map Mode
			if (validpendown) {
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*24);
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*24), (SCREEN.SRES*22));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
					PlayKeySound();
					switch ( data.input.curmaporient ) {
						case NORTHUP:
							data.config.sectormaporient = TRACKUP;
							break;
						case TRACKUP:
							data.config.sectormaporient = COURSEUP;
							break;
						case COURSEUP:
							data.config.sectormaporient = NORTHUP;
							break;
						default:
							break;
					}
					data.input.curmaporient = data.config.sectormaporient;
					updatemap = true;
					//WinInvertRectangle (&rectP, 0);
					handled=true;
				}
			}

			validpendown = false;
			if (updatemap && (inareasector > -1)) {
				// re-calculate max distance due to target change in next area
				if (selectedTaskWayIdx == inareasector) {
//					HostTraceOutputTL(appErrorClass, "Target Changed");
					tgtchg = true;
				}
				y = activetskway;
				while (data.task.waypttypes[y] & CONTROL) y++;
				if (selectedTaskWayIdx == y) {
					LatLonToRange(data.activetask.distlats[inareasector], data.activetask.distlons[inareasector], data.task.distlats[y], data.task.distlons[y], &data.activetask.arearng2);
//					HostTraceOutputTL(appErrorClass, "Re-calc Area Max Dist - Pen");
					if ((data.activetask.arearng1 + data.activetask.arearng2) > data.activetask.maxareadist[inareasector]) {
						data.activetask.maxareadist[inareasector] = data.activetask.arearng1 + data.activetask.arearng2;
					}
				}
				HandleTask(TSKNONE);
			}
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
		case fldEnterEvent:
			switch (event->data.ctlEnter.controlID) {
				case form_waypoint_sector_speed:
					PlayKeySound();
					switch (data.config.tskspdunits) {
						case METRIC:
							data.config.tskspdunits = STATUTE;
							data.input.tskspdconst = SPDMPHCONST;
							StrCopy(data.input.tskspdtext, "mph");
							break;
						case STATUTE:
							data.config.tskspdunits = NAUTICAL;
							data.input.tskspdconst = SPDNAUCONST;
							StrCopy(data.input.tskspdtext, "kts");
							break;
						case NAUTICAL:
							data.config.tskspdunits = METRIC;
							data.input.tskspdconst = SPDKPHCONST;
							StrCopy(data.input.tskspdtext, "kph");
							break;
						default:
							break;
					}
					updatemap = true;
					break;
				default:
					break;
			}
		default:
			break;
	}
	return handled;
}

void UpdateSectorInfo(double xratio, double yratio)
{
	Char tempchar[30];
	Int16 x;
	double poirange, poibearing, firstaltloss;
	double Vtsk, hwind, Vstf;
	Int16 TAKEOFFSET, LANDOFFSET;
	UInt32 startsecs, stopsecs;
	DateTimeType stopdt; //, strtdt;
	RectangleType rectP;
	FormType *pfrm = FrmGetActiveForm();
	static Boolean wasinarea = false;
	EventType newEvent;
	double temprng, tempbrg, tmpdbl;
	Boolean leftturn, showdirarrow = false;
	Int16 LblOff;
	Int32 lengthsecs, ETAsecs;
	
//	HostTraceOutputTL(appErrorClass, "UpdateSectorInfo - Begin");

	// check for takeoff and landing points
	if (tsktoedit->hastakeoff) {
		TAKEOFFSET = 1;
	} else {
		TAKEOFFSET = 0;
	}
	if (tsktoedit->haslanding) {
		LANDOFFSET = tsktoedit->numwaypts - 2;
	} else {
		LANDOFFSET = tsktoedit->numwaypts - 1;
	}

	// re-calc task distances
	if (tsktoedit != &data.task) CalcTaskDists(tsktoedit, false, false, false);

	// decide if OK to update distances
	if ((activetasksector) && (selectedTaskWayIdx == activetskway-1)) {
		updatesectordists = false;
	} else {
		updatesectordists = true;
	}

	// update values and make fields visible over map
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(kCoordinatesStandard);
	}

	// Title
	RctSetRectangle (&rectP, WIDTH_MIN, HEIGHT_MIN, SCREEN.WIDTH, 11);
	StrCopy(tempchar, tsktoedit->name);
	field_set_value(form_waypoint_sector_title1, tempchar);
	StrCopy(tempchar, ": ");
	StrCat(tempchar, tsktoedit->wayptnames[selectedTaskWayIdx]);
	field_set_value(form_waypoint_sector_title2, tempchar);
	WinInvertRectangle(&rectP, 0);

	// re-draw form elements
	ctl_set_visible(form_waypoint_sector_totlbl, true);
	ctl_set_visible(form_waypoint_sector_speedlbl, true);
	ctl_set_visible(form_waypoint_sector_tasklbl, true);
	field_set_value(form_waypoint_sector_task, DblToStr(tsktoedit->ttldist * data.input.disconst, 1));
	if (device.DIACapable) {
		// TOff is set in form_waypoint_sector_event_handler
		// This shifts the units down into the DIA area
		WinDrawChars(data.input.distext, 2, 57, TOff+1);
		WinDrawChars(data.input.tskspdtext, 3, 141, TOff+21);
	} else {
		WinDrawChars(data.input.distext, 2, 57, 15);
		WinDrawChars(data.input.tskspdtext, 3, 141, 30);
	}

	// find current task speed
	Vtsk = data.flight.tskspeed;

	// perform calculations for Speed, ETA/E and TOT
	if (activetasksector && inflight) {
		// waypoint from active task is being viewed
		ctl_set_visible(form_waypoint_sector_wptetelbl, false);
		ctl_set_visible(form_waypoint_sector_wptetalbl, true); // arrival in actual time

		if (data.activetask.tskstartsecs == 0) {
//			HostTraceOutputTL(appErrorClass, "Active Task - not started");
			// use Vxc for ETE and TOT
			// use MC speed adjusted by wind to give ground speed and calculate ETA to start
			WinDrawChars("Tlft:", 5, 1,138);
			CalcHWind(data.inuseWaypoint.bearing, data.input.wnddir, data.input.wndspd, &hwind);
			CalcSTFSpdAlt2(MCCurVal, data.inuseWaypoint.distance, data.inuseWaypoint.bearing, &Vstf, &firstaltloss);
			
			// ETA field (flying direct to start at STF then Vxc for remaining waypoints
			ETAsecs = (Int32)data.inuseWaypoint.distance/(Vstf-hwind)*3600.0;
			if (ETAsecs < 0) ETAsecs = 86399; // 24 hours
			if (selectedTaskWayIdx <= TAKEOFFSET) {
				SecondsToDateOrTimeString(gpssecs + ETAsecs, tempchar, 1, 0);
				field_set_value(form_waypoint_sector_wpteta,tempchar);
			} else if (data.input.Vxc > 0.0) {
				poirange = 0.0;
				for (x = TAKEOFFSET+1; x <= selectedTaskWayIdx; x++) {
					// distance
					poirange += tsktoedit->distances[x];
				}
				SecondsToDateOrTimeString((UInt32)(poirange/data.input.Vxc*3600.0 + gpssecs + ETAsecs), tempchar, 1, 0);
				field_set_value(form_waypoint_sector_wpteta,tempchar);
			} else {
				field_set_value(form_waypoint_sector_wpteta,"N/A");
			}

			// use MC XC speed for whole task
			field_set_value(form_waypoint_sector_speed, DblToStr(pround(data.input.Vxc*data.input.tskspdconst,1),1));
			if (data.input.Vxc > 0.0) {
				// TOT field
//				lengthsecs = (Int32)tsktoedit->ttldist/data.input.Vxc*3600.0;
				lengthsecs = data.activetask.TOTsecs;
				SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
				field_set_value(form_waypoint_sector_tot,tempchar);
				// TOT early or late
				if (tsktoedit->rulesactive && (tsktoedit->mintasktime > 0)) {
					field_set_value(form_waypoint_sector_early, CalcTOTvar(lengthsecs/60, tsktoedit->mintasktime));
				} else {
					field_set_value(form_waypoint_sector_early, " ");
				}
			} else {
				// no valid task speed, so can't calculate TOT
				field_set_value(form_waypoint_sector_tot,"N/A");
				field_set_value(form_waypoint_sector_early, " ");
			}

		} else { // task is started so use task speed for ETA and TOT
//			HostTraceOutputTL(appErrorClass, "Active Task - started");
			// get task start and stop times
			startsecs = data.activetask.tskstartsecs;
			if (tasknotfinished) {
				stopsecs = data.activetask.tskstopsecs;
			} else {
				StringToDateAndTime(data.flight.tskstopdtg, data.flight.tskstoputc, &stopdt);
				stopsecs = TimDateTimeToSeconds(&stopdt);
			}
			if (Vtsk <= 0.0) {
				Vtsk = data.input.Vxc; // use MC XC speed if no task speed available
				WinDrawChars("Tlft:", 5, 1,138);
			} else {
				WinDrawChars("MC:", 3, 1,138);
			}
			field_set_value(form_waypoint_sector_speed, DblToStr(pround(Vtsk*data.input.tskspdconst,1),1));

			if (Vtsk > 0.0) {
				// ETA field
				SecondsToDateOrTimeString(gpssecs + data.activetask.etas[selectedTaskWayIdx], tempchar, 1, 0);
				field_set_value(form_waypoint_sector_wpteta,tempchar);

				// TOT field
				if (data.activetask.TOTsecs > 0) lengthsecs = data.activetask.TOTsecs; else lengthsecs = 0;
				SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
				field_set_value(form_waypoint_sector_tot,tempchar);
				// TOT early or late
				if (data.task.rulesactive && (data.task.mintasktime > 0)) {
					field_set_value(form_waypoint_sector_early, CalcTOTvar(data.activetask.TOTsecs/60, data.task.mintasktime));
				} else {
					field_set_value(form_waypoint_sector_early, " ");
				}
			} else {
				// no valid task speed, so can't calculate ETA and TOT
				field_set_value(form_waypoint_sector_wpteta, "N/A");
				field_set_value(form_waypoint_sector_tot, "N/A");
				field_set_value(form_waypoint_sector_early, " ");
			}
		}
		// check if in-flight with task activated
		if (inflight && (data.task.numwaypts > 0)) {
			// show actual range/bearing to waypoint from current position
			LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[selectedTaskWayIdx], data.task.targetlons[selectedTaskWayIdx], &poirange, &poibearing);
			showdirarrow = true; // draw direction arrows
		} else {
			// show range/bearing from task leg
			poirange = data.task.distances[selectedTaskWayIdx];
			poibearing = data.task.bearings[selectedTaskWayIdx];
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "Not Active Task or not in-flight");
		// calculations based on MC XC speed + wind on all legs
		WinDrawChars("Tlft:", 5, 1,138);
		ctl_set_visible(form_waypoint_sector_wptetalbl, false);
		ctl_set_visible(form_waypoint_sector_wptetelbl, true); // arrival in elapsed time

		field_set_value(form_waypoint_sector_speed, DblToStr(pround(data.input.Vxc*data.input.tskspdconst,1),1));
		if (data.input.Vxc > 0.0) {
			// ETE field
			poirange = 0.0;
			lengthsecs = 0;
			ETAsecs = 0;
			hwind = 0.0;
			for (x = TAKEOFFSET+1; x <= selectedTaskWayIdx; x++) {
				// distance
				poirange += tsktoedit->distances[x];
				// adjust ETE due to wind
				if (activetasksector) CalcHWind(tsktoedit->bearings[x], data.input.wnddir, data.input.wndspd, &hwind);
				ETAsecs = ETAsecs + (Int32)(tsktoedit->distances[x]/(data.input.Vxc-hwind)*3600.0);
				// keep TOT up to date
				if (x <= LANDOFFSET) lengthsecs = ETAsecs;
			}
			if (ETAsecs < 0) ETAsecs = 86399; // 24 hours
			SecondsToDateOrTimeString(ETAsecs, tempchar, 1, 0);
			field_set_value(form_waypoint_sector_wpteta,tempchar);
			// TOT field
			hwind = 0.0;
			for (x = selectedTaskWayIdx+1; x <= LANDOFFSET; x++) {
				// distance
				poirange += tsktoedit->distances[x];
				// adjust TOT due to wind
				if (activetasksector) CalcHWind(tsktoedit->bearings[x], data.input.wnddir, data.input.wndspd, &hwind);
				lengthsecs = lengthsecs + (Int32)(tsktoedit->distances[x]/(data.input.Vxc-hwind)*3600.0);
			}
			if (lengthsecs < 0) lengthsecs = 86399; // 24 hours
			SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
			field_set_value(form_waypoint_sector_tot,tempchar);
			// TOT early or late
			if (tsktoedit->rulesactive && (tsktoedit->mintasktime > 0)) {
				field_set_value(form_waypoint_sector_early, CalcTOTvar(lengthsecs/60, tsktoedit->mintasktime));
			} else {
				field_set_value(form_waypoint_sector_early, " ");
			}
		} else {
			// no valid task speed, so can't calculate ETA and TOT
			field_set_value(form_waypoint_sector_wpteta,"N/A");
			field_set_value(form_waypoint_sector_tot,"N/A");
			field_set_value(form_waypoint_sector_early, " ");
		}
		// show range/bearing from task leg
		poirange = tsktoedit->distances[selectedTaskWayIdx];
		poibearing = tsktoedit->bearings[selectedTaskWayIdx];
	}

	// Draw the bearing and distance info
	if (device.DIACapable) {
		LblOff = 11;
	} else if (tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) {
		LblOff = 64;
	} else {
		LblOff = 42;
	}
	// Draw the In Use Waypoint direction
	FntSetFont(largeBoldFont);
	StrCopy(tempchar, print_direction2(poibearing));
	StrCat(tempchar, "°");
	WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+1, HEIGHT_MIN+LblOff);
	// Draw the In Use Waypoint distance
	if ((poirange*data.input.disconst) < 10.0) {
		StrCopy(tempchar, print_distance2(poirange, 2));
	} else if ((poirange*data.input.disconst) < 100.0) {
		StrCopy(tempchar, print_distance2(poirange, 1));
	} else if ((poirange*data.input.disconst) < 1000.0) {
		StrCopy(tempchar, print_distance2(poirange, 0));
	} else {
		StrCopy(tempchar, "999");
	}
	WinDrawChars(tempchar, StrLen(tempchar), WIDTH_BASE-21, HEIGHT_MIN+LblOff);
	// Draw the turn direction arrows
	if (showdirarrow) {
		tmpdbl = data.input.magnetic_track.value - poibearing;
		if (tmpdbl < 0.0) {
			tmpdbl = 360.0 + tmpdbl;
		}
		if (tmpdbl < 180.0) {
			leftturn = true;
		} else {
			leftturn = false;
			tmpdbl = 360.0 - tmpdbl;
		}
		FntSetFont(symbol11Font);
		if (tmpdbl > 10.0) {
			if (leftturn) {
				WinDrawChars("\002\002", 2, WIDTH_MIN+1, HEIGHT_MIN+13+LblOff);
			} else {
				WinDrawChars("\003\003", 2, WIDTH_MIN+1, HEIGHT_MIN+13+LblOff);
			}
		} else if (tmpdbl > 5.0) {
			if (leftturn) {
				WinDrawChars("\002", 1, WIDTH_MIN+5, HEIGHT_MIN+13+LblOff);
			} else {
				WinDrawChars("\003", 1, WIDTH_MIN+5, HEIGHT_MIN+13+LblOff);
			}
		}
	}
	FntSetFont(stdFont);

	// draw buttons if waypoint is an area
	if (tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) {
		if (device.DIACapable) {
			// Move the buttons to the DIA area
			FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_min)), 1, 203);
			FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_center)), 58, 203);
			FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_waypoint_sector_max)), 129, 203);
			ctl_set_visible(form_waypoint_sector_center, true);
		}
		ctl_set_visible(form_waypoint_sector_max, true);
		ctl_set_visible(form_waypoint_sector_min, true);
	} else {
		ctl_set_visible(form_waypoint_sector_center, false);
		ctl_set_visible(form_waypoint_sector_max, false);
		ctl_set_visible(form_waypoint_sector_min, false);
	}

	// check for leaving an area
	if (tsktoedit->waypttypes[selectedTaskWayIdx] & AREA) {
		// update range and bearing to area waypoint
		LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.wayptlats[activetskway-1], data.task.wayptlons[activetskway-1], &temprng, &tempbrg);
		if (InSectorTest(data.task.sectbear1[activetskway-1], data.task.sectbear2[activetskway-1], data.task.arearadii2[activetskway-1], data.task.arearadii[activetskway-1], tempbrg, temprng)) {
			wasinarea = true;
		} else if (wasinarea) {
			wasinarea = false;
			// move to next task waypoint
			selectedTaskWayIdx = activetskway;
			// add form update event to trigger form redraw
			newEvent.eType = frmUpdateEvent;
			EvtAddEventToQueue(&newEvent);
		}
	} else {
		wasinarea = false;
	}

	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
	}

//	HostTraceOutputTL(appErrorClass, "UpdateSectorInfo - Done");
}

void CheckTurn()
{
	EventType newEvent;

//	HostTraceOutputTL(appErrorClass, "Check Turn");
	data.activetask.tasktgtlat = data.task.targetlats[activetskway];
	data.activetask.tasktgtlon = data.task.targetlons[activetskway];

	// update selected task waypoint index if required
	if ((FrmGetActiveFormID() != form_set_task)
	&& (((data.task.waypttypes[activetskway-1] & AREA) == 0) || (FrmGetActiveFormID() != form_waypoint_sector))) {
		selectedTaskWayIdx = activetskway;
	}

	// add form update event to trigger form redraw
	newEvent.eType = frmUpdateEvent;
	EvtAddEventToQueue(&newEvent);
}

void HandleTurn()
{
	UInt32 duration;
	Int16 prevtp;
	double groundspeed, hwind;
	Int16 i;

//	HostTraceOutputTL(appErrorClass, "Handle Turn");

	// Record Time at turn
	data.flight.timeatturn[activetskway] = utcsecs;

	if (activetskway > 0) {
		// find previous non-control point
		prevtp = activetskway-1;
		while (data.task.waypttypes[prevtp] & CONTROL) prevtp--;

		// calculate task airspeed
		if (data.flight.timeatturn[prevtp] > 0) {
			duration = data.flight.timeatturn[activetskway] - data.flight.timeatturn[prevtp];
			groundspeed = data.task.distances[activetskway] / duration * 3600.0;
			CalcHWind(data.task.bearings[activetskway], data.input.wnddir, data.input.wndspd, &hwind);
			data.flight.tskairspeed = groundspeed + hwind;
//			HostTraceOutputT(appErrorClass,  "TP: %s",data.task.wayptnames[activetskway]);
//			HostTraceOutputT(appErrorClass,  " Wind: %s",print_horizontal_speed2(hwind,1));
//			HostTraceOutputT(appErrorClass,  " Vtsk: %s",print_horizontal_speed2(data.flight.tskspeed,1));
//			HostTraceOutputTL(appErrorClass,  " Vair: %s",print_horizontal_speed2(data.flight.tskairspeed,1));
		}
	}

	// update flight record
	if (activefltindex >= 0) {
		OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
		data.input.logfltupd = cursecs;
	}

	// play turpoint sound
	PlayTurnSound();

	// count no. of control points to correctly label the task leg in the log file.
	activectlpts = 0;
	for (i=0; i<=activetskway; i++) {
		if (data.task.waypttypes[i] & CONTROL) activectlpts++;
	}

	// activate next task leg
	activetskway++;
	CheckTurn();
}

Boolean form_task_rules_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	Char tempchar[25];
	EventType newEvent;
	MemHandle mem_hand;
	MemPtr mem_ptr;
	static Boolean exitpressed = false;
	static Boolean taskactive = false;

	switch (event->eType) {
		case frmOpenEvent:
//			HostTraceOutputTL(appErrorClass, "Open Task Rules");
			if (skipnewflight) {
				// load active task
				OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
				MemMove(tsk, mem_ptr, sizeof(TaskData));
				MemHandleUnlock(mem_hand);
			}
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "Update Task Rules - skipnewflight %s", DblToStr(skipnewflight,0));
			FrmDrawForm(pfrm);
			exitpressed = false;
			if (settaskreadonly) {
				StrCopy(tempchar, "Task");
   			} else {
				StrCopy(tempchar, tsk->name);
			}
			StrCat(tempchar, " Rules");
			frm_set_title(tempchar);

			ctl_set_value(form_task_rules_active, tsk->rulesactive);

			StrCopy(tempchar, "00");
			StrCat(tempchar, DblToStr(tsk->startlocaltime/60,0));
			field_set_value(form_task_rules_starthours, Right(tempchar,2));
			StrCopy(tempchar, "00");
			StrCat(tempchar, DblToStr(tsk->startlocaltime%60,0));
			field_set_value(form_task_rules_startmins, Right(tempchar,2));

			field_set_value(form_task_rules_maxstart, print_altitude(tsk->maxstartheight));
			WinDrawChars(data.input.alttext, 2, 93, 62);

			field_set_value(form_task_rules_startwarnheight, print_altitude(tsk->startwarnheight));
			WinDrawChars(data.input.alttext, 2, 93, 90);

			field_set_value(form_task_rules_timebelow, StrIToA(tempchar, tsk->timebelowstart));
			WinDrawChars("secs", 4, 142, 90);

			field_set_value(form_task_rules_mintime, StrIToA(tempchar, tsk->mintasktime));
			WinDrawChars("mins", 4, 83, 113);

			if (tsk->fgto1000m) {
				field_set_value(form_task_rules_minfinish, "?");
				ctl_set_visible(form_task_rules_minfinishup, false);
				ctl_set_visible(form_task_rules_minfinishdown, false);
				ctl_set_visible(form_task_rules_fgto1000mlbl, true);
			} else {
				field_set_value(form_task_rules_minfinish, print_altitude(tsk->finishheight));
				ctl_set_visible(form_task_rules_fgto1000mlbl, false);
				ctl_set_visible(form_task_rules_minfinishup, true);
				ctl_set_visible(form_task_rules_minfinishdown, true);
			}
			WinDrawChars(data.input.alttext, 2, 93, 134);
			ctl_set_value(form_task_rules_fgto1000m, tsk->fgto1000m);

			switch (tsk->startaltref) {
				case MSL:
					ctl_set_value(form_task_rules_startMSL, true);
					ctl_set_value(form_task_rules_startAGL, false);
					break;
				case AGL:
					ctl_set_value(form_task_rules_startMSL, false);
					ctl_set_value(form_task_rules_startAGL, true);
					break;
				default: // use MSL
					ctl_set_value(form_task_rules_startMSL, true);
					ctl_set_value(form_task_rules_startAGL, false);
					tsk->startaltref = MSL;
					break;
			}

			switch (tsk->finishaltref) {
				case MSL:
					ctl_set_value(form_task_rules_finishMSL, true);
					ctl_set_value(form_task_rules_finishAGL, false);
					break;
				case AGL:
					ctl_set_value(form_task_rules_finishMSL, false);
					ctl_set_value(form_task_rules_finishAGL, true);
					break;
				default: // use MSL
					ctl_set_value(form_task_rules_finishMSL, true);
					ctl_set_value(form_task_rules_finishAGL, false);
					tsk->finishaltref = MSL;
					break;
			}

			ctl_set_value(form_task_rules_startonentry, tsk->startonentry);
			ctl_set_value(form_task_rules_warnbeforestart, tsk->warnbeforestart);
			ctl_set_value(form_task_rules_cyldist, tsk->inclcyldist);
			ctl_set_value(form_task_rules_assigndef, false);

			if (settaskreadonly) {
				// disable controls
				ctl_set_enable(form_task_rules_maxstart, false);
				ctl_set_enable(form_task_rules_timebelow, false);
				ctl_set_enable(form_task_rules_mintime	, false);
				ctl_set_enable(form_task_rules_fgto1000m, false);
				ctl_set_enable(form_task_rules_active, false);
				ctl_set_enable(form_task_rules_minfinish, false);
				ctl_set_enable(form_task_rules_startMSL, false);
				ctl_set_enable(form_task_rules_startAGL, false);
				ctl_set_enable(form_task_rules_finishMSL, false);
				ctl_set_enable(form_task_rules_finishAGL, false);
				ctl_set_enable(form_task_rules_startwarnheight, false);
				ctl_set_enable(form_task_rules_starthours, false);
				ctl_set_enable(form_task_rules_startmins, false);
				ctl_set_enable(form_task_rules_startonentry, false);
				ctl_set_enable(form_task_rules_warnbeforestart, false);
				ctl_set_enable(form_task_rules_cyldist, false);
				// hide controls
				ctl_set_visible(form_task_rules_maxstartup, false);
				ctl_set_visible(form_task_rules_maxstartdown, false);
				ctl_set_visible(form_task_rules_mintimeup, false);
				ctl_set_visible(form_task_rules_mintimedown, false);
				ctl_set_visible(form_task_rules_minfinishup, false);
				ctl_set_visible(form_task_rules_minfinishdown, false);
				ctl_set_visible(form_task_rules_starttimeup, false);
				ctl_set_visible(form_task_rules_starttimedown, false);
			}
			
			if (defaultrules) {
				ctl_set_visible(form_task_rules_active, false);
				ctl_set_visible(form_task_rules_fgtostartalt, true);
				ctl_set_value(form_task_rules_fgtostartalt, data.config.fgtostartalt);
			} else {
				if (!settaskreadonly) ctl_set_visible(form_task_rules_assigndef, true);
			}

			if (skipnewflight) {
//				HostTraceOutputTL(appErrorClass, "task rules : exittaskreadonly = false");
				exittaskreadonly = false;
				//ctl_set_label(form_task_rules_assigndef, "Task");
				ctl_set_visible(form_task_rules_assigndef, false);
				if (taskIndex == 0 && mustActivateAsNew == false) {
					taskactive = true;
					ctl_set_label(form_task_rules_exit, "REACT");
				} else {
					taskactive = false;
					ctl_set_label(form_task_rules_exit, "ACT");
				}
			}
			handled=true;
			break;

		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "waypoint sector-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;

		case ctlSelectEvent:
			PlayKeySound();
			if ((event->data.ctlEnter.controlID != form_task_rules_exit) && (taskIndex == 0)) set_task_changed = true;
			switch (event->data.ctlEnter.controlID) {
				case form_task_rules_startMSL:
					tsk->startaltref = MSL;
					break;
				case form_task_rules_startAGL:
					tsk->startaltref = AGL;
					break;
				case form_task_rules_finishMSL:
					tsk->finishaltref = MSL;
					break;
				case form_task_rules_finishAGL:
					tsk->finishaltref = AGL;
					break;
				case form_task_rules_exit:
					exitpressed = true;
					if (skipnewflight) {
						FrmGotoForm(origform);
					} else if (defaultrules) {
						FrmGotoForm(form_config_task);
					} else {
						goingtotaskedit = true;
						FrmGotoForm(form_set_task);
					}
					break;
				case form_task_rules_active:
					if (taskIndex != 0) {
						tsk->rulesactive = ctl_get_value(form_task_rules_active);
					} else if (ctl_get_value(form_task_rules_active)) {
						tsk->rulesactive = true;
					} else if (data.task.numwaypts > 0) {
						//tsk->rulesactive = false;
						question->type = Qrulesoff;
						FrmPopupForm(form_question);
					} else {
						tsk->rulesactive = false;
					}
					break;
				case form_task_rules_fgto1000m:
					tsk->fgto1000m = ctl_get_value(form_task_rules_fgto1000m);
					if (tsk->fgto1000m) {
						field_set_value(form_task_rules_minfinish, "?");
						ctl_set_visible(form_task_rules_minfinishup, false);
						ctl_set_visible(form_task_rules_minfinishdown, false);
						ctl_set_visible(form_task_rules_fgto1000mlbl, true);
					} else {
						field_set_value(form_task_rules_minfinish, print_altitude(tsk->finishheight));
						ctl_set_visible(form_task_rules_fgto1000mlbl, false);
						ctl_set_visible(form_task_rules_minfinishup, true);
						ctl_set_visible(form_task_rules_minfinishdown, true);
					}
					break;
				case form_task_rules_maxstartup:
					tsk->maxstartheight = increment(field_get_double(form_task_rules_maxstart), (data.config.altunits==NAUTICAL?100.0:50.0))/data.input.altconst;
					field_set_value(form_task_rules_maxstart, print_altitude(tsk->maxstartheight));
					break;
				case form_task_rules_maxstartdown:
					tsk->maxstartheight = increment(field_get_double(form_task_rules_maxstart), -(data.config.altunits==NAUTICAL?100.0:50.0))/data.input.altconst;
					if (tsk->maxstartheight < 0.0) tsk->maxstartheight = 0.0;
					field_set_value(form_task_rules_maxstart, print_altitude(tsk->maxstartheight));
					break;
				case form_task_rules_minfinishup:
					tsk->finishheight = increment(field_get_double(form_task_rules_minfinish), (data.config.altunits==NAUTICAL?100.0:50.0))/data.input.altconst;
					field_set_value(form_task_rules_minfinish, print_altitude(tsk->finishheight));
					break;
				case form_task_rules_minfinishdown:
					tsk->finishheight = increment(field_get_double(form_task_rules_minfinish), -(data.config.altunits==NAUTICAL?100.0:50.0))/data.input.altconst;
					if (tsk->finishheight < 0.0) tsk->finishheight = 0.0;
					field_set_value(form_task_rules_minfinish, print_altitude(tsk->finishheight));
					break;
				case form_task_rules_mintimeup:
					tsk->mintasktime = increment(field_get_double(form_task_rules_mintime), 5);
					field_set_value(form_task_rules_mintime, StrIToA(tempchar, tsk->mintasktime));
					break;
				case form_task_rules_mintimedown:
					tsk->mintasktime = increment(field_get_double(form_task_rules_mintime), -5);
					if (tsk->mintasktime < 0) tsk->mintasktime = 0;
					field_set_value(form_task_rules_mintime, StrIToA(tempchar, tsk->mintasktime));
					break;
				case form_task_rules_starttimeup:
					tsk->startlocaltime = (field_get_long(form_task_rules_starthours)%24)*60 + (field_get_long(form_task_rules_startmins)%60);
					tsk->startlocaltime = increment(tsk->startlocaltime, 5);
					if (tsk->startlocaltime >= 1440) tsk->startlocaltime -= 1440;
					StrCopy(tempchar, "00");
					StrCat(tempchar, DblToStr(tsk->startlocaltime/60,0));
					field_set_value(form_task_rules_starthours, Right(tempchar,2));
					StrCopy(tempchar, "00");
					StrCat(tempchar, DblToStr(tsk->startlocaltime%60,0));
					field_set_value(form_task_rules_startmins, Right(tempchar,2));
					break;
				case form_task_rules_starttimedown:
					tsk->startlocaltime = (field_get_long(form_task_rules_starthours)%24)*60 + (field_get_long(form_task_rules_startmins)%60);
					tsk->startlocaltime = increment(tsk->startlocaltime, -5);
					if (tsk->startlocaltime < 0) tsk->startlocaltime += 1440;
					StrCopy(tempchar, "00");
					StrCat(tempchar, DblToStr(tsk->startlocaltime/60,0));
					field_set_value(form_task_rules_starthours, Right(tempchar,2));
					StrCopy(tempchar, "00");
					StrCat(tempchar, DblToStr(tsk->startlocaltime%60,0));
					field_set_value(form_task_rules_startmins, Right(tempchar,2));
					break;
				case form_task_rules_startonentry:
					tsk->startonentry = ctl_get_value(form_task_rules_startonentry);
					break;
				case form_task_rules_warnbeforestart:
					tsk->warnbeforestart = ctl_get_value(form_task_rules_warnbeforestart);
					break;
				case form_task_rules_cyldist:
					tsk->inclcyldist = ctl_get_value(form_task_rules_cyldist);
					break;
				case form_task_rules_assigndef:
					if (skipnewflight) {
						FrmGotoForm(form_set_task);
					} else {
						AssignDefaultRules(tsk);
						newEvent.eType = frmUpdateEvent;
						EvtAddEventToQueue(&newEvent);
					}
					break;
				case form_task_rules_fgtostartalt:
					data.config.fgtostartalt = ctl_get_value(form_task_rules_fgtostartalt);
					break;

				default:
					break;
			}
			handled=true;
			break;

		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Task Rules win exit : menuopen = true");
			menuopen = true;
			goingtomenu = true;
			if (!goingtotaskedit && !settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				if (!skipnewflight) {
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				}
			}
			handled = false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "task rules close event");
			if (!defaultrules && !settaskreadonly && !exittaskreadonly) {
				tsk->startwarnheight = field_get_double(form_task_rules_startwarnheight)/data.input.altconst;
				tsk->maxstartheight = field_get_double(form_task_rules_maxstart)/data.input.altconst;
				tsk->timebelowstart = (Int32)field_get_double(form_task_rules_timebelow);
				tsk->mintasktime = (Int32)field_get_double(form_task_rules_mintime);
				tsk->finishheight = field_get_double(form_task_rules_minfinish)/data.input.altconst;
				tsk->startlocaltime = (field_get_long(form_task_rules_starthours)%24)*60 + (field_get_long(form_task_rules_startmins)%60);
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				if (!skipnewflight && !goingtotaskedit && !settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				}
				goingtotaskedit = false;
			}
			if (defaultrules) {
				// save default rules to config file
				data.config.defaultrules.rulesactive = false;	//tsk->rulesactive;
				data.config.defaultrules.startwarnheight = field_get_double(form_task_rules_startwarnheight)/data.input.altconst;
				data.config.defaultrules.maxstartheight = field_get_double(form_task_rules_maxstart)/data.input.altconst;
				data.config.defaultrules.timebelowstart = (Int32)field_get_double(form_task_rules_timebelow);
				data.config.defaultrules.mintasktime = (Int32)field_get_double(form_task_rules_mintime);
				data.config.defaultrules.finishheight = field_get_double(form_task_rules_minfinish)/data.input.altconst;
				data.config.defaultrules.fgto1000m = tsk->fgto1000m;
				data.config.defaultrules.startaltref = tsk->startaltref;
				data.config.defaultrules.finishaltref = tsk->finishaltref;
				data.config.defaultrules.startlocaltime = (field_get_long(form_task_rules_starthours)%24)*60 + (field_get_long(form_task_rules_startmins)%60);
				data.config.defaultrules.startonentry = tsk->startonentry;
				data.config.defaultrules.warnbeforestart = tsk->warnbeforestart;
				data.config.defaultrules.inclcyldist = tsk->inclcyldist;
			}
			// automatically update the active task if already activated
			if (skipnewflight && (exitpressed || taskactive)) {
				if (!taskonhold) refresh_task_details(TASKACTV);
			} else {
				refresh_task_details(TASKDISP);
			}
			defaultrules = false;
			goingtomenu = false;
			break;

		default:
			break;
	}

	return handled;
}

void AssignDefaultRules(TaskData *tsk)
{
	// copy default rules to task
	//tsk->rulesactive     = data.config.defaultrules.rulesactive;
	tsk->startwarnheight = data.config.defaultrules.startwarnheight;
	tsk->maxstartheight  = data.config.defaultrules.maxstartheight;
	tsk->timebelowstart  = data.config.defaultrules.timebelowstart;
	tsk->mintasktime     = data.config.defaultrules.mintasktime;
	tsk->finishheight    = data.config.defaultrules.finishheight;
	tsk->fgto1000m       = data.config.defaultrules.fgto1000m;
	tsk->startaltref     = data.config.defaultrules.startaltref;
	tsk->finishaltref    = data.config.defaultrules.finishaltref;
	tsk->startlocaltime  = data.config.defaultrules.startlocaltime;
	tsk->startonentry    = data.config.defaultrules.startonentry;
	tsk->warnbeforestart = data.config.defaultrules.warnbeforestart;
	tsk->inclcyldist     = data.config.defaultrules.inclcyldist;
}

UInt32 CalcTOT(Int16 wptidx, double MCVal)
{
	UInt32 lengthsecs;
	Int8 x;
	double altrem, Vleg, ratio, Vtsk;
	double taskdist, hwind;
	Boolean altnegative;

//	HostTraceOutputTL(appErrorClass, "Calculate Time On Task");
	if (data.flight.tskspeed > 0.0) {
//		HostTraceOutputTL(appErrorClass, "Using Vtsk");
		Vtsk = data.flight.tskspeed;
	} else {
//		HostTraceOutputTL(appErrorClass, "Using Vxc");
		Vtsk = data.input.Vxc;
	}
	if (inflight) {
		if (tasknotfinished) {
			// calculate speed and time for each leg plus altitude remaining at each waypoint
			altrem = data.input.inusealt;
			altnegative = false;
			for (x = data.task.numwaypts-(data.task.haslanding?2:1); x >= wptidx; x--) {
				// calculate altitude at turnpoint
				if (altrem < 0.0) altnegative = true;
				altrem -= data.activetask.alts[x];
				// calculate leg speed
				if (altrem > 0.0) {
					// final glide phase
	//				HostTraceOutputT(appErrorClass, "     FG Phase");
					Vleg = data.activetask.stfs[x]-data.activetask.hwinds[x];
				} else if ((altrem < 0.0) && !altnegative) {
					// mixed phase
	//				HostTraceOutputT(appErrorClass, "     Mixed Phase");
					ratio = -altrem/data.activetask.alts[x];
					Vleg = ratio*Vtsk + (1.0-ratio)*(data.activetask.stfs[x]-data.activetask.hwinds[x]);
				} else {
					// normal xc flight phase
	//				HostTraceOutputT(appErrorClass, "     Task Speed Phase");
					Vleg = Vtsk;
				}
				// calculate leg time
				if (Vleg > 0.0) {
	//				HostTraceOutputTL(appErrorClass, " - Vleg %s", print_horizontal_speed2(Vleg,1));
					if (data.config.AATmode != AAT_MTURN_ON) {
						if (x == wptidx) {
							// activetskway
							data.activetask.times[x] = data.inuseWaypoint.distance/Vleg * 3600.0;
						} else {
							// remaining task legs
							data.activetask.times[x] = data.task.distances[x]/Vleg * 3600.0;
						}
					} else {
	//					HostTraceOutputTL(appErrorClass, "Manual Turn Case");
						if (x < activetskway) {
							// manual turn case : distance to target
							data.activetask.times[x] = data.activetask.AATtgtdist/Vleg * 3600.0;
						} else {
							// remaining task legs
							data.activetask.times[x] = data.task.distances[x]/Vleg * 3600.0;
						}
					}
				} else {
	//				HostTraceOutputTL(appErrorClass, " - Vleg N/A");
					data.activetask.times[x] = 0;
				}
			}

			// calculate etas and time remaining
			lengthsecs = 0;
			for (x = wptidx; x < data.task.numwaypts; x++) {
				lengthsecs += data.activetask.times[x];
				data.activetask.etas[x] = lengthsecs;
			}
		} else {
			// task finished so no more time to go.
			lengthsecs = 0;
		}
//		HostTraceOutputTL(appErrorClass, "CalcTOT %s", DblToStr(lengthsecs/3600.0, 2));
	} else {
		taskdist = 0.0;
		hwind = 0.0;
		lengthsecs = 0;
		// calculate TOT using Vxc and wind on each leg
		for (x = wptidx; x <= data.task.numwaypts-(data.task.haslanding?2:1); x++) {
			// distance
			taskdist += data.task.distances[x];
			// adjust TOT due to wind
			CalcHWind(data.task.bearings[x], data.input.wnddir, data.input.wndspd, &hwind);
			lengthsecs = lengthsecs + (Int32)(data.task.distances[x]/(data.input.Vxc-hwind)*3600.0);
		}
//		HostTraceOutputTL(appErrorClass, "Vxc+Wind %s", DblToStr(lengthsecs/3600.0, 2));
	}

//	HostTraceOutputTL(appErrorClass, "--------------------------");
	return(lengthsecs);
}

void setAATdist(Int8 newAATdist)
{
	Int16 x;
	
	if (settaskreadonly) return;
	
	// cycle through the waypoints and set all targets to min, ctr or max
	tsk->aataimtype = newAATdist;
	for (x=0; x<tsk->numwaypts; x++) {
		// decide if OK to update distances
		if (!((taskIndex == 0) && (selectedTaskWayIdx < (inareasector!=-1?inareasector:activetskway)) && (numWaypts >= 1) && (data.activetask.tskstartsecs > 0)))
		// check for area waypoint
		if (tsk->waypttypes[x] & AREA) {
			switch (newAATdist) {
				case AATmax:
					CalcAreaMax(tsk, x, &tsk->targetlats[x], &tsk->targetlons[x], true);
					break;
				case AATctr:
					CalcWptDists(tsk, x, true, true);
					break;
				case AATmin:
					CalcAreaMin(tsk, x, &tsk->targetlats[x], &tsk->targetlons[x], true);
					break;
				default:
					break;
			}
			// update targets
			tsk->distlats[x] = tsk->targetlats[x];
			tsk->distlons[x] = tsk->targetlons[x];
		}
	}
	OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
}

Char* CalcTOTvar(Int32 lengthmins, Int32 mintasktime)
{
	Int32 timetogo;
//	Char tempchar[10];
	Char intchar[10];
	static char timevar[10];
	
	// build string to show TOT early or late
	timetogo = lengthmins - mintasktime;
	if (timetogo < 0) {
		// predicted to arrive early
		StrCopy(timevar, "-");
		timetogo = -timetogo;
	} else {
		// predicted to arrive late
		StrCopy(timevar, "+");
	}
	if (timetogo > 99) timetogo = 99;
	//StrCopy(tempchar, "00");
	//StrCat(tempchar, StrIToA(intchar, timetogo));
	//StrCat(timevar, Right(tempchar, 2));
	StrCat(timevar, StrIToA(intchar, timetogo));

	return(timevar);
}

double calcfinishheight()
{
	double finishheight;
	Int16 LANDOFFSET = data.task.numwaypts;

	// update finishing point altitude to 1000m below start
	// or min finish height from task rules
	if (data.task.haslanding) LANDOFFSET -= 1;
	if (data.task.rulesactive && (data.task.fgto1000m || (data.task.finishheight > 0.0))) {
		if (data.task.fgto1000m && (data.activetask.tskstartsecs > 0)) {
			// based on start height
			if (data.task.elevations[LANDOFFSET-1] < (data.flight.startheight - 999/ALTMETCONST)) {
				finishheight = data.flight.startheight - 999/ALTMETCONST;
			} else {
				finishheight = data.task.elevations[LANDOFFSET-1];
			}
		} else {
			// based on finish point
			if (data.task.finishaltref == AGL) {
				finishheight = data.task.elevations[LANDOFFSET-1] + data.task.finishheight;
			} else {
				finishheight = data.task.finishheight;
			}
		}
	} else {
		finishheight = data.task.elevations[LANDOFFSET-1];
	}
//	HostTraceOutputTL(appErrorClass, "Finish Height %s", print_altitude(finishheight));
	return(finishheight);
}

double calcstartheight()
{
 	double startheight;
 	Int16 TAKEOFFSET = 0;

	// calculate start height in MSL or AGL (above start waypoint elevation)
	if (data.task.hastakeoff) TAKEOFFSET = 1;
	if (data.task.rulesactive && (data.task.maxstartheight > 0.0)) {
		if (data.task.startaltref ==  AGL) {
			startheight = data.task.maxstartheight + data.task.elevations[TAKEOFFSET];
		} else {
			startheight = data.task.maxstartheight;
		}
	} else {
		startheight = data.task.elevations[TAKEOFFSET];
	}
//	HostTraceOutputTL(appErrorClass, "Start Height %s", print_altitude(startheight));
	return(startheight);
}

