#include <PalmOS.h>
#include "soaring.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarIO.h"
#include "soarPLst.h"
#include "soarMath.h"

Int16 	numOfPolars = 0;
Int16	currentPolarPage = 0;

PolarData *selectedPolar;
Int16 selectedPolarIndex = -1;
PolarData *inusePolar;
Boolean newPolar = false;

extern Int32  rxrecs;
extern char rxtype[15];
extern Char buf[PARSELEN];

// David Lane - comparison function supplied to DmQuickSort
Int16 polar_comparison(PolarData* rec1,
			PolarData* rec2,
			Int16 order,
			SortRecordInfoPtr rec1SortInfo,
			SortRecordInfoPtr rec2SortInfo,
			MemHandle h)
{
	//Not sure it should be initialized to zero
	Int16 retval=(StrCompare(rec1->name, rec2->name));

	return (retval);
}

void refresh_polar_list(Int16 scr)
{
	#define FIELDLEN 20

	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	Int16 x;
	Int16	polarIndex;
	MemHandle polar_hand;
	MemPtr polar_ptr;
	static Char **items = NULL;
	Int16 nrecs;
	Int16 start;
	Int16 end;
	static Int16 prevNumRecs = 0;
	PolarData polar;
	Char pageString[20];
	Char tmpString[20];

	// David Lane - free up each of the previous strings and then free up
	//  the array of pointers itself.
	for (x = 0; x < prevNumRecs; x++)
	{
		MemPtrFree(items[x]);
	}
	if (items)
	{
		MemPtrFree(items);
		items = NULL;
	}

	if (scr != 9999) {
		// David Lane - get the number of polars in the database
		numOfPolars = DmNumRecords(polar_db);

		// David Lane - get the List pointer
		lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_polar_list));

		// David Lane - lets compute the "page" of polars we're
		//  currently looking at.
		if (scr > 0) {
			if (((currentPolarPage + 1) * 8) <  numOfPolars) {
				currentPolarPage++;
			} else {
				currentPolarPage = 0;
			}
		} else if (scr < 0) {
			if (currentPolarPage > 0) {
				currentPolarPage--;
			} else {
				if (numOfPolars == 0) {
					currentPolarPage = 0;
				} else if (Fmod((double)numOfPolars,8.0) == 0.0) {
					currentPolarPage = (Int16)(numOfPolars/9) - 1;
				} else {
					currentPolarPage = (Int16)(numOfPolars/9);
				}
			}
		}

		// David Lane - given the current "page", compute the starting
		//  and ending index and the number of records.
		start = currentPolarPage * 8;
		end = ((start + 8) > numOfPolars) ? numOfPolars : (start + 8);
		nrecs = end - start;

		if (nrecs > 0) {
			// David Lane - we got at least one record so allocate enough 
			//  memory to hold nrecs pointers-to-pointers
			items = (char **) MemPtrNew(nrecs * (sizeof(char *)));
			prevNumRecs = nrecs;

			// David Lane - loop through each polar record
			for (x = 0, polarIndex = start; polarIndex < end; polarIndex++, x++)
			{ 
				// David Lane - assign each of the nrecs pointers-to-pointers
				//  the address of a newly allocated 15 character array,
				//  retrieve the polar name associated with that record,
				//  and copy that name into the array.
				OpenDBQueryRecord(polar_db, polarIndex, &polar_hand, &polar_ptr);
				MemMove(&polar,polar_ptr,sizeof(PolarData));
				MemHandleUnlock(polar_hand);
				items[x] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
				MemSet(items[x],FIELDLEN,0);
				StrNCopy(items[x], polar.name, 16);
			}
					
			// David Lane - reform the list
			LstSetListChoices(lst, items, nrecs);
		} else {
			items = (char **) MemPtrNew(1 * (sizeof(char *)));
			prevNumRecs = 1;		
			items[0] = (char *) MemPtrNew(FIELDLEN * (sizeof(char)));
			MemSet(items[0],FIELDLEN,0);
			StrNCopy(items[0], "No Polars", 9);
			LstSetListChoices(lst, items, 1);
			LstSetSelection(lst, 0);
		
			selectedPolarIndex = -1;
			ctl_set_visible(form_list_polar_edit, false);
			ctl_set_visible(form_list_polar_delete, false);
		}

		// David Lane - create the "Page: # of #" string
		MemSet(pageString,20, 0);
		StrCopy(pageString,StrIToA(tmpString,(currentPolarPage+1)));
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tmpString,(numOfPolars % 8) ? (((int)(numOfPolars/8)) + 1) : (int)(numOfPolars/8)));
		field_set_value(form_list_polar_page, pageString);
		field_set_value(form_list_polar_nrecs, StrIToA(tmpString,numOfPolars));

		// David Lane - redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_polar_list));
		}

		// David Lane - if the currently selected polar is on the currently
		//  displayed page, then darken it as if it were selected.  If not then
		//  de-select everything.
	
		if ((selectedPolarIndex >= (currentPolarPage*8)) && (selectedPolarIndex < ((currentPolarPage*8) + 8)))
		{
			LstSetSelection(lst, selectedPolarIndex % 8);
			//MFH Had to do this to get selectedPolar updated after the reselection.
			//    Not sure why it wasn't working with just the LstSetSelection.
			OpenDBQueryRecord(polar_db, selectedPolarIndex, &polar_hand, &polar_ptr);
			MemMove(selectedPolar, polar_ptr, sizeof(PolarData));
			MemHandleUnlock(polar_hand);
		} else
		{
			LstSetSelection(lst, -1);
		}
		DrawHorizListLines(8, 28, 14);
	}
}

