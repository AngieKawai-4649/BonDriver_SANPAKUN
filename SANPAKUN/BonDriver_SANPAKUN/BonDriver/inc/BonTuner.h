/* SunPTV-USB   (c) 2016 trinity19683
  BonTuner.DLL (MS-Windows)
  BonTuner.h
  2016-01-23

  customized by AngieKawai_4649
  2024-05-01
*/
#pragma once

#include "IBonDriver2.h"
#include "ParseChSet.h"

#include "IB25Decoder.h"

extern "C" {
#include "em287x.h"
#include "tsthread.h"
}

class CBonTuner : public IBonDriver2
{
public:
	CBonTuner();
	virtual ~CBonTuner();

//# IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);

	const BOOL SetChannel(const BYTE bCh);
	const float GetSignalLevel(void);

	const DWORD WaitTsStream(const DWORD dwTimeOut = 0);
	const DWORD GetReadyCount(void);

	const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain);
	const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);

	void PurgeTsStream(void);

//# IBonDriver2
	LPCTSTR GetTunerName(void);

	const BOOL IsTunerOpening(void);
	
	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);

	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	
	const DWORD GetCurSpace(void);
	const DWORD GetCurChannel(void);

	void Release(void);

	static CBonTuner * m_pThis;
	static HINSTANCE m_hModule;

protected:
	DWORD m_dwCurSpace;
	DWORD m_dwCurChannel;

private:
	CParseChSet m_chSet;	// iniファイル データ格納エリア
	HANDLE m_hDev;
	HANDLE m_hUsbDev;
	struct usb_endpoint_st  m_USBEP;
	em287x_state pDev;
	void* demodDev;
	void* tunerDev[2];
	int m_selectedTuner;	// 現在使用しているチューナー 0 地デジ 1 BS/CS
	int m_iTunerID;			// BonDriver_SANPAKUN-X.ini のX（チューナー番号）をセット 0スタート 指定無しは-1
	tsthread_ptr tsthr;

	DWORD m_dwSetTunerOpenRetryMaxCnt;		// チューナーオープンリトライ回数 default 5
	DWORD m_dwSetTunerOpenRetryWaitTime;	// チューナーオープンリトライ間隔（ミリ秒で指定） default 50

	DWORD m_dwSetTeraResetDemoWaitTime;		// 地デジ：チューナー周波数を設定してからデモジュレーターをリセットするまでの時間（ミリ秒） default 30
	DWORD m_dwSetBsCsResetDemoWaitTime;		// BS/CS：チューナー周波数を設定してからデモジュレーターをリセットするまでの時間（ミリ秒） default 30

	DWORD m_dwSetDemoWaitTime;				// デモジュレーターをリセットしてから次の動作をするまでの待ち時間（ミリ秒） default 50

	DWORD m_dwSetBsCsDemoStreamMaxCnt;		// BS/CSデモジュレーターのストリームを切り替えるリトライ回数 default 20
	DWORD m_dwSetBsCsDemoStreamWaitTime;	// BS/CSデモジュレーターのストリームを切り替えるリトライ間隔（ミリ秒）default 40

	DWORD m_dwSetDevStreamMaxCnt;			// ストリームが安定するまでに要するリトライ回数 default 12
	DWORD m_dwSetDevStreamWaitTime;			// ストリームが安定するまでのリトライ間隔（ミリ秒）default 40

	HMODULE m_hDecoderModule;
	IB25Decoder2* (*m_CreateInstance)();
	IB25Decoder2* m_b25decoderIF;
	BYTE m_emm;
	BYTE m_nullPacket;
	BYTE m_multi2Round;

	UINT m_CardResponseTime;

};
