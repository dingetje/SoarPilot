#include <PalmOS.h>
#include "soarBT.h"
/**
* \file soarBT.c
* \brief SoarPilot Bluetooth code
*/

/**************/
/* BT Globals */
/**************/
//static UInt16 unPortId;
static UInt16 btLibRefNum=0;
UInt8  BTAddress[6];
static Err err;
//static SrmOpenConfigType config;
//static BtVdOpenParams    btParams;
//static BtLibSdpUuidType  sppUuid;

/***********************/
/* Load the BT library */
/***********************/
static void BT_LoadLibrary()
{
	btLibRefNum=0;

	// BT library already in memory?
	err=SysLibFind("Bluetooth Library",&btLibRefNum);
	if(err) {
		// nope, then load it
		err=SysLibLoad(sysFileTLibrary,sysFileCBtLib,&btLibRefNum);
	}
}

/****************************************/
/* Find a BT device on air              */
/* On success return 0, else error code */
/* BTAddress array is filled with user  */
/* selected address.                    */
/****************************************/
Err BT_FindDevice()
{
	if (!btLibRefNum) {
		BT_LoadLibrary();
	}
	if(btLibRefNum && !err)
	{
		err=BtLibOpen(btLibRefNum,false);
		if(!err) {
			/*
			   Discover all available devices, present them
			   in the user interface, and allow the user to select one
			   of these devices.

				Parameters

				- btLibRefNum
					Reference number for the Bluetooth library.
				- instructionTxt
					Text displayed at the top of the selection box.
					Pass NULL to display the default text. 
					The default text is "Select a device:"
				- deviceFilterList
					Array of BtLibClassOfDeviceTypes. This function
					displays only the remote devices whose class matches
					a class in this list. If deviceFilterList is NULL, 
					this function displays all discovered devices.
				- deviceFilterListLen
					Number of elements in deviceFilterList.
				- selectedDeviceP
					Pointer to a BtLibDeviceAddressType where this function
					stores the address of the device the user selects.
					You need to allocate this space before calling this function.
				- addressAsName
					If true, display the Bluetooth addresses of the remote
					devices instead of their names. This option is available
					for debugging purposes. 
				- showLastList
					If true, causes all other parameters to be ignored and
					displays the same list as the previous call to 
					BtLibDiscoverSingleDevice.
			*/
			err=BtLibDiscoverSingleDevice(btLibRefNum,
											NULL,
											NULL,
											0, 
											(BtLibDeviceAddressType *)BTAddress,
											false,
											false);
		}
		BtLibClose(btLibRefNum);
	}
	return err;
}

/************************************/
/* Pause n milliseconds             */
/************************************/
/*
static void Uti_WaitMilliSec(UInt32 ulMilliSec)
{
 UInt16  unTickPerSec;

 unTickPerSec=SysTicksPerSecond();
 if(unTickPerSec)
  SysTaskDelay(ulMilliSec*unTickPerSec/1000);
 else
  SysTaskDelay(ulMilliSec/10);
}
*/

/**************************************/
/* Close a Bluetooth serial connetion */
/**************************************/
/* not used for now, rely on SrmExtOpen instead
static void BT_Close()
{
 if(unPortId)
  {
   SrmClose(unPortId);
   unPortId=0;
   Uti_WaitMilliSec(500);
   SrmClose(unPortId); // Retry, on some system it's hard to die
  }
}
*/

/**************************************/
/* Open a Bluetooth serial connection */
/**************************************/
/* not used for now, rely on SrmExtOpen instead
static void BT_Open()
 {
  BT_Close();
  MemSet(&sppUuid, sizeof(sppUuid), 0);
  sppUuid.size = btLibUuidSize16;
  sppUuid.UUID[0] = 0x11;
  sppUuid.UUID[1] = 0x01;
  MemSet(&btParams, sizeof(btParams), 0);
  btParams.u.client.remoteDevAddr.address[0]=cAddress[0];
  btParams.u.client.remoteDevAddr.address[1]=cAddress[1];
  btParams.u.client.remoteDevAddr.address[2]=cAddress[2];
  btParams.u.client.remoteDevAddr.address[3]=cAddress[3];
  btParams.u.client.remoteDevAddr.address[4]=cAddress[4];
  btParams.u.client.remoteDevAddr.address[5]=cAddress[5];
  btParams.role = btVdClient;
  btParams.u.client.method = btVdUseUuidList;
  btParams.u.client.u.uuidList.tab = &sppUuid;
  btParams.u.client.u.uuidList.len = 1;
  MemSet(&config, sizeof(config), 0);
  config.function = serFncUndefined;
  config.drvrDataP = (MemPtr)&btParams;
  config.drvrDataSize = sizeof(btParams);
  err=SrmExtOpen(sysFileCVirtRfComm,&config,sizeof(config),&unPortId);
 }
 */

/****************/
/* Flush BT ser */
/****************/
/*
static void BT_Flush(UInt16 unTimeout)
{
 if(unPortId)
   err=SrmReceiveFlush(unPortId,unTimeout);
}
*/

/****************/
/* Send BT data */
/****************/
/*
static void BT_Send(char * pData,UInt16 unLen)
{
 if(unPortId)
   SrmSend(unPortId,pData,unLen,&err);
}
*/

/*******************/
/* Receive BT data */
/*******************/
/*
static UInt16 BT_Receive(char * pData,UInt16 unLen,UInt16 unTimeout)
{
 UInt16 unLenRead;
 if(unPortId)
   unLenRead=SrmReceive(unPortId,pData,unLen,unTimeout,&err);
 return(unLenRead);
}
*/

