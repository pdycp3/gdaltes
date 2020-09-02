#pragma once

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel> 
#include "ui_QFrameImage.h"
#include "QSettings"
#include "gdal_priv.h"
#include <vector>
#include <string>
using namespace std;
struct ImageAttributeInofomation
{
	string strImagePath;
	string strFileSize;
	string strFileType;
	string strDescription;
	string  strSpatialSystem;
	int nBandCount;
	int nWeight;
	int nHeight;
	double dX0;
	double dY0;
	double pixSizeX;
	double pixSizeY;
	double dUp;
	double dDown;
	double dLeft;
	double dRight;
};
class QFrameImage : public QMainWindow
{
	Q_OBJECT

public:
	QFrameImage(QWidget *parent = Q_NULLPTR);
	void SetList(vector<string>& vVectorName);
	void ShowImageInfo( const ImageAttributeInofomation & m_ImgAttributeInformation,QStandardItemModel * imgItemModel);
	unsigned char* ImgSketch(float* buffer, GDALRasterBand* currentBand, int bandSize, double noValue);
	void ReadImageGdal(const QString& strPath, QPixmap& dstPixmap,ImageAttributeInofomation & ImgAttribute);
	
private slots:
	void slotOpenFile();
	void slotOutShp();
	void slotFrame();
	void slotScaleChanged(int idx);
	void slotSubSet();
	void slotOutDir();
	void slotCheckDlg();

private:
	Ui::FrameImageClass ui;
	int m_nScaleFactor;
	QImage * m_ImageFile;
	ImageAttributeInofomation m_ImgAttributeInformation;
	QStandardItemModel *imgMetaModel = new QStandardItemModel;
	QSettings * m_settings=nullptr;
};
