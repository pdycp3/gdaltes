#include "ExportFrameShape.h"

#include "cpl_string.h"
#include "gdal.h"
#include "gdal_priv.h"
#include "gdal_utils_priv.h"
#include "cpl_conv.h"


#include <fstream>
#include <iomanip>

using namespace std;

//将double值转成string
inline string DoubleToString(const double& dfnum, const int nAcc)
{
	char chcode[20] = { 0 };

	if (nAcc == 1)
	{
		sprintf_s(chcode, 20, "%.1lf", dfnum);
	}
	if (nAcc == 2)
	{
		sprintf_s(chcode, 20, "%.2lf", dfnum);
	}

	return string(chcode);
}

FrameImage::FrameImage() {}

FrameImage::~FrameImage() {}

//初始化参数列表
bool FrameImage::InitialParam(
    double dfColLeft, double dfRowUp, double dfColRight, double dfRowDown, int nScaleFactor, int nExtenScale)
{
	m_ColLeft     = dfColLeft;
	m_RowUp       = dfRowUp;
	m_ColRight    = dfColRight;
	m_RowDown     = dfRowDown;
	m_ScaleFactor = nScaleFactor;
	m_ExtenScale  = nExtenScale;
	return true;
}

bool FrameImage::InitialParam(double dfColLeft, double dfRowUp, double dfColRight, double dfRowDown, int nScaleFactor)
{
	m_ColLeft     = dfColLeft;
	m_RowUp       = dfRowUp;
	m_ColRight    = dfColRight;
	m_RowDown     = dfRowDown;
	m_ScaleFactor = nScaleFactor;
	return true;
}

/*
@brief convert the double coordinate into degree,minite,second;
*/
LatLng FrameImage::DoubleConvertLatLng(const double& dfCoordinate)
{
	LatLng LatLngOut;
	int    nDegree;
	int    nMinite;
	int    nSecond;
	double dfDegree;
	double dfMinite;
	nDegree            = (int)dfCoordinate;
	LatLngOut.degree   = nDegree;
	dfDegree           = dfCoordinate - nDegree;
	double temp_minte  = dfDegree * 60;
	nMinite            = (int)temp_minte;
	LatLngOut.minute   = nMinite;
	dfMinite           = temp_minte - nMinite;
	double temp_second = dfMinite * 60;
	LatLngOut.second   = (int)(temp_second + 0.5);
	return LatLngOut;
}

double FrameImage::LatLngConvertDouble(const LatLng& sCoordinateLatLng)
{
	double dfCoordinate = 0;
	double dfTempMinite = sCoordinateLatLng.second / 60;
	dfTempMinite += sCoordinateLatLng.minute;
	double dfTempDegree = dfTempMinite / 60;
	dfCoordinate += sCoordinateLatLng.degree + dfTempDegree;
	return dfCoordinate;
}

