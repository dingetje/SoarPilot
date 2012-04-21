#ifndef SOARUTIL_H
#define SOARUTIL_H

#define SMTH __attribute__ ((section ("smth")))

#define SHOWDIALOG   0
#define UPDATEDIALOG 1
#define STOPDIALOG   2
#define CHECKCANCEL  3

// defines for user configurable find button chain
#define SCREENCHAINLIST 17	// number of items to select from

/*****************************************************************************
 * protos
 *****************************************************************************/
double presstoalt(double pressure) SMTH;
double presstoalt2(double pressure) SMTH;
double pround(double val, Int16 n) SMTH;
Char* not_valid() SMTH;
Char* print_horizontal_speed(DataValue val) SMTH;
Char* print_horizontal_speed2(double val, UInt16 precision) SMTH;
Char* print_horizontal_wndspeed(double val, UInt16 precision) SMTH;
Char* print_vertical_speed(DataValue val) SMTH;
Char* print_vertical_speed2(double val, UInt16 precision) SMTH;
Char* print_distance(DataValue val) SMTH;
Char* print_distance2(double val, UInt16 precision) SMTH;
Char* print_altitude(double val) SMTH;
Char* print_temperature(DataValue val) SMTH;
Char* print_qnh(DataValue val) SMTH;
Char* print_direction(DataValue val) SMTH;
Char* print_direction2(double val) SMTH;
Char* print_longitude(DataValue val) SMTH;
Char* print_latitude(DataValue val) SMTH;
Char* print_strnoperiod(Char *instring) SMTH;
double StrToDbl(Char* input) SMTH;
double CharToDbl(Char input) SMTH;
Char* DblToStr(double input, UInt16 prec) SMTH;
Char* DblToStr2(double value, Int16 places, Boolean IncDecPt) SMTH;
double next_comma_dbl(Char* *s) SMTH;
Char* leftpad(Char* input, char padchar, int maxlen) SMTH;
Char* rightpad(Char* input, char padchar, int maxlen) SMTH;
void field_set_value(UInt16 fieldID, Char* s) SMTH;
double field_get_double(UInt16 fieldID) SMTH;
Int32 field_get_long(UInt16 fieldID) SMTH;
void field_set_enable(UInt16 ctlID, Boolean field_enable) SMTH;
UInt16 field_get_enable(UInt16 ctlID) SMTH;
void field_set_visible(UInt16 fieldID, Boolean field_visible) SMTH;
Char* field_get_str(UInt16 fieldID) SMTH;
Boolean ctl_get_value(UInt16 ctlID) SMTH;
void ctl_set_value(UInt16 ctlID, Boolean ctl_value) SMTH;
void ctl_set_enable(UInt16 ctlID, Boolean ctl_enable) SMTH;
void ctl_set_visible(UInt16 ctlID, Boolean ctl_visible) SMTH;
void ctl_set_label(UInt16 ctlID, Char* s) SMTH;
void frm_set_label(UInt16 labelID, Char* s) SMTH;
void frm_set_title(Char* s) SMTH;
Char* ctl_get_label(UInt16 ctlID) SMTH;
void lst_set_selection(UInt16 lstID, Int16 lst_selection) SMTH;
void set_unit_constants() SMTH;
Boolean GetField(const Char *buffer, UInt16 n, Char *result) SMTH;
Boolean GetFieldDelim(const Char *buffer, UInt16 numstartdelims, Char startdelim, Char enddelim, Char *result) SMTH;
char* SecondsToDateOrTimeString(UInt32  date, char  *textP, int dttype, Int32 tz) SMTH;
void StringToDateOrTime(Char* textP, DateTimeType *dtt, Int8 dttype) SMTH;
void StringToDateAndTime(Char *dateP, Char *timeP, DateTimeType *dtt) SMTH;
Boolean RightHandToD(char  *inStr, double   *valueP) SMTH;
Char* trim(Char* string, Char trimchr, Boolean bothsides) SMTH;
UInt32 Hex2Dec(Char HexChar) SMTH;
Int32 Hex2Dec2(Char* P_Alt) SMTH;
UInt32 Hex2Dec3(Char* HexStr) SMTH;
UInt32 B362Dec(Char* HexStr) SMTH;
UInt32 B142Dec(Char* inputStr) SMTH;
void Report(Char *formatStr, ...) SMTH;
void PlaySound( Int32 hz, UInt16 ms, UInt16 amp ) SMTH;
void PlayStartSound() SMTH;
void PlayTurnSound() SMTH;
void PlayFinishSound() SMTH;
void PlayNoGPSSound() SMTH;
void PlayWarnSound() SMTH;
void PlayKeySound() SMTH;
void HandleWaitDialog(Boolean showdialog) SMTH;
Boolean HandleWaitDialogWin(Int16 cmd) SMTH;
void return_to_form(Int16 return_form) SMTH;
Boolean HandleWaitDialogUpdate(Int16 cmd, UInt16 totalnum, UInt16 curnum, Char *type) SMTH;
void GenerateIGCName(FlightData *fltdata, Char *igcname) SMTH;
//void Update_PalmTime() SMTH;
Char *GetSysRomID() SMTH;
Char *CalcChkSum(Char* inputstr) SMTH;
Char *Dec2Hex(UInt16 inputint) SMTH;
Char *Str2Hex(Char *inputstr) SMTH;
void Sleep(double SleepFor) SMTH;
Int16 toupper(Int16 c) SMTH;
Int16 ConvertToUpper(Char *str) SMTH;
Int16 tolower(Int16 c) SMTH;
Int16 ConvertToLower(Char *str) SMTH;
Char* ToNumeric(Char *str) SMTH;
Int8 ChkUnits(Char *str) SMTH;
Char* NoComma(Char *str, Char *RepChar) SMTH;
Char *Left(Char *instr, UInt16 n) SMTH;
Char* Right(Char* string, UInt16 n) SMTH;
Char *Mid(Char *s, UInt16 n, UInt16 p) SMTH;
Char *CBase(UInt32 number, UInt32 base) SMTH;
Char *GenerateUID(Char *inputStr) SMTH;
double ComputerVariation(double lat, double lon) SMTH;
void StrCatEOL(Char *output_char, Int8 xfertype) SMTH;
Boolean GetSysVersion(Char *inputStr) SMTH;
void DrawFormWithNoListBorder(FormType *frmP, UInt16 listIndex) SMTH;
void DrawHorizListLines(Int16 Num, Int16 Start, Int16 Step) SMTH;
Char* BytetoChar(UInt8 data) SMTH;
void itoa(Int16 value, Char *buf, Int16 base) SMTH;
double increment(double value, double incr) SMTH;
UInt8 Update_BatteryInfo() SMTH;
UInt32 GetOSFreeMem( UInt32* totalMemoryP, UInt32* dynamicMemoryP ) SMTH;
void quicksort(Char *buffer, Int16 count, UInt16 size, Int8 descending) SMTH;
Int8 FindScreenChainPosition(UInt16 frmID, UInt16 *screenchain) SMTH;
Int8 IsScreenInUse(UInt16 frmID, Int8 exceptionscr, UInt16 *screenchain) SMTH;
Boolean GotoNextScreen(UInt16 frmID, Int8 dir) SMTH;
UInt16 ScrToFrmID(Int16 selectitem) SMTH;
Int16 FrmIDToScr(UInt16 frmID) SMTH;
double CalcVxc(double MCVal, double *Vstf) SMTH;
void outputlog(Char *output_char, Boolean crlf) SMTH;
UInt32 StrHToI(Char *pHexaString) SMTH;
#endif
