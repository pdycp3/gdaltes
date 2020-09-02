#include "SelectProjectionDlg.h"
//#include "CommonUtil.h"

#include "cpl_string.h"
#include "ogr_spatialref.h"
#include "sqlite3.h"

CSelectProjectionDlg::CSelectProjectionDlg(QWidget* parent)
    : QDialog(parent)
    , mProjection("")
    , mProjListDone(false)
    , mShowGeogcs(true)
{
	setupUi(this);

	connect(pushButtonOK, SIGNAL(clicked()), this, SLOT(slotOK()));
	connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
	connect(pushButtonCustom, SIGNAL(clicked()), this, SLOT(slotCustom()));
	connect(comboBoxFilter, SIGNAL(editTextChanged(const QString&)), this, SLOT(FilterTextChanged(const QString&)));
	connect(
	    lstCoordinateSystems, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this,
	    SLOT(LstCoordinateSystemsCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	// 不显示经纬度坐标系
	mShowGeogcs = false;

	// 获取存储空间参考系统的数据库文件
	mSrsDatabaseFileName = QCoreApplication::applicationDirPath() + "/data/srs.db";

	QHeaderView* header = lstCoordinateSystems->header();
	header->setSectionResizeMode(AUTHID_COLUMN, QHeaderView::Stretch);
	header->resizeSection(QGIS_CRS_ID_COLUMN, 0);
	header->setSectionResizeMode(QGIS_CRS_ID_COLUMN, QHeaderView::Fixed);

	QPalette pt;
	pt.setColor(QPalette::AlternateBase, QColor::fromRgb(141, 180, 226, 150));
	lstCoordinateSystems->setPalette(pt);

	// 隐藏ID列，内部使用
	lstCoordinateSystems->setColumnHidden(QGIS_CRS_ID_COLUMN, true);

	LoadCrsList(&mCrsFilter);
	ApplySelection();
	lstCoordinateSystems->setCurrentItem(lstCoordinateSystems->topLevelItem(0)->child(0));
}

CSelectProjectionDlg::~CSelectProjectionDlg() {}

void CSelectProjectionDlg::resizeEvent(QResizeEvent* theEvent)
{
	QHeaderView* pHeader = lstCoordinateSystems->header();
	pHeader->resizeSection(NAME_COLUMN, theEvent->size().width() - 240);
	pHeader->resizeSection(AUTHID_COLUMN, 240);
	pHeader->resizeSection(QGIS_CRS_ID_COLUMN, 0);
}

void CSelectProjectionDlg::LoadCrsList(QSet<QString>* crsFilter)
{
	if (mProjListDone)
		return;

	QString sqlFilter = OgcWmsCrsFilterAsSqlExpression(crsFilter);

	// Geographic coordinate system node
	if (mShowGeogcs)
	{
		mGeoList = new QTreeWidgetItem(lstCoordinateSystems, QStringList(tr("Geographic coordinate system")));
		//mGeoList->setIcon(0, QIcon(":/res/geographic.png"));
	}
	// Projected coordinate system node
	mProjList = new QTreeWidgetItem(lstCoordinateSystems, QStringList(tr("Projected coordinate system")));
	//mProjList->setIcon(0, QIcon(":/res/transformed.png"));

	if (!QFileInfo(mSrsDatabaseFileName).exists())
	{
		mProjListDone = true;
		return;
	}

	// open the database containing the spatial reference data
	sqlite3* database;
	int      rc = sqlite3_open_v2(mSrsDatabaseFileName.toUtf8().data(), &database, SQLITE_OPEN_READONLY, nullptr);
	if (rc)
	{
		// XXX This will likely never happen since on open, sqlite creates the database if it does not exist.
		ShowDBMissingWarning(mSrsDatabaseFileName);
		return;
	}

	// prepare the sql statement
	const char*   tail;
	sqlite3_stmt* stmt;
	// get total count of records in the projection table
	QString sql = "select count(*) from tbl_srs";

	rc = sqlite3_prepare(database, sql.toUtf8(), sql.toUtf8().length(), &stmt, &tail);
	Q_ASSERT(rc == SQLITE_OK);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	// Set up the query to retrieve the projection information needed to populate the list
	// note I am giving the full field names for clarity here and in case someone
	// changes the underlying view TS
	sql = QString("select description, srs_id, upper(auth_name||':'||auth_id), is_geo, name, parameters, deprecated "
	              "from vw_srs where %1 order by name,description")
	          .arg(sqlFilter);

	rc = sqlite3_prepare(database, sql.toUtf8(), sql.toUtf8().length(), &stmt, &tail);
	// XXX Need to free memory from the error msg if one is set
	if (rc == SQLITE_OK)
	{
		QTreeWidgetItem* newItem = nullptr;
		// Cache some stuff to speed up creating of the list of projected
		// spatial reference systems
		QString          previousSrsType("");
		QTreeWidgetItem* previousSrsTypeNode = 0;

		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			// check to see if the srs is geographic
			int isGeo = sqlite3_column_int(stmt, 3);
			if (isGeo)
			{
				if (mShowGeogcs)
				{
					// this is a geographic coordinate system
					// Add it to the tree (field 0)
					newItem = new QTreeWidgetItem(
					    mGeoList, QStringList(QString::fromUtf8((char*)sqlite3_column_text(stmt, 0))));

					// display the authority name (field 2) in the second column of the list view
					newItem->setText(AUTHID_COLUMN, QString::fromUtf8((char*)sqlite3_column_text(stmt, 2)));

					// display the qgis srs_id (field 1) in the third column of the list view
					newItem->setText(QGIS_CRS_ID_COLUMN, QString::fromUtf8((char*)sqlite3_column_text(stmt, 1)));
				}
			}
			else
			{
				// This is a projected srs
				QTreeWidgetItem* node    = nullptr;
				QString          srsType = QString::fromUtf8((char*)sqlite3_column_text(stmt, 4));
				// Find the node for this type and add the projection to it
				// If the node doesn't exist, create it
				if (srsType == previousSrsType)
				{
					node = previousSrsTypeNode;
				}
				else
				{  // Different from last one, need to search
					QList<QTreeWidgetItem*> nodes =
					    lstCoordinateSystems->findItems(srsType, Qt::MatchExactly | Qt::MatchRecursive, NAME_COLUMN);
					if (nodes.count() == 0)
					{
						// the node doesn't exist -- create it
						// Make in an italic font to distinguish them from real projections
						node           = new QTreeWidgetItem(mProjList, QStringList(srsType));
						QFont fontTemp = node->font(0);
						fontTemp.setItalic(true);
						node->setFont(0, fontTemp);
					}
					else
					{
						node = nodes.first();
					}
					// Update the cache.
					previousSrsType     = srsType;
					previousSrsTypeNode = node;
				}
				// add the item, setting the projection name in the first column of the list view
				newItem =
				    new QTreeWidgetItem(node, QStringList(QString::fromUtf8((char*)sqlite3_column_text(stmt, 0))));
				// display the authority id (field 2) in the second column of the list view
				newItem->setText(AUTHID_COLUMN, QString::fromUtf8((char*)sqlite3_column_text(stmt, 2)));
				// display the qgis srs_id (field 1) in the third column of the list view
				newItem->setText(QGIS_CRS_ID_COLUMN, QString::fromUtf8((char*)sqlite3_column_text(stmt, 1)));
				// expand also parent node
				QTreeWidgetItem* parent = newItem->parent();
				if (parent != nullptr)
					parent->setExpanded(true);
			}

			// display the qgis deprecated in the user data of the item
			newItem->setData(0, Qt::UserRole, QString::fromUtf8((char*)sqlite3_column_text(stmt, 6)));
		}
		mProjList->setExpanded(true);
	}

	// close the sqlite3 statement
	sqlite3_finalize(stmt);
	// close the database
	sqlite3_close(database);

	mProjListDone = true;
}

void CSelectProjectionDlg::ApplySelection(int column, QString value)
{
	if (!mProjListDone)
	{
		// defer selection until loaded
		mSearchColumn = column;
		mSearchValue  = value;
		return;
	}

	if (column == NONE)
	{
		// invoked deferred selection
		column = mSearchColumn;
		value  = mSearchValue;

		mSearchColumn = NONE;
		mSearchValue.clear();
	}

	if (column == NONE)
		return;

	QList<QTreeWidgetItem*> nodes =
	    lstCoordinateSystems->findItems(value, Qt::MatchExactly | Qt::MatchRecursive, column);
	if (nodes.count() > 0)
	{
		lstCoordinateSystems->setCurrentItem(nodes.first());
	}
	else
	{
		// unselect the selected item to avoid confusing the user
		lstCoordinateSystems->clearSelection();
		teProjection->setText("");
		teSelected->setText("");
	}
}

QString CSelectProjectionDlg::OgcWmsCrsFilterAsSqlExpression(QSet<QString>* crsFilter)
{
	QString                    sqlExpression = "1";  // it's "SQL" for "true"
	QMap<QString, QStringList> authParts;

	if (!crsFilter)
		return sqlExpression;

	foreach (QString auth_id, crsFilter->values())
	{
		QStringList parts = auth_id.split(":");

		if (parts.size() < 2)
			continue;

		authParts[parts.at(0).toUpper()].append(parts.at(1).toUpper());
	}

	if (authParts.isEmpty())
		return sqlExpression;

	if (authParts.size() > 0)
	{
		QString prefix = " AND (";
		foreach (QString auth_name, authParts.keys())
		{
			sqlExpression += QString("%1(upper(auth_name)='%2' AND upper(auth_id) IN ('%3'))")
			                     .arg(prefix)
			                     .arg(auth_name)
			                     .arg(authParts[auth_name].join("','"));
			prefix = " OR ";
		}
		sqlExpression += ")";
	}

	return sqlExpression;
}

void CSelectProjectionDlg::LstCoordinateSystemsCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	(void*)(previous);
	if (!current)
	{
		printf("no current item");
		return;
	}

	lstCoordinateSystems->scrollToItem(current);

	// If the item has children, it's not an end node in the tree, and
	// hence is just a grouping thingy, not an actual CRS.
	if (current->childCount() == 0)
	{
		teProjection->setText(GetSelectedWKTString());
		teSelected->setText(GetSelectedName());
		lstCoordinateSystems->setFocus(Qt::OtherFocusReason);
	}
	else
	{
		current->setSelected(false);
		teProjection->setText("");
		teSelected->setText("");
	}
}