void FrameImage::CalculateFieldImageIndex(
    int nScaleFactor, LatLng& sLatlng1, LatLng& sLatlng2, LatLng& sLatlng3, LatLng& sLatlng4,
    vector<ExportImageIndexCoorLatLng>& vsEIICLL)
{
	int nRowUp100, nRowDown100;
	int nColLeft100, nColRight100;
	int nRowUpCurrent, nRowDownCurrent;
	int nColLeftCurrent, mColRightCurrent;
	// LatLng         sDifferLng, sDifferLat;
	vector<string> vsScaleIndex;
	InitialRowIndex100(vsScaleIndex);

	//计算影像范围在1：1000000所在的图幅号
	nRowUp100    = CalculateRow100(sLatlng2);
	nRowDown100  = CalculateRow100(sLatlng4);
	nColLeft100  = CalculateCol100(sLatlng1);
	nColRight100 = CalculateCol100(sLatlng3);

	if (nScaleFactor == 1000000)
	{

		LatLng sDifferLat100;
		LatLng sDifferLng100;
		sDifferLat100.degree = 4;
		sDifferLat100.minute = 0;
		sDifferLat100.second = 0;
		sDifferLng100.degree = 6;
		sDifferLng100.minute = 0;
		sDifferLng100.second = 0;

		//左下角坐标经纬度形式
		LatLng sMinLat, sMinLng;
		sMinLat.degree = (nRowDown100 - 1) * 4 + 4;
		sMinLat.minute = 0;
		sMinLat.second = 0;
		sMinLng.degree = (nColLeft100 - 31) * 6;
		sMinLng.minute = 0;
		sMinLng.second = 0;

		for (int i = nRowDown100; i <= nRowUp100; ++i)
		{
			LatLng sDifferLat_i = PlusLatlng(sDifferLat100, i - nRowDown100);
			for (int j = nColLeft100; j <= nColRight100; ++j)
			{
				LatLng                     sDifferLng_j = PlusLatlng(sDifferLng100, j - nColLeft100);
				ExportImageIndexCoorLatLng sEiicll1;
				//图幅坐标范围
				sEiicll1.Lat3 = AddLatLng(sMinLat, sDifferLat_i);
				sEiicll1.Lng3 = AddLatLng(sMinLng, sDifferLng_j);
				sEiicll1.Lat1 = AddLatLng(sEiicll1.Lat3, sDifferLat100);
				sEiicll1.Lng1 = sEiicll1.Lng3;
				sEiicll1.Lat4 = sEiicll1.Lat3;
				sEiicll1.Lng4 = AddLatLng(sEiicll1.Lng3, sDifferLng100);
				sEiicll1.Lat2 = sEiicll1.Lat1;
				sEiicll1.Lng2 = sEiicll1.Lng4;
				//图幅编号
				string sIndexRow100 = vsScaleIndex[i];
				string sIndexCol100 = to_string(j);
				string sIndex100    = sIndexRow100 + sIndexCol100;
				sEiicll1.ImageIndex = std::move(sIndex100);
				//输出
				vsEIICLL.emplace_back(sEiicll1);
			}
		}
		return;
	}

	ScaleMoudle sScaleMoudle;
	string      sRow1 = vsScaleIndex[nRowUp100];
	string      sRow2 = vsScaleIndex[nRowDown100];
	string      sCol1 = to_string(nColLeft100);
	string      sCol2 = to_string(nColRight100);
	//比例尺代码
	InitialScaleMoudle(nScaleFactor, sScaleMoudle);
	//计算行列号
	int    nRowUp      = CalculateRowOther(sLatlng2, sScaleMoudle);
	string strRowUp    = ConvertImageIndex(nRowUp);
	int    nRowDown    = CalculateRowOther(sLatlng4, sScaleMoudle);
	string strRowDown  = ConvertImageIndex(nRowDown);
	int    nCOlLeft    = CalculateColOther(sLatlng1, sScaleMoudle);
	string strColLeft  = ConvertImageIndex(nCOlLeft);
	int    nColRight   = CalculateColOther(sLatlng3, sScaleMoudle);
	string strColRight = ConvertImageIndex(nColRight);
	//输出四个角坐标所在的图幅编号
	map<string, int> ScaleMap;
	InitialScaleMap(ScaleMap);
	ExportImageIndexCoordination sIIC1, sIIC2, sIIC3, sIIC4;
	string                       strScaleIndex(1, sScaleMoudle.ScaleIndex);
	sIIC1.ImageIndex = sRow1 + sCol1 + strScaleIndex + strRowUp + strColLeft;
	sIIC2.ImageIndex = sRow1 + sCol2 + strScaleIndex + strRowUp + strColRight;
	sIIC3.ImageIndex = sRow2 + sCol1 + strScaleIndex + strRowDown + strColLeft;
	sIIC4.ImageIndex = sRow2 + sCol2 + strScaleIndex + strRowDown + strColRight;
	ConvertImageIndexToLatLngRange(sIIC1.ImageIndex, vsScaleIndex, ScaleMap, sIIC1);
	ConvertImageIndexToLatLngRange(sIIC2.ImageIndex, vsScaleIndex, ScaleMap, sIIC2);
	ConvertImageIndexToLatLngRange(sIIC3.ImageIndex, vsScaleIndex, ScaleMap, sIIC3);
	ConvertImageIndexToLatLngRange(sIIC4.ImageIndex, vsScaleIndex, ScaleMap, sIIC4);
	//计算所需要比例尺划分格网范围
	double dfMaxLat    = sIIC2.Lat2;
	double dfMaxLng    = sIIC2.Lng2;
	double dfMinLat    = sIIC3.Lat3;
	double dfMinLng    = sIIC3.Lng3;
	LatLng sDifferLat  = sScaleMoudle.DifferLat;
	LatLng sDifferLng  = sScaleMoudle.DifferLng;
	double dfDifferLat = LatLngConvertDouble(sDifferLat);
	double dfDifferLng = LatLngConvertDouble(sDifferLng);
	LatLng sMaxLat     = DoubleConvertLatLng(dfMaxLat);
	LatLng sMaxLng     = DoubleConvertLatLng(dfMaxLng);
	LatLng sMinLat     = DoubleConvertLatLng(dfMinLat);
	LatLng sMinLng     = DoubleConvertLatLng(dfMinLng);
	int    nRows       = ceil((dfMaxLat - dfMinLat) / dfDifferLat);
	int    nCols       = ceil((dfMaxLng - dfMinLng) / dfDifferLng);
	vsEIICLL.reserve(nRows * nCols);
	for (int i = 0; i < nRows; ++i)
	{

		LatLng differ_lat_i = PlusLatlng(sDifferLat, i);
		for (int j = 0; j < nCols; ++j)
		{
			ExportImageIndexCoorLatLng sEiicll1;
			LatLng                     differ_lng_j = PlusLatlng(sDifferLng, j);
			sEiicll1.Lat3                           = AddLatLng(sMinLat, differ_lat_i);
			sEiicll1.Lng3                           = AddLatLng(sMinLng, differ_lng_j);
			sEiicll1.Lat2                           = AddLatLng(sEiicll1.Lat3, sDifferLat);
			sEiicll1.Lng2                           = AddLatLng(sEiicll1.Lng3, sDifferLng);
			sEiicll1.Lat1                           = sEiicll1.Lat2;
			sEiicll1.Lng1                           = sEiicll1.Lng3;
			sEiicll1.Lat4                           = sEiicll1.Lat3;
			sEiicll1.Lng4                           = sEiicll1.Lng2;
			vsEIICLL.push_back(sEiicll1);
		}
	}
	//计算所需要比例尺的图幅编号
	int nSizeEiic = vsEIICLL.size();
	for (int i = 0; i < nSizeEiic; ++i)
	{
		CalculateImageIndexFromCoordination(vsEIICLL[i], sScaleMoudle, vsScaleIndex);
	}
}

