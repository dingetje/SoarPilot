#ifndef SOARTASK_H
#define SOARTASK_H

#define STSK __attribute__ ((section ("stsk")))
#define STSK2 __attribute__ ((section ("stsk2")))
#define STSK3 __attribute__ ((section ("stsk3")))
#define STSK4 __attribute__ ((section ("stsk4")))

// Commands for refresh_task_details function
#define TASKDISP	0	// Display selected task
#define TASKACTV	1	// Activate the selected task
#define TASKNEW		2	// Add a new task
#define TASKDEL		3	// Deleted the current task or clear if task is the active task
#define TASKADDAFTER	4	// Add new waypoint after selected
#define TASKUP		5	// Move the current waypoint above the one above
#define TASKDWN		6	// Move the current waypoint below the one below
#define TASKREM		7	// Remove the current waypoint from the task
#define TASKPRV		8	// Move to the previous task
#define TASKNXT		9	// Move to the next task
#define TASKCLOSEDEL	10	// Called when the task window is closing and current should be removed or cleared
#define TASKCOPY	11	// Copy the current task to a new one
#define TASKUPNUM	12	// Update the "of" numbers
#define TASKPGUP	13	// Move up one page worth of waypoints in the task
#define TASKPGDWN	14	// Move down one page worth of waypoint in the task
#define TASKCLRACT	15	// Delete the Task without opening Alert Screen
#define TASKADDBEFORE	16	// Add new waypoint before selected
#define TASKMVUP	17	// Move selected waypoint UP
#define TASKMVDWN	18	// Move selected waypoint DOWN
#define TASKREV		19	// Move selected waypoint DOWN
#define TASKSAVETL	20	// Save the Takeoff & Landing values for the current task
#define TASKFREE	21	// Free the memory used by the task list
#define TASKDELALL	22	// Free the memory used by the task list
#define TASKCLOSEUPDATE	23	// Called when the task window is closing and current should be updated
#define TASKSETWPT	24	// set which waypoint is active

// Commands for HandleTask function
#define TSKNONE		0
#define TSKNORMAL	1
#define TSKNEW		2
#define TSKREACTIVATE	3
#define TSKDEACTIVATE	4
#define TSKREMWAY	5
#define TSKSTART	6
#define TSKFINISH	7
#define TSKTURNPOINT	8
#define TSKFORCESTART	9

// Task Add Buttons
#define TSKADDBEFBTN		0
#define TSKDEACTIVATEBTN	1
#define TSKCANCELBTN		2
#define TSKADDAFTBTN		3

#define TSKFOUND	1
#define TSKNOTFOUND	-1

#define TFCANCEL	0
#define TFOKDEL		1
#define TFOKUP		2

#define LOGGEDPTS	2

#define AATmax		0
#define AATctr		1
#define AATmin		2
#define AATusr		3

#define STATSDIST	0
#define STATSSPEED	1
#define STATSTIME	2

#define AAT_NORMAL	0
#define AAT_UPDLEGS	1
#define AAT_MTURN	2
#define AAT_MTURN_ON	3

#define BELOW_FG	0
#define ABOVE_FG	1
#define ALERT_FG	2

#define ELEVATION	0
#define STARTHEIGHT	1
#define FINISHHEIGHT	2

typedef struct TaskSave {
	Char tskstartutc[10];
	Char tskstoputc[10];
	Char tskstartdtg[7];
	Char tskstopdtg[7];
	double   tskdist;
} TaskSave;

/*****************************************************************************
 * protos
 *****************************************************************************/
void refresh_task_details(Int16 action) STSK2;
void CalcWptDists(TaskData *tsk, Int16 x, Boolean updatedists, Boolean updatetargets) STSK3;
void CalcTaskDists(TaskData *tsk, Boolean updatedists, Boolean updatetargets, Boolean updateattribs) STSK3;
void DrawTask(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng, double maxdist, double startalt, TaskData *tsk, double mapscale, Boolean ismap) STSK4;
void HandleTask(Int16 action) STSK;
Int16 FindTaskRecordByName(Char* NameString, MemHandle *output_hand, MemPtr *output_ptr) STSK3;
Boolean InSectorTest(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng) STSK3;
Boolean ProcessStart(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng) STSK3;
Boolean ProcessArcStart(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng) STSK3;
Boolean ProcessFinish(double leftbrg, double rightbrg, double inrng, double outrng, double curbrg, double currng) STSK3;
void CalcAreaMax(TaskData *task, UInt16 wptidx, double *maxlat, double *maxlon, Boolean forcecalc) STSK2;
void CalcAreaMin(TaskData *task, UInt16 wptidx, double *minlat, double *minlon, Boolean forcecalc) STSK2;
double CalcMinDistBearing(TaskData *task, UInt16 wptidx, Boolean forcecircle) STSK3;
void CalcBisectors(TaskData *task) STSK3;
void CalcStartFinishDirs(TaskData *task) STSK3;
void task_parser(Char *serinp, UInt32 length, Boolean reset) STSK3;
void OutputTasks() STSK3;
Int8 CheckForTooFew() STSK; 
double CalcCompletedTskDist() STSK;
double CalcCompletedTskSpeed() STSK;
double CalcRemainingTskFGA(TaskData *tsk, Int16 wptidx, double startalt, double MCVal) STSK2;
void HandleTaskAutoZoom(double range, double radius, Boolean reset) STSK;
Boolean HandleTaskAutoZoom2(double range, double radius, Boolean reset) STSK;
void task_start_alert(Boolean restart) STSK;
Boolean form_task_waypt_event_handler(EventPtr event) STSK;
Boolean save_task_waypt_fields(TaskData *edittsk, Int16 wptidx, Int16 tskidx, Boolean isctlpt, Boolean writetoDB) STSK;
void refresh_task_list(Int16 scr) STSK2;
Boolean form_list_task_event_handler(EventPtr event) STSK;
Int16 task_comparison(TaskData* rec1, TaskData* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h) STSK;
void MoveTaskWaypt(TaskData *tsk, Int16 dest, Int16 src) STSK2;
void UpdateSectorInfo(double xratio, double yratio) STSK4;
void CheckTurn() STSK2;
void HandleTurn() STSK4;
void task_invalid_start_alert(Boolean overstartheight, Boolean earlystart, Boolean invalidstart, Boolean gotostart) STSK3;
Boolean form_waypoint_sector_event_handler(EventPtr event) STSK3;
Boolean form_task_rules_event_handler(EventPtr event) STSK4;
void AssignDefaultRules(TaskData *tsk) STSK2;
UInt32 CalcTOT(Int16 wptidx, double MCVal) STSK4;
void setAATdist(Int8 newAATdist) STSK2;
Char* CalcTOTvar(Int32 lengthmins, Int32 mintasktime) STSK4;
double calcfinishheight() STSK2;
double calcstartheight() STSK2;

#endif
