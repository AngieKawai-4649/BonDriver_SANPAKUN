/* SunPTV-USB   (c) 2016 trinity19683
  finding USB device file (MS-Windows)
  usbdevfile.cpp
  2016-01-23
*/

#include "stdafx.h"
#include <setupapi.h>
#include <strsafe.h>
#include <WinUSB.h>

#include "usbdevfile.h"

// Driver Instance GUID
DEFINE_GUID( GUID_WINUSB_DRV,	0xa70cc802, 0x7309, 0x486d, 0xbe, 0xe8, 0x93, 0xa0, 0x48, 0xcf, 0x6c, 0x63);

HANDLE usbdevfile_alloc(int idx)
{
	DWORD dwRet;
	ULONG length;
	HANDLE hDev = INVALID_HANDLE_VALUE;
	GUID * const pDrvID = (GUID *)&GUID_WINUSB_DRV;

	// get handle to device info.
	HDEVINFO deviceInfo = SetupDiGetClassDevs(pDrvID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(INVALID_HANDLE_VALUE == deviceInfo) return NULL;

	SP_DEVICE_INTERFACE_DATA interfaceData;
	memset(&interfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (idx == -1) {	// デバイス指定無し
		for (int i=0; i < 40; i++) {
			//# enumerate device interfaces
			if (TRUE == SetupDiEnumDeviceInterfaces(deviceInfo, NULL, pDrvID, i, &interfaceData)) {
				ULONG requiredLength = 0;
				//必要なバッファー サイズを取得する
				SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, NULL, 0, &requiredLength, NULL);
				//# allocate buffer
				requiredLength += sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + sizeof(TCHAR);

				PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
				if ((detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GMEM_FIXED, requiredLength)) != NULL) {
					detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					//#  get path name of a device
					length = requiredLength;
					if (TRUE == SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, length, &requiredLength, NULL)) {
						hDev = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
						GlobalFree(detailData);
						if (hDev == INVALID_HANDLE_VALUE) {
							dwRet = GetLastError();
							// デバイスが使用中の場合は次のデバイスをアクセスするのでエラーとしない
							if (dwRet != ERROR_ACCESS_DENIED) {
								break;
							}
						}
						else {
							// デバイスが見つかった
							break;
						}
					}
					else {
						GlobalFree(detailData);
						break;
					}
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}
	}
	else {		// デバイス指定 0～
		//# enumerate device interfaces
		if (TRUE == SetupDiEnumDeviceInterfaces(deviceInfo, NULL, pDrvID, idx, &interfaceData)) {
			ULONG requiredLength = 0;
			//必要なバッファー サイズを取得する
			SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, NULL, 0, &requiredLength, NULL);
			//# allocate buffer
			requiredLength += sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + sizeof(TCHAR);

			PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
			if ((detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GMEM_FIXED, requiredLength)) != NULL) {
				detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				//#  get path name of a device
				length = requiredLength;
				if (TRUE == SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, length, &requiredLength, NULL)) {
					//# success
					hDev = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
				}
				GlobalFree(detailData);
			}
		}
	}

	SetupDiDestroyDeviceInfoList(deviceInfo);

	if(INVALID_HANDLE_VALUE == hDev) return NULL;
	return hDev;
}

HANDLE usbdevfile_init(HANDLE hDev)
{
	HANDLE usbHandle;
	if(FALSE == WinUsb_Initialize( hDev, &usbHandle ))
		return NULL;
	return usbHandle;
}

void usbdevfile_free(HANDLE usbHandle)
{
	WinUsb_Free( usbHandle );
}

/*EOF*/