//初始化尺度参数；
bool FrameImage::InitialScaleMoudle(int& nScaleFactor, ScaleMoudle& sScaleMoudle)
{
	switch (nScaleFactor)
	{
	case 1000000:
		sScaleMoudle.DifferLat.degree = 4;
		sScaleMoudle.DifferLat.minute = 0;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 6;
		sScaleMoudle.DifferLng.minute = 0;
		sScaleMoudle.DifferLng.second = 0;
		sScaleMoudle.ImagesNumber     = 1;
		sScaleMoudle.ScaleIndex       = 'R';
		return true;

	case 500000:
		sScaleMoudle.DifferLat.degree = 2;
		sScaleMoudle.DifferLat.minute = 0;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 3;
		sScaleMoudle.DifferLng.minute = 0;
		sScaleMoudle.DifferLng.second = 0;
		sScaleMoudle.ImagesNumber     = 2;
		sScaleMoudle.ScaleIndex       = 'B';
		return true;

	case 250000:
		sScaleMoudle.DifferLat.degree = 1;
		sScaleMoudle.DifferLat.minute = 0;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 1;
		sScaleMoudle.DifferLng.minute = 30;
		sScaleMoudle.DifferLng.second = 0;
		sScaleMoudle.ImagesNumber     = 4;
		sScaleMoudle.ScaleIndex       = 'C';
		return true;

	case 100000:
		sScaleMoudle.DifferLat.degree = 0;
		sScaleMoudle.DifferLat.minute = 20;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 0;
		sScaleMoudle.DifferLng.minute = 30;
		sScaleMoudle.DifferLng.second = 0;
		sScaleMoudle.ImagesNumber     = 12;
		sScaleMoudle.ScaleIndex       = 'D';
		return true;

	case 50000:
		sScaleMoudle.DifferLat.degree = 0;
		sScaleMoudle.DifferLat.minute = 10;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 0;
		sScaleMoudle.DifferLng.minute = 15;
		sScaleMoudle.DifferLng.second = 0;
		sScaleMoudle.ImagesNumber     = 24;
		sScaleMoudle.ScaleIndex       = 'E';
		return true;

	case 25000:
		sScaleMoudle.DifferLat.degree = 0;
		sScaleMoudle.DifferLat.minute = 5;
		sScaleMoudle.DifferLat.second = 0;
		sScaleMoudle.DifferLng.degree = 0;
		sScaleMoudle.DifferLng.minute = 7;
		sScaleMoudle.DifferLng.second = 30;
		sScaleMoudle.ImagesNumber     = 48;
		sScaleMoudle.ScaleIndex       = 'F';
		return true;

	case 10000:
		sScaleMoudle.DifferLat.degree = 0;
		sScaleMoudle.DifferLat.minute = 2;
		sScaleMoudle.DifferLat.second = 30;
		sScaleMoudle.DifferLng.degree = 0;
		sScaleMoudle.DifferLng.minute = 3;
		sScaleMoudle.DifferLng.second = 45;
		sScaleMoudle.ImagesNumber     = 96;
		sScaleMoudle.ScaleIndex       = 'G';
		return true;

	case 5000:
		sScaleMoudle.DifferLat.degree = 0;
		sScaleMoudle.DifferLat.minute = 1;
		sScaleMoudle.DifferLat.second = 15;
		sScaleMoudle.DifferLng.degree = 0;
		sScaleMoudle.DifferLng.minute = 1;
		sScaleMoudle.DifferLng.second = 52.5;
		sScaleMoudle.ImagesNumber     = 192;
		sScaleMoudle.ScaleIndex       = 'H';
		return true;

	case 2000:
		sScaleMoudle.DifferX = 1000;
		sScaleMoudle.DifferY = 1000;
		return true;

	case 1000:
		sScaleMoudle.DifferX = 500;
		sScaleMoudle.DifferY = 500;
		return true;

	case 500:
		sScaleMoudle.DifferX = 250;
		sScaleMoudle.DifferY = 250;
		return true;

	default:
		break;
	}
	return true;
}

//计算1：100万比例尺分幅的行号
int FrameImage::CalculateRow100(const LatLng& sLat)
{
	int nRow = (int)(sLat.degree / 4);
	return nRow;
}

//计算1：100万比例尺分幅的列号
int FrameImage::CalculateCol100(const LatLng& sLng)
{
	int nCol = (int)(sLng.degree / 6 + 31);
	return nCol;
}

//计算大于1：100万比例尺的中小比例尺行号
int FrameImage::CalculateRowOther(const LatLng& sLat, ScaleMoudle& sScaleMoudle)
{
	double dfLat      = LatLngConvertDouble(sLat);
	double dfDeltaPhi = LatLngConvertDouble(sScaleMoudle.DifferLat);
	int    nRow       = int(4 / dfDeltaPhi + 0.5);
	double dfTemp     = dfLat / 4;
	double dfRes      = dfLat - ((int)dfTemp) * 4;
	int    nTempSub   = int(dfRes / dfDeltaPhi + 0.5);
	nRow -= nTempSub;
	return nRow;
}

//计算大于1：100万比例尺的中小比例尺列号
int FrameImage::CalculateColOther(const LatLng& sLng, ScaleMoudle& sScaleMoudle)
{
	double dfLng        = LatLngConvertDouble(sLng);
	double dfDeltaLamda = LatLngConvertDouble(sScaleMoudle.DifferLng);
	int    nCol         = 1;
	double dfTemp       = dfLng / 6;
	double dfRes        = dfLng - ((int)dfTemp) * 6;
	int    nTempSub     = int(dfRes / dfDeltaLamda);
	nCol += nTempSub;
	return nCol;
}

//初始化1：100万的行号索引
bool FrameImage::InitialRowIndex100(vector<string>& vsRowIndex)
{
	vsRowIndex.reserve(14);
	vsRowIndex.push_back("A");
	vsRowIndex.push_back("B");
	vsRowIndex.push_back("C");
	vsRowIndex.push_back("D");
	vsRowIndex.push_back("E");
	vsRowIndex.push_back("F");
	vsRowIndex.push_back("G");
	vsRowIndex.push_back("H");
	vsRowIndex.push_back("I");
	vsRowIndex.push_back("J");
	vsRowIndex.push_back("K");
	vsRowIndex.push_back("L");
	vsRowIndex.push_back("M");
	vsRowIndex.push_back("N");
	return true;
}

//将大于1：100万的比例尺行号和列号id转换成string
string FrameImage::ConvertImageIndex(int nRowOrCol)
{
	if (nRowOrCol >= 100)
	{
		return to_string(nRowOrCol);
	}
	if (nRowOrCol >= 10)
	{
		string strPre        = to_string(0);
		string strSuf        = to_string(nRowOrCol);
		string strImageIndex = strPre + strSuf;
		return strImageIndex;
	}
	if (nRowOrCol >= 0)
	{
		string strPre1       = to_string(0);
		string strPre2       = to_string(0);
		string strSuf        = to_string(nRowOrCol);
		string strImageIndex = strPre1 + strPre2 + strSuf;
		return strImageIndex;
	}
	return nullptr;
}

//将1：100万比例尺的列号转换成string
string FrameImage::ConvertImageIndex100(int nRow, int nCol, vector<string>& vsRowIndex)
{
	string strPre      = vsRowIndex[nRow - 1];
	string strSuf      = to_string(nCol);
	string strIndex100 = strPre + strSuf;
	return strIndex100;
}

