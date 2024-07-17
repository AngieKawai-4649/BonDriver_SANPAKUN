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

// iniファイル読み込み
BOOL CParseChSet::ParseText(LPCTSTR filePath)
{
	if (filePath == NULL) {
		return FALSE;
	}

	this->spaceVec.clear();
	this->spaceVec.shrink_to_fit();

	::locale::global(locale(""));

	tifstream iniFile(filePath);	// iniファイルオープン
	if (!iniFile.is_open()) {
		return FALSE;
	}

	tstring parseLine;
	BOOL rtn = FALSE;

	size_t iFind;
	// iniからチューニングスペースを読み込み展開する
	while (getline(iniFile, parseLine)) {	// 1行ずつ読み込む
		Replace(parseLine, _T("\t"), _T(""));	// tab と sp をカット
		Replace(parseLine, _T(" "), _T(""));

		// コメント行を読み飛ばす
		 if( (iFind = parseLine.find(_T(";"), 0)) != tstring::npos ) {
			parseLine.erase(iFind);
		}

		if (iFind = parseLine.find(_T("Space"), 0) != tstring::npos) {
			SPACE_DATA item;
			if (Parse1Line(parseLine, item) == TRUE) {
				if (item.cSpace != -1) {
					item.chVec.reserve(item.ucChannelCnt);	//	予めvector領域を確保する
					this->spaceVec.emplace_back(item);
				}
				rtn = TRUE;
			}
		}
	}
	// チューニングスペースが1つも見つからなかった時エラーとする
	if (!rtn) {
		return rtn;
	}

	// ファイルポインタを先頭に戻しチャンネル情報を読み込む
	iniFile.clear();
	iniFile.seekg(0, ios_base::beg);
	tstring channelName;
	tstring strLeft = _T("");
	tstring strRight = _T("");

	while (getline(iniFile, parseLine)) {	// 1行ずつ読み込む
		
		Replace(parseLine, _T("\t"), _T(""));		// tab と sp をカット
		Replace(parseLine, _T(" "), _T(""));

		// コメント行を読み飛ばす
		if ((iFind = parseLine.find(_T(";"), 0)) != tstring::npos) {
			parseLine.erase(iFind);
		}

		for (SPACE_DATA& spaceData : this->spaceVec) {
			if (spaceData.cSpace != -1) {	// iniで定義されたチューニングスペース-1は対象外
				//  行をキーワードで検索
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
	// iniで定義されたSpaceXXX のSpaceを取った文字列（キーワード）
	info.tsKeyword = strBuff;

	Separate(parseLine, _T(","), strBuff, parseLine);
	// チューニングスペース番号
	size_t s;
	LONG is = stol(strBuff, &s);
	if (strBuff.size() == s) {
		info.cSpace = static_cast<CHAR>(is);
	}
	else {
		return FALSE;
	}

	Separate(parseLine, _T(","), strBuff, parseLine);
	// チャンネル数
	is = stoul(strBuff, &s);
	if (strBuff.size() == s) {
		info.ucChannelCnt = static_cast<UCHAR>(is);
	}
	else {
		return FALSE;
	}

	Separate(parseLine, _T(","), strBuff, parseLine);
	// 使用チューナー
	is = stoul(strBuff, &s);
	if (strBuff.size() == s) {
		info.ucUseTuner = static_cast<UCHAR>(is);
	}
	else {
		return FALSE;
	}
	
	Separate(parseLine, _T(","), strBuff, parseLine);
	// チューニングスペース名称
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
	// 周波数
	Replace(strBuff, _T("MHz"), _T(""));	// MHzをカット
	Replace(strBuff, _T(" "), _T(""));		// スペースをカット
	size_t s;
	double freq = stod(strBuff, &s);
	if (strBuff.size() == s) {
		chInfo.ulFreq = static_cast<ULONG>(freq * 1000);
	}
	else {
		return FALSE;
	}
	
	Separate(parseLine, _T(","), strBuff, parseLine);
	//Ch名
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