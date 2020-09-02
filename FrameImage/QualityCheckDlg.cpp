#include "QualityCheckDlg.h"



#include "boost/algorithm/string.hpp"

#include "cpl_string.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"

using namespace std;

inline string DoubleToString(const double& dfnum, const char* pszFormat)
{
	char chcode[20] = { 0 };
	sprintf_s(chcode, 20, pszFormat, dfnum);
	return string(chcode);
}
inline string Convert2StdString(QString  qSrc)
{
	string s = qSrc.toStdString();
	return s;

}
inline QString GetLastDirectory(const char * strFiletype)
{

	QSettings setting("./Setting.ini", QSettings::IniFormat);
	QString strDir = setting.value("LastFilePath").toString();
	return strDir;
}



CQualityCheckDlg::CQualityCheckDlg(QWidget* parent)
    : QDialog(parent)
{
	ui.setupUi(this);

	QObject::connect(ui.pBtnInput, SIGNAL(clicked()), this, SLOT(slotInput()));
	
	QObject::connect(ui.pBtnCheck, SIGNAL(clicked()), this, SLOT(slotCheck()));
	QObject::connect(ui.pBtnExport, SIGNAL(clicked()), this, SLOT(slotExport()));
	QObject::connect(ui.comboBoxScale, SIGNAL(currentIndexChanged(int)), this, SLOT(slotScaleChanged(int)));

	if (GetGDALDriverManager()->GetDriverCount() < 1)
		GDALAllRegister();

	ui.tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	m_nScaleFactor = 500;

	m_strRegex = "\\d{4}\\.\\d{2}-\\d{3}\\.\\d{2}";
}

CQualityCheckDlg::~CQualityCheckDlg() {}

