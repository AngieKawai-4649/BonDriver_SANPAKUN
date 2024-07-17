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
	CParseChSet m_chSet;	// ini�t�@�C�� �f�[�^�i�[�G���A
	HANDLE m_hDev;
	HANDLE m_hUsbDev;
	struct usb_endpoint_st  m_USBEP;
	em287x_state pDev;
	void* demodDev;
	void* tunerDev[2];
	int m_selectedTuner;	// ���ݎg�p���Ă���`���[�i�[ 0 �n�f�W 1 BS/CS
	int m_iTunerID;			// BonDriver_SANPAKUN-X.ini ��X�i�`���[�i�[�ԍ��j���Z�b�g 0�X�^�[�g �w�薳����-1
	tsthread_ptr tsthr;

	DWORD m_dwSetTunerOpenRetryMaxCnt;		// �`���[�i�[�I�[�v�����g���C�� default 5
	DWORD m_dwSetTunerOpenRetryWaitTime;	// �`���[�i�[�I�[�v�����g���C�Ԋu�i�~���b�Ŏw��j default 50

	DWORD m_dwSetTeraResetDemoWaitTime;		// �n�f�W�F�`���[�i�[���g����ݒ肵�Ă���f���W�����[�^�[�����Z�b�g����܂ł̎��ԁi�~���b�j default 30
	DWORD m_dwSetBsCsResetDemoWaitTime;		// BS/CS�F�`���[�i�[���g����ݒ肵�Ă���f���W�����[�^�[�����Z�b�g����܂ł̎��ԁi�~���b�j default 30

	DWORD m_dwSetDemoWaitTime;				// �f���W�����[�^�[�����Z�b�g���Ă��玟�̓��������܂ł̑҂����ԁi�~���b�j default 50

	DWORD m_dwSetBsCsDemoStreamMaxCnt;		// BS/CS�f���W�����[�^�[�̃X�g���[����؂�ւ��郊�g���C�� default 20
	DWORD m_dwSetBsCsDemoStreamWaitTime;	// BS/CS�f���W�����[�^�[�̃X�g���[����؂�ւ��郊�g���C�Ԋu�i�~���b�jdefault 40

	DWORD m_dwSetDevStreamMaxCnt;			// �X�g���[�������肷��܂łɗv���郊�g���C�� default 12
	DWORD m_dwSetDevStreamWaitTime;			// �X�g���[�������肷��܂ł̃��g���C�Ԋu�i�~���b�jdefault 40

	HMODULE m_hDecoderModule;
	IB25Decoder2* (*m_CreateInstance)();
	IB25Decoder2* m_b25decoderIF;
	BYTE m_emm;
	BYTE m_nullPacket;
	BYTE m_multi2Round;

	UINT m_CardResponseTime;

};
