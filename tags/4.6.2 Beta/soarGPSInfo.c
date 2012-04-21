#include <PalmOS.h>  // all the system toolbox headers
#include "AddIncs/GarminIncs/GPSLib68K.h"
#include "soaring.h"
#include "soarGPSInfo.h"
#include "soarForm.h"
#include "soarMath.h" 
#include "soarUMap.h" 
#include "soarUtil.h" 
#include "soarIO.h"
#include "soarComp.h"

Boolean IsGPS = true; 		// controls display for GPS Status or Flarm traffic info
Boolean gotoGPSinfo = false;	// Override to force GPS Status display even if Flarm used
double MGCBattHours = -1.0;     // Magellen Battery status

extern GPSSatDataType *satData;
extern GPSPVTDataType *pvtData;
extern IndexedColorType  indexGreen, indexRed, indexBlack;
extern Boolean menuopen;
extern Boolean recv_data;
extern UInt32  no_read_count;
extern UInt32  origform;
extern Boolean Flarmpresent;
extern UInt8 FlarmRX;
extern Int16 FlarmVSep;
extern DateTimeType gpstime;
extern UInt32 gpssecs;

/*------------------------------------------------------
Grayscale color values
------------------------------------------------------*/
enum
	{
	grayScaleWhite      = 0,
	grayScaleLightGray,
	grayScaleDarkGray,
	grayScaleBlack
	};


/*----------------------------------------------------------
Color index
----------------------------------------------------------*/
typedef UInt8 colorIndexType; enum
	{
	backgroundColor,
	scaleColor,
	noEphColor,
	ephColor,
	ColorCount
	};

