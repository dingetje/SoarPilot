 /* Important Copyright information.
 * Copyright (C) 2001. Venu M. \URL{http://homecreations.tripod.com}
 *
 * 
 * GraphLib source files (gr.c,app.c and grlib.h) are NOT FREE WARE. 
 * You cannot redistribute this code in any form  -original or modified. However
 * you have the rights to modify and use this code for compiling and using
 * in your application. The distribution rights rest with me.
 *
 * If you are commercial developer/organization deriving graph enabling
 * Advantage to your applications, please mail me before doing so. 
 *
 * By using the program and the library you understand and agree that you
 * will not hold me liable for ANY damage, or expect any support and warranty
 * and also understand that I am in no way responsible for consequences 
 * of using this code.
 *
 * Version 2.0
 *-------------
 * MAJOR UPGRADE (requires MathLib if using Pie graphs)
 * Pie graph Support
 *
 * Version 1.3 , September 2001.
 *
 * Documentation Generated at: \today
 * V 1.3 changes/fixes
 * 1.Graph erase function
 * 2.Fixed a bug in stack graphs
 * 3.Fixed overflow bugs in any graph (when input data >  UInt16)
 *
 * V 1.2 fixes being done
 * 1)SIMPLE_FRAME for drawing rectangles
 * 2)MemPtrFree for legend string
 * 3)MAX_DATA_POINTS is 20 (not 100 anymore)
 * 4)Graph_t initial allocation is no more static - dynamic
 *   allocation for greater effeciency and also to prevent
 *   stack overflows.
 * 5)fixed grLibPrintGraphTitle to print in middle of screen
 *   based on graph params rather than the windows display
 * (this would cause problems in multiple display of graphs
 *  in the same screen) 
 *
 * V 1.1 changes
 * 
 * 1) support for line fill
 * 2) cleanup of comments and code
 */

//@Include: manual.c
// Main Include file for SDK 3.5
#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <FloatMgr.h>

// Main Include file for SDK 1.0, SDK 2.0, SDK 3.0, SDK 3.1
//#include <Pilot.h>

//defined include files
#define __FILL_H__

//if you DO NOT WANT PIE GRAPHS, then uncomment the line below
//#define __NO_MATHLIB__

#include "soarUGraph.h"
// MFH This is added because I have my on cosine function with the Mathlib one commented out
#include "soarMath.h"
#include "soarUMap.h" // for RGB function

/** [UTILITY] Draws a rectangle on the screen based on coordinates.
* @param x,y,ex,ey take the same parameters as PalmOS rect data type
* @param r tells the function to fill the rectangles with a pattern
* specified by 'r'. See the grlib.h typedefs for the fill types.
* @return none
*/
gErr grLibDrawRectangle( UInt8 x, UInt8 y, UInt8 ex, UInt8 ey ,PatternType_t r)
{
    RectangleType rect;
// MFH Unused
//    UInt16 cornerDiam = 5;
    rect.topLeft.x = x; rect.topLeft.y = y;
    rect.extent.x = ex; rect.extent.y = ey;
    DrawRectangle(rect,r);

    return grLibNone;
}

/** [OPTIONAL] Draws a frame on the screen based on coordinates in graph struct.
* @return grLibNone
*/

gErr grLibDrawGraphFrame( Graph_t *br,PatternType_t r)
{
// MFH Unused
//    UInt16 cornerDiam = 5;
    DrawRectangle(br->rect,r);
    return grLibNone;
}

/** [OPTIONAL] Erases the drawn Graph
* @return grLibNone
*/

gErr grLibEraseGraphFrame( Graph_t *br,PatternType_t r)
{
// MFH Unused
//    UInt16 cornerDiam = 5;
    DrawRectangle(br->rect,r);
    return grLibNone;
}

/** [internal UTILITY] 
* @return none
*/
gErr DrawRectangle(RectangleType rect,PatternType_t r)
{
    switch(r)
    {
        case CRAZY:
            WinSetPattern(&crazy);
            WinFillRectangle(&rect,0);
            break;

        case SOLID:
// MFH This was causing incomptible pointer warning during compile
// MFH Didn't need to do it anymore once they were defined "const" properly in gr.h
//            WinSetPattern((CustomPatternType *)&solidb);
            WinSetPattern(&solidb);
            WinFillRectangle(&rect,0);
            break;

        case SOLID_1:
            WinSetPattern(&solid_1);
            WinFillRectangle(&rect,0);
            break;

        case SOLID_2:
            WinSetPattern(&solid_2);
            WinFillRectangle(&rect,0);
            break;

        case SOLID_3:
            WinSetPattern(&solid_3);
            WinFillRectangle(&rect,0);
            break;

        case ROUNDFRAME:
            WinDrawRectangleFrame(roundFrame,&rect);
            break;

    case SIMPLE_FRAME:
            WinDrawRectangleFrame(simpleFrame,&rect);
            break;

    case ERASE_FRAME:
            WinEraseRectangle(&rect,0);
            break;
            
        default:
            WinDrawRectangleFrame(popupFrame,&rect);
            break;
    }
    return grLibNone;
}


