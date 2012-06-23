#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarConf.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarDB.h"
#include "soarPLst.h"
#include "soarUMap.h"
#include "soarWind.h"
#include "soarSUA.h"
#include "soarMem.h"
#include "soarForm.h"
#include "soarCAI.h"

extern double curmapscale;
extern IndexedColorType  indexTask, indexSUA, indexSUAwarn, indexSector, indexWaypt, indexSink, indexWeak, indexStrong;
extern PolarData *inusePolar;
extern Int16     selectedPolarIndex;
extern PolarData *selectedPolar;
extern Char buf[PARSELEN];

void OutputConfig()
{
	Char output_char[100];

	Char tempchar[25];
	Char dtgstr[15];
	DateType date;
	MemHandle record_handle;
	MemPtr record_ptr;
	CAIAddData *caidata;
	Int8 i;

	DateSecondsToDate (TimGetSeconds(), &date);
	DateToAscii (date.month, date.day, date.year+1904, dfDMYLong, dtgstr);

	// Xmit Config Data
	StrCopy(output_char, "** -------------------------------------------------------------");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "**       SOARINGPILOT Version ");
	StrCat(output_char, device.appVersion);
	StrCat(output_char, " Configuration");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "**       Date: ");
	StrCat(output_char, dtgstr);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "** -------------------------------------------------------------");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit ConfigFlight Data #1
	StrCopy(output_char, "CONFIG1,");
	StrCat(output_char, DblToStr(pround(data.config.bugfactor,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.pctwater,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.safealt,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.spdunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.disunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.altunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.lftunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wgtunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wtrunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.slowlogint));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.logstartspd,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.logstopspd,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.logstoptime));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.logonoff));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.logautooff));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.mapscaleidx));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.timezone));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.nmeaspeed));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.dataspeed));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.flowctrl));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.showrgs));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.calcwind));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.tskrestarts));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit ConfigFlight Data #2
	StrCopy(output_char, "CONFIG2,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.alttype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaulttoFG));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usecalchw));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usepalmway));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.setmcval));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.optforspd));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.ldforcalcs));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.btmlabels));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.trktrail));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thzoom));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thonoff));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wayonoff));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wayline));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.waymaxlen));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.numtrkpts));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.pressaltsrc));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usepalt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.altreftype));
	StrCat(output_char, ",");
	// msr: added flight computer config info
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.flightcomp));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usetas));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.gpsmsladj,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.ctllinevis));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.gpscalcdev));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.Flarmscaleidx));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usefgterrain));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaulthome));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.RefWptRadial));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thnumtrkpts));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit ConfigFlight Data #3
	StrCopy(output_char, "CONFIG3,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.taskonoff));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.taskdrawtype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.FlrmCopyIGC));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usechksums));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.starttype));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.startrad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.startdir,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.startdirmag));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.startdirauto));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.turntype));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.turnfairad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.turncircrad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.finishtype));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.finishrad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.finishdir,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.finishdirmag));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.finishdirauto));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.qfealt,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.earthmodel));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit ConfigFlight Data #4
	StrCopy(output_char, "CONFIG4,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.usegpstime));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.SUAmaxalt,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.suaactivetypes));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.windarrow));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.inrngcalc));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.xfertype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.hwposlabel));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.mapcircrad1,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.mapcircrad2,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.tskzoom));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.nodatatime));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.fastlogint));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.keysoundon));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.mcrngmult));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.nmeaxfertype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.llfmt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.maporient));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.useiquesim));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.useiqueser));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thmaporient));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit ConfigFlight Data #5
	StrCopy(output_char, "CONFIG5,");
	StrCat(output_char, DblToStr(pround(data.config.horiz_dist,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.vert_dist,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.CheckSUAWarn));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.suawarntypes));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wndunits));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.SUArewarn,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, data.config.stcurdevcreator));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.thzoomscale,3), 3));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.autodismisstime));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.declutter,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.keepSUA));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.gpsaltref));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAwarnColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAwarnColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAwarnColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAdisponlywarned));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAhighlightwarned));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit ConfigFlight Data #6
	StrCopy(output_char, "CONFIG6,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.TaskColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.TaskColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.TaskColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SectorColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SectorColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SectorColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.BoldTask));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.BoldSUA));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.BoldSector));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.BoldTrack));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.QNHunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.autozeroQFE));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.config.SUAdispalt,0), 0));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.shownextwpt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.AATmode));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.wpformat));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.tskspdunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SUAlookahead));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.sectormaporient));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thmapscaleidx));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.windprofileorient));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit ConfigFlight Data #7
	StrCopy(output_char, "CONFIG7,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SinkColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SinkColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SinkColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WeakColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WeakColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WeakColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.StrongColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.StrongColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.StrongColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.dynamictrkcolour));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.useinputwinds));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.profilescaleidx));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.thermalprofile));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.showQFEQNHonstartup));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.totalenergy));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.output_wypts));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.output_tasks));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.autostart));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.declaretasks));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.fgtostartalt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.accuratedistcalc));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WayptColour.r));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WayptColour.g));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.WayptColour.b));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.BoldWaypt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.netto));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit ConfigFlight Data #8
	StrCopy(output_char, "CONFIG8,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.outputSMS));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SMSouttype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SMSsendtype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.SMSsendint));
	StrCat(output_char, ",");
	StrCat(output_char, data.config.SMSaddress);
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.rulesactive));
	StrCat(output_char, ",");
	StrCat(output_char,StrIToA(tempchar, (Int32)pround(data.config.defaultrules.maxstartheight, 0)));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.timebelowstart));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.mintasktime));
	StrCat(output_char, ",");
	StrCat(output_char,StrIToA(tempchar, (Int32)pround(data.config.defaultrules.finishheight, 0)));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.fgto1000m));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.startaltref));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.finishaltref));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.startlocaltime));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.startonentry));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.warnbeforestart));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.defaultrules.inclcyldist));
	StrCat(output_char, ",");
	StrCat(output_char,StrIToA(tempchar, (Int32)pround(data.config.defaultrules.startwarnheight, 0)));
	StrCat(output_char, ",");
	StrCat(output_char,StrIToA(tempchar, (Int32)data.config.thermal_turns));
	StrCat(output_char, ",");
	StrCat(output_char,StrIToA(tempchar, (Int32)data.config.SUAformat));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.mapRHfield));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.FGalert));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.echoNMEA));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.autoIGCxfer));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.declaretoSD));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit Screen Chain Data
	StrCopy(output_char, "SCREENCHAIN,");
	for (i=0; i<SCREENCHAINMAX; i++) {
		StrCat(output_char, StrIToA(tempchar, (Int32)data.config.screenchain[i]));
		if (i < SCREENCHAINMAX-1) StrCat(output_char, ",");
	}
	
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit CAIConfig Data
	AllocMem((void *)&caidata, sizeof(CAIAddData));
	OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
	MemMove(caidata, record_ptr, sizeof(CAIAddData));
	MemHandleUnlock(record_handle);
	StrCopy(output_char, "CONFIGCAI,");
	StrCat(output_char, DblToStr(pround(caidata->stfdb,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(caidata->arvrad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(caidata->apprad,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tbtwn));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->dalt));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->sinktone));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tefg));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tempunits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->barounits));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->variotype));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->pilotinfo));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)caidata->gliderinfo));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
	FreeMem((void *)&caidata);
	caidata = NULL;