/*********************************************************************
*
*   PROCEDURE NAME:
*       DrawSatStatus - Draw the Satellite Status Form Graphics
*
*   DESCRIPTION:
*       Draws the Satellite Status Form's graphics.

*********************************************************************/
void DrawSatStatus(double gpsalt, double gpsgeoidaldiff, double MGCBattHours, Boolean cleanup)
{
	static WinHandle    gOffScreenWindow=NULL;      /* handle to offscreen drawing window   */
	static UInt32       gOldDepth;                  /* original display depth               */
	IndexedColorType    gColorTable[ ColorCount ];  /* colors for various elements          */
	UInt32              depth;                      /* color depth                          */
	UInt8               numDisplayableSats=GPSMAXSATS;
	/*----------------------------------------------------------
	Array of status string id's for PVT status. Must be in the
	same order as GPSFixType in GPSLib.h.
	----------------------------------------------------------*/
	static Int16 pvtStatusId[] =
	{
		lostSatRecepString,
		lostSatRecepString,
		d2FixString,
		d3FixString,
		d2DiffFixString,
		d3DiffFixString,
		validFixString,
		validFixString
	};
	Char                svidStr[ numDisplayableSats ][ 4 ]; // array of SVID strings
	Char                tempStr[ 32 ];                      // temporary string
	UInt8               elevDir[ numDisplayableSats ];      // elevation sort directory
																			  // array (smallest first)
	UInt8               svidDir[ numDisplayableSats ];      // SVID sort directory array 
																			  // (smallest first) 
	Coord               skyvwRadius;                        // skyview radius 
	static Coord	    trkX = 0;				// track x-coordinate value       
	static Coord	    trkY = 0;				// track y-coordinate value
	Coord               xDiff;                              // x-coordinate of differential 
																			  // indicator 
	Coord               xIn;                                // inner x spacing 
	Coord               xOut;                               // outer x spacing
	Coord      		 	  y;                                  // y-coordinate value 
	IndexedColorType    color=0;                            // drawing color 
	PointType           skyvwCenter;                        // skyview center 
	RectangleType       drawRect;                           // drawing window rectangle data
	RectangleType       sgnstrRect;                         // signal strength area 
	RectangleType       tempRect;                           // temporary rectangle for drawing 
	WinDrawOperation    drawMode;                           // drawing mode 
	WinHandle           oldWindow;                          // old window handle
	float               angleRadians;                       // angle in radians 
	float               snrToBar;                           // conversion factor from 
								// Signal-to-Noise Ratio to signal
								// strength bar height
	int                 i;                                  // loop index
	int                 j;                                  // another loop index 
	int                 sortLimit;                          // sort limit
	int                 temp;                               // temporary storage
	Int16               signalLevel;                        // signal level
	Int16               statusHeight;                       // status string height 
	UInt16              adjElev;                            // elevatiaon adjustment variable
	Boolean             changed;                            // changed during pass through the data?
	float               trkCenterX;
	float               trkCenterY;
	Err					  error;

	Coord               yExtra;                             // extra y spacing 
	Char                timeStr[ 20 ];                      // time string
	Char                dateStr[ 12 ];                      // date string 
	Char                posStr[ 20 ];                       // pos string 
	double              boldline = 1.0;

	double	mapscale = GetMapScale(data.config.Flarmscaleidx) / data.input.disconst;
	double poirange;

	if (cleanup) {
		/*----------------------------------------------------------
		Restore color settings.
		----------------------------------------------------------*/
		if (device.colorCapable) {
			WinScreenMode( winScreenModeSet, NULL, NULL, &gOldDepth, NULL );
		}
		/*----------------------------------------------------------
		Eliminate offscreen buffer
		----------------------------------------------------------*/
		WinDeleteWindow( gOffScreenWindow, false );
		gOffScreenWindow = NULL;
		return;

	} else if (menuopen) {
		return;

	} else {

/*		if (!IsGPS) {
			// Flarm test data
			MemSet(satData, GPSMAXSATS * sizeof(GPSSatDataType), 0xff);

			FlarmRX = 5;

			// svid 	: 0 = obstacle, 1 = aircraft below, 2 = aircraft level, 3 = aircraft above
			// status	: 0 = normal alarm, 1 = urgent alarm
			// snr		: -1 = traffic, 1 = alarm
			// azimuth	: relative bearing
			// elevation	: distance in m

			// Data from $PFLAU
			satData[0].svid = 2;
			satData[0].status = 1;
			satData[0].snr = 1;
			satData[0].azimuth = 330 * degToRad;
			satData[0].elevation = 100;

			// Data from $PFLAA
			satData[1].svid = 0;
			satData[1].status = 1;
			satData[1].snr = -1;
			satData[1].azimuth = 60 * degToRad;
			satData[1].elevation = 330;

			satData[2].svid = 1;
			satData[2].status = 0;
			satData[2].snr = 1;
			satData[2].azimuth = 200 * degToRad;
			satData[2].elevation = 750.0;

			satData[3].svid = 2;
			satData[3].status = 0;
			satData[3].snr = 1;
			satData[3].azimuth = 350 * degToRad;
			satData[3].elevation = 350.0;

			satData[4].svid = 3;
			satData[4].status = 0;
			satData[4].snr = 1;
			satData[4].azimuth = 310 * degToRad;
			satData[4].elevation = 250.0;

			satData[5].svid = 1;
			satData[5].status = 0;
			satData[5].snr = -1;
			satData[5].azimuth = 145 * degToRad;
			satData[5].elevation = 400.0;
		}
*/
		/*----------------------------------------------------------
		Set Window title
		----------------------------------------------------------*/
		if (!IsGPS) {
			frm_set_title("Flarm Traffic");
		} else if (MGCBattHours == -1.0) {
			frm_set_title("GPS Status");
		} else if (MGCBattHours > -1.0) {
			//----------------------------------------------------------
			//If using a Magellan Companion, Draw The Hours Left
			//on the units battery.
			//----------------------------------------------------------
			StrCopy(tempStr, "GPS Status - Batt:");
			StrCat(tempStr, DblToStr(pround(MGCBattHours,1), 1));
			StrCat(tempStr, "hrs");
			frm_set_title(tempStr);
		}

		/*----------------------------------------------------------
		Capture color settings, then set new
		----------------------------------------------------------*/
		if (device.colorCapable) {
			WinScreenMode( winScreenModeGet, NULL, NULL, &gOldDepth, NULL );
			depth = ( gOldDepth > monoColorDepth ) ? colorColorDepth : monoColorDepth;
			WinScreenMode( winScreenModeSet, NULL, NULL, &depth, NULL );
		} else {
			depth = monoColorDepth;
		}

		/*----------------------------------------------------------
		Initialize colors
		----------------------------------------------------------*/
		if ( gOldDepth < monoColorDepth ) {
			gColorTable[ backgroundColor ] = grayScaleWhite;
			gColorTable[ scaleColor      ] = grayScaleDarkGray;
			gColorTable[ noEphColor      ] = grayScaleLightGray;
			gColorTable[ ephColor        ] = grayScaleBlack;
		} else {
			gColorTable[ backgroundColor ] = UIColorGetTableEntryIndex( UIFormFill );
			// Scale Lines and Circle Color
//			gColorTable[ scaleColor      ] = UIColorGetTableEntryIndex( UIObjectFrame );
			gColorTable[ scaleColor      ] = indexBlack;
			gColorTable[ noEphColor      ] = 19; // kind of a light blue 
//			gColorTable[ ephColor        ] = UIColorGetTableEntryIndex( UIObjectSelectedFill );
			gColorTable[ ephColor        ] = indexBlack;
		}

		/*----------------------------------------------------------
		Create offscreen buffer
		----------------------------------------------------------*/
		if (!gOffScreenWindow) {
			gOffScreenWindow = WinCreateOffscreenWindow( 160, 160 - formTitleHeight, nativeFormat, &error );
			ErrFatalDisplayIf( error, "Can't create offscreen buffer" );
		}

		/*----------------------------------------------------------
		Initialize
		----------------------------------------------------------*/
		for ( i = 0; i < numDisplayableSats; ++i )
			{
			svidDir[ i ] = i;
			elevDir[ i ] = i;
			}

		/*----------------------------------------------------------
		Drawing setup
		----------------------------------------------------------*/
		oldWindow = WinGetDrawWindow();
		WinSetDrawWindow( gOffScreenWindow );
		WinEraseWindow();
		WinGetWindowFrameRect( gOffScreenWindow, &drawRect );

		/*----------------------------------------------------------
		Fill the SVID and elevation directory arrays so that the
		indexes are sorted by the SVID and elevation with the
		smallest first. Until there are no more changes left, move
		the index of the largest value to the last element, then
		look only at the rest of the data.
		----------------------------------------------------------*/
		sortLimit = numDisplayableSats;
		do {
			changed = false;
			for ( i = 1; i < sortLimit; ++i )
			{
				if ( satData[ svidDir[ i ] ].svid < satData[ svidDir[ i - 1 ] ].svid )
				{
					changed          = true;
					temp             = svidDir[ i ];
					svidDir[ i ]     = svidDir[ i - 1 ];
					svidDir[ i - 1 ] = temp;
				}
			}
			--sortLimit;
		} while ( changed );

		// if displaying Flarm data, 0th item is always the most important, so display last
		sortLimit = numDisplayableSats;
		do {
			changed = false;
			for ( i = (IsGPS?1:2); i < sortLimit; ++i )
			{
				if ( satData[ elevDir[ i ] ].elevation > satData[ elevDir[ i - 1 ] ].elevation )
				{
					changed          = true;
					temp             = elevDir[ i ];
					elevDir[ i ]     = elevDir[ i - 1 ];
					elevDir[ i - 1 ] = temp;
				}
			}
			--sortLimit;
		} while ( changed );

		/*----------------------------------------------------------
		Build the svid strings
		----------------------------------------------------------*/
		for ( i = 0; i < numDisplayableSats; ++i )
		{
			if (IsGPS) {
				/*------------------------------------------------------
				Invalid id, fill with dashes
				------------------------------------------------------*/
				if ( satData[ i ].svid == gpsInvalidSVID )
				{
					StrCopy( svidStr[ i ], "--" );
				}
				/*------------------------------------------------------
				Single digit, add leading zero
				------------------------------------------------------*/
				else if ( satData[ i ].svid < 10 )
				{
					svidStr[ i ][ 0 ] = '0';
					StrIToA( &svidStr[ i ][ 1 ] , satData[ i ].svid );
				}
				/*------------------------------------------------------
				Just convert
				------------------------------------------------------*/
				else
				{
					StrIToA( svidStr[ i ], satData[ i ].svid );
				}
			} else {
				/*------------------------------------------------------
				Set Flarm labels
				------------------------------------------------------*/
				if (satData[ i ].svid == 0) {
					// obstacle
					svidStr[ i ][0] = 10;
					svidStr[ i ][1] = 0;	
				} else if ((satData[ i ].svid & 3) == 1) {
					// aircraft - below
					if (satData[ i ].svid > 3) {
						svidStr[ i ][0] = 34;
					} else {
						svidStr[ i ][0] = 58;
					}
					svidStr[ i ][1] = 0;
				} else if ((satData[ i ].svid  & 3) == 3) {
					// aircraft - above
					if (satData[ i ].svid > 3) {
						svidStr[ i ][0] = 33;
					} else {
						svidStr[ i ][0] = 59;
					}
					svidStr[ i ][1] = 0;
				} else {
					// aircraft - level
					if (satData[ i ].svid > 3) {
						svidStr[ i ][0] = 32;
					} else {
						svidStr[ i ][0] = 31;
					}
					svidStr[ i ][1] = 0;
				}
			}
		}

		/*----------------------------------------------------------
		Draw the status string
		----------------------------------------------------------*/
		if (IsGPS) {
			FntSetFont( boldFont );
			SysCopyStringResource( tempStr, pvtStatusId[ pvtData->status.fix ] );
			if (data.input.siu  > 0) {
				StrCat(tempStr, "-SIU:");
				StrCat(tempStr, data.input.gpsnumsats);
			}
		} else {
			FntSetFont( stdFont );
			StrCopy(tempStr, "Receiving : ");
			StrCat(tempStr, DblToStr(FlarmRX,0) );
		}
		DrawCenteredString( tempStr, drawRect.topLeft.x, ( drawRect.topLeft.x + drawRect.extent.x ), drawRect.topLeft.y-2 );
		statusHeight = FntCharHeight();
		FntSetFont( stdFont );

		/*----------------------------------------------------------
		Draw skyview field.  Azimuth is in radians from north
		(0-2pi) and elevation is in radians above the horizon
		(0-pi/2).
		----------------------------------------------------------*/
		SysCopyStringResource( tempStr, neswString );

		/*----------------------------------------------------------
		Determine the radius and center of skyview.
		----------------------------------------------------------*/
		if (IsGPS) {
			skyvwRadius = drawRect.extent.x / ( 100.0 / skyvwRadPercent );
		} else {
			skyvwRadius = drawRect.extent.x / ( 100.0 / flarmvwRadPercent );
		}
		skyvwCenter.x = skyvwRadius + ( FntCharWidth( tempStr[ 3 ] ) / 2 ) + 1;
		if (!IsGPS) skyvwCenter.x += 15*boldline;
		skyvwCenter.y = skyvwRadius + statusHeight + ( FntCharHeight() / 2 ) + 1;

		/*----------------------------------------------------------
		Draw the skyview circles.
		----------------------------------------------------------*/
		// check for bold line
		if (device.HiDensityScrPresent) {
			WinSetCoordinateSystem(device.HiDensityScrPresent);
			boldline = SCREEN.SRES;
		}
		WinDrawCircle((Int32)skyvwCenter.x*boldline, (Int32)skyvwCenter.y*boldline, 
						  (Int32)skyvwRadius*boldline , SOLID);
		WinDrawCircle((Int32)skyvwCenter.x*boldline, (Int32)skyvwCenter.y*boldline, 
						  ((Int32)(skyvwRadius/2))*boldline, SOLID);
		WinDrawCircle((Int32)skyvwCenter.x*boldline, (Int32)skyvwCenter.y*boldline, 
						  1*boldline , SOLID);
		if (device.HiDensityScrPresent) {
			WinSetCoordinateSystem(kCoordinatesStandard);
			boldline = 1.0;
		}

		/*---------------------------------------------------------
		Draw direction characters
		----------------------------------------------------------*/
		if (device.colorCapable) {
			WinPushDrawState();
			WinSetTextColor( gColorTable[ scaleColor ] );
		}
		tempRect.extent.y = FntCharHeight();
		for ( i = 0; i < 4; ++i )
		{
			/*------------------------------------------------------
			Find size of character
			------------------------------------------------------*/
			tempRect.extent.x = FntCharWidth( tempStr[ i ] );
	
			/*------------------------------------------------------
			Calculate x,y location
			------------------------------------------------------*/
			if (IsGPS) {
				angleRadians = i * 90 * degToRad;
			} else {
				angleRadians = i * 90 * degToRad - data.input.true_track.value * degToRad;
			}
			tempRect.topLeft.x = (Coord)(skyvwCenter.x + ( skyvwRadius * Sin( angleRadians ) ) - ( tempRect.extent.x / 2 ));
			tempRect.topLeft.y = (Coord)(skyvwCenter.y - ( skyvwRadius * Cos( angleRadians ) ) - ( tempRect.extent.y / 2 ));

			/*------------------------------------------------------
			Clear a space and draw the character
			------------------------------------------------------*/
			WinEraseRectangle( &tempRect, 0 );
			WinDrawChars( Mid(tempStr, 1, i+1), 1, tempRect.topLeft.x + 1, tempRect.topLeft.y - 1 );
		}

		if (!IsGPS) {
			/*------------------------------------------------------
			Draw Glider Symbol in Center of Screen
			------------------------------------------------------*/
			FntSetFont((FontID)WAYSYMB11);
			tempStr[0] = 35;
			tempStr[1] = 0;
			WinDrawChars(tempStr, 1, skyvwCenter.x-FntCharWidth(tempStr[1])/2, skyvwCenter.y-FntCharWidth(tempStr[1])/2);
			FntSetFont( stdFont );
		}

		/*----------------------------------------------------------
		Calculate x, y location - add 0.5 so value will round to 
		nearest whole number.
		----------------------------------------------------------*/
		if (IsGPS) {
			trkCenterX = skyvwRadius * Sin(DegreesToRadians(data.input.true_track.value)) + 0.5;
			trkCenterY = skyvwRadius * Cos(DegreesToRadians(data.input.true_track.value)) + 0.5;
		} else {
			trkCenterX = 0.0;
			trkCenterY = skyvwRadius;
		}
		/*----------------------------------------------------------
		Draw track indicator - truncate the x, y values
		----------------------------------------------------------*/
		if (IsGPS) {
			trkX = skyvwCenter.x + ( Int16 ) trkCenterX;
			trkY = skyvwCenter.y - ( Int16 ) trkCenterY;
			if (device.colorCapable) {
				WinSetForeColor( gColorTable[ scaleColor ] );
			}
	
			tempRect.topLeft.x = trkX - trkRadius;
			tempRect.topLeft.y = trkY - trkRadius;
			tempRect.extent.x = 2 * trkRadius;
			tempRect.extent.y = 2 * trkRadius;

			WinDrawRectangleFrame( roundFrame, &tempRect );
		}

		/*----------------------------------------------------------
		Restore current drawing state
		----------------------------------------------------------*/	
		if (device.colorCapable) {
			WinPopDrawState();
		}

		/*----------------------------------------------------------
		Draw Direction Line for Flarm Alarm
		----------------------------------------------------------*/
		if (!IsGPS && (satData[0].svid < gpsInvalidSVID)) {
			trkX = skyvwRadius * Sin(satData[0].azimuth) + 0.5;
			trkY = skyvwRadius * Cos(satData[0].azimuth) + 0.5;
			WinDrawLine(skyvwCenter.x*boldline, skyvwCenter.y*boldline,
				(skyvwCenter.x+trkX)*boldline, (skyvwCenter.y-trkY)*boldline);
		}

		/*----------------------------------------------------------
		Draw svids sorted by elevation
		----------------------------------------------------------*/

		/*----------------------------------------------------------
		Save current drawing state
		----------------------------------------------------------*/
		if (device.colorCapable) {
			WinPushDrawState();
		}

		/*----------------------------------------------------------
		Set up size of rectangles
		----------------------------------------------------------*/
		tempRect.extent.y = FntBaseLine() - FntDescenderHeight() + 3;
		tempRect.extent.x = FntCharsWidth( "00", 2 ) + 2;

		if (!IsGPS) FntSetFont((FontID)WAYSYMB11);

		if ((IsGPS && (data.config.flightcomp != FLARMCOMP)) || !IsGPS)
		//for ( i = 0; i < numDisplayableSats; ++i )
		// if displaying Flarm data, 0th item is always the most important, so display last
		for ( i = numDisplayableSats-1; i >= 0; i-- )
		{
			/*------------------------------------------------------
			Sort by elevation
			------------------------------------------------------*/
			j = elevDir[ i ];
	
			if ( satData[ j ].svid < gpsInvalidSVID )
			{
				/*--------------------------------------------------
				Determine location of characters
				--------------------------------------------------*/
				tempRect.topLeft.x  = skyvwCenter.x - ( tempRect.extent.x / 2 );
				tempRect.topLeft.y  = skyvwCenter.y - ( tempRect.extent.y / 2 );
				if (IsGPS) {
					adjElev = (UInt16)(((0.5 * PI) - satData[j].elevation ) * ( skyvwRadius / (0.5 * PI)));	
				} else {
					adjElev = (UInt16)( satData[j].elevation / (mapscale * DISTKMCONST * 1000)  *  skyvwRadius ) ;
					if (adjElev > skyvwRadius) adjElev = skyvwRadius;
				}
				tempRect.topLeft.x += (Coord)(adjElev * Sin( satData[ j ].azimuth ));
				tempRect.topLeft.y -= (Coord)(adjElev * Cos( satData[ j ].azimuth ));
	
				/*--------------------------------------------------
				If PalmOS version 3.5 or above and Color Capable device,
				set colors and draw mode:
				1. black on white if no signal
				2. black on lt gray if no ephemeris
				3. white on black if ephemeris

				If less than PalmOS version 3.5 or not color capable,
				1. normal characters if no signal (black on white)
				2. normal character if no ephemeris (black on white)
				3. inverted character if ephemeris (white on black)
				--------------------------------------------------*/
				if (device.colorCapable) {
					if ( satData[ j ].snr < 0 ) {
						color    = gColorTable[ backgroundColor ];
						drawMode = winOverlay;
					} else if ( ( satData[ j ].status & gpsSatEphMask ) != satEph ) {
						color    = gColorTable[ noEphColor ];
						drawMode = winOverlay;
					} else {
						color    = gColorTable[ ephColor ];
						drawMode = winMask;
					}

					/*--------------------------------------------------
					Draw rectangle and svid text
					--------------------------------------------------*/
					WinSetForeColor( color );
					WinDrawRectangle( &tempRect, 0 );

					WinSetDrawMode( drawMode );
					WinPaintChars( svidStr[ j ], StrLen( svidStr[ j ] ), tempRect.topLeft.x + 1, tempRect.topLeft.y - 1 );
				} else {
					if ( satData[ j ].snr < 0 ) {
						WinDrawChars( svidStr[ j ], StrLen( svidStr[ j ] ), tempRect.topLeft.x + 1, tempRect.topLeft.y - 1 );
					} else if ( ( satData[ j ].status & gpsSatEphMask ) != satEph ) {
						WinDrawChars( svidStr[ j ], StrLen( svidStr[ j ] ),  tempRect.topLeft.x + 1, tempRect.topLeft.y - 1 );
					} else {
						WinDrawInvertedChars( svidStr[ j ], StrLen( svidStr[ j ] ), tempRect.topLeft.x + 1, tempRect.topLeft.y - 1 );
					}
				}
			} /* if SVID is valid */
		} /* for i */

		/*----------------------------------------------------------
		Restore current drawing state
		----------------------------------------------------------*/
		if (device.colorCapable) {
			WinPopDrawState();
		}
		FntSetFont( stdFont );

		/*----------------------------------------------------------
		Draw signal strength bars
		----------------------------------------------------------*/
		if (IsGPS) {

			/*----------------------------------------------------------
			Get the differential character
			----------------------------------------------------------*/
			SysCopyStringResource( tempStr, diffCharString );

			/*----------------------------------------------------------
			Determine height of signal strength area and conversion
			factor from SNR to bar height.
			----------------------------------------------------------*/
			sgnstrRect.topLeft.y = skyvwCenter.y + skyvwRadius + ( FntCharHeight() / 2 ) + 1;
			sgnstrRect.extent.y  = drawRect.extent.y - sgnstrRect.topLeft.y - FntBaseLine() + 1;
			snrToBar = ( ( float ) sgnstrRect.extent.y ) / ( ( float ) snrRng );

			/*----------------------------------------------------------
			Draw the scale lines (10, 30, 50, 70, 90)
			----------------------------------------------------------*/
			if (device.colorCapable) {
				WinPushDrawState();
				WinSetForeColor( gColorTable[ scaleColor ] );
			}
			for ( i = 1; i < 10; i += 2 )
			{
				y = (Coord)(sgnstrRect.topLeft.y + sgnstrRect.extent.y - ( round( i * snrRng / 10 ) * snrToBar ));
				WinDrawLine( drawRect.topLeft.x, y, drawRect.topLeft.x + drawRect.extent.x, y );
			}
			if (device.colorCapable) {
				WinPopDrawState();
			}

			/*----------------------------------------------------------
			Width of signal strength bar is based on the width of the
			SVID string minus the rectangle border.
			----------------------------------------------------------*/
			tempRect.extent.x -= ( 2 * rectBorderWidth );

			/*----------------------------------------------------------
			Determine the spacing of the bars, both between bars (inner)
			and at the edges (outer).
			----------------------------------------------------------*/
			xIn  = ( tempRect.extent.x + ( 2 * rectBorderWidth ) ) * numDisplayableSats;
			xIn  = ( drawRect.extent.x - xIn ) / ( numDisplayableSats + 1 );
			xOut = ( drawRect.extent.x - ( ( tempRect.extent.x + ( 2 * rectBorderWidth ) + xIn ) * numDisplayableSats ) + xIn ) / 2;

			/*----------------------------------------------------------
			Draw the SVID string & signal strength bars sorted by SVID
			----------------------------------------------------------*/
			
			for ( i = 0; i < numDisplayableSats ; ++i )
			{
				/*------------------------------------------------------
				Sort by SVID
				------------------------------------------------------*/
				j = svidDir[ i ];

				/*------------------------------------------------------
				Draw SVID string
				------------------------------------------------------*/
				if (data.config.flightcomp != FLARMCOMP) {
					DrawString
					( svidStr[ j ]
					, ( xOut + ( i * ( tempRect.extent.x + ( 2 * rectBorderWidth ) + xIn ) ) )
					, drawRect.extent.y - FntBaseLine()
					);
				} else {
					DrawString
					( "--"
					, ( xOut + ( i * ( tempRect.extent.x + ( 2 * rectBorderWidth ) + xIn ) ) )
					, drawRect.extent.y - FntBaseLine()
					);
				}

				/*------------------------------------------------------
				Draw bar if channel is being used
				------------------------------------------------------*/
				if ((satData[ j ].svid < gpsInvalidSVID) && (data.config.flightcomp != FLARMCOMP))
				{
					/*--------------------------------------------------
					Determine height of bar.  Compensate for the border
					being drawn outside the rectangle.
					--------------------------------------------------*/
					signalLevel       = limit( snrMin, max( 0, satData[ j ].snr ), snrMax ) - snrMin;
					tempRect.extent.y = ( ( Coord ) round( signalLevel * snrToBar ) ) + 1 - ( 2 * rectBorderWidth );
					tempRect.extent.y = max( 0, tempRect.extent.y );
					tempRect.topLeft.x = xOut + ( i * ( tempRect.extent.x + ( 2 * rectBorderWidth ) + xIn ) ) + 1;
					tempRect.topLeft.y = sgnstrRect.topLeft.y + sgnstrRect.extent.y - tempRect.extent.y - rectBorderWidth;

					/*--------------------------------------------------
					Set colors:
					1. black frame with lt gray fill if no ephemeris
					2. black frame with black fill if ephemeris
					--------------------------------------------------*/
					if (device.colorCapable) {
						if ( ( satData[ j ].status & gpsSatEphMask ) != satEph ) {
							WinSetForeColor( gColorTable[ noEphColor ] );
						}
					}

					/*--------------------------------------------------
					Draw fill rectangle and frame
					--------------------------------------------------*/
					WinDrawRectangle( &tempRect, 0 );
					if (device.colorCapable) {
						WinSetForeColor( gColorTable[ ephColor ] );
					}
					WinDrawRectangleFrame( simpleFrame, &tempRect );

					/*--------------------------------------------------
					Draw differential indicator if necessary
					--------------------------------------------------*/
					if ( ( satData[ j ].status & gpsSatDifMask ) == satDif )
					{
						/*----------------------------------------------
						Determine x-coordinate
						----------------------------------------------*/
						xDiff = tempRect.topLeft.x + ( ( tempRect.extent.x - FntCharWidth( 'D' ) + 1 ) / 2 );
	
						/*----------------------------------------------
						Put indicator inside or outside the bar based on
						the height of the bar.
						----------------------------------------------*/
						if ( tempRect.extent.y < FntCharHeight() + ( 2 * rectBorderWidth ) )
						{
							/*------------------------------------------
							Outside: Black on white; also draw a white
							line on the left side of the character.
							------------------------------------------*/
							y = sgnstrRect.topLeft.y + sgnstrRect.extent.y - ( 2 * FntCharHeight() ) - rectBorderWidth - 3;
							WinDrawChars( tempStr, 1, xDiff, y );
							if (device.colorCapable) {
								color = WinSetForeColor( gColorTable[ backgroundColor ] );
							}
							WinDrawLine( xDiff - 1, y, xDiff - 1, y + FntCharHeight() - 1 );
							if (device.colorCapable) {
								WinSetForeColor( color );
							}
						} else	{
							/*------------------------------------------
							Inside:
							1. black on lt gray if no ephemeris
							2. white on black if ephemeris
							------------------------------------------*/
							if (device.colorCapable) {
								drawMode = ( ( satData[ j ].status & gpsSatEphMask ) != satEph ) ? winOverlay : winMask;
								WinSetDrawMode( drawMode );
								WinPaintChars( tempStr, 1, xDiff, tempRect.topLeft.y + tempRect.extent.y - FntCharHeight() - 1 );
							} else {
								WinDrawChars( tempStr, 1, xDiff, tempRect.topLeft.y + tempRect.extent.y - FntCharHeight() - 1 );
							}
						}
					} /* if differential */
				} /* if channel used */
			} /* for i */
		} /* if IsGPS */

		if (IsGPS && (pvtData->status.fix > gpsFixInvalid)  && (StrCompare(data.logger.gpsstat, "A") == 0)) {
			//----------------------------------------------------------
			//Draw connect button if required
			//----------------------------------------------------------
			ctl_set_visible(form_sat_status_restart, ((!recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)) && IsGPS));

			//----------------------------------------------------------
			//Draw text fields
			//----------------------------------------------------------
			tempRect.topLeft.x = drawRect.extent.x / 2 - 5;
			tempRect.topLeft.y = statusHeight-2;
			tempRect.extent.x  = tempRect.topLeft.x;
			tempRect.extent.y  = ( 2 * skyvwRadius ) + FntCharHeight();

			//----------------------------------------------------------
			//Calculate the extra space between groupings
			//----------------------------------------------------------
			yExtra = ( tempRect.extent.y - ( 5 * FntLineHeight() ) - FntCharHeight() ) / 2;

			//----------------------------------------------------------
			//Draw location
			//----------------------------------------------------------
			y = tempRect.topLeft.y;
			if (data.config.llfmt == LLUTM) {
				LLToStringUTM(data.input.gpslatdbl, data.input.gpslatdbl, tempStr, ZONE);
				StrCopy(posStr, tempStr);
				StrCat(posStr, "   ");
				LLToStringUTM(data.input.gpslatdbl, data.input.gpslatdbl, tempStr, EASTING);
				StrCat(posStr, tempStr);
				DrawString( posStr, tempRect.topLeft.x, y );

				y += FntLineHeight();
				StrCopy(posStr, "            ");
				LLToStringUTM(data.input.gpslatdbl, data.input.gpslatdbl, tempStr, NORTHING);
				StrCat(posStr, tempStr);
				DrawString( posStr, tempRect.topLeft.x, y );
			} else {
				StrCopy(posStr, "Lat: ");
				if (data.config.llfmt == LLDM) {
					LLToStringDM(data.input.gpslatdbl, tempStr, ISLAT, true, true, 3);
				} else {
					LLToStringDMS(data.input.gpslatdbl, tempStr, ISLAT);
				}
				StrCat(posStr, tempStr);
				DrawString( posStr, tempRect.topLeft.x, y );

				y += FntLineHeight();
				StrCopy(posStr, "Lon: ");
				if (data.config.llfmt == LLDM) {
					LLToStringDM(data.input.gpslngdbl, tempStr, ISLON, true, true, 3);
				} else {
					LLToStringDMS(data.input.gpslngdbl, tempStr, ISLON);
				}
				StrCat(posStr, tempStr);
				DrawString( posStr, tempRect.topLeft.x, y );
			}
			
			//----------------------------------------------------------
			//Draw date (UTC + Time Zone)
			//----------------------------------------------------------
//			y += FntLineHeight() + yExtra;
			y += FntLineHeight();
			StrCopy(dateStr, "Date: ");
			DateToAscii(gpstime.month, gpstime.day, gpstime.year, dfDMYLong, tempStr);
			StrCat(dateStr, tempStr);
			DrawString( dateStr, tempRect.topLeft.x, y );

			//----------------------------------------------------------
			//Draw time (UTC + Time Zone)
			//----------------------------------------------------------
			y += FntLineHeight();
			StrCopy(timeStr, "Time: ");
			SecondsToDateOrTimeString(gpssecs, tempStr, 1, 0);
			StrCat(timeStr, tempStr);
			StrCat(timeStr, "  ");
			if (data.config.timezone > 0) StrCat(timeStr, "+");
			StrCat(timeStr, DblToStr(data.config.timezone, 0));
			DrawString( timeStr, tempRect.topLeft.x, y );

			//----------------------------------------------------------
			//Draw Course
			//----------------------------------------------------------
			y += FntLineHeight();
			StrCopy(tempStr, "Cse: ");
			StrCat(tempStr, print_direction2(data.input.true_track.value));
			StrCat(tempStr, "°");
			DrawString( tempStr, tempRect.topLeft.x, y );

			//----------------------------------------------------------
			//Draw Magnetic Deviation
			//----------------------------------------------------------
			StrCopy(tempStr, "Dev: ");
			if (data.input.deviation.value < 0.0) {
				StrCat(tempStr, "-");
				StrCat(tempStr, print_direction2(Fabs(data.input.deviation.value)));
			} else {
				StrCat(tempStr, print_direction2(data.input.deviation.value));
			}
			StrCat(tempStr, "°");
			DrawString( tempStr, tempRect.topLeft.x+(tempRect.topLeft.x/2)+4, y );

			//----------------------------------------------------------
			//Draw Speed
			//----------------------------------------------------------
			y += FntLineHeight();
			StrCopy(tempStr, "Spd: ");
			StrCat(tempStr, print_horizontal_speed2(data.input.ground_speed.value, 1));
			StrCat(tempStr, data.input.spdtext);
			DrawString( tempStr, tempRect.topLeft.x, y );

			//----------------------------------------------------------
			//Draw Altitude
			//----------------------------------------------------------
			y += FntLineHeight();
			StrCopy(tempStr, "Alt: ");
			StrCat(tempStr, print_altitude(gpsalt));
			StrCat(tempStr, data.input.alttext);
			DrawString( tempStr, tempRect.topLeft.x-10, y );

			//----------------------------------------------------------
			//Draw Geoidial Height
			//----------------------------------------------------------
			StrCopy(tempStr, "GH: ");
			StrCat(tempStr, print_altitude(gpsgeoidaldiff));
			StrCat(tempStr, data.input.alttext);
			DrawString( tempStr, tempRect.topLeft.x+(tempRect.topLeft.x/2)+4, y );
		}

		if (!IsGPS) {
			/*----------------------------------------------------------
			Draw Flarm Display Scale
			----------------------------------------------------------*/
			// Need to Always Draw the Scale 
			FntSetFont(stdFont);
			StrCopy(tempStr, "Scale:");
			WinDrawChars(tempStr, StrLen(tempStr), 137, 122);
			FntSetFont(largeBoldFont);

			StrCopy(tempStr, DblToStr(pround(mapscale * data.input.disconst, 1), 1));
			WinDrawChars(tempStr, StrLen(tempStr), 138, 132);
			FntSetFont(stdFont);

			/*----------------------------------------------------------
			Draw Flarm Alarm Data
			----------------------------------------------------------*/
			FntSetFont(largeBoldFont);
			if (satData[0].svid != gpsInvalidSVID) {			
				// distance
				poirange = satData[0].elevation / DISTKMCONST / 1000;
				if ((poirange*data.input.disconst) < 10.0) {
					StrCopy(tempStr, print_distance2(poirange, 2));
				} else if ((poirange*data.input.disconst) < 100.0) {
					StrCopy(tempStr, print_distance2(poirange, 1));
				} else if ((poirange*data.input.disconst) < 1000.0) {
					StrCopy(tempStr, print_distance2(poirange, 0));
				} else {
					StrCopy(tempStr, "999");
				}
				WinDrawChars(tempStr, StrLen(tempStr), 139, 0);

				// bearing
				StrCopy(tempStr, print_direction2(satData[0].azimuth * radToDeg));
				StrCat(tempStr, "°");
				WinDrawChars(tempStr, StrLen(tempStr), 1, 0);

				// vertical separation
				StrCopy(tempStr, print_altitude(FlarmVSep / ALTMETCONST));
				WinDrawChars(tempStr, StrLen(tempStr), 1, 132);	
			}			
			FntSetFont(stdFont);
		}

		/*----------------------------------------------------------
		Make it visible
		----------------------------------------------------------*/
		FntSetFont(stdFont);
		WinSetDrawWindow( oldWindow );
		WinGetWindowFrameRect( gOffScreenWindow, &tempRect );
		WinCopyRectangle( gOffScreenWindow, oldWindow, &tempRect, 0, formTitleHeight, winPaint );

		/*----------------------------------------------------------
		Make the re-connection button visible if no GPS present
		----------------------------------------------------------*/
		ctl_set_visible(form_sat_status_restart, ((!recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)) && IsGPS));
		ctl_set_visible(form_sat_status_GPSinfo, (IsGPS && (data.config.flightcomp == FLARMCOMP)));
	} 
	return;
}