/** [MANDATORY]:This function draws the graph.
* After the graph data structure is filled with data and other necessary
* information, this function must be called to create and draw the actual 
* graph.
*<br>
* <br><b>NOTE</b>: This function only draws the graph alone (Bar,Stack or Line) but
* does not draw the grids, legends or grid text. Use the respective
* API's to enhance the graph.
*
*<b> Comments </b><br>
* For BAR and LINE graphs the Y axis is typically a value that falls between
* the X and Y axis max and min data values. But for STACKED BAR graphs
* the Y value is a "height" value, so that the sum of individual
* heights in a stacked bar will exceed the max scale. Keep this in
* mind when filling data for STACKED bars.
*
* <i>CAUTION</i>: For STACK BAR graphs, it is important to Input Data in a sequence
* i.e. for a fixed X axis, all data points must be input - and then for the
* next X axis point and so on. Random inputs will not work! Also if
* stacks go beyond the boundary limits, they will be silently discarded.
* input must ensure sanity of data.
*
* <i>CAUTION</i>: For LINE_FILL graphs, remember that the entire region below
* a line is "filled" with some color, so LINE_FILL graphs are typically
* drawn for just one category i.e. one group only. It does not usually make sense
* to show multiple groups in LINE_FILL as they will only overwrite
* each other!
*
* @param  br The pointer to the graph data
* @param  group The graph is printed for a specific group of data.
* To print data with multiple groups this function must be called 
* once for each group. This provides the flexibility of printing 
* specific groups and also allows "power point" type presentations.
* 
* @return void This function does not report errors.
*/
void grLibDrawGraph(Graph_t *br,UInt8 group)
{

    UInt8 idx;
    UInt8 cY,cX; //computed height of each graph
    UInt8 topX,topY; //variables used in final computation
    
    //rectangle(bar) left X coordinate, used to calculate the width
    //of a bar
    
    PatternType_t patt;

    //previous coordinates for the line graph
    UInt8 prevX,prevY;

    //Calculated current coordinates for the line graph
    UInt8 curX,curY;
    
    //compute widths for each graph
    if(br->numDataPts <= 0 ) //data input was faulty
      return;
      
    br->barWd = (br->rect.extent.x /br->numDataPts )/2;
    if(br->color)
    {
        WIN_SET_FORE_COLOR(group); patt = SOLID;
    } else 
        patt = group;

    switch(br->gtype)
    {
        case BAR:
        {
            //compute the 'Y' value by considering the "scaled" maximum
            //value and the actual data  'Y' value
            //similarly 'X' is scaled on the actual X value
            for(idx = 0; idx < br->numDataPts;idx++)
            {
                if(br->gpt[idx].group!=group) continue;
                cX = CX((br->gpt[idx].xValue - br->minX));
                cY = CY((br->gpt[idx].yValue - br->minY));
        
                //v 1.3 fix. In cases where cX - rest was less than 0
                //there would be no rectangle. Boundary check and fix.
                topX = (2*(cX + br->rect.topLeft.x) <  br->barWd )?
                        0:(cX - br->barWd/2 + br->rect.topLeft.x);
                
                topY = (COORD_Y(cY) < 0)? 0 :COORD_Y(cY);
        
                grLibDrawRectangle( topX,topY, br->barWd,   cY,patt);
                                

                //v 1.2 new feature
                //save the information in the data structure - this will
                //aid in figuring out if a graph point exists on a stylus tap
                br->gpt[idx].r.topLeft.x = cX - br->barWd/2 + br->rect.topLeft.x;
                br->gpt[idx].r.topLeft.y = COORD_Y(cY);
                br->gpt[idx].r.extent.x =br->barWd ;
                br->gpt[idx].r.extent.y = cY;
                
            }//end of for
        }
        break;
        case LINE:
        case LINE_FILL:
        case LINE_FILL_HASH:
        {
			  Boolean firsttme = true;
           prevX = br->rect.topLeft.x;
           prevY = COORD_Y(0);

			  for (idx = 0; idx < br->numDataPts;idx++)
			  {
					if (br->gpt[idx].group!=group) {
						continue;
					} else {
						cY = CY((br->gpt[idx].yValue - br->minY));
						prevY = COORD_Y(cY);
						break;
					}
				}

            for(idx = 0; idx < br->numDataPts;idx++)
            {
                if(br->gpt[idx].group!=group) continue;

                cX = CX((br->gpt[idx].xValue - br->minX));
                cY = CY((br->gpt[idx].yValue - br->minY));
                curX = cX+br->rect.topLeft.x;
                curY = COORD_Y(cY);
                
					 if (!firsttme) {
                	if(br->gtype == LINE) {
                 		// grLibDrawRectangle(curX,curY,2,2,group);
                  	WinDrawLine(prevX,prevY,curX,curY);
                	} else if (br->gtype == LINE_FILL || br->gtype == LINE_FILL_HASH) {
                  	grLibLineFill(br,prevX, prevY,curX,curY,br->gtype);
					 	}
					 } 
                
					 firsttme = false;
                prevX = curX;
                prevY = curY;
            }//end of for
        }
        break;
        case STACK:
        {
            for(idx = 0; idx < br->numDataPts;idx++)
            {
                if(br->gpt[idx].group!=group) continue;
                if(br->stackPrevX != br->gpt[idx].xValue)
                            br->stackPrevY = 0;

                //keep track of the last Y value because
                //each subsequent bar needs to sit on top
                //of the other
                br->stackPrevX = br->gpt[idx].xValue;
                
                //V 1.3 This was a bug. stackPrevY must
                //be the scaled Y value and not the value itself!
                //br->stackPrevY += br->gpt[idx].yValue;
                cX = CX((br->gpt[idx].xValue - br->minX));
                cY = CY((br->gpt[idx].yValue + br->minY));
                br->stackPrevY += cY;

                grLibDrawRectangle( cX - br->barWd/2 +br->rect.topLeft.x,
                                        COORD_Y(0) - br->stackPrevY,
                                        br->barWd,
                                        cY,patt);           
            
            }//end of for
       } //end of case       
        break;
      #ifndef __NO_MATHLIB__
      case PIE:
      {
            grLibDrawPieGraph(br,group);
      }
      break;
      #endif
// MFH Added to get rid of warning about "MAX_GRAPH_TYPE" not being handled
      default:
      break;
    } //end of switch
  } 