Boolean save_polar_fields(double SinkConst, Boolean checkupd)
{
	Boolean OKpolar;
	Int16 i, namect;
	MemHandle polar_hand;
	MemPtr polar_ptr;
	PolarData CheckPolar;

	warning->type = Wgeneric;

	// get all data fields
	StrCopy(selectedPolar->name, trim(NoComma(field_get_str(form_av_polar_name)," "),' ',true));
	selectedPolar->v1 = field_get_double(form_av_polar_v1)/data.input.spdconst;
	selectedPolar->w1 = field_get_double(form_av_polar_w1)/SinkConst;
	selectedPolar->v2 = field_get_double(form_av_polar_v2)/data.input.spdconst;
	selectedPolar->w2 = field_get_double(form_av_polar_w2)/SinkConst;
	selectedPolar->v3 = field_get_double(form_av_polar_v3)/data.input.spdconst;
	selectedPolar->w3 = field_get_double(form_av_polar_w3)/SinkConst;
	selectedPolar->maxdrywt = field_get_double(form_av_polar_maxdrywt)/data.input.wgtconst;
	selectedPolar->maxwater = field_get_double(form_av_polar_maxwater)/data.input.wtrconst;
	
	// check for empty name
	if (checkupd && (StrLen(selectedPolar->name) == 0)) {
		StrCopy(warning->line1, "Polar name");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);	
		return false;
	}

	// check for duplicate name
	namect = 0;
	if (checkupd && DmNumRecords(polar_db) > 0) { 
		for (i=0; i<DmNumRecords(polar_db); i++) {
			OpenDBQueryRecord(polar_db, i, &polar_hand, &polar_ptr);
			MemMove(&CheckPolar, polar_ptr, sizeof(PolarData));
			MemHandleUnlock(polar_hand);
			if ((StrCompare(selectedPolar->name, CheckPolar.name) == 0) && (selectedPolarIndex != i)) namect++; 
		}
		if (namect > 0) {
			StrCopy(warning->line1, "Duplicate Polar name");
			StrCopy(warning->line2, " ");
			FrmPopupForm(form_warning);	
			return false;
		}
	}	

