#include <PalmOS.h>
#include <HAL.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Mathlib.h"
#include "soaring.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarLog.h"
#include "soarMath.h"
#include "soarUMap.h"
#include "soarIO.h"
#include "soarSUA.h"
#include "soarWind.h"
#include "soarComp.h"
#include "soarWay.h"

Boolean HwrBacklight(Boolean set, Boolean newState) HAL_CALL(sysTrapHwrBacklightV33);
Err HwrDisplayAttributes(Boolean set, UInt8 attr, void* dataP) HAL_CALL(sysTrapHwrDisplayAttributes);
Boolean PrgTextCallbackFunc(PrgCallbackData *cbP);
Char cbase[37];
ProgressPtr progP=NULL;
Boolean PrgCancel=false;
Int16 curformpriority=SUANONE;
Boolean chgscreen = true;

extern Int8 xfrdialog;
extern double curmapscale;
extern double actualmapscale;
extern SUAAlertData *suaalert;
extern SUAAlertRet   *suaalertret;
extern Boolean inflight;
extern Boolean IsGPS;
extern Boolean draw_log;
extern Boolean draw_task;
extern UInt32 cursecs;
extern Boolean menuopen;

// for screen chain goto
extern Boolean taskonhold;
extern Int16 inareasector;
extern Int16 selectedTaskWayIdx;
extern Int16 activetskway;
extern Boolean activetasksector;
extern TaskData *tsktoedit;
extern Boolean gotoGPSinfo;
extern Boolean wayselect;
extern UInt32 origform;
extern Boolean settaskreadonly;
extern Int16 suasortType;
extern Boolean skipnewflight;
extern Boolean Flarmpresent;
extern Boolean tasknotfinished;
extern Boolean savtaskonhold;
extern Int16 taskIndex;
extern Boolean dispactive;
extern Int16 defaultscreen;
extern Boolean exittaskreadonly;

// convert pressure to altitude simple version
double presstoalt(double pressure)
{
	return( (1013.25 - pressure) * FTPERMB );
}
// convert pressure to altitude accurate version
double presstoalt2(double pressure)
{
	return( (1 - pow(pressure/1013.25 , 0.190284)) * 145366.45 );
}

double pround(double val, Int16 n)
{
	double ival;
	double dec = 1.0;
	double tempdbl;

	if (val == 0.0) {
		return(0.0);
	}
	while(n>0) {
		dec *= 10.0;
		n--;
	}
	if (val < 0.0) {
		ival = (double)(trunc(val*dec - .5));
	} else {
		ival = (double)(trunc(val*dec + .5));
	}
	tempdbl = ival/dec;
	return(tempdbl);
}

Char* not_valid() 
{
	static char nv[10];
	StrCopy(nv,"XX\0");
	return(nv);
}

double CharToDbl(char input)
{
	switch (input) {
	case '0':
		return 0.0;
	case '1':
		return 1.0;
	case '2':
		return 2.0;
	case '3':
		return 3.0;
	case '4':
		return 4.0;
	case '5':
		return 5.0;
	case '6':
		return 6.0;
	case '7':
		return 7.0;
	case '8':
		return 8.0;
	case '9':
		return 9.0;
	}
	return 0.0;
}


double StrToDbl(Char* input)
{
	double result=0;
	double factor=0.1;
	double negate=1.0;
	int x=0;
	if (input == 0)
		return 0.0;
	if (input[x]=='-') {
		negate=-1.0;
		x++;
	}
	while (input[x] != 0 && input[x] != '.') {
		result*=10;
		result+=CharToDbl(input[x]);
		x++;
	}
	if (input[x] == '.') {
		x++;
		while (input[x] != 0 ) {
			result+=(factor * CharToDbl(input[x]));
			factor/=10;
			x++;
		}
	}
	return (result*negate);
}

Char* leftpad(Char* input, char padchar, int maxlen) 
{
	static char result[50];
	Int16 difflen;
	Int16 x;

	difflen = maxlen - StrLen(input);
	if (difflen > 0) {
		for (x=0;x<difflen;x++) {
			result[x] = padchar;
		}
		result[x] = '\0';
		StrCat(result, input);
	} else {
		StrCopy(result, input);
	}

	return (result);
}

Char* rightpad(Char* input, char padchar, int maxlen) 
{
	static Char result[50];
	UInt16 difflen;
	UInt16 x;

	difflen = maxlen - StrLen(input);
	if (difflen > 0) {
		StrCopy(result, input);
		for (x=StrLen(input);x<maxlen;x++) {
			result[x] = padchar;
		}
		result[x] = '\0';
	} else {
		StrCopy(result, input);
	}

	return (result);
}


Char* DblToStr(double input, UInt16 prec) 
{
	Int16 x;
	Int16 delta;
	Int16 length;
	static Char result[50];
	Char tempchar[30];
	Boolean negnum = false;

	if (input < 0.0) {
		negnum = true;
		input *= -1.0;
	}

	for(x=prec;x>0;x--) {
		input*=10.0;
	}
	StrIToA(result,(Int32)input);
	if (prec != 0){
		length=StrLen(result);
		delta = (prec-length)+1;
		if (delta > 0) {
			for(x=length-1;x>=0;x--) {
				result[x+delta]=result[x];
			}
			for(x=0;x<delta;x++) {
				result[x]='0';
			}
			result[length+delta]=0;
			length=StrLen(result);
		}
		result[length+1]=0;
		for(x=prec;x>0;x--) {
			result[length]=result[length-1];
			length--;
		}
		result[length]='.';
	} 
	if (negnum) {
		StrCopy(tempchar, "-");
		StrCat(tempchar, result);
		StrCopy(result, tempchar);
	}
	return(result);
}

// new corrected version
Char* DblToStr2(double value, Int16 places, Boolean IncDecPt) 
{
	Int16	i;
	Int32	tenNth, decPartLong;
	double	decPartDouble;
	static 	Char dStr[50];
	Char intpart[50];
	Char decpart[50];
	Char tempchar[50];

	if (dStr == NULL)
		return(dStr);

	if (value > 0.0)
		decPartDouble = value - ((Int32)value);
	else
		decPartDouble = ((Int32)value) - value;

	if (places == 0)
		// shortcut for no places
		return(StrIToA(dStr, ((Int32)value)));

	// integer part
	StrCopy(intpart, StrIToA(tempchar, ((Int32)value)));

	// don't have power function...
	for (i = 0, tenNth = 1; i < places; i++)
		tenNth *= 10;
	decPartLong = decPartDouble * ((double)tenNth);

	// decimal part
	StrCopy(decpart, StrIToA(tempchar, decPartLong));
	
	if (value < 0.0 && value > -1.0 && decPartLong != 0) {
		if (IncDecPt) {
			StrCopy(dStr,"-");
			StrCat(dStr, intpart);
			StrCat(dStr, ".");
			StrCat(dStr, leftpad(decpart, '0', places));
		} else {
			StrCopy(dStr,"-");
			StrCat(dStr, intpart);
			StrCat(dStr, leftpad(decpart, '0', places));
		}
	} else {
		if (IncDecPt) {
			StrCopy(dStr, intpart);
			StrCat(dStr, ".");
			StrCat(dStr, leftpad(decpart, '0', places));
		} else {
			StrCopy(dStr, intpart);
			StrCat(dStr, leftpad(decpart, '0', places));
		}
	}

	return(dStr);
}

double next_comma_dbl(Char* *s)
{
	Char* cur;
	int x=0;
	double ret;
	cur=*s;
	while ((cur[x] != ',') && (cur[x] != 0)) {
		x++;
	}
	cur[x]=0;
	 ret=StrToDbl(*s);
	*s=&(cur[x+1]);
	return(ret);
}

void field_set_value(UInt16 fieldID, Char* s) 
{ 
	FormPtr frm;
	UInt16    index;
	FieldPtr field;
	MemHandle h, oldHandle;
	char *p;

	frm = FrmGetFormPtr(FrmGetActiveFormID());
	if (frm == NULL) {
		return;
	}
	index = FrmGetObjectIndex(frm,fieldID);
	if (index < 0) {
		return;
	}
	field = FrmGetObjectPtr(frm,index);
	if (field == NULL) {
		return;
	}

	// From Palm OS Recipes
	h = MemHandleNew(StrLen(s)+1);
	p = (char *) MemHandleLock(h);

	StrCopy(p, s);

	MemHandleUnlock(h);

	oldHandle = FldGetTextHandle(field);
	FldSetTextHandle(field, h);
	if (oldHandle)
		MemHandleFree(oldHandle);

	FldDrawField(field);
} 

void field_set_enable(UInt16 ctlID, Boolean field_enable)
{
	FormPtr frm;
	UInt16 index;
	FieldPtr field;
	FieldAttrType field_attr;

	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	field = FrmGetObjectPtr(frm, index);
	FldGetAttributes (field, &field_attr); 
	field_attr.editable = (Int16)field_enable;
	FldSetAttributes (field, &field_attr);
	return;
}

UInt16 field_get_enable(UInt16 ctlID)
{
	FormPtr frm;
	UInt16 index;
	FieldPtr field;
	FieldAttrType field_attr;

	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	field = FrmGetObjectPtr(frm, index);
	FldGetAttributes (field, &field_attr); 
	return(field_attr.editable);
}

double field_get_double(UInt16 fieldID)
{
	FormPtr frm;
	UInt16    index;
	FieldPtr field;
	Char* field_str;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm,fieldID);
	field = FrmGetObjectPtr(frm,index);
	field_str = FldGetTextPtr(field);
	return(StrToDbl(field_str));
}
Int32 field_get_long(UInt16 fieldID)
{
	FormPtr frm;
	UInt16    index;
	FieldPtr field;
	Char* field_str;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm,fieldID);
	field = FrmGetObjectPtr(frm,index);
	field_str = FldGetTextPtr(field);
	return(StrAToI(field_str));
}
Char* field_get_str(UInt16 fieldID)
{
	FormPtr frm;
	UInt16    index;
	FieldPtr field;
	Char* field_str;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm,fieldID);
	field = FrmGetObjectPtr(frm,index);
	field_str = FldGetTextPtr(field);
	if (field_str) {
		return(field_str);
	} else {
		return("");
	}
}
Boolean ctl_get_value(UInt16 ctlID)
{
	FormPtr frm;
	UInt16 index;
	Boolean ctl_value;
	ControlPtr ctl;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	ctl = FrmGetObjectPtr(frm, index);
	ctl_value = CtlGetValue(ctl);
	return(ctl_value);
}

