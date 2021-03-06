/**
 * \file 68K/Libraries/TonesLibrary/TonesLib.h
 *
 * Tones Library owns controls the MIDI tones on the device. 
 *
 * \license
 *
 *    Copyright (c) 2002 Handspring Inc., All Rights Reserved
 *
 * \author Arun Mathias
 *
 *****************************************************************************/



Err TonesLibOpen (UInt16 refNum)
                 SYS_TRAP (sysLibTrapOpen);

Err TonesLibClose (UInt16 refNum)
                 SYS_TRAP (sysLibTrapClose);

Err TonesLibWake (UInt16 refNum) 
                 SYS_TRAP (sysLibTrapWake);

Err TonesLibSleep (UInt16 refNum)
                 SYS_TRAP (sysLibTrapSleep);

Err TonesLibGetToneList (UInt16 refNum, ToneItemPtr * list, UInt16 * listLength)
                 SYS_TRAP (tonesLibTrapGetToneList);

Err TonesLibGetToneIDs ( UInt16 refNum, ToneIdentifier ** list, UInt16 * listLength)
                 SYS_TRAP (tonesLibTrapGetToneIDs);

Err TonesLibGetToneName (UInt16 refNum, ToneIdentifier toneId, CharPtr name, UInt16
	maxLength)
                 SYS_TRAP (tonesLibTrapGetToneName);

Err TonesLibPlayTone (UInt16 refNum, ToneIdentifier toneId, UInt16 playCount,
	ToneVolume volume, ToneVibrateType vibrate, Boolean blocking)
                 SYS_TRAP (tonesLibTrapPlayTone);

Err TonesLibAddTone (UInt16 refNum, MemHandle midiTone, CharPtr toneName,
	Boolean protectedTone)
                 SYS_TRAP (tonesLibTrapAddTone);


Err TonesLibManageTones (UInt16 refNum)
                 SYS_TRAP (tonesLibTrapManageTones);

Err TonesLibDeleteTone (UInt16 refNum, ToneIdentifier toneId)
                  SYS_TRAP (tonesLibTrapDeleteTone);


Err TonesLibInitTonesDB (UInt16 refNum)
                   SYS_TRAP (tonesLibTrapInitTonesDB);


Err TonesLibGetSoundPrefs (UInt16 refNum, SoundPrefType soundType, SoundPreference * soundPrefs)
                  SYS_TRAP (tonesLibTrapGetSoundPrefs);

Err TonesLibSetSoundPrefs (UInt16 refNum, SoundPrefType soundType, SoundPreference * soundPrefs)
                  SYS_TRAP (tonesLibTrapSetSoundPrefs);

Err TonesLibPickTone (UInt16 refNum, ToneType * toneType, MemHandle *toneH, Char
	* name)
                  SYS_TRAP (tonesLibTrapPickTone);

Err TonesLibStopTone (UInt16 refNum)
                  SYS_TRAP (tonesLibTrapStopTone);

Err	TonesLibGetSoundOnVolume (UInt16 refNum, SoundPrefType soundType,
							  ToneVolume* soundOnVolumeP)
				  SYS_TRAP (tonesLibTrapGetSoundOnVolume);

Err	TonesLibSetSoundOnVolume (UInt16 refNum, SoundPrefType soundType,
							  ToneVolume soundOnVolume)
				  SYS_TRAP (tonesLibTrapSetSoundOnVolume);