//输出1：5000图幅编号和坐标范围
void FrameImage::CalculateKMField5(
    double dfX1, double dfY1, double dfX2, double dfY2, vector<ExportImageIndexCoordination>& sKmEIIC)
{
	string strConnectSyn = "-";
	int    nDiffer       = 2500;
	double dfXRange      = dfX2 - dfX1;
	double dfYRange      = dfY1 - dfY2;
	int    nXNumber      = ceil(dfXRange / nDiffer);
	int    nYNumber      = ceil(dfYRange / nDiffer);
	sKmEIIC.reserve(nXNumber * nYNumber);

	for (int i = 0; i < nYNumber; ++i)
	{
		for (int j = 0; j < nXNumber; ++j)
		{
			ExportImageIndexCoordination sEIIC;
			sEIIC.Lat3       = dfX1 + (double)(j)*2500;
			sEIIC.Lng3       = dfY2 + (double)(i)*2500;
			sEIIC.Lat2       = sEIIC.Lat3 + 2500;
			sEIIC.Lng2       = sEIIC.Lng3 + 2500;
			sEIIC.Lat1       = sEIIC.Lat3;
			sEIIC.Lng1       = sEIIC.Lng2;
			sEIIC.Lat4       = sEIIC.Lat2;
			sEIIC.Lng4       = sEIIC.Lng3;
			int nPreX        = int(sEIIC.Lat3);
			int nPreY        = int(sEIIC.Lng3);
			sEIIC.ImageIndex = to_string(nPreX) + strConnectSyn + to_string(nPreY);
			sKmEIIC.push_back(sEIIC);
		}
	}
}

//输出1：2000图幅编号和坐标范围
void FrameImage::CalculateKMField2(
    double dfX1, double dfY1, double dfX2, double dfY2, vector<ExportImageIndexCoordination>& sKmEIIC)
{
	string strConnectSyn = "-";
	int    nDiffer       = 1000;
	double dfXRange      = dfX2 - dfX1;
	double dfYRange      = dfY1 - dfY2;
	int    nXNumber      = ceil(dfXRange / nDiffer);
	int    nYNumber      = ceil(dfYRange / nDiffer);
	sKmEIIC.reserve(nXNumber * nYNumber);

	for (int i = 0; i < nYNumber; ++i)
	{
		for (int j = 0; j < nXNumber; ++j)
		{
			ExportImageIndexCoordination sEIIC;
			sEIIC.Lng3       = dfX1 + (double)(j)*nDiffer;
			sEIIC.Lat3       = dfY2 + (double)(i)*nDiffer;
			sEIIC.Lng2       = sEIIC.Lng3 + nDiffer;
			sEIIC.Lat2       = sEIIC.Lat3 + nDiffer;
			sEIIC.Lng1       = sEIIC.Lng3;
			sEIIC.Lat1       = sEIIC.Lat2;
			sEIIC.Lng4       = sEIIC.Lng2;
			sEIIC.Lat4       = sEIIC.Lat3;
			string strPreX   = DoubleToString(sEIIC.Lng3 * 0.001, 1);
			string strPreY   = DoubleToString(sEIIC.Lat3 * 0.001, 1);
			sEIIC.ImageIndex = strPreY + strConnectSyn + strPreX;
			sKmEIIC.push_back(sEIIC);
		}
	}
}

//输出1：1000图幅编号和坐标范围
void FrameImage::CalculateKMField1(
    double dfX1, double dfY1, double dfX2, double dfY2, vector<ExportImageIndexCoordination>& sKmEIIC)
{
	string strConnectSyn = "-";
	int    nDiffer       = 500;
	double dfXRange      = dfX2 - dfX1;
	double dfYRange      = dfY1 - dfY2;
	int    nXNumber      = ceil(dfXRange / nDiffer);
	int    nYNumber      = ceil(dfYRange / nDiffer);
	sKmEIIC.reserve(nXNumber * nYNumber);

	for (int i = 0; i < nYNumber; ++i)
	{
		for (int j = 0; j < nXNumber; ++j)
		{
			ExportImageIndexCoordination sEIIC;
			sEIIC.Lng3       = dfX1 + (double)(j)*nDiffer;
			sEIIC.Lat3       = dfY2 + (double)(i)*nDiffer;
			sEIIC.Lng2       = sEIIC.Lng3 + nDiffer;
			sEIIC.Lat2       = sEIIC.Lat3 + nDiffer;
			sEIIC.Lng1       = sEIIC.Lng3;
			sEIIC.Lat1       = sEIIC.Lat2;
			sEIIC.Lng4       = sEIIC.Lng2;
			sEIIC.Lat4       = sEIIC.Lat3;
			string strPreX   = DoubleToString(sEIIC.Lng3 * 0.001, 1);
			string strPreY   = DoubleToString(sEIIC.Lat3 * 0.001, 1);
			sEIIC.ImageIndex = strPreY + strConnectSyn + strPreX;
			sKmEIIC.push_back(sEIIC);
		}
	}
}

//输出1：500图幅编号和坐标范围
void FrameImage::CalculateKMField05(
    double dfX1, double dfY1, double dfX2, double dfY2, vector<ExportImageIndexCoordination>& sKmEIIC)
{
	string strConnectSyn = "-";
	int    nDiffer       = 250;
	double dfXRange      = dfX2 - dfX1;
	double dfYRange      = dfY1 - dfY2;
	int    nXNnumber     = ceil(dfXRange / nDiffer);
	int    nYNumber      = ceil(dfYRange / nDiffer);
	sKmEIIC.reserve(nXNnumber * nYNumber);

	for (int i = 0; i < nYNumber; ++i)
	{
		for (int j = 0; j < nXNnumber; ++j)
		{
			ExportImageIndexCoordination sEIIC;
			sEIIC.Lng3       = dfX1 + j * nDiffer;
			sEIIC.Lat3       = dfY2 + i * nDiffer;
			sEIIC.Lng2       = sEIIC.Lng3 + nDiffer;
			sEIIC.Lat2       = sEIIC.Lat3 + nDiffer;
			sEIIC.Lng1       = sEIIC.Lng3;
			sEIIC.Lat1       = sEIIC.Lat2;
			sEIIC.Lng4       = sEIIC.Lng2;
			sEIIC.Lat4       = sEIIC.Lat3;
			string strPreX   = DoubleToString(sEIIC.Lng3 * 0.001, 2);
			string strPreY   = DoubleToString(sEIIC.Lat3 * 0.001, 2);
			sEIIC.ImageIndex = strPreY + strConnectSyn + strPreX;
			sKmEIIC.push_back(sEIIC);
		}
	}
}