void CQualityCheckDlg::CheckFile(const QFileInfo& oFileInfo, QAInfo& info)
{
	bool bExt = ui.checkBoxExt->isChecked();

	QString strFilename = oFileInfo.absoluteFilePath();
	string  strImgFile  = Convert2StdString(strFilename);
	string  strTfwFile  = CPLResetExtension(strImgFile.c_str(), "tfw");

	// 若tfw文件不存在，直接返回
	if (CPLCheckForFile((char*)strTfwFile.c_str(), nullptr) == FALSE)
	{
		info.bFileValid = false;
		return;
	}

	FILE* pWdFile = VSIFOpen(strTfwFile.c_str(), "rt");
	if (pWdFile == nullptr)
	{
		info.bFileValid = false;
		return;
	}

	char szBuffer[128] = { 0 };
	int  nReadBytes    = (int)VSIFRead(szBuffer, 1, 128, pWdFile);
	VSIFClose(pWdFile);

	if (nReadBytes < 20)
	{
		info.bFileValid = false;
		return;
	}

	char** papszTokens = CSLTokenizeString2(szBuffer, "\n\r", CSLT_HONOURSTRINGS | CSLT_STRIPLEADSPACES);
	if (CSLCount(papszTokens) != 6)
	{
		CSLDestroy(papszTokens);
		info.bFileValid = false;
		return;
	}

	GDALDataset* poDS = (GDALDataset*)GDALOpen(strImgFile.c_str(), GA_ReadOnly);
	if (poDS == nullptr)
	{
		CSLDestroy(papszTokens);
		info.bFileValid = false;
		return;
	}

	//判断文件是否可以打开
	info.bFileValid = true;

	//检查文件名是否合规
	QString strBasename = CPLGetBasename(strImgFile.c_str());
	QRegExp oRegExp(m_strRegex);
	info.bFilenameConventions = oRegExp.exactMatch(strBasename);
	if (!info.bFilenameConventions)
	{
		CSLDestroy(papszTokens);
		info.bFileValid = false;
		return;
	}

	//判断分辨率是否合规
	if (CPLAtof(papszTokens[0]) == fabs(CPLAtof(papszTokens[3])))
		info.bResolutionCorrect = true;
	else
		info.bResolutionCorrect = false;

	//设置图像宽高大小
	m_nHeight = int(m_nScaleFactor * 0.5 / CPLAtof(papszTokens[0]));
	m_nWidth  = m_nHeight;

	//判断旋转角度是否为0
	if (CPLAtof(papszTokens[1]) == CPLAtof(papszTokens[2]) && CPLAtof(papszTokens[1]) == 0)
		info.bRotationCorrect = true;
	else
		info.bRotationCorrect = false;

	int nSizeX  = poDS->GetRasterXSize();
	int nSizeY  = poDS->GetRasterYSize();
	int nExtPix = int(0.01 * m_nScaleFactor / CPLAtof(papszTokens[0]));

	//检查文件宽高是否合规
	if (bExt)
	{
		if (nSizeX == m_nWidth + 2 * nExtPix && nSizeY == m_nHeight + 2 * nExtPix)
			info.bDimensionalityCorrect = true;
		else
			info.bDimensionalityCorrect = false;
	}
	else
	{
		if (nSizeX == m_nWidth && nSizeY == m_nHeight)
			info.bDimensionalityCorrect = true;
		else
			info.bDimensionalityCorrect = false;
	}

	QStringList list = strBasename.split("-");

	double dfTfwSwX = CPLAtof(papszTokens[4]);
	double dfTfwSwY = CPLAtof(papszTokens[5]);

	string strTfwSwX, strTfwSwY;
	if (bExt)
	{
		if (m_nScaleFactor == 500)
		{
			strTfwSwX = DoubleToString((dfTfwSwX + 0.01 * m_nScaleFactor) * 0.001, "%.2lf");
			strTfwSwY = DoubleToString((dfTfwSwY + 0.01 * m_nScaleFactor) * 0.001, "%.2lf");
		}
		else
		{
			strTfwSwX = DoubleToString((dfTfwSwX + 0.01 * m_nScaleFactor) * 0.001, "%.1lf");
			strTfwSwY = DoubleToString((dfTfwSwY + 0.01 * m_nScaleFactor) * 0.001, "%.1lf");
		}

		if (boost::iequals(Convert2StdString(list[1]).c_str(), strTfwSwX)
		    && boost::iequals(Convert2StdString(list[0]).c_str(), strTfwSwY))
			info.bSouthwestCorrdinate = true;
		else
			info.bSouthwestCorrdinate = false;
	}
	else
	{
		if (m_nScaleFactor == 500)
		{
			strTfwSwX = DoubleToString(dfTfwSwX * 0.001, "%.2lf");
			strTfwSwY = DoubleToString(dfTfwSwY * 0.001, "%.2lf");
		}
		else
		{
			strTfwSwX = DoubleToString(dfTfwSwX * 0.001, "%.1lf");
			strTfwSwY = DoubleToString(dfTfwSwY * 0.001, "%.1lf");
		}

		if (boost::iequals(Convert2StdString(list[1]).c_str(), strTfwSwX)
		    && boost::iequals(Convert2StdString(list[0]).c_str(), strTfwSwY))
			info.bSouthwestCorrdinate = true;
		else
			info.bSouthwestCorrdinate = false;
	}

	CSLDestroy(papszTokens);

	

	GDALClose((GDALDatasetH)poDS);
}

void CQualityCheckDlg::slotScaleChanged(int index)
{
	switch (index)
	{
	case 0:  // 500
	{
		m_strRegex     = "\\d{4}\\.\\d{2}-\\d{3}\\.\\d{2}";
		m_nScaleFactor = 500;
		break;
	}
	case 1:  // 1000
	{
		m_strRegex     = "\\d{4}\\.\\d{1}-\\d{3}\\.\\d{1}";
		m_nScaleFactor = 1000;
		break;
	}
	case 2:  // 2000
	{
		m_strRegex     = "\\d{4}\\.\\d{1}-\\d{3}\\.\\d{1}";
		m_nScaleFactor = 2000;
		break;
	}
	default:
		break;
	}
}

