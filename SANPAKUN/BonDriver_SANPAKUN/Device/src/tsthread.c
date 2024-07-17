/* fsusb2i   (c) 2015-2016 trinity19683
  TS USB I/O thread (MS-Windows)
  tsthread.c
  2016-02-18
*/



#include "stdafx.h"
#include <errno.h>
#include <string.h>
#include <process.h>
#include <WinUSB.h>

#include "usbops.h"
#include "osdepend.h"
#include "tsbuff.h"
#include "tsthread.h"

//#include <stdio.h>

#define ROUNDUP(n,w) (((n) + (w)) & ~(unsigned)(w))

#define INCLUDE_ISOCH_XFER
#define NUM_ISOC_PACKET  16

struct TSIO_CONTEXT {
	OVERLAPPED ol;
	int index;
#ifdef INCLUDE_ISOCH_XFER
	USBD_ISO_PACKET_DESCRIPTOR isoDesc[NUM_ISOC_PACKET];
#endif
};

struct tsthread_param {
	HANDLE hThread;							// handle to thread data
	unsigned char volatile  flags;			// 0x01: issue a new request. 0x02: cancel requests and stop thread. 0x10: Isochronous
	const struct usb_endpoint_st*  pUSB;	// USB endpoint
	char* buffer;							// data buffer (in heap memory)
	int*  actual_length;					// actual length of each buffer block  int[buff_num] 未使用 -1 / 仕掛中 -2 / バッファに取込済 実バッファ数(buff_unitSize)
	unsigned buff_unitSize;					// em287x_create()xfer_size  bulk: 188 * 245  Isochronous: 188 * 5
	int buff_num;							// TSバッファ配列数  TS_BufSize(3670016Byte) / buff_unitSize
	int buff_push;							// 次に使用するデータブロックの位置 actual_length index
	int buff_pop;							// 次に取り出すデータブロックの位置 actual_length index
	struct TSIO_CONTEXT ioContext[TS_MaxNumIO];
	HANDLE hTsEvent;
#ifdef INCLUDE_ISOCH_XFER
	WINUSB_ISOCH_BUFFER_HANDLE hIsochBuffer;
#endif
};

//static FILE* fp;

 UINT g_TsBufSize = 0;

static int submitURB(const tsthread_ptr tptr)
{
	//# isochronous URB request
	struct tsthread_param* const ps = tptr;
	DWORD i, dRet = 0;

	for(i = 0; i < TS_MaxNumIO; i++) {
		struct TSIO_CONTEXT* const pContext = &ps->ioContext[i];
		BOOL bRet;
		if(0 <= pContext->index) continue;

		ZeroMemory( &pContext->ol, sizeof(OVERLAPPED));
		pContext->ol.hEvent = ps->hTsEvent;
		pContext->index = ps->buff_push;


		if (ps->pUSB->endpoint & 0x100) { //# Isochronous
#ifdef INCLUDE_ISOCH_XFER
			int j;
			BOOL bContinue = TRUE;
			for (j = 0; j < NUM_ISOC_PACKET; j++) {
				ps->actual_length[j + ps->buff_push] = -2;
			}
			if (!(ps->flags & 0x10)) {
				bContinue = FALSE;
				ps->flags |= 0x10;
			}

			bRet = WinUsb_ReadIsochPipeAsap(ps->hIsochBuffer, ps->buff_push * ps->buff_unitSize, NUM_ISOC_PACKET * ps->pUSB->xfer_size, bContinue, NUM_ISOC_PACKET, pContext->isoDesc, &(pContext->ol));
			dRet = GetLastError();
			if (!bRet && ERROR_INVALID_PARAMETER == dRet) {
				ps->flags &= ~0x10;
			}
#else
			bRet = FALSE;
			dRet = ERROR_INVALID_FUNCTION;
			tsthread_stop(ps);
#endif
		}
		else {
			// WinUSBパイプから非同期に読み込む、即時リターンし 戻値:FALSE ERROR_CODE:ERROR_IO_PENDING となる
			// 読み込み完了時にイベント割り込みが発生し、reapURB() が呼び出され WinUsb_GetOverlappedResult() で受信確認する
			ps->actual_length[ps->buff_push] = -2;
			bRet = WinUsb_ReadPipe(ps->pUSB->fd, ps->pUSB->endpoint & 0xFF, ps->buffer + (ps->buff_push * ps->buff_unitSize), ps->buff_unitSize, NULL, &(pContext->ol));
			dRet = GetLastError();
		}
		if (FALSE == bRet && ERROR_IO_PENDING != dRet) {
			warn_info(dRet, "submitURB failed");
			pContext->index = -1;
		}
		else {
			int next_index = ps->buff_push;
			if(ps->pUSB->endpoint & 0x100) { //# Isochronous
				next_index += NUM_ISOC_PACKET;
			}
			else {
				next_index++;
			}
			ps->buff_push = (next_index < ps->buff_num) ? next_index : 0;
			dRet = 0;
		}

		if (bRet) {
			//# completed (nowait)
			SetEvent(ps->hTsEvent);
		}
		if(dRet != ERROR_SUCCESS) break;
	}

	return dRet;
}