/*********************************************************************
*
*   PROCEDURE NAME:
*       DrawCenteredString - Draw a Centered String
*
*   DESCRIPTION:
*       Draws the string at the given y coordinate, centered between
*       the right and left bounds.
*
*********************************************************************/
void DrawCenteredString( Char *str, UInt16 boundsLeft, UInt16 boundsRight, UInt16 yPos )
{
	/*----------------------------------------------------------
	Variables
	----------------------------------------------------------*/
	UInt16  length; /* length of the string                 */
	UInt16  width;  /* width of the string                  */
	UInt16  xPos;   /* horizontal position of the string    */

	/*----------------------------------------------------------
	Find the width of the string in pixels.
	----------------------------------------------------------*/
	length = StrLen( str );
	width  = FntCharsWidth( str, length );

	/*----------------------------------------------------------
	Draw the string centered within the bounds.
	----------------------------------------------------------*/
	xPos = ( boundsLeft + boundsRight - width ) / 2;
	WinDrawChars( str, length, xPos, yPos );

} /* DrawCenteredString() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       DrawString - Draw a String
*
*   DESCRIPTION:
*       Draws the string at the given coordinate.
*
*********************************************************************/
void DrawString( Char *str, UInt16 x, UInt16 y )
{
	WinDrawChars( str, StrLen( str ), x, y );

	return;

} /* DrawString() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       AppendIntToStr - Append Integer to String with Leading Zeros
*
*   DESCRIPTION:
*       Appends the integer to a string, padding with leading zeros
*       the width specified.
*
*********************************************************************/
void AppendIntToStr( Int32 number, int width, Char *outStr )
{
	/*----------------------------------------------------------
	Variables
	----------------------------------------------------------*/
	Char    buf[ 10 ];  /* temporary buffer             */
	int     padCount;   /* count of characters to pad   */
	int     i;          /* loop counter                 */

	/*----------------------------------------------------------
	Convert number to a string and determine the number of
	characters that need to be padded.
	----------------------------------------------------------*/
	StrIToA( buf, number );
	padCount = width - StrLen( buf );

	/*----------------------------------------------------------
	Write padCount zeros to output string
	----------------------------------------------------------*/
	for ( i = 0; i < padCount; ++i ) {
		StrCat( outStr, "0" );
	}

	/*----------------------------------------------------------
	Write converted number to output string
	----------------------------------------------------------*/
	StrCat( outStr, buf );

} /* AppendIntToStr() */