/** [OPTIONAL]:This function draws the grids for the graph and 
 * associated text.
 *
* Uses the coordinate and scale information from the graph
* structure and prints the horizontal and vertical grids (and the respective
* scaled values for the grid). The arguments to the function decide what 
* information is printed. See the programmer's manual for a diagrammatic
* description.
*
* Note that with the variable options it is possible to print each graph
* in a variety of combinations -see the header file for enumerated types
*
* @param  br the pointer to the graph data
* @param  gridOrient specifies which grid is printed (vertical,
* horizontal,none etc.)
* @param  gridTextOrient specifies which scaled textual information
* is printed (x axis, y axis, none etc.)
* @param  numberOfGrids specifies how many grids will be printed. 
* considering the limited width of a Handheld, it is recommended to keep
* legends about 3-5 characters, and a maximum of 3 columns per row.
* 
*
* @return none
*/
void grLibDrawGraphGrids(Graph_t *br,
                         grid_t gridOrient,
                         grid_t gridTextOrient,
                         UInt8 numberOfGrids)
{

    UInt8 idx,cY,cX;
    UInt32 yValue,xValue;

    if(br->gtype == PIE) 
        return;
        
    yValue = (br->scaleY - br->minY)/numberOfGrids; 
    xValue = (br->scaleX - br->minX)/numberOfGrids;
    if(br->color) WIN_SET_FORE_COLOR(49);

    for(idx = 1; idx <= numberOfGrids;idx++)
    {
        //compute the scaled coordinates for x and y
        cY = CY(yValue);
        cX = CX(xValue);

        //draw a line for each grid
        //horizontal (y-axis) grids
        if(gridOrient == YGRID || gridOrient == BGRID)
        WinDrawLine(br->rect.topLeft.x,
                                COORD_Y(idx*cY),
                                br->rect.extent.x+br->rect.topLeft.x,
                                COORD_Y(idx*cY));
        if(gridOrient == XGRID || gridOrient == BGRID)
        WinDrawLine(br->rect.topLeft.x+idx*cX,
                                COORD_Y(br->rect.extent.y),
                                br->rect.topLeft.x+idx*cX,
                                COORD_Y(0));

        //write the numerical info of scales on Y/X axis
        if(gridTextOrient == YTEXT || gridTextOrient == BTEXT)
            grLibDrawNumber( br->rect.topLeft.x - 5,
                                    COORD_Y(cY*idx),
                                    br->minY+(UInt32)idx*yValue);
        if(gridTextOrient == XTEXT || gridTextOrient == BTEXT)
            grLibDrawNumber( br->rect.topLeft.x+idx*cX - 2,
                                    COORD_Y(0)+5,
                                    br->minX+(UInt32)idx*xValue);
    }
}


