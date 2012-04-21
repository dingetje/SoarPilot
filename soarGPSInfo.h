#ifndef SOARGPSINFO_H
#define SOARGPSINFO_H

#define SGAR __attribute__ ((section ("sgar")))

/*--------------------------------------------------------------------
                                 MACROS
--------------------------------------------------------------------*/

//#define cntOfArray( n )                 ( sizeof( n ) / sizeof( ( n ) [ 0 ] ) )
#define limit( lower, value, upper )    ( min( max( lower, value ), upper ) )
#define round( n )                      ( ( Int32 ) ( ( n ) < 0.0f ) ? ( ( n ) - 0.5f ) : ( ( n ) + 0.5f ) )

/*--------------------------------------------------------------------
                            LITERAL CONSTANTS
--------------------------------------------------------------------*/

/*------------------------------------------------------
General
------------------------------------------------------*/
#define formTitleHeight     15                          /* height of form title             */
#define monoColorDepth      2                           /* 2 bits (4 grayshades)            */
#define colorColorDepth     8                           /* 8 bits (256 colors)              */
#define minDigits           2                           /* 2 digits for whole minutes       */
#define minPrecision        3                           /* 3 digits of minute precision     */
#define minScale            1000                        /* minute scaling                   */
#define degMax              360                         /* max degree value                 */
#define minMax              60                          /* max minute value                 */
#define snrMax              5000                        /* 50 dB Hz                         */
#define snrMin              3000                        /* 30 dB Hz                         */
#define snrRng              ( snrMax - snrMin )         /* signal-to-noise ratio range      */
#define eventWaitSeconds    2                           /* wait 2 seconds for event         */

/*------------------------------------------------------
Conversions
------------------------------------------------------*/
#define semiToDeg           (double)( 180.0 / 2147483648.0 )    /* semicircles to degrees           */
#define mpsToMph            2.2369                      /* meters per second to mph         */
#define metersToFeet        3.2808                      /* meters to feet                   */
#define secondsPerFrac      ( 1.0 / 4294967296.0 )      /* Seconds per fractional second    */
#define secondsPerMinute    60.0                        /* Seconds per minute               */
#define secondsPerHour      ( secondsPerMinute * 60.0 ) /* Seconds per hour                 */

/*------------------------------------------------------
Screen layout
------------------------------------------------------*/
#define skyvwRadPercent 	20			/* Radius is 20% of width of screen */
#define flarmvwRadPercent 	37			/* Radius is 37% of width of screen for Flarm screen */
#define rectBorderWidth 	1			/* Rectangle border width */
#define trkRadius		3			/* Radius of the track circle */

/*----------------------------------------------------------
GPSSatDataType Status Bitfield
----------------------------------------------------------*/
#define satEphShift     0
#define satNoEph        ( 0 << satEphShift )            /* no ephemeris                     */
#define satEph          ( 1 << satEphShift )            /* has ephemeris                    */

#define satDifShift     1
#define satNoDif        ( 0 << satDifShift )            /* no differential correction       */
#define satDif          ( 1 << satDifShift )            /* has differential correction      */

#define satUsedShift    2
#define satNotUsed      ( 0 << satUsedShift )           /* not used in solution             */
#define satUsed         ( 1 << satUsedShift )           /* used in solution                 */

#define satRisingShift  3
#define satSetting      ( 0 << satRisingShift )         /* satellite setting                */
#define satRising       ( 1 << satRisingShift )         /* satellite rising                 */

// Added this here to augment what is in the Garmin GPSLib.h
#define gpsFixValid 7

/*****************************************************************************
 * protos
 *****************************************************************************/
void DrawSatStatus(double gpsalt, double gpsgeoidaldiff, double MGCBattHours, Boolean cleanup) SGAR;
void DrawCenteredString( Char *str, UInt16 boundsLeft, UInt16 boundsRight, UInt16 yPos ) SGAR;
void DrawString( Char *str, UInt16 x, UInt16 y ) SGAR;
void AppendIntToStr( Int32 number, int width, Char *outStr ) SGAR;
Boolean form_sat_status_event_handler(EventPtr event) SGAR;

#endif
