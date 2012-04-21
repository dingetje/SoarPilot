/** GraphLib: Shared library routines for basic graph creation.
 *
 * The GraphLib source file. Contains the API's that implement basic
 * graph routines.
 *
 * INSTALLATION
 *
 * include grlib.h in the main application source file and then compile
 * gr.c with the other application source files.
 *
 * Copyright (C) 2001. Venugopal M. (http://homecreations.tripod.com)
 *
 * 
 * GraphLib source files (gr.c and grlib.h) are NOT FREE WARE. 
 * You cannot redistribute this code in any form  -original or modified. However
 * you have the rights to modify and use this code for compiling and using
 * in your application. The distribution rights rest with me.
 *
 * By using the program and the library you understand and agree that you
 * will not hold me liable for ANY damage, or expect any support and warranty
 * and also understand that I am in no way responsible for consequences of using
 * this code.
 *
 * Version 2.0
 *--------------
 * GraphLib premium upgrade: Pie graph related variables added
 *
 * Version 1.3 changes
 *
 * 1. stackPrevY and stackPrevX changed to UInt32 from UInt16. This fixes
 *    bugs caused by data overflow.
 *
 * 
 * Version 1.2 , 2001.
 */

#ifndef SOARUGRAPH_H
#define SOARUGRAPH_H

// MFH Added to put in its own code segment
#define SGRA __attribute__ ((section ("sgra")))

typedef enum { 
    NO_FILL,
    SOLID, 
    SOLID_1, 
    SOLID_2,
    SOLID_3,
    SOLID_4,
    SOLID_5,
  FRAME, ROUNDFRAME, CRAZY, SIMPLE_FRAME,ERASE_FRAME,MAX_RECT_TYPE
} PatternType_t;

typedef enum { 
    NO_GRID = 0, //NO grid
    YGRID,    //print the Y axis grids ----
    XGRID,      //print the X axis grids |
    BGRID,      //print the both axis grids |
    YTEXT,      //Print the scaled values along Y axis ( 1998 | )
    XTEXT,		 //Print the scaled values along X axis ( -1998 )
	 BTEXT       //Print the scaled values along both axis 
} grid_t;
 
typedef enum {
    BAR, 
    LINE, 
    LINE_FILL,
    LINE_FILL_HASH,
    STACK,
    PIE, 
    MAX_GRAPH_TYPE 
} GraphType_t;

 typedef enum grLibErrCode {
    grLibNone = 0,
    grLibInvData,   // Invalid Data
    grLibSwError, //Internal software error
    grLibBeyondMaxBound //memory bound exceeds
} gErr;

typedef enum PrintDataTypes {
    NONE = 0,
    PRINT_VALUES,
    PRINT_PERCENTAGES
} PrintDataType_t;

#define GraphLibVersion 2.0
#define GraphLibName "GrLib"
#define GraphLibCreatorID 'GLib'
#define GrLibType 'libr'

#define END_SPACE_PIXELS 5
#define GRAPH_AREA_RATIO 1
#define MAX_CHARS_PER_BAR 10
#define LINE_POINT_CHAR "."

//Pie space defines whether every pixel of the pie
//graph is plotted, or only alternative, or
//only the third etc. - The define 2 means only
//alternative points are plotted and so on. This is with a good reason
//since these require lots of floating point calculations
//they can be slow - one way to speed it up is by cutting
//processing speed by 6 times by plotting only the 6th point 


//WARNING! Do not make either of the defines below 1 -- will result
//in infinite loops in the code.
#define PIE_SPACE 4

//Bar Graph default declaration variables
#define PALM_MAX_SCR_X 158 //max width of screen
#define PALM_MAX_SCR_Y 158 //max height of the screen
#define MAX_LEGEND_STRING_LENGTH 13 //number of characters in a legend
#define MAX_LEGEND_LENGTH 30 //number of characters in a row

#define LEGEND_TEXT_COLOR 10 //legend text color,10 is a dark pink type color
#define GRID_LINE_COLOR 49 //a dark brown color

//Number of data points information
#define MAX_DATA_POINTS 20 //max number of data points in graph
#define DEFAULT_BARS 5 //default number of data points

//initial Graph Frame Coordinates
#define TOPLEFT_X 5
#define TOPLEFT_Y 5
#define EXTENT_X 130
#define EXTENT_Y 100

//scale information
#define DEFAULT_SCALE_X 100 //max X axis value for graph
#define DEFAULT_SCALE_Y 100 //max Y axis value for graph

//macros to simplify some tasks
//this macro gives the correct Y axis screen coordinate using the 
//calculated 'Y' position from scale and graph data value
#define COORD_Y(computedValue) (br->rect.topLeft.y+br->rect.extent.y - (computedValue))

//given a data value (Y or X), computes the scaled value to fit in the screen
#define CX(val) br->rect.extent.x * (val)/(br->scaleX - br->minX);
#define CY(val) br->rect.extent.y * (val)/(br->scaleY - br->minY);

#define RED (RGB(255,64,64))
#define GREEN (RGB(0,176,0))
#define DARK_BLUE (RGB(0,120,240))
#define LIGHT_GREEN (RGB(196,255,255))
#define LIGHT_YELLOW (RGB(255,255,213))
#define LIGHT_BLUE (RGB(232,232,255))
#define WIN_SET_TEXT_COLOR(indx) WinSetTextColor((UInt8)((indx+10)*(indx+10)))
#define WIN_SET_FORE_COLOR(indx) WinSetForeColor((UInt8)((indx+10)*(indx+10)))
#define ACTIVE_GRAPHS 3