static int reapURB(const tsthread_ptr tptr)
{
	struct tsthread_param* const ps = tptr;
	DWORD i, countURB = 0;

	for(i = 0; i < TS_MaxNumIO; i++) {
		struct TSIO_CONTEXT* const pContext = &ps->ioContext[i];
		BOOL bRet;
		DWORD dRet, bytesRead = 0;
		if(0 > pContext->index) continue;

		bRet = WinUsb_GetOverlappedResult( ps->pUSB->fd, &(pContext->ol), &bytesRead, FALSE );
		dRet = GetLastError();
		if(FALSE == bRet && ERROR_IO_INCOMPLETE == dRet) {
			//# incomplete
			countURB++;
		}else{
			int* const   pLen = &(ps->actual_length[pContext->index]);
			if(ps->pUSB->endpoint & 0x100) { //# Isochronous
#ifdef INCLUDE_ISOCH_XFER
				int j;
				if (bRet) {
					//# success
					for (j = 0; j < NUM_ISOC_PACKET; j++) {
						if (pContext->isoDesc[j].Status) {
							pLen[j] = 0;
							warn_msg(dRet, "reapURB%u.%u", i, j);
						}
						else {
							bytesRead = pContext->isoDesc[j].Length;
							pLen[j] = bytesRead;
							//dmsgn("reapURB%u.%u=%d, ", i, j, bytesRead);
						}
					}
				}
				else {
					//# failed
					for (j = 0; j < NUM_ISOC_PACKET; j++) {
						pLen[j] = 0;
					}
					warn_msg(dRet, "reapURB%u", i);
				}
#endif
			}
			else {
				if (bRet) {
					//# success
					if (ps->buff_unitSize < bytesRead) {
						warn_info(bytesRead, "reapURB overflow");
						bytesRead = ps->buff_unitSize;
					}
					pLen[0] = bytesRead;
					//dmsgn("reapURB%u=%d, ",i,bytesRead);
				}
				else {
					//# failed
					pLen[0] = 0;
					warn_msg(dRet, "reapURB%u", i);
				}
			}
			pContext->index = -1;
		}
	}

	return countURB;
}

/* TS thread function issues URB requests. */
static unsigned int __stdcall tsthread(void* const param)
{
	struct tsthread_param* const ps = param;
	ps->buff_push = 0;

	for(;;) {
		DWORD dRet;
		if(ps->flags & 0x01) {
			//# continue to issue a new URB request
			submitURB(ps);
		}
		if(ps->flags & 0x02) {
			//# canceled
			reapURB(ps);
			break; 
		}

		dRet = WaitForSingleObject( ps->hTsEvent , TS_PollTimeout );

		if(WAIT_OBJECT_0 == dRet || WAIT_TIMEOUT == dRet) {
			if(reapURB(ps) < 0) break;
			//# timeout
			if(WAIT_TIMEOUT == dRet && ps->flags & 0x01) {
				dmsg("poll timeout");
			}
		}else{
			dRet = GetLastError();
			warn_info(dRet,"poll failed");
			break;
		}
	}
	_endthreadex( 0 );
	return 0;
}

/* public function */