/** [UTILITY]:Draws a 'Number' on a coordinate specified on the screen.
* This is a utility function that prints a numerical value
* on specified coordinates of the screen
* @param x,y coordinates
* @param value the number that will be 'Ascii' fied and printed
* @return none
*/
void grLibDrawNumber(UInt8 x,UInt8 y,UInt32 value)
{

    //v 1.2 changed from 10 byte allocation to 5 bytes.
	 // MFH Changed - Value of 5 was too small
    Char *s = GetNewStr(10*sizeof(Char));
    StrIToA(s,value);
    WinDrawChars(s,StrLen(s),x,y);
    MemPtrFree(s);
}

/** [UTILITY]:returns a Char * after allocating memory
* @param size = size in bytes of desired string
* @return Pointer to the Character string
*/
Char *GetNewStr(UInt8 size)
{
        return( (Char *)MemPtrNew(sizeof(Char)*size) );
}

/** [UTILITY]:returns a number between 0 and n 
* @param  n = number upper limit
* @return UInt8
*/
UInt8 GetRandomNumberForScreen(UInt32 n)
{
    return SysRandom(0)/(1 + sysRandomMax / n);
}


/** [OPTIONAL]:This function draws the legends below the graph. 
* Using values in the graph data structure, it automatically calculates 
* space available for printing legends and prints the information 
* in as many columns as specified in this function call. Note that the 
* legend information is based on the group id assigned to the data.
*
* @param  br pointer to the graph data structure
* @param numLegendColumns specifies the number of columns in which the legends
* are printed. The caller must decide what would be best for the print by 
* calculating the length of the strings. For legends with 4 to 5 characters
* a 3 column fits best. It is recommended that legend strings be limited to
* about 4 or 5 characters.
* @return none
*/
void grLibDrawGraphLegends(Graph_t *br,UInt8 numLegendColumns)
{
    UInt8 lY; //Legend Y coordinates
    UInt8 idx,padWidth,idxLegendCols;
    UInt16 cX;
    Char *str; 
    //do nothing if there is no space (need at least 10 pixel height)
    //if(( br->rect.topLeft.y+br->rect.extent.y) > PALM_MAX_SCR_Y - 10)
    if( COORD_Y(0) > PALM_MAX_SCR_Y - FntCharHeight())
        return;


    //avoid horizontal grid print   
    lY = COORD_Y(0)+FntCharHeight()+5; 
    
    //draw a line between the bar graph frame and the legends
    WinDrawLine(5,lY,157,lY);
    
    //make some space below the line before beginning to print legends
    lY+=2;cX = 1;idxLegendCols = 1;

    //padWidth is space between each legend print
    padWidth = PALM_MAX_SCR_X / numLegendColumns;
    str = (Char *)GetNewStr(MAX_LEGEND_LENGTH);

    if(br->color) WinSetTextColor(LEGEND_TEXT_COLOR);
    for(idx = 0; idx < br->numDataPts; idx++)
    {
        if(br->gpt[idx].info != NULL)
        {
            StrPrintF(str,"%s", br->gpt[idx].info);
            if(br->color) 
            {
                WIN_SET_FORE_COLOR(br->gpt[idx].group);
                grLibDrawRectangle(cX,lY,5,5,SOLID);
            }
            else
                grLibDrawRectangle(cX,lY,5,5,br->gpt[idx].group);

            WinDrawChars(str,StrLen(str),cX+6,lY);idxLegendCols++;
            cX +=padWidth;
            if(idxLegendCols > numLegendColumns)
            {
                lY += FntCharHeight()+1; cX = 1;idxLegendCols = 1;
            }
        } //end of if
    }//end of for
    MemPtrFree(str);
    return;
}

/** MFH - This function makes use of the graph structure to allow for the printing of text at specific
 * locations in the graph using the actual graph X,Y values rather than the window X,Y values.  In addition,
 * the xOffset and yOffset values allow you to place the text anywhere around the actual point. 
 */
void grLibDrawGraphText(Graph_t *br, UInt32 xValue, UInt32 yValue, Int8 xOffset, Int8 yOffset, Char *text)
{
   UInt8 cY,cX; //computed height of each graph
	UInt8 curX, curY; //Calculated current coordinates for the line graph

	cX = CX((xValue - br->minX));
	cY = CY((yValue - br->minY));
	curX = cX+br->rect.topLeft.x;
	curY = COORD_Y(cY);
                
	if(text != NULL) {
		WinDrawChars(text , StrLen(text), curX+xOffset, curY+yOffset);
	}
}

