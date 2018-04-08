// PoiInfoToHNPUpdater.cpp : Defines the entry point for the console application.
//

#pragma once
#include "stdafx.h"
#include "PoiInfoToHNPUpdater.h"


PoiInfoToHNPUpdater::PoiInfoToHNPUpdater() {
	//error class setup
	IOManager::getInstance().initErrorMessage();
	m_OptionManager = new OptionManager();
	m_dataManager = new DataManager();
}

PoiInfoToHNPUpdater::~PoiInfoToHNPUpdater() {

}

int PoiInfoToHNPUpdater::setNewOption() {
	bool is_FGDB = false;
	OptionManager * opt = m_OptionManager;
	//check if the input is FGDB 
	if (opt->m_db.find(L"@") == std::string::npos  && opt->m_db.find(L".gdb") != std::string::npos) is_FGDB = true;
	//since boost can't define default value for wstring. Define the default value here
	if (opt->m_hnp.empty()) {
		opt->m_hnp = L"HNP";
	}
	if (opt->m_hnp_entrypoint.empty()) {
		opt->m_hnp_entrypoint = L"HNP_ENTRYPOINT";
	}
	if (opt->m_poi_info.empty()) {
		opt->m_poi_info = L"POI_INFO";
	}
	if (opt->m_poi_entrypoint.empty()) {
		opt->m_poi_entrypoint = L"POI_ENTRYPOINT";
	}
	if (opt->m_poi_asso.empty()) {
		opt->m_poi_asso = L"POI_ASSOCIATION";
	}
	if (opt->m_official.empty()) {
		opt->m_official = L"OFFICIAL_NAME";
	}
	if (opt->m_translation.empty()) {
		opt->m_translation = L"TRANSLATION";
	}
	if (opt->m_sql_poiinfo.empty()) {
		opt->m_sql_poiinfo = L"(ACCURACY_C between 1 and 2) and DELETION_C=0 and HOUSENUMBER is not null";
	}
	if (opt->m_sql_poientry.empty()) {
		opt->m_sql_poientry = L"PRIORITY_F = 1 and (ACCURACY_C between 1 and 2)";
	}
	if (opt->m_buffer_size.empty()) {
		opt->m_buffer_size = L"100";
	}
	if (opt->m_err_log.empty()) {
		opt->m_err_log = L"";
	}
	if (opt->m_run_log.empty()) {
		opt->m_run_log = L"";
	}
	//parse to data manager
	try {
		m_dataManager->setNewFile(opt->k_err_log, opt->m_err_log.c_str(), IOManager::FileType::ERR);
		m_dataManager->setNewFile(opt->k_run_log, opt->m_run_log.c_str(), IOManager::FileType::RUN);
		m_dataManager->setNewDB(opt->k_db, is_FGDB, false, opt->m_db.c_str() );
		m_dataManager->setNewFeatureClass(opt->k_db, opt->k_hnp, is_FGDB, opt->m_hnp.c_str());
		m_dataManager->setNewFeatureClass(opt->k_db, opt->k_poi_info, is_FGDB, opt->m_poi_info.c_str());
		m_dataManager->setNewFeatureClass(opt->k_db, opt->k_hnp_entrypoint, is_FGDB, opt->m_hnp_entrypoint.c_str());
		m_dataManager->setNewFeatureClass(opt->k_db, opt->k_poi_entrypoint, is_FGDB, opt->m_poi_entrypoint.c_str());
		m_dataManager->setNewTable(opt->k_db, opt->k_poi_asso, is_FGDB, opt->m_poi_asso.c_str());
		m_dataManager->setNewTable(opt->k_db, opt->k_official, is_FGDB, opt->m_official.c_str());
		m_dataManager->setNewTable(opt->k_db, opt->k_translation, is_FGDB, opt->m_translation.c_str());
	}
	catch (const _com_error e) {
		IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::presetData(const int& argc, _TCHAR* argv[])
{
	try {
		//Get option list from console
		if (is_success != m_OptionManager->getOption(argc, argv)) {
			return IOManager::getInstance().endProgramWithError(_T("getting options"));
		}
		//Parse all option into data manager 
		if (is_success != setNewOption()) {
			return IOManager::getInstance().endProgramWithError(_T("setting options"));
		}
		//Create files specified in option
		if (is_success != m_dataManager->createFiles()) {
			return IOManager::getInstance().endProgramWithError(_T("create files"));
		}
		//Print specified option into run log 
		m_OptionManager->printDescription();
		//Initialize connection to server
		//Initialize DBs
		if (is_success != m_dataManager->initDBs()) {
			return IOManager::getInstance().endProgramWithError(_T("initialize DB"));
		}
		//Initialize featureclasses
		if (is_success != m_dataManager->initFeatureClasses()) {
			return IOManager::getInstance().endProgramWithError(_T("initialize featureclass"));
		}
		//Initialize tables
		if (is_success != m_dataManager->initTables()) {
			return IOManager::getInstance().endProgramWithError(_T("initialize featureclass"));
		}
	}
	catch (const _com_error e) {
		std::cout << std::endl;
		IOManager::getInstance().print_error(IOManager::ECode::E_COM_ERROR_IS_CATCHED, true, _T("E_COM_ERROR"), _T(""), (CString)e.ErrorMessage());
		return IOManager::getInstance().endProgramWithError("Com Error");
		std::cout << "Press any key to continue...";
		std::cin.get();
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::run() {
	//get child POI list <OBJECTID>
	std::set<int> childPOIList;
	/*if (is_success != getChildPOI(&childPOIList)) {
		return IOManager::getInstance().endProgramWithError(_T("get child POI list from POI_ASSOCIATION"));
	}
	IOManager::getInstance().print_run(true, _T("child POI list has been acquired from POI_ASSOCIATION successfully"));*/

	//get poi entry point
	std::set<int> poiEntrypointList;
	if (is_success != getPoiEntrypoint(&poiEntrypointList)) {
		return IOManager::getInstance().endProgramWithError(_T("get POI Entrypoint list from POI_ENTRYPOINT"));
	}
	IOManager::getInstance().print_run(true, _T("child POI list has been acquired from POI_ASSOCIATION successfully"));
	//get official name list <OBJECTID, NAME>
	std::map<int, CString> officialNameList;
	/*if (is_success != getOfficalName(&childPOIList, &officialNameList)) {
		return IOManager::getInstance().endProgramWithError(_T("get name list from OFFICIAL_NAME"));
	}
	IOManager::getInstance().print_run(true, _T("NAME list has been acquired from OFFICIAL_NAME successfully"));*/

	//get only unique houseNumber information for POI_INFO
	std::vector<poiInfo> uniquePoiInfoList;
	if (is_success != getUniquePoiInfoID(&childPOIList, &poiEntrypointList, &officialNameList, &uniquePoiInfoList)) {
		return IOManager::getInstance().endProgramWithError(_T("get unique house number list from POI_INFO"));
	}

	return IOManager::getInstance().printSuccessfulEnd();
}

int PoiInfoToHNPUpdater::getChildPOI(std::set<int> * childPOIList) {
	//get information about this table from Datamanager;
	DataManager::tableDesc poi_asso_table = m_dataManager->getTable(m_OptionManager->k_poi_asso);
	CString PoiAssoTableName = poi_asso_table.tableName;
	//create condition to get only childID column
	IQueryFilterPtr PoiAssoIpQueryFilter(CLSID_QueryFilter);
	CString childIDName = sindy::schema::global::poi_association::kChildID;
	if (S_OK != PoiAssoIpQueryFilter->put_SubFields((CComBSTR)childIDName)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, PoiAssoTableName, _T(""), _T("Failed to set search column for CHILDID column"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//set all condition into query variable
	_ICursorPtr ipPoiAssoCursor;
	ITablePtr PoiAssoTable = poi_asso_table.table;
	
	if (S_OK != PoiAssoTable->Search(PoiAssoIpQueryFilter, VARIANT_FALSE, &ipPoiAssoCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, PoiAssoTableName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	// get column index for child id
	long childIDIndex = 0;
	if (S_OK != ipPoiAssoCursor->Fields->FindField((CComBSTR)childIDName, &childIDIndex)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, PoiAssoTableName, _T("FIELD INDEX"), _T("Failed to get CHILDID field index"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	IOManager::getInstance().print_run(true, PoiAssoTableName, childIDName , _T("Field has been acquired successfully"));
	//set to store childID list
	//get target postal code
	_IRowPtr ipPoiAssoRow;
	while (ipPoiAssoCursor->NextRow(&ipPoiAssoRow) == S_OK && ipPoiAssoRow) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipPoiAssoRow->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, PoiAssoTableName, _T(""), _T("Failed to get OBJECTID of one ") + PoiAssoTableName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		// get child id data
		CComVariant childID;
		if (S_OK != ipPoiAssoRow->get_Value(childIDIndex, &childID)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, PoiAssoTableName + _T(" OBJECTID"), OID, _T("Failed to get CHILDID value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		childPOIList->insert(childID.intVal);
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::getPoiEntrypoint(std::set<int> * poiEntrypointList) {
	//get information about this table from Datamanager;
	DataManager::featureClassDesc poi_entrypoint_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_poi_entrypoint);
	CString poiEntryFeatureClassName = poi_entrypoint_featureClass.featureClassName;
	//create condition to get only childID column
	IQueryFilterPtr poiEntryIpQueryFilter(CLSID_QueryFilter);
	CString poiInfoIDName = sindy::schema::global::poi_entry_point::kPoiInfoID;
	if (S_OK != poiEntryIpQueryFilter->put_SubFields((CComBSTR)poiInfoIDName)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to set search column for POIINFOID column"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//set all condition into query variable
	IFeatureCursorPtr poiEntrypointCursor;
	IFeatureClassPtr poiEntryFeatureClass = poi_entrypoint_featureClass.featureClass;

	if (S_OK != poiEntryFeatureClass->Search(poiEntryIpQueryFilter, VARIANT_FALSE, &poiEntrypointCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	// get column index for child id
	long poiInfoIDIndex = 0;
	if (S_OK != poiEntrypointCursor->Fields->FindField((CComBSTR)poiInfoIDName, &poiInfoIDIndex)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName, _T("FIELD INDEX"), _T("Failed to get POIINFOID field index"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	IOManager::getInstance().print_run(true, poiEntryFeatureClassName, poiInfoIDName, _T("Field has been acquired successfully"));
	//set to store childID list
	//get target postal code
	IFeaturePtr ipPoiEntryFeature;
	while (poiEntrypointCursor->NextFeature(&ipPoiEntryFeature) == S_OK && ipPoiEntryFeature) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipPoiEntryFeature->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to get OBJECTID of one ") + poiEntryFeatureClassName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		// get child id data
		CComVariant poiInfoID;
		if (S_OK != ipPoiEntryFeature->get_Value(poiInfoIDIndex, &poiInfoID)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get POIINFOID value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		poiEntrypointList->insert(poiInfoID.intVal);
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::getOfficalName(std::set<int> * childPOIList, std::map<int, CString> * officialNameList) {
	//get information about this table from Datamanager;
	DataManager::tableDesc official_name_table = m_dataManager->getTable(m_OptionManager->k_official);
	CString officialTableName = official_name_table.tableName;
	//create condition to get only childID column
	IQueryFilterPtr officialIpQueryFilter(CLSID_QueryFilter);
	CString nameColumn = sindy::schema::global::official_name::kName;
	if (S_OK != officialIpQueryFilter->put_SubFields((CComBSTR)nameColumn)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, officialTableName, _T(""), _T("Failed to set search column for NAME column"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//set all condition into query variable
	_ICursorPtr ipOfficialCursor;
	ITablePtr officialTable = official_name_table.table;
	//search only records with layer=34 (POI_INFO)
	CComBSTR queryFilter = _T("LAYER_C = 34");
	if (S_OK != officialIpQueryFilter->put_WhereClause(queryFilter)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, officialTableName, _T(""), _T("Failed to set search query"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	if (S_OK != officialTable->Search(officialIpQueryFilter, VARIANT_FALSE, &ipOfficialCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	// get column index for child id
	long nameColumnIndex = 0;
	if (S_OK != ipOfficialCursor->Fields->FindField((CComBSTR)nameColumn, &nameColumnIndex)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName, _T("FIELD INDEX"), _T("Failed to get NAME field index"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	IOManager::getInstance().print_run(true, officialTableName, nameColumn, _T("Field has been acquired successfully"));
	//set to store childID list
	//get target postal code
	_IRowPtr ipOfficialRow;
	while (ipOfficialCursor->NextRow(&ipOfficialRow) == S_OK && ipOfficialRow) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipOfficialRow->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName, _T(""), _T("Failed to get OBJECTID of one ") + ipOfficialRow);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		// get child id data
		CComVariant name;
		if (S_OK != ipOfficialRow->get_Value(nameColumnIndex, &name)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName + _T(" OBJECTID"), OID, _T("Failed to get NAME value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		CString convertedName = name.bstrVal;
		convertedName = convertedName.Trim();
		if (!officialNameList->insert(std::make_pair(OIDNum, name.bstrVal)).second) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_PROCESS, true, officialTableName + _T(" OBJECTID"), OID, _T("Unexpected error occured while storing OFFICIAL_NAME data"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::presetPoiInfoFeatureCursor(IQueryFilterPtr PoiInfoIpQueryFilter, IFeatureCursorPtr ipPoiInfoCursor) {
	//get information about this featureClass from Datamanager;
	DataManager::featureClassDesc poi_info_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_poi_info);
	CString poiInfoFeatureClassName = poi_info_featureClass.featureClassName;
	//create condition to get specified columns
	using namespace sindy::schema::global;
	IQueryFilterPtr PoiInfoIpQueryFilter(CLSID_QueryFilter);
	CString columnFilter;
	columnFilter.Format(_T("%s,%s,%s,%s,%s"), poi_info::kObjectID, poi_info::kHouseNumber, poi_info::kRoadNameID, poi_info::kActualAddress, poi_info::kShape);
	if (S_OK != PoiInfoIpQueryFilter->put_SubFields((CComBSTR)columnFilter)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, poiInfoFeatureClassName, _T(""), _T("Failed to set search columns"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//set all condition into query variable
	IFeatureClassPtr PoiInfoFeatureClass = poi_info_featureClass.featureClass;
	//search only records with user input condition
	CComBSTR queryFilter = m_OptionManager->m_sql_poiinfo.c_str();
	if (S_OK != PoiInfoIpQueryFilter->put_WhereClause(queryFilter)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, poiInfoFeatureClassName, _T(""), _T("Failed to set search query"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	IQueryFilterDefinitionPtr queryFilterDefinition = (IQueryFilterDefinitionPtr)PoiInfoIpQueryFilter;
	queryFilterDefinition->put_PostfixClause(_T("ORDER BY HOUSENUMBER, ACTUALADDRESS, OBJECTID"));
	if (S_OK != PoiInfoFeatureClass->Search(PoiInfoIpQueryFilter, VARIANT_FALSE, &ipPoiInfoCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::getUniquePoiInfoID(std::set<int> * childPOIList, std::set<int> * poiEntrypointList, std::map<int, CString> * officialNameList, std::vector<poiInfo> * uniquePoiInfoList) {
	//get information about this featureClass from Datamanager;
	DataManager::featureClassDesc poi_info_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_poi_info);
	CString poiInfoFeatureClassName = poi_info_featureClass.featureClassName;
	using namespace sindy::schema::global;
	IQueryFilterPtr PoiInfoIpQueryFilter(CLSID_QueryFilter);
	IFeatureCursorPtr ipPoiInfoCursor;
	if (S_OK != presetPoiInfoFeatureCursor(PoiInfoIpQueryFilter, ipPoiInfoCursor)){
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	// get column index for ACTULACDRESS, HOUSENUMBER, ROADNAMEID
	long roadNameIDIndex = 0, houseNumberIndex = 0, actualAddressIndex = 0;
	getColumnIndex(PoiInfoIpQueryFilter, ipPoiInfoCursor, poi_info::kRoadNameID, &roadNameIDIndex);
	getColumnIndex(PoiInfoIpQueryFilter, ipPoiInfoCursor, poi_info::kHouseNumber, &houseNumberIndex);
	getColumnIndex(PoiInfoIpQueryFilter, ipPoiInfoCursor, poi_info::kActualAddress, &actualAddressIndex);
	//vector to store all POI_INFO details (OBJECTID, HN, ACTUALADDRESS, ROADNAMEID, Coordinates)
	std::vector<poiInfo> poiInfoList;
	//get all details about POI_INFO
	IFeaturePtr ipPoiInfoFeature;
	while (ipPoiInfoCursor->NextFeature(&ipPoiInfoFeature) == S_OK && ipPoiInfoFeature) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipPoiInfoFeature->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName, _T(""), _T("Failed to get OBJECTID of one ") + poiInfoFeatureClassName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//if this record is child POI, skip
		if (childPOIList->find(OIDNum) != childPOIList->end()) {
			continue;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		// get actualaddress data
		CComVariant actualAddress;
		if (S_OK != ipPoiInfoFeature->get_Value(actualAddressIndex, &actualAddress)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get ACTUALADDRESS value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		CString convertedActualAddress = actualAddress.bstrVal;
		convertedActualAddress = convertedActualAddress.Trim();
		// get house number data
		CComVariant houseNumber;
		if (S_OK != ipPoiInfoFeature->get_Value(houseNumberIndex, &houseNumber)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get HOUSENUMBER value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		CString convertedHouseNumber = houseNumber.bstrVal;
		convertedHouseNumber = convertedHouseNumber.Trim();
		// get road name ID data
		CComVariant roadNameID;
		if (S_OK != ipPoiInfoFeature->get_Value(roadNameIDIndex, &roadNameID)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get ROADNAMEID value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		long convertedRoadNameID = roadNameID.lVal;
		// get shape
		IGeometryPtr ipPOIGeom;
		if (S_OK != ipPoiInfoFeature->get_ShapeCopy(&ipPOIGeom)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get shape"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		// get PP coordinates
		double orgX = 0.0, orgY = 0.0;
		if (S_OK != IPointPtr(ipPOIGeom)->QueryCoords(&orgX, &orgY) || orgX == 0 || orgY == 0) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiInfoFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get shape's coordinates"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}

		//detect complex condition
		int indexToDelete = -1;
		bool isIgnoreThisRecord = false;
		if (convertedActualAddress != "") {
			for (int i = poiInfoList.size() - 1; i >= 0; i--) {
				if (convertedActualAddress != poiInfoList[i].actualAddress) {
					break;
				}
				else if (convertedHouseNumber == poiInfoList[i].houseNumber && convertedActualAddress == poiInfoList[i].actualAddress) {
					//fix, change to official
					if (poiEntrypointList->find(OIDNum) != poiEntrypointList->end() && poiEntrypointList->find(poiInfoList[i].OBJECTID) == poiEntrypointList->end()) {
						indexToDelete = i;
						break;
					}
					else {
						isIgnoreThisRecord = true;
						break;
					}
				}
			}
		}
		//compare using road name ID
		if (!isIgnoreThisRecord && indexToDelete == -1 && convertedRoadNameID != 0) {
			for (int i = poiInfoList.size() - 1; i >= 0; i--) {
				if (houseNumber != poiInfoList[i].houseNumber) {
					break;
				}
				else if (poiInfoList[i].roadNameID == 0) {
					continue;
				}
				//fix, change to official
				else if (poiEntrypointList->find(OIDNum) != poiEntrypointList->end() && poiEntrypointList->find(poiInfoList[i].OBJECTID) == poiEntrypointList->end()) {
					isIgnoreThisRecord = true;
					break;
				}
			}
		}
		if (!isIgnoreThisRecord && indexToDelete == -1) {
			for (int i = poiInfoList.size() - 1; i >= 0; i--) {
				if (houseNumber != poiInfoList[i].houseNumber) {
					break;
				}
				//compare distance using our x y
				else if (getDist(orgX,orgY,poiInfoList[i].x, poiInfoList[i].y)) {
					if (poiInfoList[i].roadNameID == 0 && convertedRoadNameID != 0) {
						indexToDelete = i;
						break;
					}
					else {
						isIgnoreThisRecord = true;
						break;
					}
				}
			}
		}
		//skip this record
		if (isIgnoreThisRecord) {
			continue;
		}
		//delete old and add new one into target list
		if (indexToDelete != -1) {
			poiInfoList.erase(poiInfoList.begin() + indexToDelete);
			poiInfoList.push_back(poiInfo(OIDNum, convertedHouseNumber, convertedActualAddress, convertedRoadNameID, orgX, orgY));
			continue;
		}
		else {
			poiInfoList.push_back(poiInfo(OIDNum,convertedHouseNumber,convertedActualAddress,convertedRoadNameID,orgX,orgY));
		}
	}
	//store target ID into uniquePoiInfoID
	uniquePoiInfoList->assign(poiInfoList.begin(),poiInfoList.end());
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::insertNewRecordToHNP(std::vector<poiInfo> * uniquePoiInfoList) {
	using namespace sindy::schema::global;
	//GET TARGET TOTAL RECORD NUMBER
	long progressCount = uniquePoiInfoList->size();
	IOManager::getInstance().print_run(true, (CString)"Total target record for HNP : " + std::to_string(progressCount).c_str());
	long successUpdateCount = 0;	//count total successfully update records
	
	//get information about HNP featureClass from Datamanager
	DataManager::featureClassDesc hnp_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_hnp);
	CString hnpFeatureClassName = hnp_featureClass.featureClassName;
	IFeatureClassPtr hnpFeatureClass = hnp_featureClass.featureClass;
	
	//prepare column indexes for HNP
	long	operatorIndex, purposeCIndex, modifyDateIndex, updateTypeCIndex, progModifyDateIndex,
			modifyProgNameIndex, userClaimFIndex, sourceIndex, HNIndex, HNTypeIndex, linkIDIndex, roadNameIDIndex; 
	
	//fix here, change layername
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &operatorIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &purposeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyDateIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &updateTypeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &progModifyDateIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyProgNameIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &userClaimFIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &sourceIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &HNIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &HNTypeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &linkIDIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &roadNameIDIndex);
		
	//setup cursor and buffer for inputting new OFFICIAL records
	IFeatureBufferPtr featureBuffer;
	hnpFeatureClass->CreateFeatureBuffer(&featureBuffer);
	IFeatureCursorPtr insertHnpCursor;
	hnpFeatureClass->Insert(VARIANT_TRUE, &insertHnpCursor);
	
	//update HNP with POI_INFO where ROADNAMEID = NULL
	for (int i=0;i<uniquePoiInfoList->size();i++) {
		//skip not null ROADNAMEID
		if ( uniquePoiInfoList->operator[](uniqIndex).ROADNAMEID != 0){
			continue;
		}
		//create new HNP record
		//fix here for value type
		featureBuffer->set_Value(operatorIndex, "SINDY");
		featureBuffer->set_Value(purposeCIndex, 0);
		featureBuffer->set_Value(updateTypeCIndex, 6);
		//fix as today
		featureBuffer->set_Value(progModifyDateIndex, );
		featureBuffer->set_Value(modifyProgNameIndex, "PoiInfoToHnpUpdater");
		featureBuffer->set_Value(userClaimFIndex, 0);
		featureBuffer->set_Value(sourceIndex, "POI_INFO OBJECTID" + uniquePoiInfoList->operator[](i).OBJECTID);
		featureBuffer->set_Value(HNIndex, uniquePoiInfoList->operator[](i).houseNumber);
		featureBuffer->set_Value(HNTypeIndex, 1);
		featureBuffer->set_Value(linkIDIndex, 0);
		
		//copy from one POI_INFO
		IGeometryPtr ipHNPGeom;
		IPointPtr newPoint;
		newPoint->
		((IPointPtr)ipHNPGeom)->PutCoords(uniquePoiInfoList->operator[](i).x, uniquePoiInfoList->operator[](i).y);
		ipHNPGeom->
		
		long newHnpOID;
		//insert new row into OFFICIAL_NAME
		//fix here, check newOID type
		insertHnpCursor->InsertFeature(featureBuffer, &newHnpOID);
		successUpdateCount++;
	}
	//flush every 10000
	if (successUpdateCount % 10000 == 0){
		insertOfficialCursor->Flush();
	}
	
	//fix, also print total update count
	IOManager::getInstance().print_run(true, _T("Successfully updated OFFICIAL_NAME"));
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::insertNewRecordToHNPAndOfficial(std::map<int,int> * translationList, std::vector<poiInfo> * uniquePoiInfoList) {
	using namespace sindy::schema::global;
	//GET TARGET TOTAL RECORD NUMBER
	long progressCount = uniquePoiInfoList->size();
	IOManager::getInstance().print_run(true, (CString)"Total target record for HNP : " + std::to_string(progressCount).c_str());
	long successUpdateCount = 0;	//count total successfully update records
	
	//get information about HNP featureClass from Datamanager
	DataManager::featureClassDesc hnp_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_hnp);
	CString hnpFeatureClassName = hnp_featureClass.featureClassName;
	IFeatureClassPtr hnpFeatureClass = hnp_featureClass.featureClass;
	IFeatureCursorPtr ipHNPCursor;

	//get information about OFFICIAL_NAME table from Datamanager
	DataManager::tableDesc official_name_table = m_dataManager->getTable(m_OptionManager->k_official);
	ITablePtr officialTable = official_name_table.table;
	ITablePtr officialTable = official_name_table.table;
	IQueryFilterPtr officialIpQueryFilter(CLSID_QueryFilter);
	//set all condition into query variable
	_ICursorPtr ipOfficialCursor;
	//search only records with layer=34 (POI_INFO)
	CComBSTR queryFilter = _T("LAYER_C = 34");
	if (S_OK != officialIpQueryFilter->put_WhereClause(queryFilter)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, officialTableName, _T(""), _T("Failed to set search query"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	if (S_OK != officialTable->Search(officialIpQueryFilter, VARIANT_FALSE, &ipOfficialCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	
	//create new map for <POI_INFO.ROADNAMEID, position in uniquePoiInfoList>
	//fix here, get the one with ROADNAMEID only
	std::map<int, int> roadNameMap;
	createRoadNameMap(&roadNameMap);
	
	//prepare column indexes for OFFICIAL_NAME
	//fix here, change how to get
	IFields officialFields = officialTable->Fields;
	long layerCIndex = 0;
	officialTable->FindField((CComBSTR)official_name::kLayerCode, &layerCIndex);
	long objectIDIndex = 0;
	//fix here, change layername
	officialTable->FindField((CComBSTR)official_name::kLayerCode, &objectIDIndex);
	
	//prepare column indexes for HNP
	long	operatorIndex, purposeCIndex, modifyDateIndex, updateTypeCIndex, progModifyDateIndex,
			modifyProgNameIndex, userClaimFIndex, sourceIndex, HNIndex, HNTypeIndex, linkIDIndex, roadNameIDIndex; 
	
	//fix here, change layername
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &operatorIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &purposeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyDateIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &updateTypeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &progModifyDateIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyProgNameIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &userClaimFIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &sourceIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &HNIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &HNTypeCIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &linkIDIndex);
	hnpFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &roadNameIDIndex);
	
	//setup cursor and buffer for inputting new OFFICIAL_NAME records
	IRowBufferPtr tableBuffer;
	officialTable->CreateRowBuffer(&tableBuffer);
	_ICursorPtr insertOfficialCursor;
	officialTable->Insert(VARIANT_TRUE, &insertOfficialCursor);
	
	//setup cursor and buffer for inputting new OFFICIAL records
	IFeatureBufferPtr featureBuffer;
	hnpFeatureClass->CreateFeatureBuffer(&featureBuffer);
	IFeatureCursorPtr insertHnpCursor;
	hnpFeatureClass->Insert(VARIANT_TRUE, &insertHnpCursor);
	
	//create new records
	_IRowPtr ipOfficialRow;
	while (ipOfficialCursor->NextRow(&ipOfficialRow) == S_OK && ipOfficialRow) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipOfficialRow->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName, _T(""), _T("Failed to get OBJECTID of one ") + officialTableName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		//if found this OFFICIAL_NAME OBJECTID record in target POI_INFO
		if (createRoadNameMap->find(OIDNum) != std::string::npos) {
			//create new OFFICIAL_NAME record
			//fix here, change fields
			for (int i=0;i<officialFields.FieldCount;i++){
				//discard index and layer
				if (i == objectIDIndex){
					continue;
				}				
				//get value on old record
				CComVariant tempValue;
				if (S_OK != ipOfficialRow->get_Value(i, &tempValue)) {
					//fix here (THIS value)
					IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, officialTableName + _T(" OBJECTID"), OID, _T("Failed to get THIS value"));
					return IOManager::RCode::R_FAILED_FATAL_ERROR;
				}
				//set value on new record
				if (i == layerCIndex){
					tableBuffer->set_Value(i, 34);
				}
				else {
					tableBuffer->set_Value(i, tempValue);
				}
			}
			long newOfficialOID;
			//insert new row into OFFICIAL_NAME
			//fix here, check newOID type
			insertOfficialCursor->InsertRow(tableBuffer, &newOfficialOID);
			
			//get index value from uniquePoiInfoList
			int uniqIndex = createRoadNameMap[OIDNum];
			//create new HNP record
			//fix here for value type
			featureBuffer->set_Value(operatorIndex, "SINDY");
			featureBuffer->set_Value(purposeCIndex, 0);
			featureBuffer->set_Value(updateTypeCIndex, 6);
			//fix as today
			featureBuffer->set_Value(progModifyDateIndex, );
			featureBuffer->set_Value(modifyProgNameIndex, "PoiInfoToHnpUpdater");
			featureBuffer->set_Value(userClaimFIndex, 0);
			featureBuffer->set_Value(sourceIndex, "POI_INFO OBJECTID" + uniquePoiInfoList->operator[](uniqIndex).OBJECTID);
			featureBuffer->set_Value(HNIndex, uniquePoiInfoList->operator[](uniqIndex).houseNumber);
			featureBuffer->set_Value(HNTypeIndex, 1);
			featureBuffer->set_Value(linkIDIndex, 0);
			featureBuffer->set_Value(linkIDIndex, 0);
			//fix value type
			featureBuffer->set_Value(roadNameIDIndex, newOfficialOID);
			
			//copy from one POI_INFO
			IGeometryPtr ipHNPGeom;
			IPointPtr newPoint;
			newPoint->
			((IPointPtr)ipHNPGeom)->PutCoords(uniquePoiInfoList->operator[](i).x, uniquePoiInfoList->operator[](i).y);
			ipHNPGeom->
			
			long newHnpOID;
			//insert new row into OFFICIAL_NAME
			//fix here, check newOID type
			insertHnpCursor->InsertFeature(featureBuffer, &newHnpOID);
			
			//insert into translationList map to create new records on TRANSLATION later
			if (!translationList->insert(std::make_pair(OIDNum, newOfficialOID)).second) {
				IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_PROCESS, true, officialTableName + _T(" OBJECTID"), OID, _T("Unexpected error occured while storing OFFICIAL_NAME data"));
				return IOManager::RCode::R_FAILED_FATAL_ERROR;
			}
			
			successUpdateCount++;
		}
		//flush every 10000
		if (successUpdateCount % 10000 == 0){
			insertOfficialCursor->Flush();
			insertHnpCursor->Flush();
		}
	}
	//to fix, record new OFFICIAL_NAME OBJECTID to create new one on TRANSLATON
	
	//fix, also print total update count
	IOManager::getInstance().print_run(true, _T("Successfully updated to HNP and OFFICIAL_NAME"));
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::insertNewRecordToTranslation(std::map<int,int> * translationList) {
	using namespace sindy::schema::global;
	//GET TARGET TOTAL RECORD NUMBER
	long progressCount = uniquePoiInfoList->size();
	IOManager::getInstance().print_run(true, (CString)"Total target record for HNP : " + std::to_string(progressCount).c_str());
	long successUpdateCount = 0;	//count total successfully update records
	
	//get information about this table from Datamanager;
	DataManager::tableDesc translation_table = m_dataManager->getTable(m_OptionManager->k_translation);
	CString translationTableName = translation_table.tableName;
	ITablePtr translationTable = translation_table.table;
	
	//set all condition into query variable
	_ICursorPtr ipTranslationCursor;
	//search only records with layer=34 (POI_INFO)
	if (S_OK != officialTable->Search(officialIpQueryFilter, VARIANT_FALSE, &ipOfficialCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, translationTableName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	
	//prepare column indexes for OFFICIAL_NAME
	//fix here, change how to get
	IFields translationFields = translationTable->Fields;
	//fix here, change to transID
	long transIDIndex = 0;
	officialTable->FindField((CComBSTR)official_name::kLayerCode, &transIDIndex);
	long objectIDIndex = 0;
	//fix here, change layername
	officialTable->FindField((CComBSTR)official_name::kLayerCode, &objectIDIndex);
	
	//setup cursor and buffer for inputting new OFFICIAL_NAME records
	IRowBufferPtr tableBuffer;
	translationTable->CreateRowBuffer(&tableBuffer);
	_ICursorPtr insertTranslationCursor;
	translationTable->Insert(VARIANT_TRUE, &insertTranslationCursor);
	
	//create new records
	_IRowPtr ipTranslationRow;
	while (ipTranslationCursor->NextRow(&ipTranslationRow) == S_OK && ipTranslationRow) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipOfficialRow->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, translationTableName, _T(""), _T("Failed to get OBJECTID of one ") + translationTableName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		//if found this TRANSLATION OBJECTID record in target translationList
		if (translationList->find(OIDNum) != std::string::npos) {
			//create new OFFICIAL_NAME record
			//fix here, change fields
			for (int i=0;i<translationFields.FieldCount;i++){
				//discard index and transID
				if (i == objectIDIndex){
					continue;
				}				
				//get value on old record
				CComVariant tempValue;
				if (S_OK != ipTranslationRow->get_Value(i, &tempValue)) {
					//fix here (THIS value)
					IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, translationTableName + _T(" OBJECTID"), OID, _T("Failed to get THIS value"));
					return IOManager::RCode::R_FAILED_FATAL_ERROR;
				}
				//set value on new record
				if (i == transIDIndex){
					tableBuffer->set_Value(i, translationList[OIDNum]);
				}
				else {
					tableBuffer->set_Value(i, tempValue);
				}
			}
			long newTranslationOID;
			//insert new row into OFFICIAL_NAME
			//fix here, check newOID type
			insertTranslationCursor->InsertRow(tableBuffer, &newTranslationOID);
			successUpdateCount++;
		}
		//flush every 10000
		if (successUpdateCount % 10000 == 0){
			insertTranslationCursor->Flush();
		}
	}
	//fix, also print total update count
	IOManager::getInstance().print_run(true, _T("Successfully updated to HNP and OFFICIAL_NAME"));
	return IOManager::RCode::R_SUCCESS;
}

int PoiInfoToHNPUpdater::insertNewRecordToHNPEntrypoint(std::map<int,int> * updatedPoiHnpList) {
	//get information about this table from Datamanager;
	DataManager::featureClassDesc poi_entrypoint_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_poi_entrypoint);
	CString poiEntryFeatureClassName = poi_entrypoint_featureClass.featureClassName;
	//create condition to get only childID column
	IQueryFilterPtr poiEntryIpQueryFilter(CLSID_QueryFilter);
	//fix, to get OBJECTID, POIINFOID, ACCURACY_C, SHAPE
	CString poiInfoIDName = sindy::schema::global::poi_entry_point::kPoiInfoID;
	if (S_OK != poiEntryIpQueryFilter->put_SubFields((CComBSTR)poiInfoIDName)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to set search column for POIINFOID column"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//search only records with user input condition
	CComBSTR queryFilter = m_OptionManager->m_sql_poientry.c_str();
	if (S_OK != poiEntryIpQueryFilter->put_WhereClause(queryFilter)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_SET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to set search query"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//set all condition into query variable
	IFeatureCursorPtr poiEntrypointCursor;
	IFeatureClassPtr poiEntryFeatureClass = poi_entrypoint_featureClass.featureClass;

	if (S_OK != poiEntryFeatureClass->Search(poiEntryIpQueryFilter, VARIANT_FALSE, &poiEntrypointCursor)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to get all records"));
		return IOManager::RCode::R_FAILED_FATAL_ERROR;
	}
	//fix here, get column index for ???
	long poiInfoIDIndex = 0, accuracyCIndex = 0;
	getColumnIndex(poiEntryIpQueryFilter, poiEntrypointCursor, poi_info::kRoadNameID, &poiInfoIDIndex);
	getColumnIndex(poiEntryIpQueryFilter, poiEntrypointCursor, poi_info::kHouseNumber, &accuracyCIndex);
	
	
	
	
	
	
	
	
	using namespace sindy::schema::global;
	//GET TARGET TOTAL RECORD NUMBER
	long progressCount = updatedPoiHnpList->size();
	IOManager::getInstance().print_run(true, (CString)"Total target record for HNP_ENTRYPOINT : " + std::to_string(progressCount).c_str());
	long successUpdateCount = 0;	//count total successfully update records
	
	//get information about HNP featureClass from Datamanager
	DataManager::featureClassDesc hnp_entry_featureClass = m_dataManager->getFeatureClass(m_OptionManager->k_hnp_entrypoint);
	CString hnpEntryFeatureClassName = hnp_entry_featureClass.featureClassName;
	IFeatureClassPtr hnpEntryFeatureClass = hnp_entry_featureClass.featureClass;
	
	
	//prepare column indexes for HNP_ENTRYPOINT
	long	operatorIndex, purposeCIndex, modifyDateIndex, updateTypeCIndex, progModifyDateIndex,
			modifyProgNameIndex, userClaimFIndex, sourceIndex, accuracyCIndex, hnpPointIDIndex; 
	
	//fix here, change layername
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &operatorIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &purposeCIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyDateIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &updateTypeCIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &progModifyDateIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &modifyProgNameIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &userClaimFIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &sourceIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &accuracyCIndex);
	hnpEntryFeatureClass->FindField((CComBSTR)official_name::kLayerCode, &hnpPointIDIndex);
	
	//setup cursor and buffer for inputting new OFFICIAL records
	IFeatureBufferPtr featureBuffer;
	hnpEntryFeatureClass->CreateFeatureBuffer(&featureBuffer);
	IFeatureCursorPtr insertHnpEntryCursor;
	hnpEntryFeatureClass->Insert(VARIANT_TRUE, &insertHnpEntryCursor);
	
	
	
		
	
	
	//set to store childID list
	//get target postal code
	IFeaturePtr ipPoiEntryFeature;
	while (poiEntrypointCursor->NextFeature(&ipPoiEntryFeature) == S_OK && ipPoiEntryFeature) {
		// get postal point OID
		long OIDNum;
		if (S_OK != ipPoiEntryFeature->get_OID(&OIDNum)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName, _T(""), _T("Failed to get OBJECTID of one ") + poiEntryFeatureClassName);
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		//convert OID from long to CString
		CString OID;
		OID.Format(L"%ld", OIDNum);
		// get child id data
		CComVariant poiInfoID;
		if (S_OK != ipPoiEntryFeature->get_Value(poiInfoIDIndex, &poiInfoID)) {
			IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get POIINFOID value"));
			return IOManager::RCode::R_FAILED_FATAL_ERROR;
		}
		long convertedPoiInfoID = poiInfoID.lVal;
		//find if this POI_ENTRYPOINT's parent is target
		if (updatedPoiHnpList->find(convertedPoiInfoID) != std::string::npos){
			// get accuracy_c data
			CComVariant accuracyC;
			if (S_OK != ipPoiEntryFeature->get_Value(accuracyCIndex, &accuracyC)) {
				IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get ACCURACY_C value"));
				return IOManager::RCode::R_FAILED_FATAL_ERROR;
			}
			CString convertedAccuracyC = accuracyC.bstrVal;
			// get shape
			IGeometryPtr ipPOIGeom;
			if (S_OK != ipPoiEntryFeature->get_ShapeCopy(&ipPOIGeom)) {
				IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassNamee + _T(" OBJECTID"), OID, _T("Failed to get shape"));
				return IOManager::RCode::R_FAILED_FATAL_ERROR;
			}
			// get coordinates
			double orgX = 0.0, orgY = 0.0;
			if (S_OK != IPointPtr(ipPOIGeom)->QueryCoords(&orgX, &orgY) || orgX == 0 || orgY == 0) {
				IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, poiEntryFeatureClassName + _T(" OBJECTID"), OID, _T("Failed to get shape's coordinates"));
				return IOManager::RCode::R_FAILED_FATAL_ERROR;
			}
			
			
			
			
			
			//create new HNP record
			//fix here for value type
			featureBuffer->set_Value(operatorIndex, "SINDY");
			featureBuffer->set_Value(purposeCIndex, 0);
			featureBuffer->set_Value(updateTypeCIndex, 6);
			//fix as today
			featureBuffer->set_Value(progModifyDateIndex, );
			featureBuffer->set_Value(modifyProgNameIndex, "PoiInfoToHnpUpdater");
			featureBuffer->set_Value(userClaimFIndex, 0);
			featureBuffer->set_Value(sourceIndex, "POI_ENTRYPOINT OBJECTID" + OID);
			featureBuffer->set_Value(accuracyCIndex, convertedAccuracyC);
			featureBuffer->set_Value(hnpPointIDIndex,updatedPoiHnpList[convertedPoiInfoID]);
			
			featureBuffer->put_Shape(ipPOIGeom);
			
			long newHnpEntryOID;
			//insert new row into HNP_ENTRYPOINT
			//fix here, check newOID type
			insertHnpEntryCursor->InsertFeature(featureBuffer, &newHnpEntryOID);
			successUpdateCount++;			
		}
		//flush every 10000
		if (successUpdateCount % 10000 == 0){
			insertOfficialCursor->Flush();
		}
	}
	//fix, also print total update count
	IOManager::getInstance().print_run(true, _T("Successfully updated HNP_ENTRYPOINT"));
	return IOManager::getInstance().printSuccessfulEnd()
}

int PoiInfoToHNPUpdater::getColumnIndex(IQueryFilterPtr POIipQueryFilter, IFeatureCursorPtr ipPOICursor, CComBSTR fieldName, long * index ) {
	if (S_OK != ipPOICursor->Fields->FindField(fieldName, index)) {
		IOManager::getInstance().print_error(IOManager::ECode::E_FAILED_TO_GET_DATA, true, (LPCTSTR)fieldName, _T("FIELD INDEX"), _T("Failed to get field index"));
		return IOManager::getInstance().endProgramWithError(_T("get Actual Address field index"));
	}
	CString POIFeatureClassName = m_OptionManager->m_poi_info.c_str();
	IOManager::getInstance().print_run(true, POIFeatureClassName, (LPCTSTR)fieldName, "Field has been acquired successfully");
	return IOManager::RCode::R_SUCCESS;
}