//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	// Xmit PolarData
	StrCopy(output_char, "POLAR1,");
	StrCat(output_char, data.polar.name);
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.v1,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.w1,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.v2,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.w2,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.v3,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.w3,4), 4));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.maxdrywt,2), 2));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.maxwater,2), 2));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "POLAR2,");
	StrCat(output_char, DblToStr(pround(data.polar.a,6), 6));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.b,6), 6));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.c,6), 6));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.Vmin,6), 6));
	StrCat(output_char, ",");
	StrCat(output_char, DblToStr(pround(data.polar.Wsmin,6), 6));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit IGCHInfo1 Lines
	StrCopy(output_char, "IGCHINFO1,");
	StrCat(output_char, data.igchinfo.name);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.type);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.gid);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit IGCHInfo2 Lines
	StrCopy(output_char, "IGCHINFO2,");
	StrCat(output_char, data.igchinfo.cid);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.cls);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.site);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.ooid);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit IGCHInfo3 Lines
	StrCopy(output_char, "IGCHINFO3,");
	StrCat(output_char, data.igchinfo.gpsmodel);
	StrCat(output_char, ",");
	StrCat(output_char, data.igchinfo.gpsser);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit list Lines
	StrCopy(output_char, "LISTLINES,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.listlinesvert));
	StrCat(output_char, ",");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.listlineshoriz));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit Left 5-Way Action Setting
	StrCopy(output_char, "LEFTACTION,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.leftaction));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Xmit MC button Setting
	StrCopy(output_char, "MCBUTTON,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.MCbutton));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// AGM: preferred BT GPS address
	StrCopy(output_char, "BTGPSADDR,");
	for (i=0; i<6; i++) {
		StrCat(output_char, Dec2Hex(data.config.BTAddr[i]));
		if (i<5) {
			StrCat(output_char,":");
		}		
	}
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	// Xmit ENL (experimental)
	StrCopy(output_char, "ENL,");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.logenl));
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);

	return;
}

void config_parser(Char *serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	static Boolean skip = false;
	static Boolean polar1=false;
	Boolean btgpsaddr=false;
	Int8 curxfertype;
	CAIAddData *caidata;
	MemHandle record_handle;
	MemPtr record_ptr;
	Int16 polaridx;
	Int8 i;
	Int8 pos;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		polar1 = false;
		MemSet(&buf,sizeof(buf),0);
		return;
	}

//	HostTraceOutputTL(appErrorClass, "serinp:|%s|", serinp);

	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
