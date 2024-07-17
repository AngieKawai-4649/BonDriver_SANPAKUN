#pragma once
#include "Util.h"
#include "StringUtil.h"

typedef std::basic_string<_TCHAR> tstring;

#if defined(UNICODE) || defined(_UNICODE)
# define tifstream std::wifstream
//#define locale_japan() {std::locale::global(locale("jananese"));}
#else
# define tifstream std::ifstream
//#define locale_japan() {std::locale::global(locale("ja_JP.UTF-8"));}
#endif

typedef struct _CH_DATA {
	ULONG ulFreq;		// ini�Œ�`���ꂽKHz�ɕϊ��������g��
	ULONG ulTSID;		// ini�Œ�`���ꂽTSID
	tstring tszName;	// ini�Œ�`���ꂽ�`�����l����
	//=�I�y���[�^�[�̏���
	_CH_DATA(void) {
		tszName = _T("");
		ulFreq = 0;
		ulTSID = 0;
	};
	~_CH_DATA(void) {
	}
	_CH_DATA& operator= (const _CH_DATA& o) {
		tszName = o.tszName;
		ulFreq = o.ulFreq;
		ulTSID = o.ulTSID;
		return *this;
	}
}CH_DATA;

typedef struct _SPACE_DATA {

	tstring tsKeyword;	// ini�Œ�`���ꂽSpaceXXX ��Space���������������Z�b�g
	CHAR cSpace;		// ini�Œ�`���ꂽ�`�����l���X�y�[�X 0�`X �g�p���Ȃ�����-1
	UCHAR ucChannelCnt;	// ini�Œ�`���ꂽ�`�����l����
	UCHAR ucUseTuner:1;	// UHF CATV 0, BS/CS 1
	tstring tszName;	// ini�Œ�`���ꂽ�`���[�j���O�X�y�[�X����
	vector<CH_DATA>chVec;

	//=�I�y���[�^�[�̏���
	_SPACE_DATA(void) {
		tsKeyword = _T("");
		cSpace = -1;
		ucChannelCnt = 0;
		ucUseTuner = 0;
		tszName = _T("");
		
	};
	~_SPACE_DATA(void) {
	}
	_SPACE_DATA& operator= (const _SPACE_DATA& o) {
		tsKeyword = o.tsKeyword;
		cSpace = o.cSpace;
		ucChannelCnt = o.ucChannelCnt;
		ucUseTuner = o.ucUseTuner;
		tszName = o.tszName;
		chVec = o.chVec;
		return *this;
	}

}SPACE_DATA;


class CParseChSet
{
public:
	vector<SPACE_DATA> spaceVec;

public:
	CParseChSet(void);
	~CParseChSet(void);
	BOOL ParseText(LPCTSTR filePath );

protected:

private:
	BOOL Parse1Line(tstring parseLine, SPACE_DATA& info);
	BOOL Parse1Line(tstring parseLine, CH_DATA& chInfo);
};

/*
class SearchSpaceData
{
	LONG Num;
public:
	SearchSpaceData(LONG num) : Num(num) {}
	bool operator()(const SPACE_DATA& s) const {
		return Num == s.lSpace;
	}
};
vector<SPACE_DATA>::iterator itr = find_if(m_chSet.spaceVec.begin(), m_chSet.spaceVec.end(), SearchSpaceData(dwSpace));
*/
