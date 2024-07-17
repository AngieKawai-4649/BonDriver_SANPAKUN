/* SunPTV-USB   (c) 2016 trinity19683
  BonTuner.DLL (MS-Windows)
  BonTuner.cpp
  2016-02-10

  customized by AngieKawai_4649
  2024-05-01
*/

#include "stdafx.h"

//#include <fstream>

#include "BonTuner.h"
#include "usbdevfile.h"
using namespace std;
extern "C" {
#include "tc90522.h"
#include "tda20142.h"
#include "mxl136.h"

}

#pragma warning( push )
#pragma warning( disable : 4273 )
extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{ return (CBonTuner::m_pThis)? CBonTuner::m_pThis : ((IBonDriver *) new CBonTuner); }
#pragma warning( pop )

//# initialize static member variables
CBonTuner * CBonTuner::m_pThis = NULL;
HINSTANCE CBonTuner::m_hModule = NULL;

//ofstream debfile("R:\\sanpakun.txt");

EXTERN_C UINT g_TsBufSize;

/*
static BYTE* tsBlock = NULL;
static size_t tsBlockSize = 96256; // 188 * 512
*/



CBonTuner::CBonTuner():
	m_dwCurSpace(123),
	m_dwCurChannel(0),
	m_hDev(NULL),
	m_hUsbDev(NULL),
	m_iTunerID(-1),
	m_selectedTuner(-1),
	m_dwSetTunerOpenRetryMaxCnt(5),
	m_dwSetTunerOpenRetryWaitTime(50),
	m_dwSetTeraResetDemoWaitTime(30),
	m_dwSetBsCsResetDemoWaitTime(30),
	m_dwSetDemoWaitTime(50),
	m_dwSetBsCsDemoStreamMaxCnt(20),
	m_dwSetBsCsDemoStreamWaitTime(40),
	m_dwSetDevStreamMaxCnt(12),
	m_dwSetDevStreamWaitTime(40),

	pDev(NULL),
	demodDev(NULL),
	tsthr(NULL),
	tunerDev{NULL,NULL},
	m_USBEP{0,},
	m_chSet(),

	m_hDecoderModule(NULL),
	m_CreateInstance(NULL),
	m_b25decoderIF(NULL),
	m_emm(0),
	m_nullPacket(0),
	m_multi2Round(0),
	m_CardResponseTime(0)
{
	//locale(Japanese);
	m_pThis = this;
}

CBonTuner::~CBonTuner()
{
	CloseTuner();
	m_pThis = NULL;
}