int tsthread_create(tsthread_ptr* const tptr, const struct usb_endpoint_st* const pusbep)
{
	struct tsthread_param* ps;
	DWORD dwRet, i;

#ifdef _DEBUG
	{//# Enumerate endpoints
		USB_INTERFACE_DESCRIPTOR usbInterface;
		WINUSB_PIPE_INFORMATION PipeInfo;
		WinUsb_SetCurrentAlternateSetting(pusbep->fd, 1);
		if (!WinUsb_QueryInterfaceSettings(pusbep->fd, 1, &usbInterface)) {
			dwRet = GetLastError();
			warn_info(dwRet, "failed");
			usbInterface.bNumEndpoints = 0;
		}
		for (i = 0; i < usbInterface.bNumEndpoints; i++) {
			if (!WinUsb_QueryPipe(pusbep->fd, 1, (UCHAR)i, &PipeInfo)) {
				dwRet = GetLastError();
				if (ERROR_NO_MORE_ITEMS == dwRet) break;
				else
					warn_info(dwRet, "failed");
			}
			else {
				dmsg("EP%02X: Type=%u, MaxPacketSize=%u.", PipeInfo.PipeId, PipeInfo.PipeType, PipeInfo.MaximumPacketSize);
			}
		}
	}
#endif
	{//#
		const unsigned param_size  = ROUNDUP(sizeof(struct tsthread_param), 0xF);
		const unsigned buffer_size = ROUNDUP(g_TsBufSize,0xF);
		const unsigned unitSize = ROUNDUP(pusbep->xfer_size ,0x1FF);
		const unsigned unitNum = g_TsBufSize / unitSize;
		const unsigned actlen_size = sizeof(int) * unitNum;
		char *ptr, *buffer_ptr;
		unsigned totalSize = param_size + actlen_size + buffer_size;
		ptr = uHeapAlloc( totalSize );
		if(NULL == ptr) {
			dwRet = GetLastError();
			warn_msg(dwRet,"failed to allocate TS buffer");
			return -1;
		}
		buffer_ptr = ptr;
		ptr += buffer_size;
		ps = (struct tsthread_param*) ptr;
		ps->buffer = buffer_ptr;
		ptr += param_size;
		ps->actual_length = (int*)ptr;
		//ptr += actlen_size;
		ps->buff_unitSize = unitSize;
		ps->buff_num = unitNum;
		ps->actual_length[0] = -1;   //# the first block is not-used
		if (pusbep->endpoint & 0x100) { //# Isochronous
			ps->buff_unitSize = pusbep->xfer_size;
		}
	}
	ps->pUSB = pusbep;
	ps->flags = 0;
	ps->buff_pop = 0;
	
	for(i = 0; i < TS_MaxNumIO; i++) {
		ps->ioContext[i].index = -1;    //# mark it unused
	}
	ps->hTsEvent = CreateEvent ( NULL, FALSE, TRUE, NULL );
	//# USB endpoint
	WinUsb_ResetPipe(pusbep->fd, pusbep->endpoint & 0xFF);
	i = 0x01;
	WinUsb_SetPipePolicy(pusbep->fd, pusbep->endpoint & 0xFF, RAW_IO, sizeof(UCHAR), &i);
	WinUsb_SetPipePolicy(pusbep->fd, pusbep->endpoint & 0xFF, AUTO_CLEAR_STALL, sizeof(UCHAR), &i);
#ifdef INCLUDE_ISOCH_XFER
	ps->hIsochBuffer = NULL;
#else
	if (pusbep->endpoint & 0x100) { //# Isochronous
		warn_msg(0, "Please change to BULK transfer mode :-P");
	}
#endif
#ifdef _DEBUG
	dwRet = sizeof(i);
	WinUsb_GetPipePolicy(ps->pUSB->fd, ps->pUSB->endpoint & 0xFF, MAXIMUM_TRANSFER_SIZE, &dwRet, &i);
	dmsg("MAX_TRANSFER_SIZE=%u", i);
#endif

	ps->hThread = (HANDLE)_beginthreadex( NULL, 0, tsthread, ps,/* スレッド即時実行 0 / 一時停止状態 CREATE_SUSPENDED */ 0, NULL);
	//if(INVALID_HANDLE_VALUE == ps->hThread) {
	if (NULL == ps->hThread) {
		warn_info(errno,"tsthread_create failed");
		uHeapFree(ps->buffer);
		return -1;
	}else{
		SetThreadPriority( ps->hThread, THREAD_PRIORITY_TIME_CRITICAL );
	}
	*tptr = ps;
	return 0;
}