/** [MANDATORY]:fills data values to the graph data structure and ensures 
 * faulty data is not input. This function must be called once for every set
 * of data.
 * e.g.
 *
 *      if( grLibInputGraphData(br,30,2004,0,"Yahoo") != grLibNone) 
 *          return grLibInvData; 
 *
 *      if( grLibInputGraphData(br,40,2004,1,"ATT") != grLibNone) 
 *          return grLibInvData;
 *
 *      if( grLibInputGraphData(br,50,2001,2,"MSN") !=grLibNone) 
 *          return grLibInvData; 
 *
* @param  br pointer to the data structure
* @param xValue specifies the X axis value associated with the data
* @param yValue specifies the Y axis value associated with the data
* @param group Specifies a particular category the data point belongs to. 
* To understand the significance of group, see the programmer's manual.In the
* above example, notice that the group value changes and a new legend 
* is used -- so there are 3 bars, each with different color, and each
* specifying a different internet provider (yahoo,msn,att.)
* @param legend char. pointer to optional legend string, use NULL to leave empty
* @return gErr if inconsistent values are entered
*
*<b>Note</b><br>
*Graph Lib uses the parameter MAX_DATA_POINTS to do an initial
*dynamic allocation for the data point information stored in the
*graph data structure. If the points being input exceeds this
*then this function returns silently with the error code of
*grLibBeyondMaxBound
*/

gErr grLibInputGraphData(Graph_t *br,
                        UInt32 xValue, 
                        UInt32 yValue,
                        UInt16 group,
                        Char *legend)
{
    //error checks
    UInt16 idx = br->numDataPts;
    
    //don't allow beyond allocated boundaries. Would probably
    //be a good idea in next release (1.3 ?) to get rid of MAX_DATA_POINTS
    //and do all allocation dynamically.
    
    //do an exit
    if(br->numDataPts == MAX_DATA_POINTS)
      return grLibBeyondMaxBound;
    
    //for pie graphs we do not need max/min scales. Limited only
    //by variable sizes.
    if (br->gtype != PIE) 
        if((xValue > br->scaleX)|| (yValue > br->scaleY)||
              (yValue < 0 )|| (xValue < 0 ))
            return grLibInvData;
            
    if(( br->gtype != STACK) && (br->gtype != PIE))
        if((xValue < br->minX) || (yValue < br->minY))
                return grLibInvData;

    
    if(legend != NULL)
    {
        if(StrLen(legend) > MAX_LEGEND_STRING_LENGTH)
            return grLibInvData;
        br->gpt[idx].info=legend;
    }


    br->gpt[idx].xValue = xValue; br->gpt[idx].yValue = yValue;
    br->gpt[idx].group = group;
    br->totalValue+=xValue;

    ++br->numDataPts;
    return grLibNone;
}

/** [MANDATORY if not using default initialization values] sets desired graph 
 * data structure initialization values.
 * Call this function after doing a default initialization of the data struct.
 * This specifies desired graph outline (the outer frame for the graph), and 
 * the type of graph desired to be drawn and whether it must be drawn in color
 * or not.
* @param  br pointer to the data structure
* @param x topleft X
* @param y topleft Y
* @param ex the width of frame
* @param ey the height of frame
* @param gtype Graph Type.(BAR,LINE,STACK) 
* @param color whether color supported or not (1/0)
* @return gErr if inconsistent values are entered
*<br>
* [ <br><b>NOTE</b> ] : Not required for PIE graphs
*/
gErr grLibSetGraphParams(Graph_t *br,
                         Coord x,
                         Coord y,
                         Coord ex,
                         Coord ey,
                         GraphType_t gtype,
                         Boolean color)
{
    //error checks
    
    if ((x > PALM_MAX_SCR_X) || ( y > PALM_MAX_SCR_Y) ||
            (x < 0 ) || (y < 0 ))
        return grLibInvData;

    br->rect.topLeft.x = x; br->rect.topLeft.y = y;
    br->rect.extent.x = ex; br->rect.extent.y = ey;
    br->color = color;
    br->gtype = gtype;

    return grLibNone;
}

