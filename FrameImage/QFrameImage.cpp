#include "QFileDialog"

#include "QMessageBox"
#include "QFileInfo"

#include "QFrameImage.h"
#include "cpl_conv.h"
#include "ExportFrameShape.h"
#include "QualityCheckDlg.h"
#include <iostream>
using namespace std;
QFrameImage::QFrameImage(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	//初始化setting文件
	m_settings = new QSettings("./Setting.ini", QSettings::IniFormat);
	imgMetaModel->setColumnCount(2);
	connect(ui.comboBoxScale, SIGNAL(currentIndexChanged(int)), this, SLOT(slotScaleChanged(int)));
	//打开影像
	connect(ui.pushButton_OpenImage, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
	//输入适量文件
	connect(ui.pushButton_ShapeFile, SIGNAL(clicked()), this, SLOT(slotOutShp()));
	//执行适量文件生成
	connect(ui.pushButtonFrame, SIGNAL(clicked()), this, SLOT(slotFrame()));
	//裁切
	connect(ui.pushButtonSubset, SIGNAL(clicked()), this, SLOT(slotSubSet()));
	connect(ui.pushButton_OutPath, SIGNAL(clicked()), this, SLOT(slotOutDir()));
	//弹出质检对话框
	connect(ui.pushButtonQualityCheck, SIGNAL(clicked()), this, SLOT(slotCheckDlg()));
	m_nScaleFactor = 1000;
}
void QFrameImage::slotOpenFile()
{
	
	QString strDir = m_settings->value("LastFilePath").toString();
	//文件路径

	QString strFilename = QFileDialog::getOpenFileName(
		this, tr("Choose Image File"), strDir, tr("Images File(*.tif *.tiff *.img *.pix *.vrt *.jpg)"));
	if (strFilename.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Please Select the Image"));
		return;
	}
	m_settings->setValue("LastFilePath", strFilename);
	ui.lineEdit_InPutImage->setText(strFilename);
	QPixmap pImage;
	ReadImageGdal(strFilename, pImage, m_ImgAttributeInformation);
	QGraphicsScene * Pgraphicscen = new QGraphicsScene;
	Pgraphicscen->addPixmap(pImage);
	ui.graphicsView->setScene(Pgraphicscen);
	ui.graphicsView->show();
	ShowImageInfo(m_ImgAttributeInformation, imgMetaModel);
}

void QFrameImage::SetList(vector<string>& vVectorName)
{
	ui.listWidgetFrameList->clear();

	for (int i = 0; i < (int)vVectorName.size(); ++i)
		ui.listWidgetFrameList->insertItem(i, QString::fromStdString(vVectorName[i]));

	ui.listWidgetFrameList->setSelectionMode(QAbstractItemView::ExtendedSelection);


}

void QFrameImage::ShowImageInfo(const ImageAttributeInofomation & m_ImgAttributeInformation,QStandardItemModel * imgMetaModel)
{
	int row = 0; // 用来记录数据模型的行号
	imgMetaModel->setHorizontalHeaderLabels(QStringList() << QStringLiteral("数据类型") << QStringLiteral("数值"));
	// 图像的格式
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Description")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(m_ImgAttributeInformation.strDescription.c_str()));
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Data Type")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(m_ImgAttributeInformation.strFileType.c_str()));

	// 图像的大小和波段个数
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("X Size")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(QString::number(m_ImgAttributeInformation.nWeight)));
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Y Size")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(QString::number(m_ImgAttributeInformation.nHeight)));
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Bands Count")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(QString::number(m_ImgAttributeInformation.nBandCount)));

	// 图像的投影信息
	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Projection")));
	imgMetaModel->setItem(row++, 1, new QStandardItem(m_ImgAttributeInformation.strSpatialSystem.c_str()));

	// 图像的坐标和分辨率信息

	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Origin")));
	imgMetaModel->setItem(row++, 1, new QStandardItem((QString::number(m_ImgAttributeInformation.dX0)+","+QString::number(m_ImgAttributeInformation.dY0)).toStdString().c_str()));
// 	imgMetaModel->setItem(row, 0, new QStandardItem(tr("Pixel Size")));
// 	imgMetaModel->setItem(row++, 1, new QStandardItem(pixelSize));
	ui.tableView->setModel(imgMetaModel);
}