void ctl_set_value(UInt16 ctlID, Boolean ctl_value)
{
	FormPtr frm;
	UInt16 index;
	ControlPtr ctl;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	ctl = FrmGetObjectPtr(frm, index);
	CtlSetValue(ctl, ctl_value);
	return;
}

void ctl_set_enable(UInt16 ctlID, Boolean ctl_enable)
{
	FormPtr frm;
	UInt16 index;
	ControlPtr ctl;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	ctl = FrmGetObjectPtr(frm, index);
	CtlSetEnabled(ctl, ctl_enable);
	return;
}

void ctl_set_visible(UInt16 ctlID, Boolean ctl_visible)
{
	FormPtr frm;
	UInt16 index;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	if (ctl_visible) {
		FrmShowObject(frm, index);
	} else {
		FrmHideObject(frm, index);
	}
	return;
}

void field_set_visible(UInt16 fieldID, Boolean field_visible)
{
	ctl_set_visible(fieldID, field_visible);
}

void ctl_set_label(UInt16 ctlID, Char* s)
{
	FormPtr frm;
	UInt16 index;
	ControlPtr ctl;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	ctl = FrmGetObjectPtr(frm, index);
	CtlSetLabel(ctl, s);
	return;
}

Char* ctl_get_label(UInt16 ctlID)
{
	FormPtr frm;
	UInt16 index;
	ControlPtr ctl;
	static Char s[15];
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, ctlID);
	ctl = FrmGetObjectPtr(frm, index);
	StrCopy(s, CtlGetLabel(ctl));
	return(s);	
}
	
void frm_set_label(UInt16 labelID, Char* s)
{
	FormPtr frm;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	FrmCopyLabel (frm, labelID, s);
	return;
}

void frm_set_title(Char* s)
{
	FormPtr frm;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	FrmCopyTitle (frm, s);
	return;
}

void lst_set_selection(UInt16 lstID, Int16 lst_selection)
{
	FormPtr frm;
	UInt16 index;
	ListPtr lst;
	frm = FrmGetFormPtr(FrmGetActiveFormID());
	index = FrmGetObjectIndex(frm, lstID);
	lst = FrmGetObjectPtr(frm, index);
	LstSetSelection(lst, lst_selection);
	return;
}

Char* print_horizontal_speed(DataValue val)
{
	if (val.valid == NOT_VALID)
		return(not_valid());
	return(DblToStr(pround(val.value * data.input.spdconst,0),0));
}

Char* print_horizontal_speed2(double val, UInt16 precision)
{
	return(DblToStr(pround(val * data.input.spdconst,precision), precision));
}

Char* print_horizontal_wndspeed(double val, UInt16 precision)
{
	return(DblToStr(pround(val * data.input.wndconst,precision), precision));
}

Char* print_vertical_speed(DataValue val)
{
	if (val.valid == NOT_VALID)
		return(not_valid());
	return(DblToStr(pround(val.value * data.input.lftconst, data.input.lftprec), data.input.lftprec));
}

Char* print_vertical_speed2(double val, UInt16 precision)
{
	return DblToStr(pround(val * data.input.lftconst, precision), precision);
}

Char* print_distance(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return DblToStr(pround(val.value*data.input.disconst,1),1);
}

Char* print_distance2(double val, UInt16 precision)
{
	return DblToStr(pround(val * data.input.disconst,precision),precision);
}

Char* print_altitude(double val)
{
	if (pround(val * data.input.altconst,0) > 99999) {
		return ("99999");
	} else if (pround(val * data.input.altconst,0) < -99999) {
		return ("-99999");
	} else {
	 	return DblToStr(pround(val * data.input.altconst,0),0);
	}
}

Char* print_temperature(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return DblToStr(val.value,0);
}

Char* print_qnh(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return DblToStr(val.value,2);
}

Char* print_direction(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return leftpad(DblToStr(pround(val.value,0),0), '0', 3);
}

Char* print_direction2(double val)
{
	return leftpad(DblToStr(pround(val,0),0), '0', 3);
}

Char* print_longitude(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return DblToStr(val.value,3);
}

Char* print_latitude(DataValue val)
{
	if (val.valid == NOT_VALID)
		return not_valid();
	return DblToStr(val.value,3);
}

Char* print_strnoperiod(Char *instring)
{
	static Char tempchar[30];
	Int16 x, y;
	Int16 length;
	/* Remove period from the string */
	y=0;
	length = StrLen(instring);
	for (x=0;x<length;x++) {
		if (instring[x] != '.') {
			tempchar[y] = instring[x];
			y++;
		}
	}
	tempchar[y] = instring[x];
	return(tempchar);
}

void set_unit_constants()
{
	switch (data.config.altunits) {
		case NAUTICAL:
			data.input.altconst = ALTMPHNAUCONST;
			StrCopy(data.input.alttext, "ft");
			break;
		case METRIC:
			data.input.altconst = ALTMETCONST;
			StrCopy(data.input.alttext, "m ");
			break;
		default:
			break;
	}
	switch (data.config.disunits) {
		case STATUTE:
			data.input.disconst = DISTMICONST;
			StrCopy(data.input.distext, "mi");
			break;
		case NAUTICAL:
			data.input.disconst = DISTNAUCONST;
			StrCopy(data.input.distext, "nm");
			break;
		case METRIC:
			data.input.disconst = DISTKMCONST;
			StrCopy(data.input.distext, "km");
			break;
		default:
			break;
	}
	switch (data.config.spdunits) {
		case STATUTE:
			data.input.spdconst = SPDMPHCONST;
			StrCopy(data.input.spdtext, "mph"); 
			break;
		case NAUTICAL:
			data.input.spdconst = SPDNAUCONST;
			StrCopy(data.input.spdtext, "kts");
			break;
		case METRIC:
			data.input.spdconst = SPDKPHCONST;
			StrCopy(data.input.spdtext, "kph");
			break;
		default:
			break;
	}
	switch (data.config.tskspdunits) {
		case STATUTE:
			data.input.tskspdconst = SPDMPHCONST;
			StrCopy(data.input.tskspdtext, "mph"); 
			break;
		case NAUTICAL:
			data.input.tskspdconst = SPDNAUCONST;
			StrCopy(data.input.tskspdtext, "kts");
			break;
		case METRIC:
			data.input.tskspdconst = SPDKPHCONST;
			StrCopy(data.input.tskspdtext, "kph");
			break;
		default:
			break;
	}
	switch (data.config.lftunits)
	{
		case STATUTE:
			data.input.lftconst = AIRFPMCONST;
			StrCopy(data.input.lfttext, "fpm");
			data.input.lftincr = AIRFPMINCR; 
			data.input.lftprec = 0;
			break;
		case NAUTICAL:
			data.input.lftconst = AIRKTSCONST;
			StrCopy(data.input.lfttext, "kts");
			data.input.lftincr = AIRKTSINCR; 
			data.input.lftprec = 1;
			break;
		case METRIC:
			data.input.lftconst = AIRMPSCONST;
			StrCopy(data.input.lfttext, "m/s");
			if (data.config.flightcomp == C302COMP) {
				data.input.lftincr = AIRC302INCR; 
				data.input.lftprec = 1;
			} else {
				data.input.lftincr = AIRMPSINCR; 
				data.input.lftprec = 2;
			}
			break;
		default:
			break;
	}
	switch (data.config.wgtunits) {
		case NAUTICAL:
			data.input.wgtconst = WGTLBSCONST;
			StrCopy(data.input.wgttext, "lbs");
			break;
		case METRIC:
			data.input.wgtconst = WGTKGCONST;
			StrCopy(data.input.wgttext, "kg ");
			break;
		default:
			break;
	}
	switch (data.config.wtrunits) {
		case NAUTICAL:
			data.input.wtrconst = WTRGALCONST;
			StrCopy(data.input.wtrtext, "gal");
			break;
		case METRIC:
			data.input.wtrconst = WTRLTRCONST;
			StrCopy(data.input.wtrtext, "ltr");
			break;
		case STATUTE:
			data.input.wtrconst = WTRIMPCONST;
			StrCopy(data.input.wtrtext, "imp");
			break;
		default:
			break;
	}
	switch (data.config.wndunits) {
		case STATUTE:
			data.input.wndconst = SPDMPHCONST;
			StrCopy(data.input.wndtext, "mph"); 
			break;
		case NAUTICAL:
			data.input.wndconst = SPDNAUCONST;
			StrCopy(data.input.wndtext, "kts");
			break;
		case METRIC:
			data.input.wndconst = SPDKPHCONST;
			StrCopy(data.input.wndtext, "kph");
			break;
		case METRICMPS:
			data.input.wndconst = SPDMPSCONST;
			StrCopy(data.input.wndtext, "m/s");
			break;
		default:
			break;
	}
	switch (data.config.QNHunits) {
		case MILLIBARS:
			data.input.QNHconst = 1.0;
			StrCopy(data.input.QNHtext, "mb");
			break;
		case INHG:
			data.input.QNHconst = INPERMB;
			StrCopy(data.input.QNHtext, "in");
			break;
		default:
			break;
	}

	// actualmapscale holds the unconverted value representing the mapscale
	// in whatever distance units have been selected
	actualmapscale = GetMapScale(data.config.mapscaleidx);

	// curmapscale holds the value converted to nautical miles for all plotting
	curmapscale = actualmapscale / data.input.disconst;

	// re-scale wind profile
	CalcWindProfile(true, false);
	return;
}