const BOOL CBonTuner::OpenTuner()
{
	if (IsTunerOpening())return FALSE;

	TCHAR strExePath[_MAX_PATH] = _T("");
	GetModuleFileName(m_hModule, strExePath, _MAX_PATH);

	TCHAR szPath[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFname[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	_tsplitpath_s(strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT);
	_tmakepath_s(szPath, _MAX_PATH, szDrive, szDir, NULL, NULL);

	if (_tcslen(szFname) > _tcslen(_T("BonDriver_SANPAKUN-"))) {
		m_iTunerID = _ttoi(szFname + _tcslen(_T("BonDriver_SANPAKUN-")));
	}

	TCHAR ini_file_path[MAX_PATH];
	_tcscpy_s(ini_file_path, MAX_PATH, szPath);
	_tcsncat_s(ini_file_path, MAX_PATH - _tcslen(ini_file_path), szFname, _MAX_FNAME);
	_tcsncat_s(ini_file_path, MAX_PATH - _tcslen(ini_file_path), _T(".ini"), sizeof(_T(".ini")) / sizeof(TCHAR));

	// iniファイルからパラメータを読み込む
	m_dwSetTunerOpenRetryMaxCnt		= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetTunerOpenRetryMaxCnt"),		5,  ini_file_path);
	m_dwSetTunerOpenRetryWaitTime	= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetTunerOpenRetryWaitTime"),	50, ini_file_path);
	m_dwSetTeraResetDemoWaitTime	= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetTeraResetDemoWaitTime"),	30, ini_file_path);
	m_dwSetBsCsResetDemoWaitTime	= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetBsCsResetDemoWaitTime"),	30, ini_file_path);
	m_dwSetDemoWaitTime				= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetDemoWaitTime"),				50, ini_file_path);
	m_dwSetBsCsDemoStreamMaxCnt		= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetBsCsDemoStreamMaxCnt"),		20, ini_file_path);
	m_dwSetBsCsDemoStreamWaitTime	= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetBsCsDemoStreamWaitTime"),	40, ini_file_path);
	m_dwSetDevStreamMaxCnt			= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetDevStreamMaxCnt"),			12, ini_file_path);
	m_dwSetDevStreamWaitTime		= GetPrivateProfileInt(_T("PARAMETERS"), _T("SetDevStreamWaitTime"),		40, ini_file_path);

	g_TsBufSize = GetPrivateProfileInt(_T("PARAMETERS"), _T("TsBufSize"), 3670016, ini_file_path);
	m_CardResponseTime = GetPrivateProfileInt(_T("PARAMETERS"), _T("CardResponseTime"), 100, ini_file_path);

	// デコーダーdllをロードする
	TCHAR decoder[MAX_PATH]{};
	GetPrivateProfileString(_T("DECODER"), _T("InternalReader"), NULL, decoder, MAX_PATH, ini_file_path);
	if (_tcslen(decoder) > 0) {
		LPTSTR dec, emm, np, mr;
		dec = decoder;
		emm = _tcschr(decoder, _T(','));
		if (emm == NULL) {
			return(FALSE);
		}
		*emm = _T('\0');
		emm++;
		m_emm = static_cast<BYTE>(_tcstoul(emm, &np, 10));
		if (*np != _T(',')) {
			return(FALSE);
		}
		*np = _T('\0');
		np++;
		m_nullPacket = static_cast<BYTE>(_tcstoul(np, &mr, 10));
		if (*mr != _T(',')) {
			return(FALSE);
		}
		*mr = _T('\0');
		mr++;
		m_multi2Round = static_cast<BYTE>(_tcstoul(mr, &np, 10));
		if (*np != _T('\0')) {
			return(FALSE);
		}
		 
		m_hDecoderModule = LoadLibrary(dec);
		if (m_hDecoderModule == NULL) {
			return(FALSE);
		}
		m_CreateInstance = (IB25Decoder2 * (*)())::GetProcAddress(m_hDecoderModule, "CreateB25Decoder2");
		if (m_CreateInstance == NULL) {
			FreeLibrary(m_hDecoderModule);
			return(FALSE);
		}
		m_b25decoderIF = m_CreateInstance();
		if (m_b25decoderIF == NULL) {
			FreeLibrary(m_hDecoderModule);
			return(FALSE);
		}
	}

	// iniファイルからチューニングスペース、チャンネル情報を読み込む
	if (!m_chSet.ParseText(ini_file_path)) {
		CloseTuner();
		return FALSE;
	}

	BOOL Success = FALSE;
	HANDLE hDev;
	for (DWORD i = 0; i < m_dwSetTunerOpenRetryMaxCnt; i++) {
		Sleep(m_dwSetTunerOpenRetryWaitTime);
		// em287x   : usb interface chip
		// tc90522  : demodulator
		// mxl136   : terrestrial tuner
		// tda20142 : bs/cs tuner

		// usb tuner device search
		if ((hDev = usbdevfile_alloc(m_iTunerID)) != NULL) {
			m_hDev = hDev;
			// Win usb initialize
			if ((hDev = usbdevfile_init(m_hDev)) != NULL) {
				m_hUsbDev = hDev;
				//# usb interface initialize
				m_USBEP.fd = m_hUsbDev;
				if (em287x_create(&pDev, &m_USBEP) == 0) {

					struct i2c_device_st* pI2C;
					// demodulator memory allocate
					if (tc90522_create(&demodDev) == 0) {
						pI2C = (struct i2c_device_st*)tc90522_i2c_ptr(demodDev);
						pI2C->addr = 0x20;
						// usb interface attach
						em287x_attach(pDev, pI2C);

						if (tc90522_init(demodDev) == 0) {
							// tuner 0 terrestrial

							if (mxl136_create(&tunerDev[0]) == 0) {
								pI2C = (struct i2c_device_st*)mxl136_i2c_ptr(tunerDev[0]);
								pI2C->addr = 0xc0;

								tc90522_attach(demodDev, 0, pI2C);

								if (mxl136_init(tunerDev[0]) == 0) {
									// tuner 1 BS/CS
									if (tda20142_create(&tunerDev[1]) == 0) {
										pI2C = (struct i2c_device_st*)tda20142_i2c_ptr(tunerDev[1]);
										pI2C->addr = 0xa8;

										tc90522_attach(demodDev, 1, pI2C);

										if (tda20142_init(tunerDev[1]) == 0) {
											// TS receive thread

											if (tsthread_create(&tsthr, &m_USBEP) == 0) {
												Success = TRUE;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		CloseTuner();
	}

	if (m_b25decoderIF != NULL) {
		m_b25decoderIF->usbHandleExport(m_hUsbDev, m_CardResponseTime);
		if (Success = m_b25decoderIF->Initialize()) {
			m_b25decoderIF->EnableEmmProcess(m_emm);
			m_b25decoderIF->DiscardNullPacket(m_nullPacket);
			m_b25decoderIF->SetMulti2Round(m_multi2Round);
			m_b25decoderIF->DiscardScramblePacket(FALSE);
		}
		else {
			CloseTuner();
		}
	}

	/*
	tsBlock = (BYTE *)VirtualAlloc(NULL, tsBlockSize, MEM_COMMIT, PAGE_READWRITE);
	if (tsBlock == NULL) {
		Success = FALSE;
	}
	*/

	return(Success);
}

void CBonTuner::CloseTuner()
{
	if(tsthr) {
		tsthread_stop(tsthr);
		tsthread_destroy(tsthr);
		tsthr = NULL;
	}
	if(tunerDev[0]) {
		mxl136_destroy(tunerDev[0]);
		tunerDev[0] = NULL;
	}
	if(tunerDev[1]) {
		tda20142_destroy(tunerDev[1]);
		tunerDev[1] = NULL;
	}
	if(demodDev) {
		tc90522_destroy(demodDev);
		demodDev = NULL;
	}
	if(pDev) {
		em287x_destroy(pDev);
		pDev = NULL;
	}

	if (m_b25decoderIF != NULL) {
		m_b25decoderIF->Release();
		m_b25decoderIF = NULL;
	}
	if (m_hDecoderModule) {
		FreeLibrary(m_hDecoderModule);
		m_hDecoderModule = NULL;
	}

	if(m_hUsbDev) {
		usbdevfile_free(m_hUsbDev);
		m_hUsbDev = NULL;
	}
	if(m_hDev) {
		CloseHandle( m_hDev );
		m_hDev = NULL;
	}
}

const BOOL CBonTuner::SetChannel(const BYTE bCh)
{
	//# compatible with IBonDriver
	return TRUE;
}

const float CBonTuner::GetSignalLevel(void)
{
	if(0 > m_selectedTuner || (! demodDev) ) return -3.1f;
	unsigned statData[4];
	if(tc90522_readStatistic(demodDev, m_selectedTuner, statData) ) return -3.2f;
	return statData[1] * 0.01f;
}

const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
	const int remainTime = (dwTimeOut < 0x10000000) ? dwTimeOut : 0x10000000;
	if(! tsthr) return WAIT_FAILED;

	const int r = tsthread_wait(tsthr, remainTime);
	if(0 > r)  return WAIT_FAILED;
	else if(0 < r)  return WAIT_OBJECT_0;
	else  return WAIT_TIMEOUT;
}

const DWORD CBonTuner::GetReadyCount()
{//# number of call GetTsStream()
	if (!tsthr) {
		return 0;
	}
	const int ret = tsthread_readable(tsthr);
	return (ret > 0) ? 1 : 0;
}

const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	BYTE *pSrc = NULL;
	if(GetTsStream(&pSrc, pdwSize, pdwRemain)){
		if(*pdwSize) ::CopyMemory(pDst, pSrc, *pdwSize);
		return TRUE;
	}
	return FALSE;
}

const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	if (!tsthr) {
		return FALSE;
	}

	const int ret = tsthread_readable(tsthr);
	if(ret <= 0) {
		//# no readable data
		*pdwSize = 0;
		*pdwRemain = 0;
		return TRUE;
	}

	*pdwSize = tsthread_read(tsthr, (void**)ppDst);
	*pdwRemain = GetReadyCount();
	if (m_b25decoderIF != NULL && *pdwSize > 0) {
		m_b25decoderIF->Decode(*ppDst, *pdwSize, ppDst, pdwSize);
	}

	return TRUE;
/*
	BYTE* p;
	DWORD size = 0;
	int ret;
	ret = tsthread_readable(tsthr);
	if (ret <= 0) {
		*pdwSize = 0;
		*pdwRemain = 0;
		return TRUE;
	}


	while (1) {
		ret = tsthread_readable(tsthr);
		if (ret <= 0) {
			*pdwSize = size;
			break;
		}
		else if (((size_t)size + ret) > tsBlockSize) {
			*pdwSize = size;
			break;
		}else{
			ret = tsthread_read(tsthr, (void**) & p);
			memcpy(tsBlock + size, p, ret);
			size += ret;
		}
	}

	if (m_b25decoderIF != NULL && *pdwSize > 0) {
		m_b25decoderIF->Decode(tsBlock, *pdwSize, ppDst, pdwSize);
	}
	//*ppDst = tsBlock;
	*pdwRemain = GetReadyCount();
	return TRUE;
*/
}

void CBonTuner::PurgeTsStream()
{
	if(! tsthr) return;
	//# purge available data in TS buffer
	tsthread_read(tsthr, NULL);
}

void CBonTuner::Release()  //# release the instance
{
	delete this;
}

LPCTSTR CBonTuner::GetTunerName(void)
{
	return _T("SANPAKUN");
}

const BOOL CBonTuner::IsTunerOpening(void)
{
	return m_hUsbDev ? TRUE : FALSE;
}

LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
	
	vector<SPACE_DATA>::iterator itr = find_if(m_chSet.spaceVec.begin(), m_chSet.spaceVec.end(), [dwSpace](const SPACE_DATA& s) {return(s.cSpace == dwSpace); });
	return( (itr == m_chSet.spaceVec.end()) ? NULL : itr->tszName.c_str());

}

LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	vector<SPACE_DATA>::iterator itr = find_if(m_chSet.spaceVec.begin(), m_chSet.spaceVec.end(), [dwSpace](const SPACE_DATA& s) {return(s.cSpace == dwSpace); });
	if (itr == m_chSet.spaceVec.end()) {
		return NULL;
	}
	else {
		return((itr->chVec.size() <= dwChannel) ? NULL : itr->chVec[dwChannel].tszName.c_str());
	}
}

const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	vector<SPACE_DATA>::iterator itr = find_if(m_chSet.spaceVec.begin(), m_chSet.spaceVec.end(), [dwSpace](const SPACE_DATA& s) {return(s.cSpace == dwSpace); });
	if (itr == m_chSet.spaceVec.end()) {
		return FALSE;
	}

	if (itr->chVec.size() <= dwChannel) {
		return FALSE;
	}

	// チューニングスペース、チャンネル共に変更が無い場合、この関数は呼び出されないが念の為
	if (!(m_dwCurSpace == dwSpace && dwChannel == m_dwCurChannel)) {
	
		if (tsthr) {
			tsthread_stop(tsthr);
		}

		// change tuner
		if (itr->ucUseTuner != m_selectedTuner) {
			if (tc90522_selectDevice(demodDev, itr->ucUseTuner)) {
				return FALSE;
			}
			if (itr->ucUseTuner == 1) {
				mxl136_sleep(tunerDev[0]);
			}
			else {
				mxl136_wakeup(tunerDev[0]);
			}
		}
		bool hasStream = TRUE;
		// bs/cs tuner
		if (itr->ucUseTuner == 1) {
			if (tda20142_setFreq(tunerDev[1], itr->chVec[dwChannel].ulFreq)) {
				return FALSE;
			}
			Sleep(m_dwSetBsCsResetDemoWaitTime);
			if (tc90522_resetDemod(demodDev, itr->ucUseTuner)) {
				return FALSE;
			}
			
			Sleep(m_dwSetDemoWaitTime);

			for (DWORD i = 0; i < m_dwSetBsCsDemoStreamMaxCnt; i++) {
				int ret = tc90522_selectStream(demodDev, itr->ucUseTuner, itr->chVec[dwChannel].ulTSID );
				if (ret == 0) {
					hasStream = TRUE;
					break;
				}
				else if (ret < 0) {
					hasStream = FALSE;
					break;
				}
				else {
					Sleep(m_dwSetBsCsDemoStreamWaitTime);
				}
			}
			if (!hasStream) {
				return FALSE;
			}
		}
		// terrestrial tuner
		else {
			if (mxl136_setFreq(tunerDev[0], itr->chVec[dwChannel].ulFreq)) {
				return FALSE;
			}
			Sleep(m_dwSetTeraResetDemoWaitTime);
			if (tc90522_resetDemod(demodDev, itr->ucUseTuner)) {
				return FALSE;
			}
			Sleep(m_dwSetDemoWaitTime);
		}

		//PurgeTsStream();
		/*
		if (tsthr && hasStream) {
			tsthread_start(tsthr);
		}
		*/

		for (DWORD i = 0; i < m_dwSetDevStreamMaxCnt; i++) {
			unsigned statData[4];
			Sleep(m_dwSetDevStreamWaitTime);
			if (tc90522_readStatistic(demodDev, itr->ucUseTuner, statData)) {
				continue;
			}
			if (statData[0] & 0x10) {
				break;
			}
		}
		PurgeTsStream();
		//Sleep(200);
		/*
		tsthread_stop(tsthr);
		*/
		if (m_b25decoderIF) {
			m_b25decoderIF->Reset();
		}

		Sleep(200);
		tsthread_start(tsthr);


		// set variables
		m_dwCurSpace = dwSpace;
		m_dwCurChannel = dwChannel;
		m_selectedTuner = itr->ucUseTuner;
	
	}
	else {
	}
	
	return TRUE;
}

const DWORD CBonTuner::GetCurSpace(void) {
	return m_dwCurSpace ;
}

const DWORD CBonTuner::GetCurChannel(void) {
	return m_dwCurChannel;
}


/*EOF*/