//计算公里网图像索引
void FrameImage::CalculatImageIndexKM(
    double dfX1, double dfY1, double dfX2, double dfY2, int nScaleFactor, ScaleMoudle& sScaleMoudle,
    vector<ExportImageIndexCoordination>& vsKMEIIC)
{
	if (nScaleFactor >= 2000)
	{
		CalculateKMField2(dfX1, dfY1, dfX2, dfY2, vsKMEIIC);
		return;
	}
	if (nScaleFactor >= 1000)
	{
		CalculateKMField1(dfX1, dfY1, dfX2, dfY2, vsKMEIIC);
		return;
	}
	if (nScaleFactor >= 500)
	{
		CalculateKMField05(dfX1, dfY1, dfX2, dfY2, vsKMEIIC);
		return;
	}
}

//计算中小比例尺的通过图幅索引来计算图像的四至范围；
void FrameImage::ConvertImageIndexToLatLngRange(
    string& strImageIndex, vector<string>& vstrRowIndex, map<string, int>& vScaleMap,
    ExportImageIndexCoordination& EIIC)
{
	string                     strRow        = strImageIndex.substr(4, 3);
	string                     strCol        = strImageIndex.substr(7, 3);
	string                     strRow100     = strImageIndex.substr(0, 1);
	string                     strCol100     = strImageIndex.substr(1, 2);
	string                     strScaleIndex = strImageIndex.substr(3, 1);
	int                        nRow          = stoi(strRow);
	int                        nCol          = stoi(strCol);
	int                        nCol100       = stoi(strCol100);
	vector<string>::iterator   vstrIter      = find(vstrRowIndex.begin(), vstrRowIndex.end(), strRow100);
	int                        nId           = distance(vstrRowIndex.begin(), vstrIter);
	map<string, int>::iterator IterMap       = vScaleMap.find(strScaleIndex);
	int                        nScaleFactor  = IterMap->second;
	ScaleMoudle                sScaleMoudle;
	InitialScaleMoudle(nScaleFactor, sScaleMoudle);
	LatLng sDifferLat   = sScaleMoudle.DifferLat;
	LatLng sDifferLng   = sScaleMoudle.DifferLng;
	double dfDifferLat  = LatLngConvertDouble(sDifferLat);
	double dfDifferLng  = LatLngConvertDouble(sDifferLng);
	int    nMaxLat100   = 4 * (nId + 1);
	double dfMaxLat     = nMaxLat100 - (nRow - 1) * dfDifferLat;
	double dfMinLat     = dfMaxLat - dfDifferLat;
	double dfMinKLng100 = (nCol100 - 31) * 6;
	double dfMinLng     = dfMinKLng100 + (nCol - 1) * dfDifferLng;
	double dfMaxLng     = dfMinLng + dfDifferLng;
	EIIC.ImageIndex     = strImageIndex;
	EIIC.Lat1           = dfMaxLat;
	EIIC.Lng1           = dfMinLng;
	EIIC.Lat2           = dfMaxLat;
	EIIC.Lng2           = dfMaxLng;
	EIIC.Lat3           = dfMinLat;
	EIIC.Lng3           = dfMinLng;
	EIIC.Lat4           = dfMaxLat;
	EIIC.Lng4           = dfMaxLng;
}

//初始化map，“通过比例尺代号来获得比例尺尺度”
bool FrameImage::InitialScaleMap(map<string, int>& scale_map)
{
	scale_map["R"] = 1000000;
	scale_map["B"] = 500000;
	scale_map["C"] = 250000;
	scale_map["D"] = 100000;
	scale_map["E"] = 50000;
	scale_map["F"] = 25000;
	scale_map["G"] = 10000;
	scale_map["H"] = 5000;
	return true;
}

//通过图幅号获得该图幅在分幅的行号
int FrameImage::GetRowFromImageIndex(string& strImageIndex)
{
	string strRow = strImageIndex.substr(4, 3);
	int    nRow   = stoi(strRow);
	return nRow;
}

//通过图幅号获得该图幅在分幅的列号
int FrameImage::GetColFromImageIndex(string& strImageIndex)
{
	string strCol = strImageIndex.substr(7, 3);
	int    nCol   = stoi(strCol);
	return nCol;
}

//通过图幅号获得分幅的比例尺代码
string FrameImage::GetScaleIndex(string& strImageIndex)
{
	string strScaleIndex = strImageIndex.substr(3, 1);
	return strScaleIndex;
}

//通过图幅号获得在1：100万所在的列号
int FrameImage::GetColFromImageIndex100(string& strImageIndex)
{
	string strCol100 = strImageIndex.substr(1, 2);
	int    nCol100   = stoi(strCol100);
	return nCol100;
}

//通过图幅号获得在1：100万所在的行号
int FrameImage::GetRowFromImageIndex100(string& strImageIndex, vector<string>& vsScaleIndex)
{
	string                   strRow100 = strImageIndex.substr(0, 1);
	vector<string>::iterator vsIter    = find(vsScaleIndex.begin(), vsScaleIndex.end(), strRow100);
	int                      nRow100   = distance(vsScaleIndex.begin(), vsIter);
	return nRow100;
}

//度分秒格式的经纬度相加
LatLng FrameImage::AddLatLng(LatLng& sLatLng1, LatLng& sLatLng2)
{
	LatLng sLatLngOut;
	double dfDegree    = sLatLng1.degree + sLatLng2.degree;
	double dfMinute    = sLatLng1.minute + sLatLng2.minute;
	double dfSecond    = sLatLng1.second + sLatLng2.second;
	int    nTempMinute = int(dfSecond / 60);
	sLatLngOut.second  = dfSecond - nTempMinute * 60;
	int nTempDegree    = int((dfMinute + nTempMinute) / 60);
	sLatLngOut.minute  = dfMinute + nTempMinute - nTempDegree * 60;
	sLatLngOut.degree  = dfDegree + nTempDegree;
	return sLatLngOut;
}