/**[MANDATORY if not using default initialization] sets the minimum and 
 * maximum X and Y scales.
 * While a graph's actual X and Y axis lengths are values less than
 * the screen's width and height, the actual values associated
 * with a graph could be anything (e.g. X-axis showing 1998 to 2005 and Y axis
 * showing 5000 to 10000). The scale values specify these values - and
 * must be determined before setting the values. 
 *
 * The Max scale values for each axis MUST be greater >= the highest numerical 
 * value in any data point of the graph for that axis.
 *
 * The Min scale values for each axis MUST be <= the lowest numerical 
 * value in any data point of the graph for that axis.
 *
* @param  br pointer to the data structure
* @param minX The minimum scale for X axis
* @param minY The minimum scale for Y axis
* @param maxX Minimum X axis scale 
* @param maxY Maximum Y axis scale
* @return gErr for incorrect values,grLibNone for no error.
*<br>
* [ <br><b>NOTE</b> ] not required for PIE graphs.
*/
gErr grLibSetMaxMinScales(Graph_t *br,
                          UInt32 minX,
                          UInt32 minY,
                          UInt32 maxX,
                          UInt32 maxY)
{
    if((maxX < 0)||(maxY < 0 ) || (minX < 0 ) || (minY < 0))
        return grLibInvData;

    if((maxX <= minX )||(maxY <= minY ))
        return grLibInvData;

    br->minX = minX;
    br->minY = minY;
    br->scaleX = maxX;
    br->scaleY = maxY;

    return grLibNone;
}
/**[MANDATORY]:default initialization. This will fill the graph data structure
 * with general default parameters that can be used directly. This would
 * typically be the first API to be called in the process of creating
 * graphs.
 *
 * e.g.
 *
 * GraphInfo_t gr;
 *
 * grLibInitializeGraphStruct(&gr);
 *<br>
 * <br><b>NOTE</b>:It is advisable to save the current pattern/color etc. attributes
 * before drawing a graph. Due to OS version API differences, the API's
 * do not save current attributes and restore on exit.
 *
* @param  br pointer to the data structure
* @return None
*/
void grLibInitializeGraphStruct(Graph_t *br)
{
    UInt32 idx;
// MFH Unused
//    static UInt8 gcount = 0;
    br->numDataPts = 0;
    br->scaleY = DEFAULT_SCALE_Y;
    br->scaleX = DEFAULT_SCALE_X;
    br->minX = 0;
    br->minY = 0;
    br->rect.topLeft.x = TOPLEFT_X;
    br->rect.topLeft.y = TOPLEFT_Y;
    br->rect.extent.x = EXTENT_X;
    br->rect.extent.y = EXTENT_Y;
    br->scaleY = EXTENT_Y;
    br->barWd = 2;

    br->grMemH = MemHandleNew(MAX_DATA_POINTS*(sizeof(dataPt_t)));
    br->gpt = (dataPt_t *)MemHandleLock(br->grMemH);
    MemSet(br->gpt,MAX_DATA_POINTS*(sizeof(dataPt_t)),0);

    for(idx = 0; idx < MAX_DATA_POINTS; idx++)
    {
        br->gpt[idx].group = -1;
        br->gpt[idx].info = NULL;
        br->gpt[idx].xValue = 0; br->gpt[idx].yValue = 0;
    }

    br->gtype = BAR;
    br->color = true;
	 br->stackPrevX = 0; 
    br->stackPrevY = 0;
    br->radius = 0;
    br->centerX = 0;
    br->centerY = 0;
    br->totalValue = 0;
    return;
}

/**[MANDATORY]:Releases memory resources used by graph data structure
 *<br>
 * <br><b>NOTE</b>: This must be called to "clean up the mess" once done with graph
 * and the user presses a button/menu etc. to return from screen.
* @param  br pointer to the data structure
* @return None
*/

gErr grLibReleaseResources(Graph_t *br)
{
  if(br->grMemH != NULL)
  {
    MemHandleUnlock(br->grMemH);
    MemHandleFree(br->grMemH);
    }
    return grLibNone;
}

/**[OPTIONAL]:Prints a graph title.
* Ensure that the title is less than the width of the graph
* frame, and also that it is valid and not null. All error
* conditions will result in the title not being printed.
* @return None
*/

void grLibDrawGraphTitle(Graph_t *br,const Char *title)
{
    Coord x,y;
	 FontID savedFont;

    if(( title == NULL) || (FntCharsWidth(title, StrLen(title) > br->rect.extent.x)))
    	return;
//	        (StrLen(title)*FntAverageCharWidth() > br->rect.extent.x))

//    x = br->rect.topLeft.x+(br->rect.extent.x - StrLen(title))/4;
    x = (((br->rect.extent.x - br->rect.topLeft.x)/2)+br->rect.topLeft.x) - (FntCharsWidth(title, StrLen(title))/2);
//    y = br->rect.topLeft.y+1;
    y = br->rect.topLeft.y-12;
    if(br->color)WinSetTextColor(RED);
	 savedFont = FntGetFont();
	 FntSetFont(boldFont);
    WinDrawChars(title, StrLen(title), x, y);
	 FntSetFont(savedFont);
}
/** [INTERNAL ]: Enhances a line graph by filling the region below the lines.
* 
* creates "mountain shades", is an ideal representation for 
* certain data where the region below the data points is shown
* in solid shade.
*
*/
void grLibLineFill(Graph_t *br,UInt8 x1, UInt8 y1,UInt8 x2,UInt8 y2,GraphType_t grtype)
{
//got to to figure out all the pixels between x and y
//using the line equation.
	float slope,newY;
	UInt8 idx,tmp;
  
	tmp = (x2 > x1 )?(x2-x1):(x1-x2);
	slope = (float)(y2-y1)/(float)(x2-x1);
	for(idx = 0;idx < tmp; idx++) {
		newY = slope*((idx+x1)-x2)+y2;
		if (grtype == LINE_FILL_HASH) {
			WinDrawGrayLine(idx+x1,newY,idx+x1,COORD_Y(0));
		} else {
			WinDrawLine(idx+x1,newY,idx+x1,COORD_Y(0));
		}
	}
}

 /**[OPTIONAL]:returns graph data for a point where the stylus taps
 * the screen (if a data point exists there)
 *<br>
 * <br><b>NOTE</b>: the return value must be typecast to the proper user data
 * as was initially input by the user.
 *
 * @param  br pointer to the data structure of the actual graph
 * @param  x,y coordinates where the stylus tapped the screen
 * @return None
 */

