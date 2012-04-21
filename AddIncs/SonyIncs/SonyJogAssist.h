/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyJogAssist.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Jog Event Mask definitions                                           *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/10/24                                              *
 *       Last Modified: $Date: 2003/01/06 23:02:02 $
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */

#ifndef __SONYJOGASSIST_H__
#define __SONYJOGASSIST_H__

/******************************************************************************
 *    Includes                                                                *
 ******************************************************************************/


/******************************************************************************
 *    Mask for vchrJogEvent                                                   *
 ******************************************************************************/

/*** Type ***/
#define sonyJogAstMaskType1			(0x0001)
#define sonyJogAstMaskType2			(0x0002)

/*** Mask bitfields ***/
#define sonyJogAstMaskUp				(0x0001)
#define sonyJogAstMaskDown				(0x0002)
#define sonyJogAstMaskPushedUp		(0x0004)
#define sonyJogAstMaskPushedDown		(0x0008)
#define sonyJogAstMaskPush				(0x0010)
#define sonyJogAstMaskRelease			(0x0020)
#define sonyJogAstMaskPushRepeat		(0x0040)
#define sonyJogAstMaskBack				(0x0080)

#define sonyJogAstMaskReserved		(0xFF00)
#define sonyJogAstMaskAll				~sonyJogAstMaskReserved
#define sonyJogAstMaskNone				(0x0000)

	/* Application is discouraged to use Back unless it uses Back in accordance
	     with the guidline specified by Sony SDK */


#endif // __SONYJOGASSIST_H__