unsigned char * QFrameImage::ImgSketch(float * buffer, GDALRasterBand * currentBand, int bandSize, double noValue)
{
		unsigned char* resBuffer = new unsigned char[bandSize];
		double max, min;
		double minmax[2];
		currentBand->ComputeRasterMinMax(1, minmax);
		min = minmax[0];
		max = minmax[1];
		if (min <= noValue && noValue <= max)
		{
			min = 0;
		}
		for (int i = 0; i < bandSize; i++)
		{
			if (buffer[i] > max)
			{
				resBuffer[i] = 255;
			}
			else if (buffer[i] <= max && buffer[i] >= min)
			{
				resBuffer[i] = static_cast<uchar>(255 - 255 * (max - buffer[i]) / (max - min));
			}
			else
			{
				resBuffer[i] = 0;
			}
		}
		return resBuffer;
}

void QFrameImage::ReadImageGdal(const QString& strPath, QPixmap& dstPixmap, ImageAttributeInofomation & ImgAttribution)
{
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	GDALAllRegister();
	std::string  strFilename = strPath.toStdString();
	GDALDataset* poDS = (GDALDataset*)GDALOpen(strFilename.c_str(), GA_ReadOnly);
	if (poDS == nullptr)
	{
		QMessageBox::information(nullptr, QObject::tr("Information"), QObject::tr("Open image failed!"));
		GDALClose(poDS);
		return;
	}
	//获取图像信息
	ImgAttribution.strImagePath = strFilename;
	int nSizeX = poDS->GetRasterXSize();
	ImgAttribution.nWeight = nSizeX;
	int nSizeY = poDS->GetRasterYSize();
	ImgAttribution.nHeight = nSizeY;
	int nBandCount = poDS->GetRasterCount();
	ImgAttribution.nBandCount = nBandCount;
	ImgAttribution.strDescription = string(poDS->GetDescription());
	ImgAttribution.strFileType = GDALGetDataTypeName(poDS->GetRasterBand(1)->GetRasterDataType());
	
	if (nBandCount <= 0)
	{
		QMessageBox::information(nullptr, QObject::tr("Information"), QObject::tr("Open image failed!"));
		GDALClose(poDS);
		return;
	}
	const char * srcSpatialRef = poDS->GetProjectionRef();
	ImgAttribution.strSpatialSystem = srcSpatialRef;
	char * tempRef = const_cast<char *>(srcSpatialRef);
	OGRSpatialReference srcSpRef, dstSpRef;
	srcSpRef.importFromWkt(&tempRef);
	double dfTrans[6];
	double dfx1, dfy1, dfx2, dfy2;
	dfx1 = dfTrans[0];
	dfy1 = dfTrans[3];
	dfx2 = dfx1 + nSizeX * dfTrans[1];
	dfy2 = dfy1 + nSizeY * dfTrans[5];
	ImgAttribution.dX0 = dfx1;
	ImgAttribution.dY0 = dfy1;
	ImgAttribution.pixSizeX = dfTrans[1];
	ImgAttribution.pixSizeY = dfTrans[5];
	OGREnvelope sOgrenvelope;
	sOgrenvelope.MinX = dfx1;
	sOgrenvelope.MaxX = dfx2;
	sOgrenvelope.MaxY = dfy1;
	sOgrenvelope.MinY = dfy2;
	if (!srcSpRef.IsProjected())
	{
		dstSpRef.SetWellKnownGeogCS("WGS84");

		OGRCoordinateTransformation* coordTrans;
		coordTrans = OGRCreateCoordinateTransformation(&srcSpRef, &dstSpRef);
		coordTrans->Transform(1, &sOgrenvelope.MaxX, &sOgrenvelope.MinY);
		coordTrans->Transform(1, &sOgrenvelope.MinX, &sOgrenvelope.MaxY);
	}
	ImgAttribution.dUp = sOgrenvelope.MaxX;
	ImgAttribution.dLeft = sOgrenvelope.MinY;
	ImgAttribution.dDown = sOgrenvelope.MinX;
	ImgAttribution.dRight = sOgrenvelope.MaxY;
	QFileInfo qFile(strPath);
	qint64  nFileSize = 0;
	nFileSize = qFile.size();
	char unit = 'B';
	quint64 curSize = nFileSize;
	if (curSize > 1024) {
		curSize /= 1024;
		unit = 'K';
		if (curSize > 1024) {
			curSize /= 1024;
			unit = 'M';
			if (curSize > 1024) {
				curSize /= 1024;
				unit = 'G';
			}
		}
	}
	ImgAttribution.strFileSize = to_string(curSize) + unit;
	QList<GDALRasterBand*> bandList;
	if (nBandCount!=3)
	{
		bandList.append(poDS->GetRasterBand(1));
	}
	else
	{
		bandList.append(poDS->GetRasterBand(1));
		bandList.append(poDS->GetRasterBand(2));
		bandList.append(poDS->GetRasterBand(3));
	}

	int imgWidth = bandList[0]->GetXSize();
	int imgHeight = bandList[0]->GetYSize();
	double ShowScaleFactor = this->height() * 1.0 / imgHeight;

	int iScaleWidth = (int)(imgWidth *ShowScaleFactor - 1);
	int iScaleHeight = (int)(imgHeight *ShowScaleFactor - 1);

	GDALDataType dataType = bandList[0]->GetRasterDataType();
	// 首先分别读取RGB三个波段
	float* rBand = new float[iScaleWidth *iScaleHeight];
	float* gBand = new float[iScaleWidth *iScaleHeight];
	float* bBand = new float[iScaleWidth *iScaleHeight];
	unsigned char *rBandUC, *gBandUC, *bBandUC;
	// 如果图像正好三个波段，则默认以RGB的顺序显示彩色图
	if (nBandCount!=3)
	{
		bandList[0]->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, rBand, iScaleWidth, iScaleHeight, GDT_Float32, 0, 0);

		rBandUC = ImgSketch(rBand, bandList[0], iScaleWidth * iScaleHeight, bandList[0]->GetNoDataValue());
		gBandUC = rBandUC;
		bBandUC = rBandUC;
	}
	else
	{
		// 根据是否显示彩色图像，判断RGB三个波段的组成方式，并分别读取

		bandList[0]->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, rBand, iScaleWidth, iScaleHeight, GDT_Float32, 0, 0);
		bandList[1]->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, gBand, iScaleWidth, iScaleHeight, GDT_Float32, 0, 0);
		bandList[2]->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, bBand, iScaleWidth, iScaleHeight, GDT_Float32, 0, 0);

		// 分别拉伸每个波段并将Float转换为unsigned char
		rBandUC = ImgSketch(rBand, bandList[0], iScaleWidth * iScaleHeight, bandList[0]->GetNoDataValue());
		gBandUC = ImgSketch(gBand, bandList[1], iScaleWidth * iScaleHeight, bandList[1]->GetNoDataValue());
		bBandUC = ImgSketch(bBand, bandList[2], iScaleWidth * iScaleHeight, bandList[2]->GetNoDataValue());
	}
	// 将三个波段组合起来
	int bytePerLine = (iScaleWidth * 24 + 31) / 8;
	unsigned char* allBandUC = new unsigned char[bytePerLine * iScaleHeight];
	for (int h = 0; h < iScaleHeight; h++)
	{
		for (int w = 0; w < iScaleWidth; w++)
		{
			allBandUC[h * bytePerLine + w * 3 + 0] = rBandUC[h * iScaleWidth + w];
			allBandUC[h * bytePerLine + w * 3 + 1] = gBandUC[h * iScaleWidth + w];
			allBandUC[h * bytePerLine + w * 3 + 2] = bBandUC[h * iScaleWidth + w];
		}
	}
	// 构造图像并显示
	GDALClose(poDS);
	dstPixmap = QPixmap::fromImage(QImage(allBandUC, iScaleWidth, iScaleHeight, bytePerLine, QImage::Format_RGB888));

}

