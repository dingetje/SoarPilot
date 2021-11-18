Readme for SoarPilot 4.6.2 Beta
For installation and operating details see the online manual at:
http://www.soaringpilot.org/dokuwiki/doku.php

Note: Installing new version of SoarPilot may causes databases to be reset.
Always save your data using the Settings / Transfer screen prior to installing a new version.
Then import the data back into SoarPilot only using the Settings Transfer screen. 
Do not copy database files directly.
*** Flights are saved to an IGC file, and cannot be re-imported into SoarPilot ***

Note: Regarding OpenAir files.
SoarPilot is fairly strict on the formatting of co-ordinates. e.g. extra spaces, tabs, position of commas, extra comments after the line, leading zeros etc etc. If some SUA items are shown in error, please check those items carefully in the text file to correct the formatting. 
See http://www.winpilot.com/UsersGuide/UserAirspace.asp

Changes for 4.6.2 Beta - April 2012

  * New - ability to save preferred bluetooth GPS address in config. First time the bluetooth
    search dialog will show, select preferred GPS, next time SoarPilot will connect to this
    address directly. Enable search again by changing NMEA/Port setting and then back to BT.
    Address is saved in exported config file, i.e. BTGPSADDR,00:1B:C1:06:D3:2C

Changes for 4.6.1 Final - April 2012

  * Change - The terrain on the task is now updated when target points are moved in Area tasks.

  * Fix - In some cases selecting Max or Min in an area would not be retained.
  * Plus some minor code optimizations.


Changes for 4.6.0 Final - May 2011

  * New - Support for Cylinder races with various radii centered on the same waypoint. This type of task is typically used in Paragliding competitions.
    * There is a new type of area called an "Exit Area", this is set by creating an Area waypoint and tapping on the sector type. This will change the type to an "Exit Area"
    * This "Exit Area" is considered achieved once you have entered it, and then left it.
    * Typically you will build a task with all the waypoints the same, but different combinations of cylinders, Start, finish, turnpoint and exit areas.
    * Before you activate the task, be sure to change the AAT type on the bottom right of the Task Edit screen to "Max"
    * For example, a simple out and return is made from 3 waypoints. Start Cylinder, Exit Area, Finish Cylinder. The radius settings for the start and finish are set on the Settings / Task screen as normal.
  * New - Support for Westerboer Flight computers for altitude and vario data.
  * New - On the Flarm display aircraft that are climbing at more than 50% of the current MC value are shown with solid arrow heads. All other aircraft are shown with open arrow heads
  * New - Support for iPhone hardware running StyleTap software. Only difference to released version 4.5.3 is the GPS Info screen is modified to display the iPhone GPS data correctly.

  * Change - Improvements in key distance / bearing calculations
  * Change - Improved behaviour when trying to move a target point out of the area defined.

  * Fix - Removed some debug code that required a SD card.
  * Fix - Adding thermal waypoints could cause a crash. This is now fixed.
  * Fix - Corrected version numbers in IGC files
  * Fix - Correction of small errors in the screens
  * Fix - Edits to area sector target points were not always saved correctly
  * Fix - After a task was finished you could not select another waypoint
  * Fix - Potential crash when switching to/from the map screen
  * Fix - If many thermal waypoints were created, there was a chance the program would crash.
  * Fix - Some OpenAir SUA files would not load correctly if spaces were missing in arc items.
  * Fix - On a small number of devices, parsing SUA files in TNP format would cause errors. This will reset your SUA database.
  * Fix - Flarm traffic display now works without flashing and maximum range increased to 20km.
  * Fix - Waypoint colours are now displayed correctly on the Waypoint Sector screen.
  * Fix - Fix to allow parsing data files with lines longer than 256 chars.
  * Fix - Further tweaks for running on the iPhone using StyleTap
  * Fix - Fixes map screen not being displayed on certain Windows mobile devices
  * Fix - Further tweaks for running on the iPhone using StyleTap