void CSelectProjectionDlg::FilterTextChanged(const QString& theFilterTxt)
{
	QString filterTxt = theFilterTxt;
	filterTxt.replace(QRegExp("\\s+"), ".*");
	QRegExp re(filterTxt, Qt::CaseInsensitive);

	// filter crs's
	QTreeWidgetItemIterator it(lstCoordinateSystems);
	while (*it)
	{
		if ((*it)->childCount() == 0)  // it's an end node aka a projection
		{
			if ((*it)->text(NAME_COLUMN).contains(re) || (*it)->text(AUTHID_COLUMN).contains(re))
			{
				(*it)->setHidden(false);
				QTreeWidgetItem* parent = (*it)->parent();
				while (parent)
				{
					parent->setExpanded(true);
					parent->setHidden(false);
					parent = parent->parent();
				}
			}
			else
			{
				(*it)->setHidden(true);
			}
		}
		else
		{
			(*it)->setHidden(true);
		}
		++it;
	}
}

long CSelectProjectionDlg::GetSelectedCrsId()
{
	QTreeWidgetItem* item = lstCoordinateSystems->currentItem();

	if (item && !item->text(QGIS_CRS_ID_COLUMN).isEmpty())
		return lstCoordinateSystems->currentItem()->text(QGIS_CRS_ID_COLUMN).toLong();
	else
		return 0;
}