void * grLibGetUserValueFromPenPoint(Graph_t *br,Coord x,Coord y)
{
  UInt16 idx;
  for(idx = 0;idx < br->numDataPts; idx++)
  {
    if(RctPtInRectangle(x,y,&(br->gpt[idx].r)))
      return(br->gpt[idx].user);
  }
  return NULL;
}
   
   
 /**[OPTIONAL]:Sets the user defined information for a data point
 *
 *
 * @param  br pointer to the data structure of the actual graph
 * @param  idx the index within the graph data where the information needs
 * to be set in
 * @param  uptr any user defined data structure
 * @return None
 * 
 *<br>
 * <br><b>NOTE</b>: uptr can be a structure, integer or char * - anything as long
 * as the developer allocates necessary memory and sets the values.
 * this information can be "supplemental" or "additional" information
 * for a specific graph point. Graph Lib routines as such do not use this
 * value anywhere - this will be returned if a stylus tap on the screen
 * falls within this graph point plot. The developer must decide what to do
 * with this information. See app.c on how this is used.
*/
    
void grLibSetUserValue(Graph_t *br, UInt16 idx, void **uptr)
{
  br->gpt[idx].user = *uptr;
  return;
}

 /**[OPTIONAL]:Draws a "thermometer" type graph with color shade
 *
 *
 * @param  x,y,ex,ey coordinates of graph. Note that here the graph (a bar) is actually
 *         drawn from x,y to x+ex and x+ey. Keep ex or ey small for better effect.
 * @param  coljmp To obtain the "shade", the function draws each new line with a color
 *         index value 1 less or more than the previous. To obtain shades of any color
 *         pass an index for the color (See RGB() function usage)
 * @param  orient orientation - true for horizontal and false for vertical.
 *         the lines are drawn either horizontally or vertically, you could
 *         consider this as "grain" orientation. See demo in app.c for example.
 * @param  color whether shade is color or greyscale. At this version only color is supported
 * @return None
 *
 *<br>
 * <br><b>NOTE</b>: Thermo graph is not inherently a part of any actual graph and can be 
 * considered  as a library to display certain information graphically - an 
 * example is a progress  indicator, or a bar showing 2 activities. It is 
 * left to the user to ensure right coordinates  are passed after calculating 
 * the scales from the actual graph values. Maybe I will integrate
 * this to graph routines sometime in the future.
*/
void drawThermoGraph(UInt8 x,UInt8 y, UInt8 ex, 
                         UInt8 ey, UInt8 coljmp, Boolean orient,Boolean color)
{
  UInt8 idx = 0;
// MFH Changed to get rid of "might be used uninitialized" warning
  IndexedColorType colr=1;
// MFH Changed to get rid of "might be used uninitialized" warning
	if (color) colr=WinSetForeColor(coljmp);
  if(orient)
  {
    while( idx++ < ey)
    {
       if(color) colr = WinSetForeColor(coljmp);
       WinDrawLine(x,y+idx,x+ex,y+idx);
       if((ey - idx) > (idx<<2)) coljmp++;
       else coljmp--;
    }
  }
  else
  {
    while( idx++ < ex)
    {
       if(color) colr = WinSetForeColor(coljmp);
       WinDrawLine(x+idx,y,x+idx,y+ey);
       if((ex - idx) > (idx>>2)) coljmp++;
       else coljmp--;
    }
  }
  if (color) WinSetForeColor(colr);
}

 /* All the functions below pertain to those that use MathLib (Pie graph etc)
  * These are being grouped here to make it easy for any user
  * to simply "delete all these" and use graphlib without having to
  * force the use of MathLib.
  *
  */
 
 #ifndef __NO_MATHLIB__
 
 /** [MANDATORY if using Pie Graphs]: Set the basic parameters for Pie graphs.
  * @param br the pointer to the initialized graph structure
  * @param radius of the circle
  * @param centerX the center X coordinate of the circle
  * @param centerY the center Y coordinate of the circle
  * @param color whether the graph supports color or not
  * @param prn PrintDatatype_t can be NONE, PRINT_VALUES or 
  * PRINT_PERCENTAGES. This tells the graph plotter to print no values
  * by the side of the Pie, or print the actual data values, or print the
  * percentage representation of the graph.
  *<br>
  * <br><b>NOTE</b>: Pie graphs DO NOT USE grLibSetMaxMinScales or grLibSetGraphParams
  *
  */
  
  gErr grLibSetPieGraphParams(Graph_t *br,
                              Coord radius,
                              Coord centerX, 
                              Coord centerY,
                              Boolean color,
                              PrintDataType_t prn)
  {
        //note that centerX+radius > PALM_MAX_SCR_X is ignored so as 
        //to allow partial circle plots.
        if(   ( (radius > (PALM_MAX_SCR_X<<2)) )||
                (centerX > PALM_MAX_SCR_X) ||
                (centerY > PALM_MAX_SCR_Y) )
                return grLibInvData;
        
        br->radius = radius;
        br->centerX = centerX;
        br->centerY = centerY;        
        br->color = color;
        br->gtype = PIE;
        br->prn = prn;
        
        return grLibNone;
  }

 /**[INTERNAL]:Draws a circle with the given radius, center points
 *
 * @param br pointer to graph variable
 * @return none 
*/
 void grLibDrawCircle(Graph_t *br)
 {
    int idx = 0;
// MFH Unused
//    double rad = 0;
    Coord x,y;
    for ( idx = 0; idx < 360 ; idx+=PIE_SPACE)
    {
        x = (Coord)(br->radius* Sin(idx * PI/180.0));
        y = (Coord)(br->radius* Cos(idx * PI/180.0));       
	if (device.romVersion < SYS_VER_35) {
		WinDrawLine(x+br->centerX,y+br->centerY, x+br->centerX,y+br->centerY);
	} else {
		WinDrawPixel(x+br->centerX,y+br->centerY);
	}
    }
 }          
 
 /**[INTERNAL]:Draws the actual pie slices
 *
 * @param br pointer to graph variable
 * @param group the group of slice to be drawn
 * @return none
 * <br>
 *<br><b>NOTE</b>: This method may also be called directly
 * by an application provided the data is initialized
 * and ready.
*/
 
 void grLibDrawPieGraph(Graph_t *br, UInt8 group)
 {
    int idx = 0;
    double value = br->stackPrevX;
    UInt32 groupTotal = 0;
// MFH Changed to get rid of "might be used uninitialized" warning
    Coord dx=0,dy=0;
// MFH Unused
//    Boolean valuePrinted = false;
    UInt8 line_space;
      
    for(idx = 0; idx < br->numDataPts;idx++)
    {                
        //take the data point and calculate the overall percentage
        //in radians
        if(br->gpt[idx].group != group)
            continue;            
        groupTotal += br->gpt[idx].xValue  ;
        value += ((double)(br->gpt[idx].xValue*360))/br->totalValue;
    }        
    //draw the colored/patterned lines from the previous end to
    //the current point. Change the idx increment to space lines 
    //as you please and reduce processing overheads.
    if(br->color)
    {
        WIN_SET_TEXT_COLOR(group);
        
        //increase this value to space the filling
        //lines - greatly increases processing speed.
        //(e.g. 2 => 2 times faster, 3 => 3 tmes faster etc)
        line_space = 1;
    } else 
       line_space = value - br->stackPrevX - 1;
    for(idx = br->stackPrevX; idx < value ; idx+=line_space)
    {
        dx = br->centerX+(Coord)(br->radius* Sin(idx * 3.14/180));
        dy = br->centerY+(Coord)(br->radius* Cos(idx * 3.14/180));
        WinDrawLine(br->centerX,
                    br->centerY,
                    dx,dy);
    }
    
    //print based on quadrant of print - the position of the text has to change
    //slightly depending on the quadrant - or the text gets eaten by the slice.
    if(br->prn == PRINT_VALUES)
    {
       if(br->color)
       {
        if( (dx >= br->centerX) && (dy >= br->centerY))//Q4
            grLibDrawNumber(dx+15,dy,groupTotal);        
        else  if( (dx >= br->centerX) && (dy <= br->centerY)) //Q1
            grLibDrawNumber(dx+15,dy,groupTotal);        
        else  if( (dx <= br->centerX) && (dy >= br->centerY)) //Q2
            grLibDrawNumber(dx-15,dy,groupTotal);            
        else  if( (dx <= br->centerX) && (dy <= br->centerY))  //Q3
             grLibDrawNumber(dx-15,dy,groupTotal);            
        else
             grLibDrawNumber(dx,dy,groupTotal);            
        }
        else
             grLibDrawNumber(dx,dy,groupTotal); 
        
    }
    br->stackPrevX = value;
 }

#endif
