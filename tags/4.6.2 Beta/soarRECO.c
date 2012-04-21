#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarForm.h"
#include "soarRECO.h"
#include "soarUtil.h"
#include "soarDB.h"
#include "Mathlib.h"

RECOData *recodata;
extern Boolean menuopen;

double RECOB2Alt(Char *altstr)
{	
	double ralt, palt;
	double tmp2, nom, denom;
	double a = 8430.0764525993883792048929663609;
	double b = 0.095081549439347604485219164118247;

	ralt = (double)(StrAToI(altstr)) / 100.0;
//	HostTraceOutputTL(appErrorClass, "ralt:|%s|", DblToStr(ralt, 1));
	tmp2 = log(ralt / 1013.25);
	nom = tmp2 * a;
	denom = 1.0 - (tmp2 * b);
	palt = -1.0 * (nom / denom) / ALTMETCONST;
//	HostTraceOutputTL(appErrorClass, "palt:|%s|", DblToStr(palt, 1));

	return(palt);
}

void RECOEvent()
{
	static RECOData prevrecodata;

	field_set_value(form_config_recoinst_temp, DblToStr(recodata->temp,3));
	field_set_value(form_config_recoinst_voltage, DblToStr(recodata->voltage,3));
	field_set_value(form_config_recoinst_firmwarever, recodata->recofirmwarever);
	field_set_value(form_config_recoinst_serial, recodata->recoserial);
	field_set_value(form_config_recoinst_gpsmodel, data.igchinfo.gpsmodel);
	field_set_value(form_config_recoinst_gpsser, data.igchinfo.gpsser);

	if ((StrCompare(recodata->recofirmwarever, prevrecodata.recofirmwarever) != 0) ||
	    (StrCompare(recodata->recoserial, prevrecodata.recoserial) != 0)) {
		OpenDBUpdateRecord(config_db, sizeof(RECOData), recodata, RECOINFO_REC);
	}

	StrCopy(prevrecodata.recofirmwarever, recodata->recofirmwarever);
	StrCopy(prevrecodata.recoserial, recodata->recoserial);
}

Boolean form_config_recoinst_event_handler(EventPtr event)
{
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			field_set_value(form_config_recoinst_temp, DblToStr(recodata->temp,3));
			field_set_value(form_config_recoinst_voltage, DblToStr(recodata->voltage,3));
			field_set_value(form_config_recoinst_firmwarever, recodata->recofirmwarever);
			field_set_value(form_config_recoinst_serial, recodata->recoserial);
			field_set_value(form_config_recoinst_gpsmodel, data.igchinfo.gpsmodel);
			field_set_value(form_config_recoinst_gpsser, data.igchinfo.gpsser);
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
			handled=false;
			break;
		case frmCloseEvent:
			handled=false;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_config_recoinst_okbtn:
					FrmGotoForm(form_set_logger);
					break;
				default:
					break;
			}
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