//度分秒的格式经纬度与整数相乘
LatLng FrameImage::PlusLatlng(LatLng& sLatLng1, int i)
{
	LatLng sLatLngOut;
	double dfDegree    = sLatLng1.degree * i;
	double dfMinute    = sLatLng1.minute * i;
	double dfSecond    = sLatLng1.second * i;
	int    nTempMinute = int(dfSecond / 60);
	sLatLngOut.second  = dfSecond - nTempMinute * 60;
	int nTempDegree    = int((dfMinute + nTempMinute) / 60);
	sLatLngOut.minute  = dfMinute + nTempMinute - nTempDegree * 60;
	sLatLngOut.degree  = dfDegree + nTempDegree;
	return sLatLngOut;
}

//通过坐标计算所在分幅的图幅编号
void FrameImage::CalculateImageIndexFromCoordination(
    ExportImageIndexCoorLatLng& EIIC, ScaleMoudle& scale_moudle, vector<string>& row_index)
{
	LatLng sMinLat     = EIIC.Lat3;
	LatLng sMinLng     = EIIC.Lng3;
	int    nRow100     = CalculateRow100(sMinLat);
	int    nCol100     = CalculateCol100(sMinLng);
	int    nRowCurrent = CalculateRowOther(sMinLat, scale_moudle);
	int    nColCurrent = CalculateColOther(sMinLng, scale_moudle);
	string strRow100   = row_index[nRow100];
	string strCol100   = to_string(nCol100);
	string strScaleIndex(1, scale_moudle.ScaleIndex);
	string strRow   = ConvertImageIndex(nRowCurrent);
	string strCol   = ConvertImageIndex(nColCurrent);
	EIIC.ImageIndex = strRow100 + strCol100 + strScaleIndex + strRow + strCol;
}

//输出1：100万图幅四至坐标
void FrameImage::Export100ImageIndexField(
    ExportImageIndexCoorLatLng& sEIIC, vector<string>& vsScaleIndex, int nRow100, int nCol100)
{
	int nMinLat       = (nRow100 - 1) * 4;
	int nMinLng       = (nCol100 - 31) * 6;
	sEIIC.Lat3.degree = nMinLat;
	sEIIC.Lat3.minute = 0;
	sEIIC.Lat3.second = 0;
	sEIIC.Lng3.degree = nMinLng;
	sEIIC.Lng3.minute = 0;
	sEIIC.Lng3.second = 0;
	LatLng sDifferLat, sDifferLng;
	sDifferLat.degree = 4;
	sDifferLat.minute = 0;
	sDifferLat.second = 0;
	sDifferLng.degree = 6;
	sDifferLng.minute = 0;
	sDifferLng.second = 0;
	sEIIC.Lat2        = AddLatLng(sEIIC.Lat3, sDifferLat);
	sEIIC.Lng2        = AddLatLng(sEIIC.Lng3, sDifferLng);
	sEIIC.Lat1        = AddLatLng(sEIIC.Lat3, sDifferLat);
	sEIIC.Lng1        = sEIIC.Lng3;
	sEIIC.Lat4        = sEIIC.Lat3;
	sEIIC.Lng4        = sEIIC.Lng2;
	sEIIC.ImageIndex  = vsScaleIndex[nRow100] + to_string(nCol100);
}

//中小比例尺写入几何图形
OGRPolygon* FrameImage::WriteGeometry(ExportImageIndexCoorLatLng& sEIIC, OGRFeature* poFeature)
{
	OGRPolygon*    pGeometry = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
	OGRLinearRing* pRing     = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
	OGRPoint       pt;
	double         x1, y1, x2, y2, x3, y3, x4, y4;
	x1 = LatLngConvertDouble(sEIIC.Lng1);
	x2 = LatLngConvertDouble(sEIIC.Lng2);
	x4 = LatLngConvertDouble(sEIIC.Lng3);
	x3 = LatLngConvertDouble(sEIIC.Lng4);
	y1 = LatLngConvertDouble(sEIIC.Lat1);
	y2 = LatLngConvertDouble(sEIIC.Lat2);
	y4 = LatLngConvertDouble(sEIIC.Lat3);
	y3 = LatLngConvertDouble(sEIIC.Lat4);
	pt.setX(x1);
	pt.setY(y1);
	pRing->addPoint(&pt);
	pt.setX(x2);
	pt.setY(y2);
	pRing->addPoint(&pt);
	pt.setX(x3);
	pt.setY(y3);
	pRing->addPoint(&pt);
	pt.setX(x4);
	pt.setY(y4);
	pRing->addPoint(&pt);
	pt.setX(x1);
	pt.setY(y1);
	pRing->addPoint(&pt);
	pRing->closeRings();
	pGeometry->addRing(pRing);
	return pGeometry;
}

//大比例尺地图写几何图形
OGRPolygon* FrameImage::WriteGeometryKm(
    ExportImageIndexCoordination& sKMEIIC, int i, OGRLayer* poLayer, OGRFeature* poFeature, int nExtension)
{

	OGRPolygon* PoloygonTwo = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);

	OGRLinearRing* pRing1 = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
	// OGRPoint       pt;
	OGRPoint pt1;
	//内多边形从右下角逆时针旋转,外多边形同；
	double x1, y1, x2, y2, x3, y3, x4, y4;
	x1 = sKMEIIC.Lng1;
	x2 = sKMEIIC.Lng2;
	x3 = sKMEIIC.Lng3;
	x4 = sKMEIIC.Lng4;
	y1 = sKMEIIC.Lat1;
	y2 = sKMEIIC.Lat2;
	y3 = sKMEIIC.Lat3;
	y4 = sKMEIIC.Lat4;
	//外多边形
	double x1_out = x1 - nExtension;
	double y1_out = y1 + nExtension;
	double x2_out = x2 + nExtension;
	double y2_out = y2 + nExtension;
	double x3_out = x3 - nExtension;
	double y3_out = y3 - nExtension;
	double x4_out = x4 + nExtension;
	double y4_out = y4 - nExtension;
	pt1.setX(x1_out);
	pt1.setY(y1_out);
	pRing1->addPoint(&pt1);
	pt1.setX(x2_out);
	pt1.setY(y2_out);
	pRing1->addPoint(&pt1);
	pt1.setX(x4_out);
	pt1.setY(y4_out);
	pRing1->addPoint(&pt1);
	pt1.setX(x3_out);
	pt1.setY(y3_out);
	pRing1->addPoint(&pt1);
	pt1.setX(x1_out);
	pt1.setY(y1_out);
	pRing1->addPoint(&pt1);
	pRing1->closeRings();
	PoloygonTwo->addRing(pRing1);
	return PoloygonTwo;
}

