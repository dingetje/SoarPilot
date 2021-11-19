# SoarPilot ![](largec325.gif)

![Moving Map](/images/moving_map_dia.jpg)

SoarPilot was designed with both the sport and competition sailplane/paraglider pilots in mind. 
It enhances a pilot's level of information, helping him/her to make informed speed & distance decisions when flying around the local airfield or going cross-country. 
In addition, using an attached GPS and the logger function, the pilot can download and plot the flight track information after flying for futher analysis or just for fun!


It will run on the popular PalmOS compatible devices with PalmOS 3.0 or higher as well as most Windows WinMobile/PocketPC devices using [StyleTap®](https://soaringpilot.meansolutions.nl/doku.php/soarpilot:styletap_differences) support. 
Best of all, it's available free of charge. It uses McCready theory to calculate the speed-to-fly/final glide and the altitude required to fly a certain 
distance assuming a given head/tailwind and airmass movement. If a GPS is not available, the distance to fly can be entered manually. 
However if one is available, SoarPilot can process GPS data using the Palm PDA's serial interface. The GPS must be capable of outputting data which is compatible with 
the NMEA-0183 version 1.5 or 2.0+ (auto-select) standards. The program makes use of the GPRMB, GPRMC, GPGGA & PGRMZ sentences. 
The PGRMZ sentence is used for NMEA-183 version 1.5 to get GPS altitude from a Garmin GPS. Altitude is taken from the standard GPGGA sentence for version 2.0+.


It can also now parse the proprietary sentences from the Volkslogger, LX, Filser Cambridge 302/302A/GPSNAV, Garmin (select models), PosiGraph, Zander and 
other loggers for pressure altitude information. In addition, if connected to a Borgelt B50 or Cambridge 302/302A, all additional information from both units 
is used to enhance the accuracy of many of the calculated values.

## Manual
The official DokuWiki community driven user manual is lost over the years, but a backup copy is hosted here:

[SoarPilot User Manual](https://soaringpilot.meansolutions.nl/doku.php/soarpilot:sp_intro).

(_excuses for the platitude of link rot in the manual_)

# Legacy Code
While the development for Soaring Pilot has stopped for some years now (mainly due to other alternatives running on more modern hardware, like LK8000 and XCSoar), 
the source code has now been placed in the public domain.

# Credits
The original developers: 
- Oliver Spatscheck, 
- Mark Hawkins, 
- Paul Gleeson, 
- Marc Ramsey 
- MANY Others!!!