// returns n'th (0-based) comma-delimeted field within buffer, ignoring commas in quotes
// true if field found, false otherwise
Boolean GetField(const Char *buffer, UInt16 n, Char *result)
{
	int i;
	Boolean inquotes = false;

	// skip n commas, as long as they are not enclosed in quotes
	for (i = 0; i < n; i++) {
		while (*buffer && *buffer != ',' && *buffer != '\0' && *buffer != '\r') {
			if (*buffer == '"') inquotes = !inquotes;
			buffer++;
		}
		if (inquotes) i -= 1;
		if (*buffer == '\0') {
			return(false);
		}
		buffer++;
	}

	// start reading 
	inquotes = false;
	while (*buffer) {
		if (*buffer == ',' || *buffer == '*' || *buffer == '\r' || *buffer == '\0') {
			if (!inquotes) break;
		}
		if (*buffer == '"') inquotes = !inquotes;
		*result++ = *buffer++;
	}
	*result = '\0';
	if (*buffer != ',' && *buffer != '*' && *buffer != '\r' && *buffer != '\0') {
		return(false);
	}
	return(true);
}


/*******************************************************
*  GetFieldDelim
*	Skips over the indicated number of fields using the start delimeter then returns all data found
*	until the end delimeter is found
*	Return: True if field found, False if not found
********************************************************/
Boolean GetFieldDelim(const Char *buffer, UInt16 numstartdelims, Char startdelim, Char enddelim, Char *result)
{
	int i;
	Char *temptr=result;

	// skip N delimiters
	for (i = 0; i < numstartdelims; i++) {
		while (*buffer && *buffer != startdelim && *buffer != '\0' && *buffer != '\r') {
			buffer++;
		}
		if (*buffer == '\0') {
			return false;
		}
		buffer++;
		// special case to treat multiple spaces as single delimiter
		if (startdelim == ' ')
			while (*buffer && *buffer == startdelim && *buffer != '\0' && *buffer != '\r') {
				buffer++;
			}
	}

	while (*buffer) {
		if (*buffer == enddelim || *buffer == '\r' || *buffer == '\0') {
			break;
		}
		*result++ = *buffer++;
	}
	*result = '\0';

	if (*buffer != enddelim && *buffer != '\r' && *buffer != '\0') {
		return(false);
	}
	trim(temptr, ' ', true);
	return(true);
}


/*********************************************************************************
 * SecondsToDateOrTimeString
 *    return a string of the date in HH:MM:SS or DD:MM:YY format
 * Parameters:
 *   UInt32 date     - date in seconds
 *   char  *textP - string to store the date in
 *   int   dttype - format of data to return 
 *                  0 for date(DD:MM:YY)
 *                  1 for time(HH:MM:SS)
 *                  2 for time(HHMMSS)
 *                  3 for time(YYMMDD)
 *                  4 for time(DDMMYY)
 *   Int32 tz     - time zone value to be applied to the hour value
 *********************************************************************************/
Char* SecondsToDateOrTimeString(UInt32	date, Char	*textP, int dttype, Int32 tz)
{
	DateTimeType	dtt;

	//Had to do this to get the DateTimeType structure populated
	//correctly.  Would work fine on everything but PalmIII and IIIx
	//as well as some Visors.
	TimSecondsToDateTime(cursecs, &dtt);

	if (textP == NULL)
		return(textP);

	// convert to a date time struct
	TimSecondsToDateTime(date, &dtt);
	// Adjust the Date and Time to account for a timezone shift
	if (tz != 0) {
		TimAdjust(&dtt, (tz * 3600 ));
	}

	if (dttype == 0) {
		// the day
		textP[0] = (dtt.day / 10) + '0';
		textP[1] = (dtt.day % 10) + '0';

		textP[2] = ':';
	
		// the month
		textP[3] = (dtt.month / 10) + '0';
		textP[4] = (dtt.month % 10) + '0';

		textP[5] = ':';

		// the year
		textP[6] = ((dtt.year-2000) / 10) + '0';
		textP[7] = (dtt.year % 10) + '0';

	} else if (dttype == 1) {
		// the hour
		textP[0] = (dtt.hour / 10) + '0';
		textP[1] = (dtt.hour % 10) + '0';

		textP[2] = ':';
	
		// the minute
		textP[3] = (dtt.minute / 10) + '0';
		textP[4] = (dtt.minute % 10) + '0';

		textP[5] = ':';

		// the second
		textP[6] = (dtt.second / 10) + '0';
		textP[7] = (dtt.second % 10) + '0';

	} else if (dttype == 2) {
		// the hour
		textP[0] = (dtt.hour / 10) + '0';
		textP[1] = (dtt.hour % 10) + '0';

		// the minute
		textP[2] = (dtt.minute / 10) + '0';
		textP[3] = (dtt.minute % 10) + '0';

		// the second
		textP[4] = (dtt.second / 10) + '0';
		textP[5] = (dtt.second % 10) + '0';

	} else if (dttype == 3) {
		// the year
		textP[0] = ((dtt.year-2000) / 10) + '0';
		textP[1] = (dtt.year % 10) + '0';

		// the month
		textP[2] = (dtt.month / 10) + '0';
		textP[3] = (dtt.month % 10) + '0';

		// the day
		textP[4] = (dtt.day / 10) + '0';
		textP[5] = (dtt.day % 10) + '0';

	} else if (dttype == 4) {
		// the day
		textP[0] = (dtt.day / 10) + '0';
		textP[1] = (dtt.day % 10) + '0';

		// the month
		textP[2] = (dtt.month / 10) + '0';
		textP[3] = (dtt.month % 10) + '0';

		// the year
		textP[4] = ((dtt.year-2000) / 10) + '0';
		textP[5] = (dtt.year % 10) + '0';
	}

	if (dttype == 2 || dttype == 3 || dttype == 4) {
		textP[6] = '\0';
	} else {
		textP[8] = '\0';
	}

	return(textP);
}

/*****************************************************************************
 * StringToDateOrTime
 *    return a date/time structure with the date or time info filled in
 * Parameters:
 *   Char  textP - string containing the date or time info
 *   DateTimeType dtt  - date/time structure to fill in
 *   int dttype - 0 for date(DDMMYY), 1 for time(HHMMSS)
 *****************************************************************************/
void StringToDateOrTime(Char* textP, DateTimeType *dtt, Int8 dttype)
{
	Char tempchar[4];
	Int16 int1=0, int2=0, int3=0;

	//Had to do this to get the DateTimeType structure populated
	//correctly.  Would work fine on everything but PalmIII and IIIx
	//as well as some Visors.
	TimSecondsToDateTime (cursecs, dtt);

	tempchar[0]=textP[0];
	tempchar[1]=textP[1];
	tempchar[2]='\0';
	int1=(Int16)StrAToI(tempchar);
	tempchar[0]=textP[2];
	tempchar[1]=textP[3];
	tempchar[2]='\0';
	int2=(Int16)StrAToI(tempchar);
	tempchar[0]=textP[4];
	tempchar[1]=textP[5];
	tempchar[2]='\0';
	int3=(Int16)StrAToI(tempchar);
	if (dttype == 1) {
		if (int3>59) { int3=59; int2++; }
		dtt->second=int3;
		if (int2>59) { int2=59; int1++; }
		dtt->minute=int2;
		dtt->hour=(int1%23);
	} else {
		dtt->day=(int1>31?31:int1);
		dtt->month=(int2>12?12:int2);
		dtt->year=(int3<32?2000:1900)+int3; // fix for invalid parameter values in debug simulator
		if (dtt->day == 0) {
			dtt->day = 1;
//			HostTraceOutputTL(appErrorClass, "     Setting day to-|%hd|", dtt->day);
		}
		if (dtt->month == 0) {
			dtt->month = 1;
//			HostTraceOutputTL(appErrorClass, "     Setting month to-|%hd|", dtt->month);
		}
	}
	return;
}

/*****************************************************************************
 * StringToDateAndTime
 *    return a date/time structure with the date and time info filled in
 * Parameters:
 *   Char  dateP - string containing the date info (DDMMYY)
 *   Char  timeP - string containing the time info (HHMMSS)
 *   DateTimeType dtt  - date/time structure to fill in
 *****************************************************************************/
void StringToDateAndTime(Char *dateP, Char *timeP, DateTimeType *dtt)
{
	Char tempchar[4];
	Int16 int1=0, int2=0, int3=0;

//	HostTraceOutputTL(appErrorClass, "Inside StringToDateAndTime");
//	HostTraceOutputTL(appErrorClass, "     dateP-|%s|", dateP);
//	HostTraceOutputTL(appErrorClass, "     timeP-|%s|", timeP);
	//Had to do this to get the DateTimeType structure populated
	//correctly.  Would work fine on everything but PalmIII and IIIx
	//as well as some Visors.
	TimSecondsToDateTime (cursecs, dtt);

	// Parse out the date
	tempchar[0]=dateP[0];
	tempchar[1]=dateP[1];
	tempchar[2]='\0';
	int1=(Int16)StrAToI(tempchar);
	tempchar[0]=dateP[2];
	tempchar[1]=dateP[3];
	tempchar[2]='\0';
	int2=(Int16)StrAToI(tempchar);
	tempchar[0]=dateP[4];
	tempchar[1]=dateP[5];
	tempchar[2]='\0';
	int3=(Int16)StrAToI(tempchar);
	dtt->day=(int1>31?31:int1);
	dtt->month=(int2>12?12:int2);
	dtt->year=(int3<32?2000:1900)+int3; // fix for invalid parameter values in debug simulator
	if (dtt->day == 0) {
		dtt->day = 1;
//		HostTraceOutputTL(appErrorClass, "     Setting day to-|%hd|", dtt->day);
	}
	if (dtt->month == 0) {
		dtt->month = 1;
//		HostTraceOutputTL(appErrorClass, "     Setting month to-|%hd|", dtt->month);
	}
	
	// Parse out the time
	tempchar[0]=timeP[0];
	tempchar[1]=timeP[1];
	tempchar[2]='\0';
	int1=(Int16)StrAToI(tempchar);
	tempchar[0]=timeP[2];
	tempchar[1]=timeP[3];
	tempchar[2]='\0';
	int2=(Int16)StrAToI(tempchar);
	tempchar[0]=timeP[4];
	tempchar[1]=timeP[5];
	tempchar[2]='\0';
	int3=(Int16)StrAToI(tempchar);
	if (int3>59) { int3=59; int2++; }
	dtt->second=int3;
	if (int2>59) { int2=59; int1++; }
	dtt->minute=int2;
	dtt->hour=(int1%24);

//	HostTraceOutputTL(appErrorClass, "About to return:");
//	HostTraceOutputTL(appErrorClass, "     day-|%hd|", dtt->day);
//	HostTraceOutputTL(appErrorClass, "     month-|%hd|", dtt->month);
//	HostTraceOutputTL(appErrorClass, "     year-|%hd|", dtt->year);
//	HostTraceOutputTL(appErrorClass, "     hour-|%hd|", dtt->hour);
//	HostTraceOutputTL(appErrorClass, "     minute-|%hd|", dtt->minute);
//	HostTraceOutputTL(appErrorClass, "     second-|%hd|", dtt->second);

//	HostTraceOutputTL(appErrorClass, "================================");
	return;
}