void QFrameImage::slotOutShp()
{
	//QSettings setting("./Setting.ini", QSettings::IniFormat);
	QString strDir = m_settings->value("LastFilePath").toString();
	QString strFilename =
		QFileDialog::getSaveFileName(this, tr("Export Vector File"), strDir, tr("ESRI ShapeFile(*.shp)"));
	if (strFilename.isEmpty())
		return;

	m_settings->setValue("LastFilePath", strFilename);
	ui.lineEdit_OutShp->setText(strFilename);
}

void QFrameImage::slotFrame()
{

	QString strRaster = ui.lineEdit_InPutImage->text();
	QString strVector = ui.lineEdit_OutShp->text();

	if (strRaster.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Raster filename is empty."));
		return;
	}

	if (strVector.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Vector filename is empty."));
		return;
	}

	//执行生成矢量文件
	string strFormat = "ESRI Shapefile";
	string strRasterPath = strRaster.toStdString();
	string strVectorPath = strVector.toStdString();

	std::vector<string> vVectorName;
	int            nExtension = ui.spinBox->value();

	if (ImageStdFrame2Vector(
		vVectorName, strRasterPath.c_str(), m_nScaleFactor, nExtension, strVectorPath.c_str(), strFormat.c_str()))
	{
		SetList(vVectorName);
		QMessageBox::information(this, tr("Information"), tr("Subset vector was created successfully."));
	}
	else
	{
		QMessageBox::information(this, tr("Information"), tr("Large scales do not support geographical coordinates."));
	}
}