Boolean form_sat_status_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[10];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			origform = form_sat_status;
			IsGPS = (gotoGPSinfo || (data.config.flightcomp != FLARMCOMP));
			if (Flarmpresent && !gotoGPSinfo) IsGPS = false;
			//if (IsGPS && (data.config.flightcomp == FLARMCOMP)) {
			//	ctl_set_label(form_sat_status_GPSinfo, "Flarm");
			//} else {
			//	ctl_set_label(form_sat_status_GPSinfo, "GPS");
			//}			
			//if (data.config.flightcomp == FLARMCOMP) data.config.Flarmscaleidx = (data.input.disconst<DISTKMCONST?2:3);
			FrmDrawForm(FrmGetActiveForm());
			gotoGPSinfo = false;
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "final_glide-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-frmCloseEvent");
			DrawSatStatus(0.0, 0.0, -1.0, true);
			handled=false;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_sat_status_restart:
					// stop and start the GPS connection
					XferClose(data.config.nmeaxfertype);
					SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
					XferInit(tempchar, NFC, data.config.nmeaxfertype);
					break;
				case form_sat_status_GPSinfo:
					gotoGPSinfo = false;
					FrmGotoForm(form_sat_status);
					break;
				default:
					break;
			}
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				DrawSatStatus(data.logger.gpsalt, data.input.gpsgeoidaldiff, MGCBattHours, false);
			}
			handled=false;
			break;
		default:
			break;
	}
	return handled;
}