void CQualityCheckDlg::slotInput()
{
	QString strLastDir = GetLastDirectory("qa_src_dir");
	QString strSrcDir  = QFileDialog::getExistingDirectory(this, tr("Select Folder"), strLastDir);

	//SetLastDirectory("qa_src_dir", strSrcDir);
	ui.lineEditInput->setText(strSrcDir);
}


void CQualityCheckDlg::slotCheck()
{
	ui.tableWidget->clearContents();
	ui.tableWidget->setRowCount(0);

	// 1 获取文件夹中的文件
	QString strCurrentFormat = ui.comboBoxFmt->currentText();
	QString strFormat        = strCurrentFormat.right(6).left(5);

	QStringList strFilters;
	strFilters << strFormat;
	QDir oDir(ui.lineEditInput->text());

	QFileInfoList listFileInfo = oDir.entryInfoList(strFilters, QDir::Files | QDir::Readable, QDir::Name);

	// 2 插入列表
	for (auto& oFileInfo : listFileInfo)
	{
		QAInfo info;
		CheckFile(oFileInfo.absoluteFilePath(), info);

		// 3 检查每个文件
		int nRow = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(nRow);

		QTableWidgetItem* pItemName = new QTableWidgetItem(oFileInfo.absoluteFilePath());
		ui.tableWidget->setItem(nRow, 0, pItemName);                                                               // name
		ui.tableWidget->setItem(nRow, 1, new QTableWidgetItem(info.bFilenameConventions ? "TRUE" : "FALSE"));      // name
		ui.tableWidget->setItem(nRow, 2, new QTableWidgetItem(info.bResolutionCorrect ? "TRUE" : "FALSE"));        // res
		ui.tableWidget->setItem(nRow, 3, new QTableWidgetItem(info.bRotationCorrect ? "TRUE" : "FALSE"));          // rot
		ui.tableWidget->setItem(nRow, 4, new QTableWidgetItem(info.bSouthwestCorrdinate ? "TRUE" : "FALSE"));      // sw
		ui.tableWidget->setItem(nRow, 5, new QTableWidgetItem(info.bDimensionalityCorrect ? "TRUE" : "FALSE"));    // dim
 	}
}

void CQualityCheckDlg::slotExport()
{
	QString qstrDir = ui.lineEditInput->text();
	if (ui.tableWidget->rowCount() == 0)
	{
		QMessageBox::information(this, tr("Information"), tr("Please click the check button first."));
		return;
	}

	QString qstrFileName = qstrDir + "/" + "QualityCheckReport" + ".txt";

	QFile Out(qstrFileName);
	if (Out.exists())
		Out.remove();

	bool bFlag = Out.open(QIODevice::Truncate | QIODevice::ReadWrite);
	if (!bFlag)
	{
		qDebug() << "open failure";
		return;
	}

	QTextStream  stream(&Out);
	QHeaderView* header = ui.tableWidget->horizontalHeader();
	if (header != nullptr)
	{
		for (int i = 0; i < header->count(); i++)
		{
			QTableWidgetItem* item = ui.tableWidget->horizontalHeaderItem(i);
			if (item != nullptr)
			{
				QString strhead(item->text());
				stream << strhead.simplified() << ",";
			}
		}
		stream << "\n";
	}

	for (int nRow = 0; nRow < ui.tableWidget->rowCount(); nRow++)
	{
		for (int nColumn = 0; nColumn < ui.tableWidget->columnCount(); nColumn++)
		{
			QTableWidgetItem* item = ui.tableWidget->item(nRow, nColumn);
			if (item != nullptr)
			{
				stream << item->text() << ",";
			}
		}
		stream << "\n";
	}
	Out.close();

	QMessageBox::information(this, tr("Information"), tr("Export report successfully."));
}