//	HostTraceOutputTL(appErrorClass, "v1=|%s|", DblToStr(selectedPolar->v1, 1));
	if (checkupd && (selectedPolar->v1 == 0.0)) {
		StrCopy(warning->line1, "Speed of Point #1 must");
		StrCopy(warning->line2, "be greater than zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "w1=|%s|", DblToStr(selectedPolar->w1, 1));
	if (selectedPolar->w1 > 0.0) {
		selectedPolar->w1 *= -1.0;
	}
	if (checkupd && (selectedPolar->w1 == 0.0)) {
		StrCopy(warning->line1, "Sink of Point #1");
		StrCopy(warning->line2, "must not be zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "v2=|%s|", DblToStr(selectedPolar->v2, 1));
	if (checkupd && (selectedPolar->v2 == 0.0)) {
		StrCopy(warning->line1, "Speed of Point #2 must");
		StrCopy(warning->line2, "be greater than zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "w2=|%s|", DblToStr(selectedPolar->w2, 1));
	if (selectedPolar->w2 > 0.0) {
		selectedPolar->w2 *= -1.0;
	}
	if (checkupd && (selectedPolar->w2 == 0.0)) {
		StrCopy(warning->line1, "Sink of Point #2");
		StrCopy(warning->line2, "must not be zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "v3=|%s|", DblToStr(selectedPolar->v3, 1));
	if (checkupd && (selectedPolar->v3 == 0.0)) {
		StrCopy(warning->line1, "Speed of Point #3 must");
		StrCopy(warning->line2, "be greater than zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "w3=|%s|", DblToStr(selectedPolar->w3, 1));
	if (selectedPolar->w3 > 0.0) {
		selectedPolar->w3 *= -1.0;
	}
	if (checkupd && ( selectedPolar->w3 == 0.0)) {
		StrCopy(warning->line1, "Sink of Point #3");
		StrCopy(warning->line2, "must not be zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	HostTraceOutputTL(appErrorClass, "weight=|%s|", DblToStr(selectedPolar->maxdrywt, 1));
	if (checkupd && (selectedPolar->maxdrywt == 0.0)) {
		StrCopy(warning->line1, "Max. Dry Gross Wt must");
		StrCopy(warning->line2, "be greater than zero");
		FrmPopupForm(form_warning);	
		return false;
	}

//	CalcPolarABC(selectedPolar, 1.0);
	
//	ldspd = Sqrt(selectedPolar->c/selectedPolar->a);
//	HostTraceOutputTL(appErrorClass, "ldspd=|%s|", DblToStr(ldspd, 6));
//	ldsnk = (selectedPolar->a*ldspd*ldspd) + (selectedPolar->b*ldspd) + selectedPolar->c;
//	HostTraceOutputTL(appErrorClass, "ldsnk=|%s|", DblToStr(ldsnk, 6));
//	ld = ldspd / ldsnk * -1.0;

//	field_set_value(form_av_polar_ldspd, print_horizontal_speed2(ldspd, 1));
//	field_set_value(form_av_polar_maxld, DblToStr(pround(ld, 1), 1));

	OKpolar = CalcPolarABC(selectedPolar, data.config.bugfactor);

	if (OKpolar && checkupd) {
		if (newPolar) {
			OpenDBAddRecord(polar_db, sizeof(PolarData), selectedPolar, true);
		} else {
			OpenDBUpdateRecord(polar_db, sizeof(PolarData), selectedPolar, selectedPolarIndex);
		}
		return(OKpolar);
	} else {
		warning->type = Werror;
		StrCopy(warning->line1, "Invalid polar data!");
		StrCopy(warning->line2, "Cannot save");
		FrmPopupForm(form_warning);	
		return(false);
	}
}

Int16 FindPolarRecordByName(Char* NameString) 
{
	Int16 polarindex = 0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	PolarData polardata;
	Int16 retval=-1;

	nrecs = OpenDBCountRecords(polar_db);

	for (polarindex=0; polarindex<nrecs; polarindex++) {
		OpenDBQueryRecord(polar_db, polarindex, &output_hand, &output_ptr);
		MemMove(&polardata, output_ptr, sizeof(PolarData));
		MemHandleUnlock(output_hand);

		if (StrCompare(NameString, polardata.name) == 0) {
			retval = polarindex;
			polarindex = nrecs;
		}
	}
	return(retval);
}

void polar_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  polaridx;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	PolarData polar;
	static Boolean skip = false;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		return;
	}

	MemSet(&polar, sizeof(PolarData), 0);
	StrCopy(rxtype, "Polars");

	while (cur < length) {
		while( (cur < length) && (next < PARSELEN) && (serinp[cur] != '\n')) {
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
			lineoverflow = true;
			next=0;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if ((cur <= length) && (skip == false)) {
				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {
					//	Getting polar name
					GetField(buf, 0, tempchar);
					StrNCopy(polar.name, trim(NoComma(tempchar," "), ' ', true), 15);

					//	Getting v1
					GetField(buf, 1, tempchar);
					polar.v1 = StrToDbl(tempchar);

					//	Getting w1
					GetField(buf, 2, tempchar);
					polar.w1 = StrToDbl(tempchar);

					//	Getting v2
					GetField(buf, 3, tempchar);
					polar.v2 = StrToDbl(tempchar);

					//	Getting w2
					GetField(buf, 4, tempchar);
					polar.w2 = StrToDbl(tempchar);

					//	Getting v3
					GetField(buf, 5, tempchar);
					polar.v3 = StrToDbl(tempchar);

					//	Getting w3
					GetField(buf, 6, tempchar);
					polar.w3 = StrToDbl(tempchar);

					//	Getting maxdrywt
					GetField(buf, 7, tempchar);
					polar.maxdrywt = StrToDbl(tempchar);

					//	Getting maxwater
					GetField(buf, 8, tempchar);
					polar.maxwater = StrToDbl(tempchar);

					if ( (polar.name[0] != '*') &&
					     (StrLen(polar.name) != 0) &&
					     (polar.v1 != 0.0) && (polar.w1 != 0.0) &&
					     (polar.v2 != 0.0) && (polar.w2 != 0.0) &&
					     (polar.v3 != 0.0) && (polar.w3 != 0.0))
					{
						CalcPolarABC(&polar, data.config.bugfactor);
						polaridx = OpenDBAddRecord(polar_db, sizeof(PolarData), &polar, false);
						rxrecs = polaridx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(polaridx+1,0));
						}
	//					HostTraceOutputTL(appErrorClass, "polaridx |%hd|", polaridx);
					}
				}
			}
			next=0;
		}
		skip = false;
	}
	return;
}

void OutputPolarHeader()
{
	Char output_char[80];
	Char dtgstr[15];
	DateType date;

	DateSecondsToDate (TimGetSeconds(), &date);
	DateToAscii (date.month, date.day, date.year+1904, dfDMYLong, dtgstr);

	StrCopy(output_char, "** -------------------------------------------------------------");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "**      SOARINGPILOT Version ");
	StrCat(output_char, device.appVersion);
	StrCat(output_char, " Polars");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "**      Date: ");
	StrCat(output_char, dtgstr);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "** -------------------------------------------------------------");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	return;
}

void OutputPolarRecords()
{
	Char output_char[80];
	Char *charP=NULL;
	Int16 polarindex=0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	PolarData polardata;

	nrecs = OpenDBCountRecords(polar_db);

	for (polarindex=0; polarindex<nrecs; polarindex++) {
		if ((polarindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, polarindex, "Polars");

		OpenDBQueryRecord(polar_db, polarindex, &output_hand, &output_ptr);
		MemMove(&polardata, output_ptr, sizeof(PolarData));
		MemHandleUnlock(output_hand);

		while ((charP=StrChr(polardata.name, ','))) {
			*charP = ' ';
		}
		StrCopy(output_char, polardata.name);
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.v1,1), 1));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.w1,3), 3));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.v2,1), 1));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.w2,3), 3));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.v3,1), 1));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.w3,3), 3));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.maxdrywt,1), 1));
		StrCat(output_char, ",");

		StrCat(output_char,DblToStr(pround(polardata.maxwater,1), 1));

		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	return;
}