//中小比例尺写shape文件
void FrameImage::WriteShapeFile(
    vector<ExportImageIndexCoorLatLng>& sEIIC, const char* pszVectorFile, const char* pszDriverName)
{
	//矢量文件个数
	int    nFeaturesize   = sEIIC.size();
	string pShapeFilename = CPLGetBasename(pszVectorFile);
	string pShapeDirName  = CPLGetDirname(pszVectorFile);
	string pExtension     = CPLGetExtension(pszVectorFile);

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
	GDALAllRegister();
	const char* vecfilename = CPLGetFilename(pszVectorFile);
	GDALDriver* poDriver    = GetGDALDriverManager()->GetDriverByName(pszDriverName);
	if (poDriver == nullptr)
	{
		printf("%s 驱动不可用！\n", pszDriverName);
		return;
	}
	//创建数据源
	string       pstrShapeFileName = pShapeDirName + "/" + pShapeFilename + "." + pExtension;
	char*        pVectorFilename   = (char*)pstrShapeFileName.c_str();
	GDALDataset* poDS              = poDriver->Create(pVectorFilename, 0, 0, 0, GDT_Unknown, nullptr);
	if (poDS == nullptr)
	{
		printf("创建shapefile文件失败");
		return;
	}
	//定义空间参考
	OGRSpatialReference spatial_reference;
	spatial_reference.SetWellKnownGeogCS("wgs84");
	OGRSpatialReference* oSRS = &spatial_reference;
	//创建图层
	OGRLayer* poLayer = poDS->CreateLayer("shapelayer", oSRS, wkbPolygon, nullptr);
	if (poLayer == nullptr)
	{
		printf("图层创建失败！\n");
		return;
	}
	//创建属性表
	//添加属性字段
	OGRFieldDefn firstField("NAME", OFTString);
	poLayer->CreateField(&firstField);

	for (int i = 0; i < nFeaturesize; ++i)
	{
		OGRFeature* poFeature;
		poFeature          = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
		char* polygon_name = (char*)(sEIIC[i].ImageIndex.c_str());
		//属性字段赋值
		poFeature->SetField("NAME", polygon_name);
		OGRPolygon* pn = WriteGeometry(sEIIC[i], poFeature);
		poFeature->SetGeometry(pn);
		poLayer->CreateFeature(poFeature);
		OGRFeature::DestroyFeature(poFeature);
	}

	GDALClose(poDS);
}

//大比例尺写入shape文件
void FrameImage::WriteShapeFileKm(
    vector<ExportImageIndexCoordination>& vsKMEIIC, int nExtension, const char* pszVectorFile,
    const char* pszDriverName, OGRSpatialReference& sShapeReference)
{
	int    feature_size   = vsKMEIIC.size();
	string pShapeFilename = CPLGetBasename(pszVectorFile);
	string pShapeDirName  = CPLGetDirname(pszVectorFile);
	string pExtension     = CPLGetExtension(pszVectorFile);

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
	GDALAllRegister();
	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
	if (poDriver == nullptr)
	{
		printf("%s 驱动不可用！\n", pszDriverName);
		return;
	}
	//创建数据源
	string       pstrShapeFileName = pShapeDirName + "/" + pShapeFilename + "." + pExtension;
	char*        pVectorFilename   = (char*)pstrShapeFileName.c_str();
	GDALDataset* poDS              = poDriver->Create(pVectorFilename, 0, 0, 0, GDT_Unknown, nullptr);
	if (poDS == nullptr)
	{
		printf("创建shapefile文件失败");
		return;
	}
	//创建图层
	OGRSpatialReference* sReference = &sShapeReference;
	OGRLayer*            poLayer    = poDS->CreateLayer("shapelayer", sReference, wkbMultiPolygon, nullptr);
	if (poLayer == nullptr)
	{
		printf("图层创建失败！\n");
		return;
	}
	//创建属性表
	OGRFieldDefn firstField("NAME", OFTString);
	poLayer->CreateField(&firstField);

	for (int i = 0; i < feature_size; ++i)
	{
		OGRFeature* poFeature;
		poFeature          = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
		char* polygon_name = (char*)(vsKMEIIC[i].ImageIndex.c_str());
		poFeature->SetField("NAME", polygon_name);
		OGRPolygon* pn = WriteGeometryKm(vsKMEIIC[i], i, poLayer, poFeature, nExtension);
		poFeature->SetGeometry(pn);
		poLayer->CreateFeature(poFeature);
		OGRFeature::DestroyFeature(poFeature);
	}

	GDALClose(poDS);
}

OGRSpatialReference GetImageFieldCoordination(OGREnvelope& sOgrenvelope, const char* pszName, int nScaleFactor)
{
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	GDALAllRegister();
	OGRSpatialReference sGPref, sWPref;

	GDALDataset* poDataset = (GDALDataset*)GDALOpen(pszName, GA_ReadOnly);
	if (poDataset == nullptr)
	{
		printf("fail in open files");
		return OGRSpatialReference();
	}

	int    nImgSizeX = poDataset->GetRasterXSize();
	int    nImgSizeY = poDataset->GetRasterYSize();
	double dfTrans[6];
	CPLErr aa = poDataset->GetGeoTransform(dfTrans);
	double dfx1, dfy1, dfx2, dfy2;
	dfx1              = dfTrans[0];
	dfy1              = dfTrans[3];
	dfx2              = dfx1 + nImgSizeX * dfTrans[1];
	dfy2              = dfy1 + nImgSizeY * dfTrans[5];
	sOgrenvelope.MinX = dfx1;
	sOgrenvelope.MaxX = dfx2;
	sOgrenvelope.MaxY = dfy1;
	sOgrenvelope.MinY = dfy2;
	const char* Proj  = poDataset->GetProjectionRef();
	char*       tmp   = const_cast<char*>(Proj);
	sWPref.importFromWkt(&tmp);
	if (nScaleFactor >= 5000)
	{
		sGPref.SetWellKnownGeogCS("WGS84");
		OGRCoordinateTransformation* coordTrans;
		coordTrans = OGRCreateCoordinateTransformation(&sWPref, &sGPref);
		coordTrans->Transform(1, &sOgrenvelope.MaxX, &sOgrenvelope.MinY);
		coordTrans->Transform(1, &sOgrenvelope.MinX, &sOgrenvelope.MaxY);
	}
	return sWPref;
}