//		HostTraceOutputTL(appErrorClass, "buf:|%s|", buf);

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
//			 HostTraceOutputTL(appErrorClass, "Parsing error->PARSELEN Chars");
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {
//				HostTraceOutputTL(appErrorClass, "Data: %s",buf);

				GetField(buf, 0, tempchar);

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else if (StrCompare(tempchar, "CONFIG1") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG1 record");

					//	Getting the bugfactor
					if (GetField(buf, 1, tempchar)) {
						data.config.bugfactor = StrToDbl(tempchar);
						if ( data.config.bugfactor < 0.5 )
						{
							data.config.bugfactor = 0.5;
						}
					}
					//	Getting the pctwater
					if (GetField(buf, 2, tempchar)) {
						data.config.pctwater = StrToDbl(tempchar);
					}
					
					//	Getting the safety height
					if (GetField(buf, 3, tempchar)) {
						data.config.safealt = StrToDbl(tempchar);
					}
					//	Getting the speed units
					if (GetField(buf, 4, tempchar)) {
						data.config.spdunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the distance units
					if (GetField(buf, 5, tempchar)) {
						data.config.disunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the altitude units
					if (GetField(buf, 6, tempchar)) {
						data.config.altunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the lift units
					if (GetField(buf, 7, tempchar)) {
						data.config.lftunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the weight units
					if (GetField(buf, 8, tempchar)) {
						data.config.wgtunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the water units
					if (GetField(buf, 9, tempchar)) {
						data.config.wtrunits = (Int8)StrAToI(tempchar);
					}
					//	Getting the slow logger interval
					if (GetField(buf, 10, tempchar)) {
						data.config.slowlogint = (Int32)StrAToI(tempchar);
					}
					//	Getting the logger start speed
					if (GetField(buf, 11, tempchar)) {
						data.config.logstartspd = StrToDbl(tempchar);
					}
					//	Getting the logger stop speed
					if (GetField(buf, 12, tempchar)) {
						data.config.logstopspd = StrToDbl(tempchar);
					}
					//	Getting the logger stop time
					if (GetField(buf, 13, tempchar)) {
						data.config.logstoptime = (Int32)StrAToI(tempchar);
						if (data.config.logstoptime < 60) data.config.logstoptime = 60;
					}
					//	Getting the logger on/off
					if (GetField(buf, 14, tempchar)) {
						data.config.logonoff = (Boolean)StrAToI(tempchar);
					}
					//	Getting the logger auto off
					if (GetField(buf, 15, tempchar)) {
						data.config.logautooff = (Boolean)StrAToI(tempchar);
					}
					//	Getting the mapscaleidx
					if (GetField(buf, 16, tempchar)) {
						data.config.mapscaleidx = (Int8)StrAToI(tempchar);
					}
					if (data.config.mapscaleidx <= 0 || data.config.mapscaleidx > 20) {
						data.config.mapscaleidx = 10;
					}
					//	Getting the timezone
					if (GetField(buf, 17, tempchar)) {
						data.config.timezone = (Int8)StrAToI(tempchar);
					}
					//	Getting the nmea port speed
					if (GetField(buf, 18, tempchar)) {
						data.config.nmeaspeed = (UInt16)StrAToI(tempchar);
					}
					//	Getting the data transfer speed
					if (GetField(buf, 19, tempchar)) {
						data.config.dataspeed = (UInt16)StrAToI(tempchar);
					}
					//	Getting the flowcontrol setting
					if (GetField(buf, 20, tempchar)) {
						data.config.flowctrl = (Boolean)StrAToI(tempchar);
					}
					//	Getting the show required ground speed setting
					if (GetField(buf, 21, tempchar)) {
						data.config.showrgs = (Boolean)StrAToI(tempchar);
					}
					//	Getting the calculate wind setting
					if (GetField(buf, 22, tempchar)) {
						data.config.calcwind = (Boolean)StrAToI(tempchar);
					}
					//	Getting the allow task re-starts setting
					if (GetField(buf, 23, tempchar)) {
						data.config.tskrestarts = (Boolean)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG2") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG2 record");

					//	Getting the Show Required Altitude(vs. Arrival Alt) setting
					if (GetField(buf, 1, tempchar)) {
						data.config.alttype = (UInt8)StrAToI(tempchar);
					}
					//	Getting the default screen (Final Glide or Moving Map)
					if (GetField(buf, 2, tempchar)) {
						data.config.defaulttoFG = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Use Calculated Headwind setting
					if (GetField(buf, 3, tempchar)) {
						data.config.usecalchw = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Use Palm Waypoints setting
					if (GetField(buf, 4, tempchar)) {
						data.config.usepalmway = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Set the MC Value
					if (GetField(buf, 5, tempchar)) {
						data.config.setmcval = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Optimize Final Glide Calcs for Speed vs. Distance
					if (GetField(buf, 6, tempchar)) {
						data.config.optforspd = (Boolean)StrAToI(tempchar);
					}
					//	Getting the L/D value to use in Waypoint List
					if (GetField(buf, 7, tempchar)) {
						data.config.ldforcalcs = (Int8)StrAToI(tempchar);
					}
					//	Getting the Map Bottom Labels On/Off
					if (GetField(buf, 8, tempchar)) {
						data.config.btmlabels = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Track Trail On/Off
					if (GetField(buf, 9, tempchar)) {
						data.config.trktrail = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Map Auto Thermal Zoom
					if (GetField(buf, 10, tempchar)) {
						data.config.thzoom = (Int8)StrAToI(tempchar);
					}
					//	Getting the Map Plotting of Thermal Markers On/Off 
					if (GetField(buf, 11, tempchar)) {
						data.config.thonoff = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Drawing of Waypoints on Map On/Off
					if (GetField(buf, 12, tempchar)) {
						data.config.wayonoff = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Drawing Waypoint Connect Line On/Off
					if (GetField(buf, 13, tempchar)) {
						data.config.wayline = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Max Length of Waypoint Labels on Map
					if (GetField(buf, 14, tempchar)) {
						data.config.waymaxlen = (Int8)StrAToI(tempchar);
					}
					//	Getting the Number of Track Trail Points to Plot on Map
					if (GetField(buf, 15, tempchar)) {
						data.config.numtrkpts = (UInt16)StrAToI(tempchar);
					}
					//	Getting the Pressure Altitude Source
					if (GetField(buf, 16, tempchar)) {
						data.config.pressaltsrc = (Int8)StrAToI(tempchar);
					}
					//	Getting the Use Pressure Altitude On/Off
					if (GetField(buf, 17, tempchar)) {
						data.config.usepalt = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Altitude Reference Setting
					// Can be MSL, QFE or AGL
					if (GetField(buf, 18, tempchar)) {
						data.config.altreftype = (Int8)StrAToI(tempchar);
					}
					// msr: Getting Flight Computer type
					if (GetField(buf, 19, tempchar)) {
						data.config.flightcomp = (Int8)StrAToI(tempchar);
					}
					// msr: Getting the Use TAS On/Off
					if (GetField(buf, 20, tempchar)) {
						data.config.usetas = (Boolean)StrAToI(tempchar);
					}
					// getting the GPS MSL adjust value
					if (GetField(buf, 21, tempchar)) {
						data.config.gpsmsladj = StrToDbl(tempchar);
					}
					//	Getting the Control Line Visible On/Off 
					if (GetField(buf, 22, tempchar)) {
						data.config.ctllinevis = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Calc Deviation Setting 
					if (GetField(buf, 23, tempchar)) {
						data.config.gpscalcdev = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task start height altitude ref setting
					if (GetField(buf, 24, tempchar)) {
						data.config.Flarmscaleidx = (Int8)StrAToI(tempchar);
					}
					if (data.config.Flarmscaleidx <= 0 || data.config.Flarmscaleidx > 20) {
						data.config.Flarmscaleidx = 7;
					}
					//	Getting the use terrain in final glide setting
					if (GetField(buf, 25, tempchar)) {
						data.config.usefgterrain = (Boolean)StrAToI(tempchar);
					}
					//	Getting the default to home waypoint setting
					if (GetField(buf, 26, tempchar)) {
						data.config.defaulthome = (Boolean)StrAToI(tempchar);
					}
					//	Getting the use radial for ref wpt setting
					if (GetField(buf, 27, tempchar)) {
						data.config.RefWptRadial = (Boolean)StrAToI(tempchar);
					}
					//	Getting the Number of Track Trail Points to Plot in Thermal Mode
					if (GetField(buf, 28, tempchar)) {
						data.config.thnumtrkpts = (UInt16)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG3") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG3 record");

					//	Getting the task on/off setting
					if (GetField(buf, 1, tempchar)) {
						data.config.taskonoff = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task drawking type
					if (GetField(buf, 2, tempchar)) {
						data.config.taskdrawtype = (Int8)StrAToI(tempchar);
					}
					//	Getting the copy IGC info to Flarm setting
					if (GetField(buf, 3, tempchar)) {
						data.config.FlrmCopyIGC = (Boolean)StrAToI(tempchar);
					}
					//	Getting the use checksums setting
					if (GetField(buf, 4, tempchar)) {
						data.config.usechksums = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task start type
					if (GetField(buf, 5, tempchar)) {
						data.config.starttype = (Int8)StrAToI(tempchar);
					}
					//	Getting the task start radius
					if (GetField(buf, 6, tempchar)) {
						data.config.startrad = StrToDbl(tempchar);
					}
					//	Getting the task start direction
					if (GetField(buf, 7, tempchar)) {
						data.config.startdir = StrToDbl(tempchar);
					}
					//	Getting the task start dir mag setting showing how to use the start dir value
					if (GetField(buf, 8, tempchar)) {
						data.config.startdirmag = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task start dir auto setting showing whether the start dir is
					// calculated automatically
					if (GetField(buf, 9, tempchar)) {
						data.config.startdirauto = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task turn type
					if (GetField(buf, 10, tempchar)) {
						data.config.turntype = (Int8)StrAToI(tempchar);
					}
					//	Getting the task turn radius if using FAI sectors
					if (GetField(buf, 11, tempchar)) {
						data.config.turnfairad = StrToDbl(tempchar);
					}
					//	Getting the task turn radius if using circular sectors
					if (GetField(buf, 12, tempchar)) {
						data.config.turncircrad = StrToDbl(tempchar);
					}
					//	Getting the task finish type
					if (GetField(buf, 13, tempchar)) {
						data.config.finishtype = (Int8)StrAToI(tempchar);
					}
					//	Getting the task finish radius
					if (GetField(buf, 14, tempchar)) {
						data.config.finishrad = StrToDbl(tempchar);
					}
					//	Getting the task finish direction
					if (GetField(buf, 15, tempchar)) {
						data.config.finishdir = StrToDbl(tempchar);
					}
					//	Getting the task finish dir mag setting showing how to use the finish dir value
					if (GetField(buf, 16, tempchar)) {
						data.config.finishdirmag = (Boolean)StrAToI(tempchar);
					}
					//	Getting the task finish dir auto setting showing whether the finish dir is
					// calculated automatically
					if (GetField(buf, 17, tempchar)) {
						data.config.finishdirauto = (Boolean)StrAToI(tempchar);
					}
					//	Getting the QNH 
					if (GetField(buf, 18, tempchar)) {
						data.config.qfealt = StrToDbl(tempchar);
					}
					//	Getting the Use WGS84 setting
					// True = WGS84  False = FAI
					if (GetField(buf, 19, tempchar)) {
						data.config.earthmodel = (Int8)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG4") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG4 record");

					//	Getting the use gps to set time setting
					if (GetField(buf, 1, tempchar)) {
						data.config.usegpstime = (Boolean)StrAToI(tempchar);
					}
					// Getting the Max Altitude for SUA data
					if (GetField(buf, 2, tempchar)) {
						data.config.SUAmaxalt = StrToDbl(tempchar);
					}
					// Getting the Active Types of SUA data
					if (GetField(buf, 3, tempchar)) {
						data.config.suaactivetypes = (Int32)StrAToI(tempchar);
					}
					// Determine whether to display the wind arrow on map
					if (GetField(buf, 4, tempchar)) {
						data.config.windarrow = (Boolean)StrAToI(tempchar);
					}
					// Determine whether to display the final glide around info on map
					if (GetField(buf, 5, tempchar)) {
						data.config.inrngcalc = (Boolean)StrAToI(tempchar);
					}
					if (GetField(buf, 6, tempchar)) {
						curxfertype = data.config.xfertype;
						data.config.xfertype = (Int8)StrAToI(tempchar);
						if (!device.BTCapable && data.config.xfertype == USEBT) {
							data.config.xfertype = USESER;
						} else if (!device.iQueCapable && (data.config.xfertype == USEIQUE || data.config.xfertype == USEIQUESER)) {
							data.config.xfertype = USESER;
						}
						// This fixes a problem with trying to reset the wrong xfer type
						if (data.config.xfertype != curxfertype) {
							data.config.xfertype = curxfertype;
						}
					}
					// Determine whether to display the headwind as positive or negative
					if (GetField(buf, 7, tempchar)) {
					data.config.hwposlabel = (Boolean)StrAToI(tempchar);
					}
					//	Getting the first map circle radius
					if (GetField(buf, 8, tempchar)) {
						data.config.mapcircrad1 = StrToDbl(tempchar);
					} else {
						data.config.mapcircrad1 = 5.0;
					}
					//	Getting the Second map circle radius
					if (GetField(buf, 9, tempchar)) {
						data.config.mapcircrad2 = StrToDbl(tempchar);
					} else {
						data.config.mapcircrad2 = 10.0;
					}
					// Determine whether using hi or low task auto zooming
					if (GetField(buf, 10, tempchar)) {
						data.config.tskzoom = (Boolean)StrAToI(tempchar);
					} else {
						data.config.tskzoom = true;
					}
					// Getting the how long to wait if no data/no sat fix from GPS before
					// stopping the logger
					if (GetField(buf, 11, tempchar)) {
						data.config.nodatatime = (Int32)StrAToI(tempchar);
						if (data.config.nodatatime < 60) data.config.nodatatime = 60;
					}
					// Getting the fast logger interval value
					if (GetField(buf, 12, tempchar)) {
						data.config.fastlogint = (Int32)StrAToI(tempchar);
					}
					// Getting the Key Sound setting
					if (GetField(buf, 13, tempchar)) {
						data.config.keysoundon = (Boolean)StrAToI(tempchar);
					} 
					// Getting the MC Current Range Multiplier
					if (GetField(buf, 14, tempchar)) {
						data.config.mcrngmult = (Int16)StrAToI(tempchar);
					}
					// Getting the NMEA Input Type
					if (GetField(buf, 15, tempchar)) {
						data.config.nmeaxfertype = (Int8)StrAToI(tempchar);
						if (!device.BTCapable && data.config.nmeaxfertype == USEBT) {
							data.config.nmeaxfertype = USESER;
						} else if (!device.iQueCapable &&
										(data.config.nmeaxfertype == USEIQUE || data.config.nmeaxfertype == USEIQUESER)) {
							data.config.nmeaxfertype = USESER;
						}
					} else {
						data.config.nmeaxfertype = USESER;
					}
					// Getting the Lat/Long Format setting
					if (GetField(buf, 16, tempchar)) {
						data.config.llfmt = (Int8)StrAToI(tempchar);
					}
					// Getting map orientation/mode 
					if (GetField(buf, 17, tempchar)) {
						data.config.maporient = (Int8)StrAToI(tempchar);
					}
					// Getting the Use iQue Sim setting
					if (GetField(buf, 18, tempchar)) {
						data.config.useiquesim = (Boolean)StrAToI(tempchar);
					} 
					// Getting the Use iQue Serial setting
					if (GetField(buf, 19, tempchar)) {
						data.config.useiqueser = (Boolean)StrAToI(tempchar);
					} 
					// Getting map orientation for thermal mode 
					if (GetField(buf, 20, tempchar)) {
						data.config.thmaporient = (Int8)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG5") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG5 record");

					//	Getting the SUA Warning Horizontal Distance
					if (GetField(buf, 1, tempchar)) {
						data.config.horiz_dist = StrToDbl(tempchar);
					} else {
						data.config.horiz_dist = 1.0;
					}

					//	Getting the SUA Warning Vertical Distance
					if (GetField(buf, 2, tempchar)) {
						data.config.vert_dist = StrToDbl(tempchar);
					} else {
						data.config.vert_dist = 500.0;
					}

					// Getting the Check SUA Warnings setting
					if (GetField(buf, 3, tempchar)) {
						data.config.CheckSUAWarn = (Boolean)StrAToI(tempchar);
					} 

					// Getting the SUA Warnings Types setting
					if (GetField(buf, 4, tempchar)) {
						data.config.suawarntypes = (Int32)StrAToI(tempchar);
					} else {
						data.config.suawarntypes = SUAALL;
					}

					// Getting the Wind Speed Units Type setting
					if (GetField(buf, 5, tempchar)) {
						data.config.wndunits = (Int8)StrAToI(tempchar);
					} else {
						data.config.wndunits = NAUTICAL;
					}

					//	Getting the SUA Urgent Warning Ratio
					if (GetField(buf, 6, tempchar)) {
						data.config.SUArewarn = StrToDbl(tempchar);
					} else {
						data.config.SUArewarn = 0.33;
					}

					// Getting the Minimum Task Length in Seconds
					// Set to zero for disabled.
					if (GetField(buf, 7, tempchar)) {
						data.config.stcurdevcreator = (UInt32)StrAToI(tempchar);
					} else {
						data.config.stcurdevcreator = 0;
					}

					//	Getting the thermal zoom scale
					if (GetField(buf, 8, tempchar)) {
						data.config.thzoomscale = StrToDbl(tempchar);
					} else {
						data.config.thzoomscale = THZOOMSCALELO;
					}

					//	Getting the Auto Dismiss Time
					if (GetField(buf, 9, tempchar)) {
						data.config.autodismisstime = (UInt32)StrToDbl(tempchar);
					} else {
						data.config.autodismisstime = 120;
					}

					//	Getting the Declutter value
					if (GetField(buf, 10, tempchar)) {
						data.config.declutter = StrToDbl(tempchar);
					} else {
						data.config.declutter = 0.0;
					}

					//	Keep SUA in thermal mode value
					if (GetField(buf, 11, tempchar)) {
						data.config.keepSUA = (Boolean)StrAToI(tempchar);
					} 

					//	Getting the GPS Altitude Reference Value
					if (GetField(buf, 12, tempchar)) {
						data.config.gpsaltref = (Int8)StrAToI(tempchar);
					} 

					// Getting the SUA warn line colour settings
					if (GetField(buf, 13, tempchar)) {
						data.config.SUAwarnColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 14, tempchar)) {
						data.config.SUAwarnColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 15, tempchar)) {
						data.config.SUAwarnColour.b = (UInt8)StrAToI(tempchar);
					} 
					// SUA warn lines
					if (device.colorCapable) {
						indexSUAwarn = RGB(data.config.SUAwarnColour.r, data.config.SUAwarnColour.g, data.config.SUAwarnColour.b);
					}

					// Getting the SUA Display only Warned items setting
					if (GetField(buf, 16, tempchar)) {
						data.config.SUAdisponlywarned = (Boolean)StrAToI(tempchar);
					} 

					// Getting the Highlight Warned SUA setting
					if (GetField(buf, 17, tempchar)) {
						data.config.SUAhighlightwarned = (Boolean)StrAToI(tempchar);
					} 
				}

				if (StrCompare(tempchar, "CONFIG6") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG6 record");


					// Getting the Task line colour settings
					if (GetField(buf, 1, tempchar)) {
						data.config.TaskColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 2, tempchar)) {
						data.config.TaskColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 3, tempchar)) {
						data.config.TaskColour.b = (UInt8)StrAToI(tempchar);
					} 
					// Task lines
					if (device.colorCapable) {
						indexTask = RGB(data.config.TaskColour.r, data.config.TaskColour.g, data.config.TaskColour.b);
					}

					// Getting the SUA line colour settings
					if (GetField(buf, 4, tempchar)) {
						data.config.SUAColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 5, tempchar)) {
						data.config.SUAColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 6, tempchar)) {
						data.config.SUAColour.b = (UInt8)StrAToI(tempchar);
					} 
					// SUA lines
					if (device.colorCapable) {
						indexSUA = RGB(data.config.SUAColour.r, data.config.SUAColour.g, data.config.SUAColour.b);
					}

					// Getting the Sector line colour settings
					if (GetField(buf, 7, tempchar)) {
						data.config.SectorColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 8, tempchar)) {
						data.config.SectorColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 9, tempchar)) {
						data.config.SectorColour.b = (UInt8)StrAToI(tempchar);
					} 
					// Sectors
					if (device.colorCapable) {
						indexSector = RGB(data.config.SectorColour.r, data.config.SectorColour.g, data.config.SectorColour.b);
					}

					// getting the Bold Task line setting
					if (GetField(buf, 10, tempchar)) {
						data.config.BoldTask = (Boolean)StrAToI(tempchar);
					}
					// getting the Bold SUA line setting
					if (GetField(buf, 11, tempchar)) {
						data.config.BoldSUA = (Boolean)StrAToI(tempchar);
					}
					// getting the Bold Sector line setting
					if (GetField(buf, 12, tempchar)) {
						data.config.BoldSector = (Boolean)StrAToI(tempchar);
					}
					// getting the Bold Sector line setting
					if (GetField(buf, 13, tempchar)) {
						data.config.BoldTrack = (Boolean)StrAToI(tempchar);
					}
					// getting the QNH in mb setting
					if (GetField(buf, 14, tempchar)) {
						data.config.QNHunits = (Int8)StrAToI(tempchar);
					}
					// getting the auto zero QFE line setting
					if (GetField(buf, 15, tempchar)) {
						data.config.autozeroQFE = (Boolean)StrAToI(tempchar);
					}
					// getting the SUA display altitude setting
					if (GetField(buf, 16, tempchar)) {
						data.config.SUAdispalt = StrToDbl(tempchar);
					}
					// getting the show next waypoint line setting
					if (GetField(buf, 17, tempchar)) {
						data.config.shownextwpt = (Int8)StrAToI(tempchar);
					}
					// getting the AAT update task legs
					if (GetField(buf, 18, tempchar)) {
						data.config.AATmode =(Int8)StrAToI(tempchar);
					}
					// getting the transfer waypoint type
					if (GetField(buf, 19, tempchar)) {
						data.config.wpformat = (Int8)StrAToI(tempchar);
					}
					// getting the task speed units setting
					if (GetField(buf, 20, tempchar)) {
						data.config.tskspdunits = (Int8)StrAToI(tempchar);
					}
					// getting the SUA look ahead time setting
					if (GetField(buf, 21, tempchar)) {
						data.config.SUAlookahead = (UInt32)StrAToI(tempchar);
					}
					// getting the sector view map orientation setting
					if (GetField(buf, 22, tempchar)) {
						data.config.sectormaporient = (Int8)StrAToI(tempchar);
					}
					// getting the thermal mode map scale setting
					if (GetField(buf, 23, tempchar)) {
						data.config.thmapscaleidx = (Int8)StrAToI(tempchar);
					}
					if (data.config.thmapscaleidx <= 0 || data.config.thmapscaleidx > 20) {
						data.config.thmapscaleidx = 1;
					}
					// getting the wind profile orientation setting
					if (GetField(buf, 24, tempchar)) {
						data.config.windprofileorient = (Int8)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG7") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG7 record");

					// Getting the Sink line colour settings
					if (GetField(buf, 1, tempchar)) {
						data.config.SinkColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 2, tempchar)) {
						data.config.SinkColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 3, tempchar)) {
						data.config.SinkColour.b = (UInt8)StrAToI(tempchar);
					} 
					// Sink lines
					if (device.colorCapable) {
						indexSink = RGB(data.config.SinkColour.r, data.config.SinkColour.g, data.config.SinkColour.b);
					}

					// Getting the Weak line colour settings
					if (GetField(buf, 4, tempchar)) {
						data.config.WeakColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 5, tempchar)) {
						data.config.WeakColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 6, tempchar)) {
						data.config.WeakColour.b = (UInt8)StrAToI(tempchar);
					} 
					// Weak lines
					if (device.colorCapable) {
						indexWeak = RGB(data.config.WeakColour.r, data.config.WeakColour.g, data.config.WeakColour.b);
					}

					// Getting the Strong line colour settings
					if (GetField(buf, 7, tempchar)) {
						data.config.StrongColour.r = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 8, tempchar)) {
						data.config.StrongColour.g = (UInt8)StrAToI(tempchar);
					} 
					if (GetField(buf, 9, tempchar)) {
						data.config.StrongColour.b = (UInt8)StrAToI(tempchar);
					} 
					// Strong
					if (device.colorCapable) {
						indexStrong = RGB(data.config.StrongColour.r, data.config.StrongColour.g, data.config.StrongColour.b);
					}
					// get the include cylinder distance setting
					if (GetField(buf, 10, tempchar)) {
						data.config.dynamictrkcolour = (Boolean)StrAToI(tempchar);
					}
					// get the use input winds setting
					if (GetField(buf, 11, tempchar)) {
						data.config.useinputwinds = (Boolean)StrAToI(tempchar);
					}
					// get the use wind profile step setting
					if (GetField(buf, 12, tempchar)) {
						data.config.profilescaleidx = (Int16)StrAToI(tempchar);
					}
					// get the use show thermal profile setting
					if (GetField(buf, 13, tempchar)) {
						data.config.thermalprofile = (Boolean)StrAToI(tempchar);
					}
					// get the show QFE.QNH on startup setting
					if (GetField(buf, 14, tempchar)) {
						data.config.showQFEQNHonstartup = (Boolean)StrAToI(tempchar);
					}

					// get the total energy setting
					if (GetField(buf, 15, tempchar)) {
						data.config.totalenergy = (Boolean)StrAToI(tempchar);
					}

					// get the output waypoints setting
					if (GetField(buf, 16, tempchar)) {
						data.config.output_wypts = (Boolean)StrAToI(tempchar);
					}

					// get the output tasks setting
					if (GetField(buf, 17, tempchar)) {
						data.config.output_tasks = (Boolean)StrAToI(tempchar);
					}

					// get the auto re-start setting
					if (GetField(buf, 18, tempchar)) {
						data.config.autostart = (Boolean)StrAToI(tempchar);
					}

					// get the declare tasks setting
					if (GetField(buf, 19, tempchar)) {
						data.config.declaretasks = (Boolean)StrAToI(tempchar);
					}

					// get the final glide to start altitude setting
					if (GetField(buf, 20, tempchar)) {
						data.config.fgtostartalt = (Boolean)StrAToI(tempchar);
					}

					// get the accurate distance calculation setting
					if (GetField(buf, 21, tempchar)) {
						data.config.accuratedistcalc = (Boolean)StrAToI(tempchar);
					}

					// Getting the Waypoint line colour settings
					if (GetField(buf, 22, tempchar)) {
						data.config.WayptColour.r = (UInt8)StrAToI(tempchar);
					}
					if (GetField(buf, 23, tempchar)) {
						data.config.WayptColour.g = (UInt8)StrAToI(tempchar);
					}
					if (GetField(buf, 24, tempchar)) {
						data.config.WayptColour.b = (UInt8)StrAToI(tempchar);
					}
					// Sectors
					if (device.colorCapable) {
						indexWaypt = RGB(data.config.WayptColour.r, data.config.WayptColour.g, data.config.WayptColour.b);
					}

					// getting the Bold Waypoint line setting
					if (GetField(buf, 25, tempchar)) {
						data.config.BoldWaypt = (Boolean)StrAToI(tempchar);
					}

					// get the netto setting
					if (GetField(buf, 26, tempchar)) {
						data.config.netto = (Boolean)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "CONFIG8") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIG8 record");

					// get the show output SMS setting
					if (GetField(buf, 1, tempchar)) {
						data.config.outputSMS = (Boolean)StrAToI(tempchar);
					}
					// get the SMS output type setting
					if (GetField(buf, 2, tempchar)) {
						data.config.SMSouttype = (Int8)StrAToI(tempchar);
					}
					// get the SMS send type setting
					if (GetField(buf, 3, tempchar)) {
						data.config.SMSsendtype = (Int16)StrAToI(tempchar);
					}
					// get the SMS int type setting
					if (GetField(buf, 4, tempchar)) {
						data.config.SMSsendint = (Int32)StrAToI(tempchar);
					}
					// get the SMS send address setting
					if (GetField(buf, 5, tempchar)) {
						StrNCopy(data.config.SMSaddress, tempchar, 40);
					}

					// task rules
					// getting the rules active setting
					if (GetField(buf, 6, tempchar)) {
						data.config.defaultrules.rulesactive = (Boolean)StrAToI(tempchar);
					}
					
					// getting the max start height
					if (GetField(buf, 7, tempchar)) {
						data.config.defaultrules.maxstartheight = StrToDbl(tempchar);
					}
				
					// getting the min time below start height
					if (GetField(buf, 8, tempchar)) {
						data.config.defaultrules.timebelowstart = (Int32)StrAToI(tempchar);
					}
				
					// getting the min task time
					if (GetField(buf, 9, tempchar)) {
						data.config.defaultrules.mintasktime = (Int32)StrAToI(tempchar);
					}

					// getting the finish height
					if (GetField(buf, 10, tempchar)) {
						data.config.defaultrules.finishheight = StrToDbl(tempchar);
					}

					// getting the final glide to 1000m below start height setting
					if (GetField(buf, 11, tempchar)) {
						data.config.defaultrules.fgto1000m = (Boolean)StrAToI(tempchar);
					}

					// getting the start altitude reference setting
					if (GetField(buf, 12, tempchar)) {
						data.config.defaultrules.startaltref = (Int8)StrAToI(tempchar);
					}

					// getting the finish altitude reference setting
					if (GetField(buf, 13, tempchar)) {
						data.config.defaultrules.finishaltref = (Int8)StrAToI(tempchar);
					}

					// getting the start local time setting
					if (GetField(buf, 14, tempchar)) {
						data.config.defaultrules.startlocaltime = (Int32)StrAToI(tempchar);
					}

					// getting the start on entry setting
					if (GetField(buf, 15, tempchar)) {
						data.config.defaultrules.startonentry = (Boolean)StrAToI(tempchar);
					}

					// getting the warn before start setting
					if (GetField(buf, 16, tempchar)) {
						data.config.defaultrules.warnbeforestart = (Boolean)StrAToI(tempchar);
					}

					// getting the include cylinder distance setting
					if (GetField(buf, 17, tempchar)) {
						data.config.defaultrules.inclcyldist = (Boolean)StrAToI(tempchar);
					}

					// getting the start warn height
					if (GetField(buf, 18, tempchar)) {
						data.config.defaultrules.startwarnheight = StrToDbl(tempchar);
					}

					// getting the thermal turns
					if (GetField(buf, 19, tempchar)) {
						data.config.thermal_turns = (Int8)StrAToI(tempchar);
					}

					// getting the SUA format
					if (GetField(buf, 20, tempchar)) {
						data.config.SUAformat = (Int8)StrAToI(tempchar);
					}

					// getting the right hand map field for non-DIA palms
					if (GetField(buf, 21, tempchar)) {
						data.config.mapRHfield = (Int8)StrAToI(tempchar);
					}

					// getting the final glide alert
					if (GetField(buf, 22, tempchar)) {
						data.config.FGalert = (Boolean)StrAToI(tempchar);
					}

					// getting the echo NMEA
					if (GetField(buf, 23, tempchar)) {
						data.config.echoNMEA = (Boolean)StrAToI(tempchar);
					}

					// getting the Auto IGC Transfer setting
					if (GetField(buf, 24, tempchar)) {
						data.config.autoIGCxfer = (Boolean)StrAToI(tempchar);

					}

					// getting the Declare to SD setting
					if (GetField(buf, 25, tempchar)) {
						data.config.declaretoSD = (Boolean)StrAToI(tempchar);
					}
				}
				
				if (StrCompare(tempchar, "SCREENCHAIN") == 0) {
//					HostTraceOutputTL(appErrorClass, "SCREENCHAIN record");

					// Getting the screen chain settings
					for (i=0; i<SCREENCHAINMAX; i++) {
						if (GetField(buf, i+1, tempchar)) {
							data.config.screenchain[i] = (UInt16)StrAToI(tempchar);
							// to capture change in form.h
							if (data.config.screenchain[i] == form_set_scrorder_noscrold) data.config.screenchain[i] = form_set_scrorder_noscr;
						} 
					}
				}
				
				if (StrCompare(tempchar, "CONFIGCAI") == 0) {
//					HostTraceOutputTL(appErrorClass, "CONFIGCAI record");

					AllocMem((void *)&caidata, sizeof(CAIAddData));
					OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
					MemMove(caidata, record_ptr, sizeof(CAIAddData));
					MemHandleUnlock(record_handle);			

					// Getting the CAI settings
					if (GetField(buf, 1, tempchar)) {
						caidata->stfdb = StrToDbl(tempchar);
					} 
					if (GetField(buf, 2, tempchar)) {
						caidata->arvrad = StrToDbl(tempchar);
					} 
					if (GetField(buf, 3, tempchar)) {
						caidata->apprad = StrToDbl(tempchar);
					} 
					if (GetField(buf, 4, tempchar)) {
						caidata->tbtwn = (Int16)StrAToI(tempchar);
					}
					if (GetField(buf, 5, tempchar)) {
						caidata->dalt = (Boolean)StrAToI(tempchar);
					}
					if (GetField(buf, 6, tempchar)) {
						caidata->sinktone = (Boolean)StrAToI(tempchar);
					}
					if (GetField(buf, 7, tempchar)) {
						caidata->tefg = (Boolean)StrAToI(tempchar);
					}
					if (GetField(buf, 8, tempchar)) {
						caidata->tempunits = (Int8)StrAToI(tempchar);
					}
					if (GetField(buf, 9, tempchar)) {
						caidata->barounits = (Int8)StrAToI(tempchar);
					}
					if (GetField(buf, 10, tempchar)) {
						caidata->variotype = (Int16)StrAToI(tempchar);
					}
					if (GetField(buf, 11, tempchar)) {
						caidata->pilotinfo = (Boolean)StrAToI(tempchar);
					}
					if (GetField(buf, 12, tempchar)) {
						caidata->gliderinfo = (Boolean)StrAToI(tempchar);
					}

					OpenDBUpdateRecord(config_db, sizeof(CAIAddData), caidata, CAIINFO_REC);
					FreeMem((void *)&caidata);
				}

				if (StrCompare(tempchar, "POLAR1") == 0) {
//					HostTraceOutputTL(appErrorClass, "POLAR1 record");

					polar1 = true;
					//	Getting the name
					if (GetField(buf, 1, tempchar)) {
						StrCopy(data.polar.name, tempchar);
					}
					//	Getting the v1
					if (GetField(buf, 2, tempchar)) {
						data.polar.v1 = StrToDbl(tempchar);
					}
					//	Getting the w1
					if (GetField(buf, 3, tempchar)) {
						data.polar.w1 = StrToDbl(tempchar);
						if (data.polar.w1 > 0.0) {
							data.polar.w1 *= -1.0;
						}
					}
					//	Getting the v2
					if (GetField(buf, 4, tempchar)) {
						data.polar.v2 = StrToDbl(tempchar);
					}
					//	Getting the w2
					if (GetField(buf, 5, tempchar)) {
						data.polar.w2 = StrToDbl(tempchar);
						if (data.polar.w2 > 0.0) {
							data.polar.w2 *= -1.0;
						}
					}
					//	Getting the v3
					if (GetField(buf, 6, tempchar)) {
						data.polar.v3 = StrToDbl(tempchar);
					}
					//	Getting the w3
					if (GetField(buf, 7, tempchar)) {
						data.polar.w3 = StrToDbl(tempchar);
						if (data.polar.w3 > 0.0) {
							data.polar.w3 *= -1.0;
						}
					}
					//	Getting the Max Dry Weight
					if (GetField(buf, 8, tempchar)) {
						data.polar.maxdrywt = StrToDbl(tempchar);
					}
					//	Getting the Max Water Volume
					if (GetField(buf, 9, tempchar)) {
						data.polar.maxwater = StrToDbl(tempchar);
					}
					CalcPolarABC(&data.polar, data.config.bugfactor);

					polaridx = FindPolarRecordByName(data.polar.name);
					if (polaridx == -1) {
						// make new record
						OpenDBAddRecord(polar_db, sizeof(PolarData), &data.polar, true);
					} else {
						// update existing record 
						OpenDBUpdateRecord(polar_db, sizeof(PolarData), &data.polar, polaridx);
						if (polaridx != 0) {
							// move updated record to the top of the database
							DmMoveRecord(polar_db, polaridx, 0);
						}
					}
					*inusePolar = data.polar;
					*selectedPolar = data.polar;
					selectedPolarIndex = 0;
				}

//	Not used anymore as a,b,c are calculated on load
//				if (StrCompare(tempchar, "POLAR2") == 0) {
//					HostTraceOutputTL(appErrorClass, "POLAR2 record");
//
//					if (polar1) {
//						polar1 = false;
//						//	Getting the a
//						if (GetField(buf, 1, tempchar)) {
//							data.polar.a = StrToDbl(tempchar);
//						}
//						//	Getting the b
//						if (GetField(buf, 2, tempchar)) {
//							data.polar.b = StrToDbl(tempchar);
//						}
//						//	Getting the c
//						if (GetField(buf, 3, tempchar)) {
//							data.polar.c = StrToDbl(tempchar);
//						}
//						//	Getting the Vmin
//						if (GetField(buf, 4, tempchar)) {
//							data.polar.Vmin = StrToDbl(tempchar);
//						}
//						//	Getting the Wsmin
//						if (GetField(buf, 5, tempchar)) {
//							data.polar.Wsmin = StrToDbl(tempchar);
//						}
//						OpenDBUpdateRecord(polar_db, sizeof(PolarData), &data.polar, POLAR_REC);
//					}
//				}

				if (StrCompare(tempchar, "IGCHINFO1") == 0) {
//					HostTraceOutputTL(appErrorClass, "IGCHINFO1 record");

					//	Getting the Pilot Name
					if (GetField(buf, 1, tempchar)) {
						StrCopy(data.igchinfo.name, tempchar);
					}
					//	Getting the Aircraft Type
					if (GetField(buf, 2, tempchar)) {
						StrCopy(data.igchinfo.type, tempchar);
					}
					//	Getting the Glider ID
					if (GetField(buf, 3, tempchar)) {
						StrCopy(data.igchinfo.gid, tempchar);
					}
				}

				if (StrCompare(tempchar, "IGCHINFO2") == 0) {
//					HostTraceOutputTL(appErrorClass, "IGCHINFO2 record");

					//	Getting the Contest ID
					if (GetField(buf, 1, tempchar)) {
						StrCopy(data.igchinfo.cid, tempchar);
					}
					//	Getting the Contest Class
					if (GetField(buf, 2, tempchar)) {
						StrCopy(data.igchinfo.cls, tempchar);
					}
					//	Getting the Site
					if (GetField(buf, 3, tempchar)) {
						StrCopy(data.igchinfo.site, tempchar);
					}
					//	Getting the OO ID
					if (GetField(buf, 4, tempchar)) {
						StrCopy(data.igchinfo.ooid, tempchar);
					}
				}

				if (StrCompare(tempchar, "IGCHINFO3") == 0) {
//					HostTraceOutputTL(appErrorClass, "IGCHINFO3 record");

					//	Getting the Contest ID
					if (GetField(buf, 1, tempchar)) {
						StrCopy(data.igchinfo.gpsmodel, tempchar);
					}
					//	Getting the Contest Class
					if (GetField(buf, 2, tempchar)) {
						StrCopy(data.igchinfo.gpsser, tempchar);
					}
				}

				if (StrCompare(tempchar, "LISTLINES") == 0) {
//					HostTraceOutputTL(appErrorClass, "LISTLINES record");

					//	Getting the vertical line setting
					if (GetField(buf, 1, tempchar)) {
						data.config.listlinesvert = (Int8)StrAToI(tempchar);
					}
					//	Getting the horizontal line setting
					if (GetField(buf, 2, tempchar)) {
						data.config.listlineshoriz = (Int8)StrAToI(tempchar);
					}
				}

				if (StrCompare(tempchar, "LEFTACTION") == 0) {
//					HostTraceOutputTL(appErrorClass, "LEFTACTION record");

					//	Getting the Left 5-Way action setting
					if (GetField(buf, 1, tempchar)) {
						data.config.leftaction = (Int8)StrAToI(tempchar);
						if ((data.config.leftaction < 0) || (data.config.leftaction > 4)) data.config.leftaction = 1;
					}
				}

				if (StrCompare(tempchar, "MCBUTTON") == 0) {
//					HostTraceOutputTL(appErrorClass, "MCBUTTON record");

					//	Getting the MC Button setting
					if (GetField(buf, 1, tempchar)) {
						data.config.MCbutton = (Int8)StrAToI(tempchar);
						if ((data.config.MCbutton < 0) || (data.config.MCbutton > 1)) data.config.leftaction = 0;
					}
				}
				// AGM: get preferred BT GPS address
				if (StrCompare(tempchar, "BTGPSADDR") == 0) {
//					HostTraceOutputTL(appErrorClass, "BTGPSADDR record");
					btgpsaddr=true;
					if (GetField(buf, 1, tempchar)) {
//						HostTraceOutputTL(appErrorClass, tempchar);
						// BT Address is 6 hexadecimal 2 char. numbers
						// separated with a colon, like so:
						//           1111111
						// 12345678901234567
						// 00:00:00:00:00:00
						if (StrLen(tempchar)==17) {
							pos=1;
							for(i=0; i<6; i++) {
								data.config.BTAddr[i]=(UInt8) StrHToI(Mid(tempchar,2,pos));
								pos += 3;
							}
						}
					}
				}
				if (StrCompare(tempchar, "ENL") == 0) {
//					HostTraceOutputTL(appErrorClass, "ENL record");
					// Getting the Engine Noise Level on/off switch
					if (GetField(buf, 1, tempchar)) {
						data.config.logenl = (Boolean)StrAToI(tempchar);
					}
				}
			}
			next=0;
		}
		skip = false;
	}
	// no preferred BT GPS address?
	if (!btgpsaddr)
	{
		// then set address to 00:00:00:00:00:00
		for (i=0; i<6; i++) {
			data.config.BTAddr[i] = 0;
		}
	}
	set_unit_constants();
	// check logger on / off speeds
	if (data.config.logstartspd < 1.0/data.input.spdconst) data.config.logstartspd = 1.0/data.input.spdconst;
	if (data.config.logstopspd > data.config.logstartspd) data.config.logstopspd = data.config.logstartspd;
	return;
}