Boolean CalcPolarABC(PolarData *polar, double bugfactor)
{
	double v1,v2,v3,w1,w2,w3,d;
	double scalefactor;

	v1 = polar->v1;
	w1 = polar->w1/bugfactor;
	v2 = polar->v2;
	w2 = polar->w2/bugfactor;
	v3 = polar->v3;
	w3 = polar->w3/bugfactor;

	d = v1*v1*(v2-v3)+v2*v2*(v3-v1)+v3*v3*(v1-v2);
//	HostTraceOutputTL(appErrorClass, "d1=|%s|", DblToStr(d, 1));
	if (d == 0.0) {
		polar->a = 0.0;
	} else {
		polar->a = ((v2-v3)*(w1-w3)+(v3-v1)*(w2-w3))/d;
//		HostTraceOutputTL(appErrorClass, "a=|%s|", DblToStr(polar->a, 6));
	}
	d = v2-v3;
//	HostTraceOutputTL(appErrorClass, "d2=|%s|", DblToStr(d, 1));
	if (d == 0.0) {
		polar->b = 0.0;
	} else {
		polar->b = (w2-w3-polar->a*(v2*v2-v3*v3))/d;
//		HostTraceOutputTL(appErrorClass, "b=|%s|", DblToStr(polar->b, 6));
	}	 
	polar->c = w3 - polar->a*v3*v3 - polar->b*v3;
//	HostTraceOutputTL(appErrorClass, "c=|%s|", DblToStr(polar->c, 6));


	// Shifting the polar to account for water ballast
	if (polar->maxdrywt <= 0.0 || polar->maxwater <= 0.0) {
		scalefactor = 1.0;
	} else {
		scalefactor = Sqrt((polar->maxdrywt + (data.config.pctwater*polar->maxwater*PPGW)) / polar->maxdrywt);
	}
	polar->a /= scalefactor;
//	HostTraceOutputTL(appErrorClass, "Scaled a=|%s|", DblToStr(polar->a, 6));
	polar->c *= scalefactor;
//	HostTraceOutputTL(appErrorClass, "Scaled c=|%s|", DblToStr(polar->c, 6));

//	HostTraceOutputTL(appErrorClass, "=====================================");

	// check for valid polar
	if ((polar->a > -0.0001) || (polar->b*polar->b - 4*polar->a*polar->c >= 0) || (polar->c/polar->a < 0.0)) {
		return(false);
	}		

	// Calc Min Sink Speed and Sink value at Min Sink Speed
	polar->Vmin = (polar->b * -1.0) / (polar->a * 2.0); // Min Sink Speed
//	HostTraceOutputTL(appErrorClass, "Vmin=|%s|", DblToStr(Vmin, 1));
	polar->Wsmin = polar->a*polar->Vmin*polar->Vmin + polar->b*polar->Vmin + polar->c; // Sink value at Min Sink Speed
//	HostTraceOutputTL(appErrorClass, "Wsmin=|%s|", DblToStr(Wsmin, 1));

	return(true);
}
