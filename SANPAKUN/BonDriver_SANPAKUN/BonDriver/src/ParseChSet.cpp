#include "StdAfx.h"
#include "ParseChSet.h"

#include <fstream>
#include <algorithm>
#include <string>

using namespace std;

CParseChSet::CParseChSet(void)
{
}

CParseChSet::~CParseChSet(void)
{
	for (SPACE_DATA& s : this->spaceVec) {
		s.chVec.clear();
		s.chVec.shrink_to_fit();
	}
	this->spaceVec.clear();
	this->spaceVec.shrink_to_fit();
}

// ini�t�@�C���ǂݍ���
BOOL CParseChSet::ParseText(LPCTSTR filePath)
{
	if (filePath == NULL) {
		return FALSE;
	}

	this->spaceVec.clear();
	this->spaceVec.shrink_to_fit();

	::locale::global(locale(""));

	tifstream iniFile(filePath);	// ini�t�@�C���I�[�v��
	if (!iniFile.is_open()) {
		return FALSE;
	}

	tstring parseLine;
	BOOL rtn = FALSE;

	size_t iFind;
	// ini����`���[�j���O�X�y�[�X��ǂݍ��ݓW�J����
	while (getline(iniFile, parseLine)) {	// 1�s���ǂݍ���
		Replace(parseLine, _T("\t"), _T(""));	// tab �� sp ���J�b�g
		Replace(parseLine, _T(" "), _T(""));

		// �R�����g�s��ǂݔ�΂�
		 if( (iFind = parseLine.find(_T(";"), 0)) != tstring::npos ) {
			parseLine.erase(iFind);
		}

		if (iFind = parseLine.find(_T("Space"), 0) != tstring::npos) {
			SPACE_DATA item;
			if (Parse1Line(parseLine, item) == TRUE) {
				if (item.cSpace != -1) {
					item.chVec.reserve(item.ucChannelCnt);	//	�\��vector�̈���m�ۂ���
					this->spaceVec.emplace_back(item);
				}
				rtn = TRUE;
			}
		}
	}
	// �`���[�j���O�X�y�[�X��1��������Ȃ��������G���[�Ƃ���
	if (!rtn) {
		return rtn;
	}

	// �t�@�C���|�C���^��擪�ɖ߂��`�����l������ǂݍ���
	iniFile.clear();
	iniFile.seekg(0, ios_base::beg);
	tstring channelName;
	tstring strLeft = _T("");
	tstring strRight = _T("");

	while (getline(iniFile, parseLine)) {	// 1�s���ǂݍ���
		
		Replace(parseLine, _T("\t"), _T(""));		// tab �� sp ���J�b�g
		Replace(parseLine, _T(" "), _T(""));

		// �R�����g�s��ǂݔ�΂�
		if ((iFind = parseLine.find(_T(";"), 0)) != tstring::npos) {
			parseLine.erase(iFind);
		}

		for (SPACE_DATA& spaceData : this->spaceVec) {
			if (spaceData.cSpace != -1) {	// ini�Œ�`���ꂽ�`���[�j���O�X�y�[�X-1�͑ΏۊO
				//  �s���L�[���[�h�Ō���
				Separate(parseLine, _T(","), strLeft, strRight);
				if( strLeft.compare(spaceData.tsKeyword) == 0){
					CH_DATA chData;
					if (Parse1Line(strRight, chData) == TRUE) {
						spaceData.chVec.emplace_back(chData);
					}
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseChSet::Parse1Line(tstring parseLine, SPACE_DATA& info)
{
	if (parseLine.empty() == true) {
		return FALSE;
	}
	Replace(parseLine, _T("Space"), _T(""));
	tstring strBuff = _T("");

	Separate(parseLine, _T(","), strBuff, parseLine);
	// ini�Œ�`���ꂽSpaceXXX ��Space�������������i�L�[���[�h�j
	info.tsKeyword = strBuff;

	Separate(parseLine, _T(","), strBuff, parseLine);
	// �`���[�j���O�X�y�[�X�ԍ�
	size_t s;
	LONG is = stol(strBuff, &s);
	if (strBuff.size() == s) {
		info.cSpace = static_cast<CHAR>(is);
	}
	else {
		return FALSE;
	}

	Separate(parseLine, _T(","), strBuff, parseLine);
	// �`�����l����
	is = stoul(strBuff, &s);
	if (strBuff.size() == s) {
		info.ucChannelCnt = static_cast<UCHAR>(is);
	}
	else {
		return FALSE;
	}

	Separate(parseLine, _T(","), strBuff, parseLine);
	// �g�p�`���[�i�[
	is = stoul(strBuff, &s);
	if (strBuff.size() == s) {
		info.ucUseTuner = static_cast<UCHAR>(is);
	}
	else {
		return FALSE;
	}
	
	Separate(parseLine, _T(","), strBuff, parseLine);
	// �`���[�j���O�X�y�[�X����
	info.tszName = strBuff;
	
	return TRUE;
}

BOOL CParseChSet::Parse1Line(tstring parseLine, CH_DATA& chInfo)
{
	if (parseLine.empty() == true) {
		return FALSE;
	}
	tstring strBuff = _T("");

	Separate(parseLine, _T(","), strBuff, parseLine);
	// ���g��
	Replace(strBuff, _T("MHz"), _T(""));	// MHz���J�b�g
	Replace(strBuff, _T(" "), _T(""));		// �X�y�[�X���J�b�g
	size_t s;
	double freq = stod(strBuff, &s);
	if (strBuff.size() == s) {
		chInfo.ulFreq = static_cast<ULONG>(freq * 1000);
	}
	else {
		return FALSE;
	}
	
	Separate(parseLine, _T(","), strBuff, parseLine);
	//Ch��
	chInfo.tszName = strBuff;

	Separate(parseLine, _T(","), strBuff, parseLine);
	// TSID
	ULONG tsid = stoul(strBuff, &s, 0);
	if (strBuff.size() == s) {
		chInfo.ulTSID = tsid;
	}
	else {
		return FALSE;
	}

	return TRUE;
}