/*****************************************************************************
 * RightHandToD - convert the right hand side of a decimal fraction to a
 *		double.  This allows the '.' in the string but does NOT allow the
 *		'-' negative sign.
 *****************************************************************************/
Boolean RightHandToD(char	*inStr, double	*valueP)
{
	double	divisor;
	Int16	len;

	if (! inStr || ! valueP)
		return false;

	// skip the decimal if there is one
	if (*inStr == '.')
		inStr++;

	// count the places in the string & build the divisor without using "pow"
	for (divisor = 1.0, len = StrLen(inStr); len > 0; len--)
		divisor *= 10.0;

	// convert the string and fraction-ize it
	*valueP = StrAToI(inStr);
	*valueP = *valueP / divisor;

	return true;
}

/* ====================================================================
    Description: trim() removes leading and trailing characters that
    match the inputted one from the string.
    Retn. value: trimmed string.
  ==================================================================== */

Char* trim(Char* string, Char trimchr, Boolean bothsides) /* string to be trimmed */
{
	Char* r;
	Char* p;
	Char* q;  /* pointers used to trim string */
	Boolean whitespace = false;
	
	if (string == 0) {
		return(0);
	}
	// special case : tab character will remove spaces and tabs
	if (trimchr == '\t') whitespace = true;

	p = q = r = string;
	/* find the first character */
	while ((*p == trimchr) || (whitespace && (*p == ' '))) {
		p++;
	}

	/* blank string ? */
	if (StrLen(string) == (p - string)) {
		*string = '\0';
		return(string);
	}

	if (p != string) {
		do {
			*q++ = *p++;
		} while (*p);
		*q = '\0';
	}

	if (bothsides) {
		r = StrChr(string,'\0');
		/* there is something still in string */
		if (r > string) {
			r--;	/* move to last character */
			while ((*r == trimchr) || (whitespace && (*r == ' ')))
				r--;
				/* r now points to the last non-space character. Set the next char
					to null to end the string */
			*(r+1) = '\0';
		}
	}
	return(string);
}

/* ====================================================================
    Description: Left() removes all but the left "n" characters and 
    returns the result.
    Retn. value: string.
  ==================================================================== */

Char *Left(Char *instr, UInt16 n)
{
	static Char retchar[64];

	StrCopy(retchar, instr);
	if (StrLen(instr) > n) {
		retchar[n] = '\0';
	}
	return(retchar);
}

/*
///Char *Left(Char *instr, UInt16 n)
///{
//	Char* r;
//	Char* p;
//	Char* q;  // pointers used to trim string
//	Int16 i;
//	static Char retchar[20];

///	instr[n] = '\0';
//	HostTraceOutputTL(appErrorClass, "instr-|%s|", instr);
//	StrCopy(retchar, instr);

	if (string == 0) {
		return(0);
	}
	p = q = r = string;

	// blank string ? 
	if (StrLen(string) == (p - string)) {
		*string = '\0';
		return(string);
	}

	if (p != string) {
		for (i=0; i < n; i++) {
			*q++ = *p++;
		};
		*q = '\0';
	}

	if (bothsides) {
		r = StrChr(string,'\0');
		// there is something still in string 
		if (r > string) {
			r--;	// move to last character 
			while (*r == trimchr)
				r--;
					// r now points to the last non-space character. Set the next char
					// to null to end the string
			*(r+1) = '\0';
		}
	}

///	return(instr);
//	return(retchar);
//	return(q);
///}
*/

/* ====================================================================
    Description: Right() removes all but the right "n" characters and 
    returns the result.
    Retn. value: string.
  ==================================================================== */

Char *Right(Char *instr, UInt16 n)
{
	Int16 i, j=0, instrlen;
	static Char retchar[64];

//	HostTraceOutputTL(appErrorClass, "instr-|%s|", instr);

	// blank string ? 
	if (StrLen(instr) == 0) {
		retchar[0] = '\0';
		return(retchar);
	}

	instrlen = StrLen(instr);

	for (i=(instrlen-n); i < instrlen; i++) {
		retchar[j] = instr[i];
		j++;
	}
	retchar[j] = '\0';

	return(retchar);
}

//This function computes decimal altitude from a 4-digit hex-code
/*
Int16 Hex2Dec(Char* P_Alt)
{
	Int16 Dec = 0;
	
	Dec = (Int16)strtol(P_Alt, NULL, 16);

	return(Dec);
}
*/

//This function converts a single HEX character to a decimal number 
UInt32 Hex2Dec(Char HexChar) 
{
	UInt32 Dec = 0;

	//all letters (ABCDEF) have ASCII codes  > 60, all numbers have ASCII codes < 60;
	if(HexChar < 60) {
		//numbers: ASCII-code - 48 = actual number
		Dec = (HexChar-48);
	} else {
		//letters: ASCII-code - 55 = (10...15)
		Dec = (HexChar-55); 
	}
	return(Dec);
}

//This function computes decimal altitude from a 4-digit hex-code
Int32 Hex2Dec2(Char* P_Alt) 
{
	//The hex-string is converted to an int array in order to use ASCII values for the letters
	Int32 Hex[4] = {P_Alt[3], P_Alt[2], P_Alt[1], P_Alt[0]};
	Int32 Dec = 0;
	Int32 i = 0;
	for(i=0; (!(i>3)); i++) {
		//all letters (ABCDEF) have ASCII codes  > 60, all numbers have ASCII codes < 60;
		if(Hex[i] < 60) {
			//numbers: ASCII-code - 48 = actual number
			Dec = Dec + ((Hex[i]-48) * ((Int32)pround(pow(16.0, (double)i), 0)));
		} else {
			//letters: ASCII-code - 55 = (10...15)
			Dec = Dec + ((Hex[i]-55) * ((Int32)pround(pow(16.0, (double)i), 0))); 
		}
	}
	//conversion for postive or negative altitudes.
	if (Dec < 32768) {
		// I'm not really sure whether the volkslogger designer Johannes Garrecht 
		// does this but it seems to be the most logical way
		return(Dec);
	} else {
		Dec = Dec - 65536;
		return(Dec);
	}
}

//This function computes decimal altitude from a 8-digit hex-code
UInt32 Hex2Dec3(Char* HexStr) 
{
	//The hex-string is converted to an int array in order to use ASCII values for the letters
	UInt32 Hex[8] = {HexStr[7], HexStr[6], HexStr[5], HexStr[4], HexStr[3], HexStr[2], HexStr[1], HexStr[0]};
	UInt32 Dec = 0;
	Int32 i = 0;
	for(i=0; (!(i>7)); i++) {
		//all letters (ABCDEF) have ASCII codes  > 60, all numbers have ASCII codes < 60;
		if(Hex[i] < 60) {
			//numbers: ASCII-code - 48 = actual number
			Dec = Dec + ((Hex[i]-48) * ((UInt32)pround(pow(16.0, (double)i), 0)));
		} else {
			//letters: ASCII-code - 55 = (10...15)
			Dec = Dec + ((Hex[i]-55) * ((UInt32)pround(pow(16.0, (double)i), 0))); 
		}
	}
	return(Dec);
}

//This function computes converts a base36 digit to decimal 
UInt32 B362Dec(Char* inputStr) 
{
	Char baseStr[37];
	//The hex-string is converted to an int array in order to use ASCII values for the letters
	UInt32 Hex[36];
	UInt32 Dec = 0;
	Int32 i = 0;

	StrCopy(baseStr, leftpad(inputStr, '0', 36));
//	HostTraceOutputTL(appErrorClass, "baseStr |%s|", baseStr);

	for(i=0; i<36; i++) {
		Hex[i] = baseStr[35-i];
	}

	for(i=0; (!(i>35)); i++) {
		//all letters (ABCDEF) have ASCII codes  > 60, all numbers have ASCII codes < 60;
		if(Hex[i] < 60) {
			//numbers: ASCII-code - 48 = actual number
			Dec = Dec + ((Hex[i]-48) * ((UInt32)pround(pow(36.0, (double)i), 0)));
		} else {
			//letters: ASCII-code - 55 = (10...15)
			Dec = Dec + ((Hex[i]-55) * ((UInt32)pround(pow(36.0, (double)i), 0))); 
		}
	}
	return(Dec);
}

