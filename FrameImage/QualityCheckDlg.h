#ifndef QUALITYCHECKDLG_H
#define QUALITYCHECKDLG_H



#include "ui_QualityCheckDlg.h"
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class CQualityCheckDlg : public QDialog
{
	Q_OBJECT

public:
	CQualityCheckDlg(QWidget* parent = nullptr);
	~CQualityCheckDlg(void);

private:
	struct QAInfo
	{
		bool bFileValid;
		bool bFilenameConventions;
		bool bResolutionCorrect;
		bool bRotationCorrect;
		bool bSouthwestCorrdinate;
		bool bDimensionalityCorrect;
		

		QAInfo()
		{
			bFileValid               = false;
			bFilenameConventions     = false;
			bResolutionCorrect       = false;
			bRotationCorrect         = false;
			bSouthwestCorrdinate     = false;
			bDimensionalityCorrect   = false;
			
		}
	};

	QString m_strRegex;

	int m_nWidth;
	int m_nHeight;
	int m_nScaleFactor;

private:
	void CheckFile(const QFileInfo& oFileInfo, QAInfo& info);

private slots:

	void slotInput();

	void slotCheck();
	void slotExport();
	void slotScaleChanged(int index);
private:
	Ui::QualityCheckDlg ui;
};

#endif  // QUALITYCHECKDLG_H