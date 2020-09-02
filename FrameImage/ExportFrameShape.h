#pragma once


#include "ogrsf_frmts.h"

struct LatLng
{
	float degree;
	float minute;
	float second;
};

struct ScaleMoudle
{
	char   ScaleIndex;
	int    ImagesNumber;
	double DifferX;
	double DifferY;
	LatLng DifferLat;
	LatLng DifferLng;
};

//输出图幅号和四角所在的经纬度坐标
//编号顺序按照左上、右上、左下、右下分别编号1、2、3、4；
struct ExportImageIndexCoordination
{
	std::string ImageIndex;
	double      Lat1;
	double      Lng1;
	double      Lat2;
	double      Lng2;
	double      Lat3;
	double      Lng3;
	double      Lat4;
	double      Lng4;
};

struct ExportImageIndexCoorLatLng
{
	std::string ImageIndex;
	LatLng      Lat1;
	LatLng      Lng1;
	LatLng      Lat2;
	LatLng      Lng2;
	LatLng      Lat3;
	LatLng      Lng3;
	LatLng      Lat4;
	LatLng      Lng4;
};

class FrameImage
{
public:
	FrameImage();
	~FrameImage();
	// 类函数在cpp文件有详细的注释
	// initial param list
	bool InitialParam(
	    double dfColLeft, double dfRowUp, double dfColRight, double dfRowDown, int nScaleFactor, int nExtenScale);
	bool InitialParam(double dfColLeft, double dfRowUp, double dfColRight, double dfRowDown, int nScaleFactor);

	// calculate the index of the image;
	void CalculateFieldImageIndex(
	    int nScaleFactor, LatLng& sLatlng1, LatLng& sLatlng2, LatLng& sLatlng3, LatLng& sLatlng4,
	    std::vector<ExportImageIndexCoorLatLng>& vsEIICLL);
	void CalculatImageIndexKM(
	    double dfX1, double dfY1, double dfX2, double dfY2, int nScaleFator, ScaleMoudle& sScaleMoudle,
	    std::vector<ExportImageIndexCoordination>& vsKMEIIC);
	// write into shape_file
	void WriteShapeFile(std::vector<ExportImageIndexCoorLatLng>& vsEIIC, const char* psOutPath, const char* psFormate);
	void WriteShapeFileKm(
	    std::vector<ExportImageIndexCoordination>& vsKMEIIC, int nExtension, const char* psOutPath,
	    const char* psFormate, OGRSpatialReference& sShapeReference);
	// get member
	double GetColLeft()
	{
		return m_ColLeft;
	}
	double GetColRigt()
	{
		return m_ColRight;
	}
	double GetRowUp()
	{
		return m_RowUp;
	}
	double GetRowDown()
	{
		return m_RowDown;
	}
	int GetScaleFactor()
	{
		return m_ScaleFactor;
	}
	int GetExtenScale()
	{
		return m_ExtenScale;
	}

	// convert the Latitude & Longitude
	LatLng DoubleConvertLatLng(const double& dfCoordinate);
	double LatLngConvertDouble(const LatLng& sCoordinateLatLng);

private:
	// Initial scale moudle param
	bool InitialScaleMoudle(int& ScaleFactor, ScaleMoudle& sScaleMoudle);
	// calculate the row &col;
	int  CalculateRow100(const LatLng& sLat);
	int  CalculateCol100(const LatLng& sLng);
	int  CalculateRowOther(const LatLng& sLat, ScaleMoudle& sScaleMoudle);
	int  CalculateColOther(const LatLng& sLng, ScaleMoudle& sScaleMoudle);
	bool InitialRowIndex100(std::vector<std::string>& vstrRowIndex);
	// convert the  int into string
	std::string ConvertImageIndex(int nRowOrCol);
	std::string ConvertImageIndex100(int nRow, int nCol, std::vector<std::string>& vstrRowIndex);
	// calculate ImageIndex by km
	void CalculateKMField5(
	    double dfX1, double dfY1, double dfX2, double dfY2, std::vector<ExportImageIndexCoordination>& vsKmEIIC);
	void CalculateKMField2(
	    double dfX1, double dfY1, double dfX2, double dfY2, std::vector<ExportImageIndexCoordination>& dfKmEIIC);
	void CalculateKMField1(
	    double dfX1, double dfY1, double dfX2, double dfY2, std::vector<ExportImageIndexCoordination>& vsKmEIIC);
	void CalculateKMField05(
	    double dfX1, double dfY1, double dfX2, double dfY2, std::vector<ExportImageIndexCoordination>& vsKmEIIC);
	// Calculate Coordinate from image index;
	void ConvertImageIndexToLatLngRange(
	    std::string& strImageIndex, std::vector<std::string>& vsRowIndex, std::map<std::string, int>& ScaleMap,
	    ExportImageIndexCoordination& sEIIC);
	bool        InitialScaleMap(std::map<std::string, int>& ScaleMap);
	int         GetRowFromImageIndex(std::string& strImageIndex);
	int         GetColFromImageIndex(std::string& strImageIndex);
	std::string GetScaleIndex(std::string& strImageIndex);
	int         GetColFromImageIndex100(std::string& strImageIndex);
	int         GetRowFromImageIndex100(std::string& strImageIndex, std::vector<std::string>& vsScaleIndex);
	// LatLng function
	LatLng AddLatLng(LatLng& sLatlng1, LatLng& sLatlng2);
	LatLng PlusLatlng(LatLng& sLatlng1, int i);
	// calculate imageindex from coodination
	void CalculateImageIndexFromCoordination(
	    ExportImageIndexCoorLatLng& sEIIC, ScaleMoudle& sScaleMoudle, std::vector<std::string>& vsScaleIndex);
	// export 1000000
	void Export100ImageIndexField(
	    ExportImageIndexCoorLatLng& sEIIC, std::vector<std::string>& vsScaleIndex, int nRow100, int nCol100);
	// write into geometry
	OGRPolygon* WriteGeometry(ExportImageIndexCoorLatLng& sEIIC, OGRFeature* poFeature);
	OGRPolygon* WriteGeometryKm(
	    ExportImageIndexCoordination& sKmEIIC, int nIndex, OGRLayer* poLayer, OGRFeature* poFeature, int nExtension);

private:
	double m_ColLeft;
	double m_RowUp;
	double m_ColRight;
	int    m_ScaleFactor;
	double m_RowDown;
	int    m_ExtenScale = 1;
};
bool  ImageStdFrame2Vector(
	std::vector<std::string>& vVectorName, const char* pszSrcFile, int nScaleFactor, int nExtensionField,
	const char* pszDstFile, const char* pszFormat = "ESRI Shapefile");

/**
 * @brief       影像裁切
 * @param       pszSrcFile	输入影像路径
 * @param		pszSubFile	输入裁切影像
 * @param		pszFilter	过滤参数
 * @param		pszDstFile	输出影像路径
 * @param       pFun		进度回调函数
 * @param       pArg		进度回调函数参数
 */
bool ImageSubset4Vector(
	const char* pszSrcFile, const char* pszSubFile, const char* pszFilter, const char* pszDstFile,
	GDALProgressFunc pFun = nullptr, void* pArg = nullptr);