QString CSelectProjectionDlg::GetSelectedName()
{
	QTreeWidgetItem* lvi = lstCoordinateSystems->currentItem();
	return lvi ? lvi->text(NAME_COLUMN) : QString::null;
}

QString CSelectProjectionDlg::GetSelectedWKTString()
{
	// 获取当前选中的节点
	QTreeWidgetItem* item = lstCoordinateSystems->currentItem();
	if (!item || item->text(QGIS_CRS_ID_COLUMN).isEmpty())
		return "";

	// 获取ID
	QString srsId = item->text(QGIS_CRS_ID_COLUMN);

	sqlite3* database = nullptr;
	int      rc       = sqlite3_open_v2(mSrsDatabaseFileName.toUtf8().data(), &database, SQLITE_OPEN_READONLY, nullptr);
	if (rc)
	{
		ShowDBMissingWarning(mSrsDatabaseFileName);
		return "";
	}

	// 构造SQL进行查询
	const char*   tail;
	sqlite3_stmt* stmt;
	QString       sql = QString("select parameters from tbl_srs where srs_id=%1").arg(srsId);

	rc = sqlite3_prepare(database, sql.toUtf8(), sql.toUtf8().length(), &stmt, &tail);

	QString projString;
	if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
	{
		projString = QString::fromUtf8((char*)sqlite3_column_text(stmt, 0));
	}

	// close the statement
	sqlite3_finalize(stmt);
	// close the database
	sqlite3_close(database);
	QString             strProjName = GetSelectedName();
	OGRSpatialReference osr;
	osr.importFromProj4(Convert2StdString(projString).c_str());
	if (osr.IsProjected())
		osr.SetProjCS(Convert2StdString(strProjName).c_str());
	else if (osr.IsGeocentric())
		osr.SetGeocCS(Convert2StdString(strProjName).c_str());
	else
		;  //暂时不支持设置其他坐标系统名称

	char* pszWkt = nullptr;
	osr.exportToPrettyWkt(&pszWkt);

	projString = pszWkt;
	CPLFree(pszWkt);

	return projString;
}