void tsthread_destroy(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;

	tsthread_stop(ptr);
	p->flags |= 0x02;    //# canceled = T
	SetEvent(p->hTsEvent);
	if(WaitForSingleObject(p->hThread, 1000) != WAIT_OBJECT_0) {
		warn_msg(GetLastError(),"tsthread_destroy timeout");
		TerminateThread(p->hThread, 0);
	}

	CloseHandle(p->hTsEvent);
	CloseHandle(p->hThread);
#ifdef INCLUDE_ISOCH_XFER
	if(p->hIsochBuffer) {
		WinUsb_UnregisterIsochBuffer( p->hIsochBuffer );
		p->hIsochBuffer = NULL;
	}
#endif
	uHeapFree(p->buffer);
}

void tsthread_start(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;
	WinUsb_FlushPipe(p->pUSB->fd, p->pUSB->endpoint & 0xFF);
	p->flags |= 0x01;    //# continue = T
	if(p->pUSB->startstopFunc)
		p->pUSB->startstopFunc(p->pUSB->dev, 1);
#ifdef INCLUDE_ISOCH_XFER
	if( (p->pUSB->endpoint & 0x100) && ! p->hIsochBuffer) { //# Isochronous
		if (!WinUsb_RegisterIsochBuffer(p->pUSB->fd, p->pUSB->endpoint & 0xFF, p->buffer, p->buff_unitSize * p->buff_num, &(p->hIsochBuffer))) {
			DWORD dwRet = GetLastError();
			p->hIsochBuffer = NULL;
			warn_info(dwRet, "WinUsb_RegisterIsochBuffer failed");
		}
	}
#endif
	SetEvent(p->hTsEvent);
}

void tsthread_stop(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;

	p->flags &= ~0x01U;    //# continue = F
	if(p->pUSB->startstopFunc)
		p->pUSB->startstopFunc(p->pUSB->dev, 0);
#ifdef INCLUDE_ISOCH_XFER
	if (p->hIsochBuffer) {
		WinUsb_UnregisterIsochBuffer(p->hIsochBuffer);
		p->hIsochBuffer = NULL;
	}
#endif
	if(!(p->pUSB->endpoint & 0x100) ) { //# Bulk
		WinUsb_AbortPipe(p->pUSB->fd, p->pUSB->endpoint & 0xFF);
	}
}

int tsthread_read(const tsthread_ptr tptr, void ** const ptr)
{
	struct tsthread_param* const ps = tptr;
	int i, j;
	i = tsthread_readable(tptr);
	if(0 >= i) return 0;

	j = ps->buff_pop;
	ps->actual_length[ps->buff_pop] = -1;
	if(ptr) {
		*ptr = ps->buffer + (j * ps->buff_unitSize);
		ps->buff_pop = (ps->buff_num - 1 > j) ? j + 1 : 0;
	}else{
		/*
		if (ps->pUSB->endpoint & 0x100) { //# Isochronous
			int rtn;
			for (int k = 0; k < ps->buff_num; k++) {
				rtn = tsthread_readable(tptr);
				if (rtn < 0) {
					break;
				}
			}
		}
		*/

		ps->actual_length[ps->buff_push] = -1;
		ps->buff_pop = ps->buff_push;
		
	}
	return i;
}

int tsthread_readable(const tsthread_ptr tptr)
{
	struct tsthread_param* const ps = tptr;
	int j = ps->buff_pop;

	if(0 > j || ps->buff_num <= j) {  //# bug check
		warn_info(j,"ts.buff_pop Out of range");
		ps->buff_pop = 0;
		return 0;
	}
	do {  //# skip empty blocks
		if(0 != ps->actual_length[j] ) break;
		if(ps->buff_num -1 > j) {
			j++;
		}else{
			j = 0;
		}
	} while(j != ps->buff_pop);
	ps->buff_pop = j;
	return ps->actual_length[j];
}

int tsthread_wait(const tsthread_ptr tptr, const int timeout)
{
	struct tsthread_param* const ps = tptr;
	DWORD dRet = WaitForSingleObject( ps->hTsEvent , timeout );
	if(WAIT_OBJECT_0 == dRet)  return 1;
	else if(WAIT_TIMEOUT == dRet)  return 0;

	warn_info(dRet,"poll failed");
	return -1;
}


/*EOF*/