void QFrameImage::slotScaleChanged(int idx)
{
	switch (idx)
	{
	default:
		break;
	case 0:
		m_nScaleFactor = 1000000;
		break;
	case 1:
		m_nScaleFactor = 500000;
		break;
	case 2:
		m_nScaleFactor = 250000;
		break;
	case 3:
		m_nScaleFactor = 100000;
		break;
	case 4:
		m_nScaleFactor = 50000;
		break;
	case 5:
		m_nScaleFactor = 25000;
		break;
	case 6:
		m_nScaleFactor = 10000;
		break;
	case 7:
		m_nScaleFactor = 5000;
		break;
	case 8:
		m_nScaleFactor = 2000;
		break;
	case 9:
		m_nScaleFactor = 1000;
		break;
	case 10:
		m_nScaleFactor = 500;
		break;
	}
}

void QFrameImage::slotSubSet()
{
	QList<QListWidgetItem*> qListChecked = ui.listWidgetFrameList->selectedItems();
	if (qListChecked.empty())
	{
		QMessageBox::information(this, tr("Information"), tr("Please select vector field."));
		return;
	}

	//输出路径
	QString qstrFolder = ui.lineEdit_OutImage->text();
	QString qstrSrcFile = ui.lineEdit_InPutImage->text();
	QString qstrSrcShape = ui.lineEdit_OutShp->text();

	if (qstrFolder.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Output folder is empty."));
		return;
	}
	if (qstrSrcFile.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Imput image path is empty."));
		return;
	}

	string strFolder = qstrFolder.toStdString();
	string strSrcFile =qstrSrcFile.toStdString();
	string strSubFile =qstrSrcShape.toStdString();

	QString qsFailedSubName = tr("Image ");
	int     nFaildNumber = 0;
	for (auto iter : qListChecked)
	{
		QString strShape = iter->text();

		string strBasename = strShape.toStdString();
		string strWhereFilter = "NAME='" + strBasename + "'";
		string strDstFileName =
			CPLFormFilename(strFolder.c_str(), strBasename.c_str(), CPLGetExtension(strSrcFile.c_str()));

		if (!ImageSubset4Vector(strSrcFile.c_str(), strSubFile.c_str(), strWhereFilter.c_str(), strDstFileName.c_str()))
		{
			qsFailedSubName = qsFailedSubName + strShape + ",";
			nFaildNumber++;
		}
	}

	if (nFaildNumber != 0)
		QMessageBox::information(this, tr("Information"), qsFailedSubName + tr(" was subset failed."));
	else
		QMessageBox::information(this, tr("Information"), tr("Image was subset successfully."));
}

void QFrameImage::slotOutDir()
{
	//QSettings setting("./Setting.ini", QSettings::IniFormat);
	QString strDir = m_settings->value("LastFilePath").toString();
	QString strFolder = QFileDialog::getExistingDirectory(this, tr("Select folder to save subset images"), strDir);
	if (strFolder.isEmpty())
		return;
	ui.lineEdit_OutImage->setText(strFolder);

}

void QFrameImage::slotCheckDlg()
{

	CQualityCheckDlg * pCheckDlg = new CQualityCheckDlg();
	pCheckDlg->setWindowIcon(QIcon("check.ico"));
	pCheckDlg->setModal(true);
	pCheckDlg->exec();
	
}