void CSelectProjectionDlg::slotOK()
{
	QString strPrettyWkt = teProjection->toPlainText();
	if (strPrettyWkt.isEmpty())
	{
		QMessageBox::information(this, tr("Information"), tr("Please select a SRS."));
		return;
	}

	OGRSpatialReference osr(Convert2StdString(strPrettyWkt).c_str());

	char* pszWkt = nullptr;
	osr.exportToWkt(&pszWkt);
	mProjection = pszWkt;
	CPLFree(pszWkt);

	accept();
}

void CSelectProjectionDlg::SetProjection(const QString& strProject)
{
	mProjection = strProject;
	OGRSpatialReference osr(Convert2StdString(strProject).c_str());

	char* pszWkt = nullptr;
	osr.exportToPrettyWkt(&pszWkt);
	teProjection->setText(QString(pszWkt));
	CPLFree(pszWkt);
}

QString CSelectProjectionDlg::GetProjection()
{
	return mProjection;
}

QString CSelectProjectionDlg::GetPrettyProjection()
{
	OGRSpatialReference osr(Convert2StdString(mProjection).c_str());

	char* pszWkt = nullptr;
	osr.exportToPrettyWkt(&pszWkt);
	QString strWkt = pszWkt;
	CPLFree(pszWkt);
	return strWkt;
}

void CSelectProjectionDlg::ShowDBMissingWarning(const QString& theFileName)
{
	QMessageBox::critical(
	    this, tr("Resource Error"), tr("From path: \n %1\n read database file failed!").arg(theFileName));
}

void CSelectProjectionDlg::slotCustom()
{
	const QString& strDir         = GetLastDirectory("open_srs_file");
	const QString& strSrsFilename = QFileDialog::getOpenFileName(
	    this, tr("Open Spatial Reference System File"), strDir, tr("Spatial Reference System File(*.prj *.txt)"));

	if (strSrsFilename.isEmpty())
		return;

	SetLastDirectory("open_srs_file", strSrsFilename);

	std::string strFilename = Convert2StdString(strSrsFilename);

	OGRSpatialReference oSRS;
	if (oSRS.SetFromUserInput(strFilename.c_str()) != OGRERR_NONE)
	{
		QMessageBox::warning(
		    this, tr("Warning"),
		    tr("Selected file %1 is not valid spatial reference system file.").arg(strSrsFilename));
		return;
	}

	char* pszWkt = nullptr;
	oSRS.exportToWkt(&pszWkt);
	mProjection = pszWkt;
	CPLFree(pszWkt);
}