//this structure defines the data information for a graph
 typedef struct 
    {
        Char *info; //legend string to be filled once for each group
        void *user; //user-definable void * value
        UInt32 yValue; //value for the Y axis 
        UInt32 xValue; //value for x axis
        UInt16 group;//identifies multiple groups within a graph
        RectangleType r; //hold the data boundaries.
    }dataPt_t;

typedef struct
{
    GraphType_t gtype;
    RectangleType rect; //defines bar graphs boundaries

    //the scale values denoted on the graph axis. This is not the
    //pixel coordinate but the actual data scale values. e.g X axis
    //having values from years 1999 to 2010 and Y-axis showing output
    //from 500 to 1500 tons. In that case minX = 1999 and scaleX = 2010
    //minY = 500 and scaleY = 1500
    UInt32 minX;
    UInt32 minY;
    UInt32 scaleY;
    UInt32 scaleX;

    //internal computed width of bars in a bar/stack graph
    UInt32 barWd;
    //internal count of number of data points
    UInt32 numDataPts;

    //stackable graph holder value (used in STACK graph only)
    UInt32 stackPrevY;
    UInt32 stackPrevX;
    dataPt_t *gpt; //graph point information

    //does the device support color? value set after calling routine 
    //establishes the result
    Boolean color;
    MemHandle grMemH; //handle to allocate for gpt.
    
    //Pie graph related values. 
    //Used only in graphLib premium version
    Coord radius;
    Coord centerX;
    Coord centerY; 
    UInt32 totalValue;
    PrintDataType_t prn;

} Graph_t;

typedef struct 
{
    Graph_t *gr;
} graphmgr_t;


//fill variables

#ifdef __FILL_H__
const CustomPatternType hgray25={  0xFF,0x00,0x00,0x00,
                                                         0xFF,0x00,0x00,0x00 };
const CustomPatternType crazy={  0xFF,0xAA,0xAA,0xAA,
                                                         0xFF,0xC0,0xC0,0xAA };
// MFH Had to put const infront of these to get it to compile correctly
//CustomPatternType solidb={  0xFF,0xFF,0xFF,0xFF,
const CustomPatternType solidb={  0xFF,0xFF,0xFF,0xFF,
                                                         0xFF,0xFF,0xFF,0xFF }; //full solid
//CustomPatternType solid_1={  0xAA,0x00,0xAA,0x00,
const CustomPatternType solid_1={  0xAA,0x00,0xAA,0x00,
                                                         0xAA,0x00,0xAA,0x00 };//dotted fill
//CustomPatternType solid_2={  0xFF,0xFF,0x00,0xFF,
const CustomPatternType solid_2={  0xFF,0xFF,0x00,0xFF,
                                                         0x00,0xFF,0xFF,0x00 }; //horiz barcode
//CustomPatternType solid_4={  0x00,0xFF,0xFF,0x00,
const CustomPatternType solid_4={  0x00,0xFF,0xFF,0x00,
                                                         0xAA,0xAA,0xAA,0xAA }; //vert+horiz
//CustomPatternType solid_3={  0xAA,0xAA,0xAA,0xAA,
const CustomPatternType solid_3={  0xAA,0xAA,0xAA,0xAA,
                                                         0xFF,0xFF,0xFF,0xFF};//(darker horiz)+vert 
#endif

//function prototypes

gErr grLibDrawRectangle(UInt8, 
                        UInt8,
                        UInt8, 
                        UInt8,
                        PatternType_t) SGRA;

void grLibDrawGraph(Graph_t *,UInt8) SGRA;

void grLibDrawGraphGrids(Graph_t *, grid_t, grid_t,UInt8) SGRA;
void grLibDrawGraphLegends(Graph_t *,UInt8) SGRA;
void grLibDrawGraphText(Graph_t *br, UInt32 xValue, UInt32 yValue, Int8 xOffset, Int8 yOffset, Char *text) SGRA;
void grLibDrawNumber(UInt8,UInt8,UInt32) SGRA;
gErr grLibInputGraphData(Graph_t *,UInt32, UInt32,UInt16,Char *) SGRA;
gErr grLibSetGraphParams(Graph_t *br, Coord x, Coord y, Coord ex, Coord ey, GraphType_t gtype, Boolean color) SGRA;
gErr grLibDrawGraphFrame( Graph_t *br,PatternType_t r) SGRA;
gErr DrawRectangle(RectangleType rect,PatternType_t r) SGRA;
gErr grLibSetMaxMinScales(Graph_t *br,UInt32,UInt32,UInt32,UInt32) SGRA;
UInt8 GetRandomNumberForScreen(UInt32) SGRA;
void grLibInitializeGraphStruct(Graph_t *) SGRA;
gErr grLibReleaseResources(Graph_t *) SGRA;
void grLibDrawGraphTitle(Graph_t *,const Char *) SGRA;
Char *GetNewStr(UInt8) SGRA;
void grLibLineFill(Graph_t *,UInt8 , UInt8 ,UInt8 ,UInt8, GraphType_t) SGRA;
void * grLibGetUserValueFromPenPoint(Graph_t *,Coord ,Coord ) SGRA;
void grLibSetUserValue(Graph_t *, UInt16 , void **) SGRA;
gErr grLibEraseGraphFrame( Graph_t *,PatternType_t) SGRA;
void drawThermoGraph(UInt8 ,UInt8 , UInt8 ,UInt8 , UInt8 , Boolean,Boolean ) SGRA;
// IndexedColorType RGB(UInt8 r, UInt8 g, UInt8 b) SGRA;
gErr grLibSetPieGraphParams(Graph_t *br,Coord,Coord,Coord, Boolean, PrintDataType_t) SGRA;
void grLibDrawCircle(Graph_t *br) SGRA;
void grLibDrawPieGraph(Graph_t *br, UInt8 group) SGRA;

#endif