//This function computes converts a base14 strings to decimal 
UInt32 B142Dec(Char* inputStr) 
{
	Char baseStr[15];
	//The hex-string is converted to an int array in order to use ASCII values for the letters
	UInt32 Hex[14];
	UInt32 Dec = 0;
	Int32 i = 0;

	StrCopy(baseStr, leftpad(inputStr, '0', 14));
//	HostTraceOutputTL(appErrorClass, "baseStr |%s|", baseStr);

	for(i=0; i<14; i++) {
		Hex[i] = baseStr[13-i];
	}

	for(i=0; (!(i>13)); i++) {
		//all letters (ABCDEF) have ASCII codes  > 60, all numbers have ASCII codes < 60;
		if(Hex[i] < 60) {
			//numbers: ASCII-code - 48 = actual number
			Dec = Dec + ((Hex[i]-48) * ((UInt32)pround(pow(14.0, (double)i), 0)));
		} else {
			//letters: ASCII-code - 55 = (10...15)
			Dec = Dec + ((Hex[i]-55) * ((UInt32)pround(pow(14.0, (double)i), 0))); 
		}
	}
	return(Dec);
}

void Report(Char *formatStr, ...)
{
	va_list args;
	Char text[40];
	va_start(args, formatStr);
	StrVPrintF(text, formatStr, args);
	va_end(args);
	FrmCustomAlert (WarningAlert, text," "," ");
}

void PlaySound( Int32 hz, UInt16 ms, UInt16 amp )
{
	Err err;
	SndCommandType sndcmd;

	sndcmd.cmd = sndCmdFreqDurationAmp;
	sndcmd.param1 = hz;
	sndcmd.param2 = ms;
	sndcmd.param3 = amp;
	err = SndDoCmd( NULL, &sndcmd, 0 );
}

void PlayStartSound()
{
	// play a C, G, C as loud as possible
	PlaySound(523, 125, sndMaxAmp);
	PlaySound(784, 125, sndMaxAmp);
	PlaySound(1047, 125, sndMaxAmp);
}

void PlayTurnSound()
{
	// play a G, G, G as loud as possible
	PlaySound(784, 125, sndMaxAmp);
	PlaySound(784, 125, sndMaxAmp);
	PlaySound(784, 125, sndMaxAmp);
}

void PlayFinishSound()
{
	// play a C, G, C as loud as possible
	PlaySound(1047, 125, sndMaxAmp);
	PlaySound(784, 125, sndMaxAmp);
	PlaySound(523, 125, sndMaxAmp);
}

void PlayNoGPSSound()
{
	// play a C, G, C as loud as possible
	PlaySound(880, 125, sndMaxAmp);
	PlaySound(698, 125, sndMaxAmp);
	PlaySound(880, 125, sndMaxAmp);
}

void PlayWarnSound()
{
	// play a C, G, C as loud as possible
	PlaySound(880, 125, sndMaxAmp);
	PlaySound(890, 125, sndMaxAmp);
	PlaySound(880, 125, sndMaxAmp);
}

void PlayKeySound()
{
	if (data.config.keysoundon) {
		// play a G as loud as possible
		PlaySound(784, 125, sndMaxAmp);
	}
}

Boolean PrgTextCallbackFunc(PrgCallbackData *cbP)
{
	static Char message[80];
	Boolean handled = false;
	Char tempchar[10];
		
	if (cbP->canceled) {
		PrgCancel = true;
	}
	switch (xfrdialog) {
		case XFRRECEIVE:
			StrCopy(message, "Receiving Data\nPlease Wait...");		
			break;
		case XFRTRANSMIT:
			StrCopy(message, "Transmitting Data\nPlease Wait...");
			break;
		case XFRDELETE:
			StrCopy(message, "Deleting Data\nPlease Wait...");
			break;
		case XFRXFR:
			StrCopy(message, "Transferring Data\nPlease Wait...");
			break;
		case XFRRECONNECT:
			StrCopy(message, "Lost Connection!\n\nRe-Connecting...");
			break;			
		case XFRC302:
			StrCopy(message, "Connecting to C302\n\nPlease Wait...");
			break;
		case XFRGPSNAV:
			StrCopy(message, "Connecting to GPSNAV\n\nPlease Wait...");
			break;
		default:
			StrCopy(message, "Transferring Data\nPlease Wait...");
			break;
	}
	if (StrLen(cbP->message) > 0) {
		StrCat(message, "\n");
		if (cbP->stage != 65535) {
			StrCat(message, StrIToA(tempchar, (UInt32)cbP->stage));
			StrCat(message, " of ");
		}
		StrCat(message, cbP->message);
	}
	cbP->textLen = StrLen(message)+1;
	StrCopy(cbP->textP, message);
	handled = true;
			
	return(handled);
}

void HandleWaitDialog(Boolean showdialog)
{
	static ProgressPtr progP=NULL;

	if (showdialog) {
		progP = PrgStartDialogV31("Please Wait...", PrgTextCallbackFunc);
		PrgUpdateDialog(progP, 0, 0, 0, true);
	} else if (progP){
		PrgStopDialog(progP, true);
		progP = NULL;
	}
	return;
}

Boolean HandleWaitDialogWin(Int16 cmd)
{
	static UInt16 curformID=0;
	EventType newEvent;
	Boolean handled = false;

//	HostTraceOutputTL(appErrorClass, "Priority Open %s", DblToStr(curformpriority ,0));
//	HostTraceOutputTL(appErrorClass, "Priority Next %s", DblToStr(suaalert->priority ,0));

	if (cmd == 1) {
		if (curformID == 0) {
			curformID = FrmGetActiveFormID();
			curformpriority = suaalert->priority;
//			curformID = data.application.form_id;
//			HostTraceOutputTL(appErrorClass, "About to popup alert curFormID-|%hu|", curformID);
//			HostTraceOutputTL(appErrorClass, "         FrmGetActiveFormID()-|%hu|", FrmGetActiveFormID());
			suaalertret->alerttype = suaalert->alerttype;

			FrmPopupForm(form_genalert);
//			HostTraceOutputTL(appErrorClass, "FrmPopupFrom done", FrmGetActiveFormID());			
		} else if (curformpriority <= suaalert->priority && FrmGetActiveFormID() == form_genalert) {
//			HostTraceOutputTL(appErrorClass, "About to update the current Alert");
			newEvent.eType = frmUpdateEvent;
			suaalertret->alerttype = suaalert->alerttype;
			form_genalert_event_handler(&newEvent);
		} else {
//			HostTraceOutputTL(appErrorClass, "Alert already open & >= current priority");
		} 
	} else if (cmd == 2) {
		WinDrawChars(suaalert->displaytext, StrLen(suaalert->displaytext), 40*SCREEN.SRES, 80*SCREEN.SRES);
	} else {
//		HostTraceOutputTL(appErrorClass, "About to close alert curFormID-|%hu|", curformID);
//		HostTraceOutputTL(appErrorClass, "         FrmGetActiveFormID()-|%hu|", FrmGetActiveFormID());

/*		FrmReturnToForm(curformID);
		// added form update event to redraw form correctly with DIA capable Palm eg: Moving Map Screen
		if (device.DIACapable) {
			newEvent.eType = frmUpdateEvent;
			EvtAddEventToQueue(&newEvent);
		}

		handled = true;
//		FrmReturnToForm(0);
		newEvent.eType = reloadAppEvents;
//		newEvent.data.generic.datum[0] = 777;
//		HostTraceOutputTL(appErrorClass, "About to encode newEvent for reloadAppEvents");
		EvtAddEventToQueue(&newEvent);*/

		return_to_form(curformID);
		curformID = 0;
		curformpriority=SUANONE;
		handled = true;
	}
	return handled;
}

void return_to_form(Int16 return_form)
{
	EventType newEvent;

	// return from popup form
	FrmReturnToForm(return_form);
	// reload events
	newEvent.eType = reloadAppEvents;
	EvtAddEventToQueue(&newEvent);
	// trigger form update
	newEvent.eType = frmUpdateEvent;
	EvtAddEventToQueue(&newEvent);
	return;
}

Boolean HandleWaitDialogUpdate(Int16 cmd, UInt16 totalnum, UInt16 curnum, Char *type)
{
	Char tempchar[25];
	Boolean retval= true;
	EventType event;

	if (cmd == SHOWDIALOG) {
		progP = PrgStartDialogV31("Please Wait...", PrgTextCallbackFunc);
		PrgUpdateDialog(progP, 0, curnum, "", true);
		PrgCancel = false;
		retval = true;
	} else if (progP) {
		if (cmd == UPDATEDIALOG) {
			if (PrgCancel == false) {
				StrIToA(tempchar, (UInt32)totalnum);
				StrCat(tempchar, " ");
				StrCat(tempchar, type);
				PrgUpdateDialog(progP, 0, curnum, tempchar, true);
				//PrgUpdateDialog(progP, 1, curnum, tempchar, true);
			} else {
				retval = false;
//				FrmCustomAlert(WarningAlert, "OK Button Pressed"," "," ");
			}
		} else if (cmd == CHECKCANCEL) {
			EvtGetEvent(&event, 0);
			retval = PrgHandleEvent(progP, &event);
			if (!retval) PrgCancel = PrgUserCancel(progP);
			//retval = PrgHandleEvent (progP, (EventType *)type);
			//if (retval == false && PrgUserCancel (progP)) {
			//	PrgCancel = true;
			//}
		} else if (cmd == STOPDIALOG) {
			PrgStopDialog(progP, true);
			progP = NULL;
			PrgCancel = false;
			retval = true;
		}
	} else if (cmd == CHECKCANCEL) {
		retval = SysHandleEvent((EventType *)type);
	}
	return(retval);
}