//分幅调用函数
bool ImageStdFrame2Vector(
    std::vector<std::string>& vVectorName, const char* pszSrcFile, int nScaleFactor, int nExtensionField,
    const char* pszDstFile, const char* pszFormat)
{
	OGREnvelope         ogrenvelope;
	OGRSpatialReference sShapeReference = GetImageFieldCoordination(ogrenvelope, pszSrcFile, nScaleFactor);

	//参数设置：左上角经度，纬度，右下角经度，纬度，比例尺、可输入1000000，500000，250000，100000，50000，25000，10000，5000；外扩参数中小比例尺不可用，因此输入可省略；
	//参数设置：左上角x，y，右下角x，y，比例尺参数，外扩参数（单位厘米）外扩默认参数1可省略；
	FrameImage spl1;

	bool bFlag = spl1.InitialParam(
	    ogrenvelope.MinX, ogrenvelope.MaxY, ogrenvelope.MaxX, ogrenvelope.MinY, nScaleFactor, nExtensionField);

	if (nScaleFactor >= 5000)
	{
		vector<ExportImageIndexCoorLatLng> vsEIIC;
		LatLng                             sColLeft  = spl1.DoubleConvertLatLng(spl1.GetColLeft());
		LatLng                             sRowUp    = spl1.DoubleConvertLatLng(spl1.GetRowUp());
		LatLng                             sColRight = spl1.DoubleConvertLatLng(spl1.GetColRigt());
		LatLng                             sRowDown  = spl1.DoubleConvertLatLng(spl1.GetRowDown());
		spl1.CalculateFieldImageIndex(nScaleFactor, sColLeft, sRowUp, sColRight, sRowDown, vsEIIC);
		spl1.WriteShapeFile(vsEIIC, pszDstFile, pszFormat);
		for (auto& vout : vsEIIC)
		{
			vVectorName.emplace_back(vout.ImageIndex);
		}
	}
	else
	{
		if (sShapeReference.IsProjected())
		{
			vector<ExportImageIndexCoordination> KMEIIC;
			double                               X1 = spl1.GetColLeft();
			double                               Y1 = spl1.GetRowUp();
			double                               X2 = spl1.GetColRigt();
			double                               Y2 = spl1.GetRowDown();
			ScaleMoudle                          scale_moudle;
			int                                  nExtensionField = spl1.GetExtenScale() * nScaleFactor / 100;
			spl1.CalculatImageIndexKM(X1, Y1, X2, Y2, nScaleFactor, scale_moudle, KMEIIC);
			spl1.WriteShapeFileKm(KMEIIC, nExtensionField, pszDstFile, pszFormat, sShapeReference);
			for (auto& vout : KMEIIC)
			{
				vVectorName.emplace_back(vout.ImageIndex);
			}
		}
		else
		{
			bFlag = false;
		}
	}

	return bFlag;
}

//裁切影像
bool ImageSubset4Vector(
    const char* pszSrcFile, const char* pszSubFile, const char* pszFilter, const char* pszDstFile,
    GDALProgressFunc pFun, void* pArg)
{
	if (GetGDALDriverManager()->GetDriverCount() < 0)
		GDALAllRegister();
	CPLSetConfigOption("GDAL_DATA", "D://data//gdal-2.4.3_proj//gdal-2.4.3//gdal_r64proj//data");
	GDALDatasetH hSrcDS = GDALOpen(pszSrcFile, GA_ReadOnly);
	
	if (hSrcDS == nullptr)
		return false;

	char** papszCmd = nullptr;
	papszCmd        = CSLAddString(papszCmd, "-cutline");
	papszCmd        = CSLAddString(papszCmd, pszSubFile);
	papszCmd        = CSLAddString(papszCmd, "-crop_to_cutline");
	papszCmd        = CSLAddString(papszCmd, "-cwhere");
	papszCmd        = CSLAddString(papszCmd, pszFilter);
	papszCmd        = CSLAddString(papszCmd, "-overwrite");
	papszCmd        = CSLAddString(papszCmd, pszSrcFile);
	papszCmd        = CSLAddString(papszCmd, pszDstFile);

	GDALWarpAppOptions* pOptions = GDALWarpAppOptionsNew(papszCmd, nullptr);
	GDALWarpAppOptionsSetProgress(pOptions, pFun, pArg);

	int bUsageError = false;

	GDALDatasetH hDstDS=GDALWarp(pszDstFile, nullptr, 1, &hSrcDS, pOptions, &bUsageError);
	

	bool bSuccess = (hDstDS == nullptr) ? false : true;
	//输出tfw文件
	if (bSuccess)
	{
		double dfGeoTransForm[6];
		GDALGetGeoTransform(hDstDS, dfGeoTransForm);
		int         nImgSizeY = GDALGetRasterYSize(hSrcDS);
		fstream     tfw;
		const char* pTFW = CPLResetExtension(pszDstFile, "tfw");
		tfw.open(pTFW, ios::out);
		tfw << fixed;
		tfw << setprecision(10);
		tfw << dfGeoTransForm[1] << endl;
		tfw << dfGeoTransForm[2] << endl;
		tfw << dfGeoTransForm[4] << endl;
		tfw << dfGeoTransForm[5] << endl;
		tfw << dfGeoTransForm[0] << endl;
		tfw << dfGeoTransForm[3] + nImgSizeY * dfGeoTransForm[5] << endl;
		tfw.close();
	}

	GDALWarpAppOptionsFree(pOptions);
	CSLDestroy(papszCmd);
	GDALClose(hSrcDS);
	if (hDstDS != nullptr)
		GDALClose(hDstDS);

	return bSuccess;
}