void GenerateIGCName(FlightData *fltdata, Char *igcname)
{
	DateTimeType IGCDate;
	Char strIgc[15];
	Int16 fltNum = 1;
	Char tempchar[10];

	StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &IGCDate);
	// hour minute second day month year
	// extract the year
	StrIToA(tempchar, IGCDate.year);
	strIgc[0] = tempchar[StrLen(tempchar)-1];
	strIgc[1] = '\0';
	// extract the month
	if (IGCDate.month <= 9) {
		StrIToA(tempchar, IGCDate.month);
	} else {
		tempchar[0] = (IGCDate.month+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);

	// extract the day
	if (IGCDate.day <= 9) {
		StrIToA(tempchar, IGCDate.day);
	} else {
		tempchar[0] = (IGCDate.day+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);

	// Add the manaufacturer code
	if (fltdata->flightcomp == RECOCOMP) {
		// Using O for the ReCo 
		StrCat(strIgc, "O");
//		StrCat(strIgc, GenerateUID(fltdata->fcserial));
		StrCat(strIgc, fltdata->fcserial);
	} else {
		// Using X 
		StrCat(strIgc, "X");
		// Add the ID
		StrCat(strIgc, GenerateUID(GetSysRomID()));
	}
	// Add flight number
	fltNum = FindFltNumOfDay(fltdata->startdtg, IGCDate.hour, IGCDate.minute, IGCDate.second);
	if (fltNum <= 9) {
		StrIToA(tempchar, fltNum);
	} else {
		tempchar[0] = (fltNum+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);
	// Add the extension
	StrCat(strIgc, ".igc\0");

	StrCopy(igcname, strIgc);

	return;
}

Char* GetSysRomID()
{
	Char* buffer = NULL;
	static Char retStr[13];
	UInt16 length = 0;
	Err error = errNone;

	error = SysGetROMToken(0,sysROMTokenSnum,(UInt8**)&buffer,&length);

	if((!error) && (buffer) && ((UInt8)*buffer != 0xFF)) {
		StrNCopy(retStr,buffer,length);
		retStr[length] = '\0';
	} else {
		StrCopy(retStr, "NOSERIAL");
	}
	return(retStr);
}


Char *CalcChkSum(Char* inputstr)
{
//	http://www.codepedia.com/1/Calculating+and+Validating+NMEA+Checksums
	static Char mychksumstr[10];
	UInt16 mychksum = 0;
	UInt16 i;
	UInt16 length;

	length = StrLen(inputstr);

	for (i=0; i < length; i++) {
		if ((inputstr[i] != '$') && (inputstr[i] != '*'))
			mychksum ^= inputstr[i];
	}

	StrCopy(mychksumstr, Dec2Hex(mychksum));

	return(mychksumstr);
}

Char *Dec2Hex(UInt16 inputint)
{
	static Char hexstr[20];
	Char tempchar [20];
	UInt16 hexstrlen;

	StrIToH(tempchar, inputint);
//	HostTraceOutputTL(appErrorClass, "Dec2Hex before trim:|%s|", tempchar);
	StrCopy(hexstr, trim(tempchar, '0', false));
//	HostTraceOutputTL(appErrorClass, "Dec2Hex after trim:|%s|", hexstr);
	hexstrlen = StrLen(hexstr);
	if ((hexstrlen % 2) != 0 || hexstrlen == 0) {
		if (hexstrlen == 0) {
			hexstrlen++;
		}
		StrCopy(hexstr, leftpad(hexstr, '0', hexstrlen+1));
//		HostTraceOutputTL(appErrorClass, "Dec2Hex after pad:|%s|", hexstr);
	}

	return(hexstr);

}

// Mid Function
//Get the substring denoted by the start character position and
// number of characters to return
Char *Mid(Char *s, UInt16 n, UInt16 p)
{
	//s = input-String
	//n = number of chars to return
	//p = start-Position
	UInt16 l; //length of input-string
	static Char retchar[64];
 
	l = StrLen(s); 
	p = p - 1; 

	StrCopy(retchar, Right(s, l-p));
//	HostTraceOutputTL(appErrorClass, "After Right:|%s|", retchar);
	retchar[n] = '\0';
//	HostTraceOutputTL(appErrorClass, "After Left:|%s|", retchar);
 
	return(retchar);
}

//Convert a decimal number in another base
Char *CBase(UInt32 number, UInt32 base) 
{
	//symbols to code the number
	Char chars[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	UInt32 r;
	Char tempchar[37];
	Char midchar[2];

//	HostTraceOutputTL(appErrorClass, "number outside:|%lu|", number);
	//verify if there are enough symbols to code the number in the selected base
	if (base < 2 || base > StrLen(chars)) { 
		return(" ");
	}
	StrCopy(cbase, "");
	while (number >= base) {
		StrCopy(midchar, "");
//		HostTraceOutputTL(appErrorClass, "number inside:|%lu|", number);
		r = (UInt32)Fmod((double)number, (double)base);
//		HostTraceOutputTL(appErrorClass, "r inside:|%lu|", r);
		StrCopy(midchar, Mid(chars, 1, r+1)); 
		StrCopy(tempchar, midchar);
//		HostTraceOutputTL(appErrorClass, "cbase1:|%s|", cbase);
		StrCat(tempchar, cbase);
//		HostTraceOutputTL(appErrorClass, "cbase2:|%s|", cbase);
		number = number / base;
		StrCopy(cbase, tempchar);
//		HostTraceOutputTL(appErrorClass, "----------------------------");
	}
	StrCopy(midchar, "");
	StrCopy(midchar, Mid(chars, 1, number+1)); 
	StrCopy(tempchar, midchar);
//	HostTraceOutputTL(appErrorClass, "cbase3:|%s|", cbase);
	StrCat(tempchar, cbase);
	StrCopy(cbase, tempchar);

//	StrCopy(cbase, tempchar);
//	HostTraceOutputTL(appErrorClass, "cbase4:|%s|", cbase);
//	HostTraceOutputTL(appErrorClass, "============================");
	return(cbase);
}

Char *Str2Hex(Char *inputstr)
{
	static Char output_char[81];
	UInt16 x;
	UInt16 inputstrlen;

	inputstrlen = StrLen(inputstr);
//	HostTraceOutputTL(appErrorClass, "inputstrlen|%hu|", inputstrlen);
	for (x=0; x<inputstrlen; x++) {
//		HostTraceOutputTL(appErrorClass, "character to convert|%c|", inputstr[x]);
		if (x == 0) {
			StrCopy(output_char, Dec2Hex(inputstr[x]));
		} else {
			StrCat(output_char, Dec2Hex(inputstr[x]));
		}
//		HostTraceOutputTL(appErrorClass, "output_char before null|%s|", output_char);
	}
	output_char[inputstrlen*2] = '\0';
//	HostTraceOutputTL(appErrorClass, "output_char after null|%s|", output_char);
	return(output_char);
}

void Sleep(double SleepFor)
{

	SysTaskDelay((Int32)(SleepFor*(double)(SysTicksPerSecond())));

}

Int16 toupper(Int16 c)
{
	if ('a' <= c && c<= 'z') {
		return(c-'a'+'A');
	}
	else {
		return(c);
	}
}

Int16 ConvertToUpper(Char *str)
{
	int i = 0;

	while (str[i] != '\0'){
		str[i] = toupper(str[i]);
		i++;
	}
	return(0);
}

Int16 tolower(Int16 c)
{
	if ('A' <= c && c<= 'Z') {
		return(c-'A'+'a');
	}
	else {
		return(c);
	}
}

Int16 ConvertToLower(Char *str)
{
	int i = 0;

	while (str[i] != '\0'){
		str[i] = tolower(str[i]);
		i++;
	}
	return(0);
}

Char* ToNumeric(Char *str)
{
	int i = 0;

	while (str[i] != '\0'){
		if (StrChr("0123456789.-+",str[i]) == NULL) {
			str[i] = ' ';
		}
		i++;
	}

	return(trim(str, ' ', true));
}

Int8 ChkUnits(Char *str)
{
	int i = 0;

	// skip to first non-numeric or non-space character
	while (str[i] != '\0'){
		if (StrChr("0123456789.-+ ",str[i]) == NULL) {
			break;
		}
	i++;
	}
	// check for "M" followed by space or end of string
	if ((str[i] == 'M') && ((str[i+1] == '\0') || (str[i+1] == ' '))) {
		return(METRIC);
	} else {
		return(NAUTICAL);
	}
}

Char* NoComma(Char *str, Char *RepChar)
{
	int i =0, len =0;

	if (str) {
		len = StrLen( str );
		for ( i=0; i < len; i++ )
		{
			if ( str[i] == ',' )
			{
				str[i] = RepChar[0];
			}
		}
	}
	return(str);
}


Char *GenerateUID(Char *inputStr)
{
	UInt32 tempint;
	double remainder;
	static Char UID[6];

//	HostTraceOutputTL(appErrorClass, "inputStr-|%s|", inputStr);
	tempint = B362Dec(inputStr);
//	HostTraceOutputTL(appErrorClass, "Base36 to Dec-%lu", tempint);
//	HostTraceOutputTL(appErrorClass, "CBase Conversion of tempint-|%s|", CBase((UInt32)tempint, 36));
	remainder = Fmod((double)tempint, 46649.0);
//	HostTraceOutputTL(appErrorClass, "Fmod of Dec=remainder-%s", DblToStr(remainder, 1));
	StrCopy(UID, leftpad(CBase((UInt32)remainder, 36), '0', 3));

//	HostTraceOutputTL(appErrorClass, "CBase Conversion of remainder-|%s|", UID);
	return(UID);
}

void StrCatEOL(Char *output_char, Int8 xfertype)
{
	if (xfertype == USEDOC) {
		StrCat(output_char, "\n");
	} else {
		StrCat(output_char, "\r\n");
	}
}

Boolean GetSysVersion(Char *inputStr)
{
	Err error;
	MemHandle verH;
	Char* verS = NULL;

	verH = DmGet1Resource (verRsc, appVersionID); 
	verS = (Char *)MemHandleLock(verH);

	StrNCopy(inputStr, verS, 30);

	error = MemHandleUnlock(verH);
	error = DmReleaseResource(verH);

	return(true);
}

void DrawFormWithNoListBorder(FormType *frmP, UInt16 listIndex)
{
	RectangleType clip;
	RectangleType newClip;
	ListType *listP = FrmGetObjectPtr(frmP, listIndex);

	// Hide the list object and then draw the rest of the
	// form.
	FrmHideObject(frmP, listIndex);
	// Set the clipping rectangle to the list boundaries and
	// draw the list. This suppreses the list border.
	WinGetClip(&clip);
	FrmGetObjectBounds(frmP, listIndex, &newClip);
	if (data.config.listlinesvert == 1) {
		// draw gray lines
		WinDrawGrayLine(newClip.topLeft.x-1, 			newClip.topLeft.y-1,
				newClip.topLeft.x+newClip.extent.x, 	newClip.topLeft.y-1);

		WinDrawGrayLine(newClip.topLeft.x+newClip.extent.x, 	newClip.topLeft.y+newClip.extent.y,
				newClip.topLeft.x+newClip.extent.x, 	newClip.topLeft.y-1);

		WinDrawGrayLine(newClip.topLeft.x-1, 			newClip.topLeft.y+newClip.extent.y,
				newClip.topLeft.x+newClip.extent.x, 	newClip.topLeft.y+newClip.extent.y);

		WinDrawGrayLine(newClip.topLeft.x-1, 			newClip.topLeft.y-1,
				newClip.topLeft.x-1, 			newClip.topLeft.y+newClip.extent.y);
	}
	WinSetClip(&newClip);
	LstSetSelection(listP, noListSelection);
	FrmShowObject(frmP, listIndex);
	// Reset the clipping rectangle.
	WinSetClip(&clip);
}

void DrawHorizListLines(Int16 Num, Int16 Start, Int16 Step)
{
	Int16 x, Finish;

	if (data.config.listlinesvert > 0) {
		Finish = Start+Step*(Num-2);
	} else {
		Finish = Start+Step*(Num-1);	
	}

	// draw the horizontal lines
	switch (data.config.listlineshoriz) {
		case 1: 
			for (x=Start; x<=Finish; x+=Step) WinDrawGrayLine(1,x,158,x);
			break;
		case 2: 
			for (x=Start; x<=Finish; x+=Step) WinDrawLine(1,x,158,x);
			break;
		default:
			break;
	}
}

Char* BytetoChar(UInt8 data)
{
	static char retstr[2];
	retstr[0] = data;
	retstr[1] = '\0';
	return(retstr);	
}

void itoa(Int16 value, Char *buf, Int16 base)
{
	Int16 i = 30;
	buf = "";
	for(; value && i ; --i, value /= base) buf = "0123456789abcdef"[value % base] + buf;
}

double increment(double value, double incr)
{
	value = pround(value,5);
	return(value - Fmod(value, incr) + incr);
}

UInt8 Update_BatteryInfo()
{
	Boolean pluggedin;
	Boolean backlighton=false;
	UInt8 battpercent;
	Char TempChar[6];
	Char tempchar[6];

	SysBatteryInfo(false, NULL, NULL, NULL, NULL, &pluggedin, &battpercent);
	if (battpercent == 100) {
		battpercent = 99;
	}

	if (FrmGetActiveFormID() == form_final_glide) {
		StrIToA(tempchar, battpercent); 
		if (pluggedin) {
			StrCopy(TempChar, "C");
		} else {
			StrCopy(TempChar, "B");
			if (inflight && data.config.flightcomp == RECOCOMP) {
				data.flight.valid = false;
			}
		}
		StrCat(TempChar, leftpad(tempchar, '0', 2)); 
		if (battpercent > LOWBATWARNLEVEL) {
			WinDrawChars("    ", 4, BATX, BATY);
			WinDrawChars(TempChar, 3, BATX, BATY);
		} else {
			WinDrawInvertedChars("    ", 4, BATX, BATY);
			WinDrawInvertedChars(TempChar, 3, BATX, BATY);
		}
		if (!device.StyleTapPDA) {
			if (device.romVersion >= SYS_VER_35) {
				HwrDisplayAttributes(false, 18, &backlighton);
			} else {
				backlighton = HwrBacklight(false, false);
			}
		}

		if (!menuopen) {
			if (backlighton) {
//				WinDrawInvertedChars("BL", 2, BLX, BLY);
				FntSetFont((FontID)WAYSYMB11);
				tempchar[0] = 56;
				tempchar[1] = 0;
				WinDrawChars(tempchar, 1, BLX, BLY);
				FntSetFont(stdFont);
			} else {
				FntSetFont((FontID)WAYSYMB11);
				tempchar[0] = 57;
				tempchar[1] = 0;
				WinDrawChars(tempchar, 1, BLX, BLY);
				FntSetFont(stdFont);
			}
		}
	}
//	HostTraceOutputTL(appErrorClass, "backlighton = %hu", (UInt16)backlighton);
//	if (pluggedin) {
//		return(100);
//	} else {
		return(battpercent);
//	}
}

UInt32 GetOSFreeMem( UInt32* totalMemoryP, UInt32* dynamicMemoryP )
{
#define memoryBlockSize (1024L)

UInt32 heapFree;
UInt32 max;
Int16 i;
Int16 nCards;
UInt16 cardNo;
UInt16 heapID;
UInt32 freeMemory = 0;
UInt32 totalMemory = 0;
UInt32 dynamicMemory = 0;
 
// Iterate through each card to support devices with multiple cards.
nCards = MemNumCards();
for (cardNo = 0; cardNo < nCards; cardNo++) {

	// Iterate through the RAM heaps on a card (excludes ROM).
	for (i=0; i < MemNumRAMHeaps(cardNo); i++) {

		// Obtain the ID of the heap.
		heapID = MemHeapID(cardNo, i);

		// If the heap is dynamic, increment the dynamic memory total.
		if (MemHeapDynamic(heapID)) {
			dynamicMemory += MemHeapSize(heapID);
		} else {
			// The heap is nondynamic.
			// Calculate the total memory and free memory of the heap.
			totalMemory += MemHeapSize(heapID);
			MemHeapFreeBytes(heapID, &heapFree, &max);
			freeMemory += heapFree;
		}
	}
}
 
// Reduce the stats to KB.  Round the results.
freeMemory  = freeMemory / memoryBlockSize;
totalMemory = totalMemory  / memoryBlockSize;
dynamicMemory = dynamicMemory / memoryBlockSize;

if (totalMemoryP) *totalMemoryP = totalMemory;
if (dynamicMemoryP) *dynamicMemoryP = dynamicMemory;

return (freeMemory);
}// GetOSFreeMem

/* The Quicksort. */
void qs(Char *buffer, Int16 left, Int16 right, UInt16 size, Int8 descending)
{
	Int16 i, j;
	Char *x, *y;

	x = MemPtrNew(size); y = MemPtrNew(size);

	i = left; j = right;
	StrCopy(x, &buffer[((left+right)/2)*size]);

	do {
		while((StrCompare(&buffer[i*size], x)*descending < 0) && (i < right)) i++;
		while((StrCompare(x, &buffer[j*size])*descending < 0) && (j > left)) j--;

		if (i <= j) {
			StrCopy(y, &buffer[i*size]);
			StrCopy(&buffer[i*size], &buffer[j*size]);
			StrCopy(&buffer[j*size], y);
			i++; j--;
		}
	} while(i <= j);

	if(left < j) qs(buffer, left, j, size, descending);
	if(i < right) qs(buffer, i, right, size, descending);

	MemPtrFree(x); MemPtrFree(y);
}

/* Quicksort setup function. */
void quicksort(Char *buffer, Int16 count, UInt16 size, Int8 descending)
// quick sort an array of "count" strings of max length "size", starting at "buffer"
// if "descending" is negative, sort in descending order, otherwise sort in ascending order
{
	if (descending == 0) descending = 1;
	if (buffer && (count > 0) && (size < MAXINT16)) qs(buffer, 0, count-1, size, descending);
}

// find position of form in screen chain
Int8 FindScreenChainPosition(UInt16 frmID, UInt16 *screenchain)
{
	Int8 pos = SCREENCHAINMAX;
	Int8 i;

	for (i=0; i<SCREENCHAINMAX; i++) {
		if (screenchain[i] == frmID) {
			pos = i;
			break;
		}
	}

//	HostTraceOutputTL(appErrorClass, "screen chain pos %s", DblToStr(pos,0));
	return(pos);
}

// Determine if inputted screen is already in use
Int8 IsScreenInUse(UInt16 frmID, Int8 exceptionscr, UInt16 *screenchain)
{
	Int8 pos = SCREENCHAINLIST;
	Int8 i;

	for (i=0; i<SCREENCHAINLIST; i++) {
		if (screenchain[i] == frmID) {
			if (i != exceptionscr) {
				pos = i;
				break;
			}
		}
	}
	return(pos);
}

// jump to next screen in change
Boolean GotoNextScreen(UInt16 frmID, Int8 dir)
{
	Int8 i;
	Int8 screenchainpos;

	chgscreen = true;

//	HostTraceOutputTL(appErrorClass, "frmID1=|%s|", DblToStr((double)frmID,0));
	// If going left and LEFTACTION is set to normal action, do nothing
	// but return false so it will continue down the case statement
	if (data.config.leftaction == 1 && dir < 0) return(false);

	// If going left and LEFTACTION has it turned off, do nothing
	// but return true so that it will not do the rest of the case statement
	if (data.config.leftaction == 0 && dir < 0) return(true);

	// check for exceptions where one form is used for 2 or more screens
	// Flarm traffic display & form_sat_status
	if ((frmID == form_sat_status) && !IsGPS) frmID = form_flarm_traffic;

	// flight log map & form_moving_map
	if ((frmID == form_moving_map) && draw_log && !draw_task) frmID = form_flightlog_map;

	// task view map & form_moving_map
	if ((frmID == form_moving_map) && draw_log && draw_task) frmID = form_taskview_map;

//	HostTraceOutputTL(appErrorClass, "frmID2=|%s|", DblToStr((double)frmID,0));

	// Main Find Button Chain
	screenchainpos = FindScreenChainPosition(frmID, data.config.screenchain);
//	HostTraceOutputTL(appErrorClass, "screenchainpos1=|%s|", DblToStr((double)screenchainpos,0));

	// not in current list, but is could be in the list, so default to first or last screen depending on direction
	if (data.config.leftaction <= 1) {        // only goto first in list if default leftacion
		if ((screenchainpos >= SCREENCHAINMAX) && (FrmIDToScr(frmID) < SCREENCHAINLIST)) {
			if (dir > 0) {
				screenchainpos = SCREENCHAINMAX-1;
			} else {
				screenchainpos = 0;
			}
		}
	}
//	HostTraceOutputTL(appErrorClass, "screenchainpos2=|%s|", DblToStr((double)screenchainpos,0));

	// check if screen found in the list, or could have been in the list so default to first item
	if (screenchainpos < SCREENCHAINMAX) {

		for (i=0; i<SCREENCHAINMAX; i++) {
			// screen found so increment position in chain
			screenchainpos += dir;
			if (screenchainpos >= SCREENCHAINMAX) {
				screenchainpos = 0;
				if (data.config.leftaction == 3) chgscreen = false;
			}
				
			if (screenchainpos < 0) {
				screenchainpos = SCREENCHAINMAX-1;
				if (data.config.leftaction == 3) chgscreen = false;
			}
			
			// skip over in-active Flarm traffic screen
			if ((data.config.screenchain[screenchainpos] == form_flarm_traffic) 
				&& !Flarmpresent && (data.config.flightcomp != FLARMCOMP)) {
				screenchainpos += dir;
				if (screenchainpos >= SCREENCHAINMAX) {
					screenchainpos = 0;
					if (data.config.leftaction == 3) chgscreen = false;
				}
				
				if (screenchainpos < 0) {
					screenchainpos = SCREENCHAINMAX-1;
					if (data.config.leftaction == 3) chgscreen = false;
				}
			}

			// check conditions for Waypoint Sector screen are met
			if ((data.config.screenchain[screenchainpos] == form_waypoint_sector) 
				&& !((data.task.numwaypts > 0) && !taskonhold)) {
				screenchainpos += dir;
				if (screenchainpos >= SCREENCHAINMAX) {
					screenchainpos = 0;
					if (data.config.leftaction == 3) chgscreen = false;
				}
				
				if (screenchainpos < 0) {
					screenchainpos = SCREENCHAINMAX-1;
					if (data.config.leftaction == 3) chgscreen = false;
				}
			}

			// check for blank entry in list
			if (data.config.screenchain[screenchainpos] != form_set_scrorder_noscr) {
				break;
			}
		}

		// goto new form
		if (chgscreen) {
			PlayKeySound();
			switch (data.config.screenchain[screenchainpos]) {
				case form_waypoint_sector:	
					if ((data.task.numwaypts > 0) && !taskonhold) {
						if (inareasector > -1) {
							selectedTaskWayIdx = inareasector;
						} else {
							selectedTaskWayIdx = activetskway;
						}
						activetasksector = true;
						tsktoedit = &data.task;
						FrmGotoForm(form_waypoint_sector);
					}
					break;
				 case form_flarm_traffic:
					gotoGPSinfo = false;
					FrmGotoForm(form_sat_status);
					break;
				case form_sat_status:
					gotoGPSinfo = true;
					FrmGotoForm(form_sat_status);
					break;
				case form_moving_map:
					draw_log = false;
					draw_task = false;
					FrmGotoForm(form_moving_map);
					break;
				case form_list_waypt:
					if (FrmGetActiveFormID() == form_set_task) {
						origform = defaultscreen;
					}
					if ((data.task.numwaypts > 0) && tasknotfinished) {
						// task active
						if (taskonhold) savtaskonhold = true; else savtaskonhold = false;
						select_fg_waypoint(WAYTEMP);    // select TEMP waypoint - puts task on hold
//						select_fg_waypoint(WAYSELECT);  // popup normal waypoint select dialog for task
					} else {
						// no task active
						select_fg_waypoint(WAYNORM);
					}
					break;
				case form_flt_info:
					FrmGotoForm(form_flt_info);
					break;
				case form_wind_disp:
					FrmGotoForm(form_wind_disp);
					break;
				case form_wind_3dinfo:
					FrmGotoForm(form_wind_3dinfo);
					break;
				case form_thermal_history:
					FrmGotoForm(form_thermal_history);
					break;
				case form_set_fg:
					FrmGotoForm(form_set_fg);
					break;
				case form_set_qnh:
					FrmGotoForm(form_set_qnh);
					break;
				case form_final_glide:
					FrmGotoForm(form_final_glide);
					break;
				case form_list_sua:
					suasortType = SUASortByDistA;
					FrmGotoForm(form_list_sua);
					break;
				case form_set_task:
					// always go to active task
					dispactive = true;
					if ((taskIndex == 0) && (FrmGetActiveFormID() == form_set_task)) {
						FrmUpdateForm(form_set_task, frmRedrawUpdateCode);
 					} else {     	
						taskIndex = 0;
						FrmGotoForm(form_set_task);
					}
					break;
				case form_task_rules:
					// always go to active task
					dispactive = true;
					skipnewflight = true;
					if ((taskIndex == 0) && (FrmGetActiveFormID() == form_task_rules)) {
						FrmUpdateForm(form_task_rules, frmRedrawUpdateCode);
 					} else {     	
//						HostTraceOutputTL(appErrorClass, "chain to active rules");
						exittaskreadonly = true;
						taskIndex = 0;
						origform = defaultscreen;
						FrmGotoForm(form_task_rules);
					}
					break;
				case form_list_task:
					FrmGotoForm(form_list_task);
					break;
				default:
//					HostTraceOutputTL(appErrorClass, "main chain default - should never get here");
					break;
			}
		} else {
			// at end of chain
			PlayKeySound();
			PlayKeySound();
		}
		return(true);
	} else {
	       	// not in screen chain
		return(false);
	}
}

// Given a selection from one of the drop down menus on the screen order config window 
// find the correct Form ID to put into the chain array
UInt16 ScrToFrmID(Int16 selectitem) {
	UInt16 frmID;

	switch (selectitem) {
		case 0:
			frmID = form_final_glide;
			break;
		case 1:
			frmID = form_flt_info;
			break;
		case 2:
			frmID = form_wind_disp;
			break;
		case 3:
			frmID = form_wind_3dinfo;
			break;
		case 4:
			frmID = form_list_waypt;
			break;
		case 5:
			frmID = form_set_fg;
			break;
		case 6:
			frmID = form_set_qnh;
			break;
		case 7:
			frmID = form_set_task;
			break;
		case 8:
			frmID = form_moving_map;
			break;
		case 9:
			frmID = form_sat_status;
			break;
		case 10:
			frmID = form_flarm_traffic;
			break;
		case 11:
			frmID = form_thermal_history;
			break;
		case 12:
			frmID = form_waypoint_sector;
			break;
		case 13:
			frmID = form_list_sua;
			break;
		case 14:
			frmID = form_task_rules;
			break;
		case 15:
			frmID = form_list_task;
			break;
		case 16: // must be last item in list and equal to SCREENCHAINLIST-1
			frmID = form_set_scrorder_noscr;
			break;
		default:
			frmID = form_final_glide;
			break;
	}

	return(frmID);
}

// Given a form ID find the correct selection for the drop down menus on the screen order 
// config window 
Int16 FrmIDToScr(UInt16 frmID) {
	Int16 selectitem;

	switch (frmID) {
		case form_final_glide:
			selectitem = 0;
			break;
		case form_flt_info:
			selectitem = 1;
			break;
		case form_wind_disp:
			selectitem = 2;
			break;
		case form_wind_3dinfo:
			selectitem = 3;
			break;
		case form_list_waypt:
			selectitem = 4;
			break;
		case form_set_fg:
			selectitem = 5;
			break;
		case form_set_qnh:
			selectitem = 6;
			break;
		case form_set_task:
			selectitem = 7;
			break;
		case form_moving_map:
			selectitem = 8;
			break;
		case form_sat_status:
			selectitem = 9;
			break;
		case form_flarm_traffic:
			selectitem = 10;
			break;
		case form_thermal_history:
			selectitem = 11;
			break;
		case form_waypoint_sector:
			selectitem = 12;
			break;
		case form_list_sua:
			selectitem = 13;
			break;
		case form_task_rules:
			selectitem = 14;
			break;
		case form_list_task:
			selectitem = 15;
			break;
		case form_set_scrorder_noscr:
			// must be last item in list and equal to SCREENCHAINLIST-1
			selectitem = 16;
			break;
		default:
			selectitem = SCREENCHAINLIST;
			//selectitem = 0;
			break;
	}
	return(selectitem);
}

// calculate McCready cross country speed for given thermal strength
double CalcVxc(double MCVal, double* Vstf)
{
	double a=data.polar.a;
	double b=data.polar.b;
	double c=data.polar.c;
	double Wsstf, Vxc;

	// calculate Vstf - no wind
	if (data.config.optforspd) {
		*Vstf = -1/(2*a)*Sqrt(-4*a*(MCVal-c));
	} else {
		*Vstf = (-b-Sqrt(b*b+8*a*(-MCVal)))/(4*a);
	}
	// calculate min sink at Vstf
	Wsstf = a * (*Vstf) * (*Vstf) + b * (*Vstf) + c;
	// calculate MC XC speed
	Vxc = -MCVal / (Wsstf-MCVal) * (*Vstf);

	return(Vxc);
}

// outputs text data to the currently open VFS file
void outputlog(Char *output_char, Boolean crlf) {
	Char tempchar[10];
	if (device.VFSCapable) {
		TxData(output_char, USEVFS);
		StrCopy(tempchar, "\r\n");
		if (crlf) TxData(tempchar, USEVFS);
	}
}

UInt32 StrHToI(Char *pHexaString) {
	UInt32 value = 0;
//	HostTraceOutputTL(appErrorClass, pHexaString);
    while (*pHexaString) {
		value = value << 4;
		value += Hex2Dec(*pHexaString);
		pHexaString++;
	}
//	HostTraceOutputTL(appErrorClass, " -> %ld", value);
    return value;
}

