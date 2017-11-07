using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
/// <summary>
/// Manage all input and output files (except log file)
/// 1. config file
/// 2. TSV file
/// Parse input data from config file to interfaceData for server-side management
/// Parse output data from interfaceData to output TSV file and config file
/// </summary>
public class IOManager
{
    //necessary variable to automatically close messagebox
    [DllImport("user32.dll", EntryPoint = "FindWindow", SetLastError = true)]
    static extern IntPtr FindWindowByCaption(IntPtr ZeroOnly, string lpWindowName);
    [DllImport("user32.Dll")]
    static extern int PostMessage(IntPtr hWnd, UInt32 msg, int wParam, int lParam);
    const UInt32 WM_CLOSE = 0x0010;

    //related class
    private interfaceData m_interfaceData;

    //variables
    private String user1Variable;
    private String DB1Variable;
    private String version1Variable;
    private String user2Variable;
    private String DB2Variable;
    private String version2Variable;
    private String delimiter = "\t";

    //initializer
    public IOManager(interfaceData interfaceData)
    {
        m_interfaceData = interfaceData;
    }
	//get/set DBuser-related data
    public String getUser1Variable()
    {
        return user1Variable;
    }
    public String getDB1Variable()
    {
        return DB1Variable;
    }
    public String getVersion1Variable()
    {
        return version1Variable;
    }
    public String getUser2Variable()
    {
        return user2Variable;
    }
    public String getDB2Variable()
    {
        return DB2Variable;
    }
    public String getVersion2Variable()
    {
        return version2Variable;
    }
	/**
	* @brief Read a config file, get all config data into interfaceData before connecting to target DBuser 
	*/
    public bool readConfigFile()
    {
        // Create OpenFileDialog 
        Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
        // Set filter for file extension and default file extension 
        dlg.Filter = "Text Files (*.txt)|*.txt|INI Files (*.ini)|*.ini";
        // Display OpenFileDialog by calling ShowDialog method 
        Nullable<bool> result = dlg.ShowDialog();
        // Get the selected file name and proceed the settings 
        if (result == true)
        {
            // Open document 
            String filename = dlg.FileName;
            String datasetName = "";
			bool isGettingTableOrLayer = true;
			bool isGettingWhereOrField = false;
			bool isGettingField = false;
            List<String> lines = new List<String>();
            //clear comment
			foreach (String line in File.ReadLines(filename))
            { 
				String editedLine = excludeCommentandTrim(line);
				//get DBuser info line, if not then add to processing lines
                if (!getDBUserInfo(editedLine)){
					lines.Add(editedLine);
				}
			}
			for( int i = 0 ;i < lines.Count; i++)
            { 
				//get dataset name and create record into m_interfaceData 
				if ((lines[i] == "%TABLE" || lines[i] == "%LAYER") && isGettingTableOrLayer){
					if (i+1>=lines.Count){
						System.Windows.MessageBox.Show("Found incorrect config file format while reading layer or table name", "File Error");
                        return false;
					}
					datasetName = lines[i + 1];
					if (lines[i] == "%TABLE") 
						m_interfaceData.setNewDataset(DB1Variable, user1Variable, version1Variable, datasetName, true, true);	//create new dataset
					else
						m_interfaceData.setNewDataset(DB1Variable, user1Variable, version1Variable, datasetName, false, true);	//create new dataset
					m_interfaceData.setDatasetSelected(DB1Variable, user1Variable, version1Variable, datasetName, true);		//set new dataset as selected
					isGettingTableOrLayer = false;
					isGettingWhereOrField = true;
					i++;
				}
				//get where clause (if available) and record into m_interfaceDAta
				else if (lines[i] == "%WHERE" && isGettingWhereOrField){
					if (i+1>=lines.Count){
						System.Windows.MessageBox.Show("Found incorrect config file format while reading where clause", "File Error");
                        return false;
					}
					String conditionTxt = "";
					int j;
					for (j=i+1;j<lines.Count;j++){
						if (lines[j] == "%FIELD")
							break;
						if (lines[j] == "%WHERE" || lines[j] == "%TABLE" || lines[j] == "%LAYER" || j+1 >= lines.Count){
							System.Windows.MessageBox.Show("Found incorrect config file format while reading where clause", "File Error");
							return false;
						}
						conditionTxt += lines[j] + " ";
					}
					i = j;
					m_interfaceData.setDatasetCondition(DB1Variable, user1Variable, version1Variable, datasetName, conditionTxt);
					isGettingWhereOrField = false;
					isGettingField = true;
				}
				//get fields and record into m_interfaceDAta
				else if (lines[i] == "%FIELD" && (isGettingWhereOrField || isGettingField)){
					if (i+1>=lines.Count){
						System.Windows.MessageBox.Show("Found incorrect config file format while reading field", "File Error");
                        return false;
					}
					if (lines[i+1] == "-"){
						i++;
					}
					else if (lines[i+1] == "*"){
						m_interfaceData.setAllFieldSelected(DB1Variable, user1Variable, version1Variable, datasetName, true);
						i++;
					}
					else {
						int j = i;
						for (j=i+1;j<lines.Count;j++){
							if (lines[j] = "*" || lines[j] == "-" || lines[j] == "%WHERE" || lines[j] == "%FIELD" ){
								System.Windows.MessageBox.Show("Found incorrect config file format while reading field", "File Error");
								return false;
							}
							if (lines[j] == "%TABLE" || lines[j] == "%LAYER" ){
								j--;
								break;
							}
							if (j+1>=lines.Count){
								break;
							}
							if (!String.IsNullOrWhiteSpace(lines[j])){
								String fieldName = lines[j];
								m_interfaceData.setDatasetFieldSelected(DB1Variable, user1Variable, version1Variable, datasetName, fieldName, true);
							}
						}
						i = j;
					}
					isGettingWhereOrField = false;
					isGettingField = false;
					isGettingTableOrLayer = true;
				}
				//if line is not empty and has incorrect format
				else if (!String.IsNullOrWhiteSpace(lines[i])) {
					System.Windows.MessageBox.Show("Config file format is incorrect", "File Error");
					return false;
				}
			}
			//should set to reading table or layer when finish last line
			if (!isGettingTableOrLayer)
            {
                System.Windows.MessageBox.Show("Config file format is incorrect", "File Error");
                return false;
            }
            return true;
        }
		//cancel read
        else
        {
            return false;
        }
    }
    /**
    * @brief Get a string line, delete comment (#...) and trim whitespace
    * @param line		    [in]	input string line
    * @return   processed string
    */
    public String excludeCommentandTrim(String line)
    {
        if (line.IndexOf("#") != -1)
            return line.Substring(0, line.IndexOf("#")).Trim();
        else
            return line.Trim();
    }
    /**
    * @brief Get a string line, detect if a line is related to DBuser information and copy to a class variable
    * @param userInfo		    [in]	input string line
    */
    public bool getDBUserInfo(String userInfo)
    {
        if (userInfo.Contains("user_1"))
        {
            user1Variable = getLineVariable(userInfo);
			return true;
        }
        else if (userInfo.Contains("user_2"))
        {
            user2Variable = getLineVariable(userInfo);
			return true;
        }
        else if (userInfo.Contains("server_1"))
        {
            DB1Variable = getLineVariable(userInfo);
			return true;
        }
        else if (userInfo.Contains("server_2"))
        {
            DB2Variable = getLineVariable(userInfo);
			return true;
        }
        else if (userInfo.Contains("version_1"))
        {
            version1Variable = getLineVariable(userInfo);
            if (String.IsNullOrWhiteSpace(version1Variable))
            {
                version1Variable = "SDE.DEFAULT";
            }
			return true;
        }
        else if (userInfo.Contains("version_2"))
        {
            version2Variable = getLineVariable(userInfo);
            if (String.IsNullOrWhiteSpace(version2Variable))
            {
                version2Variable = "SDE.DEFAULT";
            }
			return true;
        }
		return false;
    }
    /**
    * @brief Get variable value from a line in config file [ex. user_1 = TEST2017, get TEST2017]
    * @param userInfo	    [in]	input string line
    * @return   variable value [ex. TEST2017]
    */
    public String getLineVariable(String userInfo)
    {
        int start = userInfo.IndexOf('=') + 1;
        int end = userInfo.Length;
        String variableValue = userInfo.Substring(start, end - start).Trim();
        return variableValue;
    }
    /**
    * @brief Get field selection into interfaceData for later process.
    * @param fieldName		    [in]	column name
    * @param datasetName		[in]	dataset name
    */
    public bool setField(String fieldName, String datasetName)
    {
        //if none selected
        if (fieldName == "-")
        {
            return true;
        }
        if (String.IsNullOrWhiteSpace(fieldName))
        {
            return false;
        }
        //if all selected
        else if (fieldName == "*")
        {
            m_interfaceData.setAllFieldSelected(DB1Variable, user1Variable, version1Variable, datasetName, true);

            return true;
        }
        //insert 1 field
        m_interfaceData.setDatasetFieldSelected(DB1Variable, user1Variable, version1Variable, datasetName, fieldName, true);
        return true;
    }
	/**
	* @brief Write a config file, get all data from interfaceData, convert into config file format and output the data. 
	*/
    public bool writeConfigFile()
    {
        // Create OpenFileDialog 
        Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();

        // Set filter for file extension and default file extension 
        dlg.Filter = "Text Files (*.txt)|*.txt|INI Files (*.ini)|*.ini";

        // Display OpenFileDialog by calling ShowDialog method 
        Nullable<bool> result = dlg.ShowDialog();

        // Get the selected file name and display in a TextBox 
        if (result == true)
        {
            String filename = dlg.FileName;
            if (!filename.EndsWith(".txt") && !filename.EndsWith(".ini"))
            {
                System.Windows.MessageBox.Show("Target file is not .txt or .ini file type.", "File Error");
                return false;
            }
            // Open document 
            
            List<String> configText = new List<String>();
            configText.Add("###CONFIG LIST###");
            configText.Add("");
            configText.Add("##User 1 information");
            configText.Add("user_1=" + m_interfaceData.getCurrentUser());
            configText.Add("server_1=" + m_interfaceData.getCurrentDB());
            configText.Add("version_1=" + m_interfaceData.getCurrentVersion() + "\t#BLANK = 'SDE.DEFAULT'");
            configText.Add("##User 2 information");
            configText.Add("user_2=" + m_interfaceData.getCurrentUser2());
            configText.Add("server_2=" + m_interfaceData.getCurrentDB2());
            configText.Add("version_2=" + m_interfaceData.getCurrentVersion2() + "\t#BLANK = 'SDE.DEFAULT'");
            configText.Add("");
            configText.Add("##Layer, table and field list");
            configText.Add("#Below is format for input data");
            configText.Add("#%TABLE");
            configText.Add("#<TABLE_NAME>");
            configText.Add("#%WHERE");
            configText.Add("#<CONDITION>");
            configText.Add("#%FIELD");
            configText.Add("#<FIELD_NAME1>");
            configText.Add("#<FIELD_NAME2>");
            configText.Add("#:");
            configText.Add("#%LAYER");
            configText.Add("#<LAYER_NAME>");
            configText.Add("#%FIELD");
            configText.Add("#<FIELD_NAME1>");
            configText.Add("#<FIELD_NAME2>");
            configText.Add("#:");
            configText.Add("");
            configText.Add("#If you want a table or layer to output all fields: use '*' under %FIELD line");
            configText.Add("#If you want a table or layer to output no field: use '-' under %FIELD line");
            //get table with selected condition
            Dictionary<String, interfaceData.datasetInfo> datasetList = m_interfaceData.getDatasetList();
            foreach (KeyValuePair<String, interfaceData.datasetInfo> dataset in datasetList)
            {
                interfaceData.datasetInfo datasetInf = dataset.Value;
                //if not select, skip
                if (!datasetInf.isDatasetSelected)
                    continue;
                String datasetName = dataset.Key;
                int a = datasetName.LastIndexOf(m_interfaceData.getDatasetListKeywordSeparator());
                int b = datasetName.Length;
                datasetName = datasetName.Substring(a + 1, b - a - 1);
                //add %TABLE or %LAYER and dataset line
                if (datasetInf.isTable) configText.Add("%TABLE");
                else configText.Add("%LAYER");
                configText.Add(datasetName);
                //if condition is not empty, add %WHERE and condition line
                String datasetCondition = datasetInf.searchCondition;
                if (!String.IsNullOrWhiteSpace(datasetCondition))
                {
                    configText.Add("%WHERE");
                    configText.Add(datasetCondition);
                }
                //get selected column
                configText.Add("%FIELD");
                List<Tuple<String, bool>> fieldList = datasetInf.fieldInfo;
                //if select all
                bool isSelectedAllManually = true;
                bool isAllFalse = true;
                if (fieldList.Count <= 0)
                {
                    configText.Add("-");
                    continue;
                }
                foreach (Tuple<String, bool> field in fieldList)
                {
                    if (field.Item2 == false) //if not selected
                    {
                        isSelectedAllManually = false;
                    } 
                    else if (field.Item2 == true)
                    {
                        isAllFalse = false;
                    }
                }
                if (datasetInf.isFieldSelectedAll || isSelectedAllManually)
                {
                    configText.Add("*");
                    continue;
                }
                //if select none
                if (isAllFalse)
                {
                    configText.Add("-");
                    continue;
                }
                foreach (Tuple<String, bool> field in fieldList)
                {
                    if (field.Item2 == true) //if selected
                        configText.Add(field.Item1); //add field name
                }
                configText.Add("");
            }
            configText.Add("###End of File###");
            File.WriteAllLines(filename, configText);
            return true;
        }
        else
        {
            return false;
        }
    }
	/**
	* @brief Write all data from interfaceData into TSV format file
	*/
    public string selectWriteFileForTSV()
    {
        String filename;
        // Create OpenFileDialog 
        Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();

        // Set filter for file extension and default file extension 
        dlg.Filter = "TSV Files (*.tsv)| *.tsv";

        // Display OpenFileDialog by calling ShowDialog method 
        Nullable<bool> fileResult = dlg.ShowDialog();

        // Get the selected file name and display in a TextBox 
        if (fileResult == true)
        {
            // Open document 
            filename = dlg.FileName;
            return filename;
        }
        else
        {
            return "cancel";
        }
    }
    /**
    * @brief write all data to TSV file. While including header and check for error
    * @param fileName	    [in]	TSV filename
    * @param isCompare     	[in]    only get 1 user or compare 2 users data
    */
    public bool writeToTSV(String filename, bool isCompare)
    {
        List<String> tsvText = new List<String>();
        DB1Variable = m_interfaceData.getCurrentDB();
        user1Variable = m_interfaceData.getCurrentUser();
        version1Variable = m_interfaceData.getCurrentVersion();
        String DBUser1 = user1Variable + "@" + DB1Variable + "(" + version1Variable + ")";
        tsvText.Add("User1 : " + DBUser1);
        //print TSV header
        List<String> tsvHeader;
        if (isCompare)
        {
            tsvHeader = new List<String> { "DATASET", "NAME", "QUERY_KEYWORD", "CODE", "DESCRIPTION", "RECORD_COUNT1", "DISTANCE(KM) AREA(KM2)", "RECORD_COUNT2", "DISTANCE(KM) AREA(KM2)" };
            //show connecting messagebox
            Task.Factory.StartNew(() =>
            {
                System.Windows.MessageBox.Show("Connecting to DBUser2...", "Connecting");
            });
            bool isSuccess = m_interfaceData.connectToUser2();
            if (!isSuccess)
            {
                IntPtr hWns = FindWindowByCaption(IntPtr.Zero, "Connecting");
                if (hWns != IntPtr.Zero)
                    PostMessage(hWns, WM_CLOSE, 0, 0);
                System.Windows.MessageBox.Show("Cannot connect to DBUser 2", "Connection Error");
                return false;
            }
            DB2Variable = m_interfaceData.getCurrentDB2();
            user2Variable = m_interfaceData.getCurrentUser2();
            version2Variable = m_interfaceData.getCurrentVersion2();
            if (DB1Variable == DB2Variable && user1Variable == user2Variable && version1Variable == version2Variable)
            {
                IntPtr hWns = FindWindowByCaption(IntPtr.Zero, "Connecting");
                if (hWns != IntPtr.Zero)
                    PostMessage(hWns, WM_CLOSE, 0, 0);
                System.Windows.MessageBox.Show("DBUser1 and DBUser2 are identical. Stop comparing", "Error");
                return false;
            }
            String DBUser2 = user2Variable + "@" + DB2Variable + "(" + version2Variable + ")";
            tsvText.Add("User2 : " + DBUser2);
        }
        else
        {
            tsvHeader = new List<String> { "DATASET", "NAME", "QUERY_KEYWORD", "CODE", "DESCRIPTION", "RECORD_COUNT", "DISTANCE(KM) AREA(KM2)" };
        }
        IntPtr hWnj = FindWindowByCaption(IntPtr.Zero, "Connecting");
        if (hWnj != IntPtr.Zero)
            PostMessage(hWnj, WM_CLOSE, 0, 0);
        //show writing messagebox
        Task.Factory.StartNew(() =>
        {
            System.Windows.MessageBox.Show("Writing to TSV file...", "Writing");
        });
        String headerLine = String.Join(delimiter, tsvHeader);
        tsvText.Add(headerLine);
        //get dataset 1 by 1
        Dictionary<String, interfaceData.datasetInfo> datasetList = new Dictionary<string, interfaceData.datasetInfo>(m_interfaceData.getDatasetList());
        foreach (KeyValuePair<String, interfaceData.datasetInfo> dataset in datasetList)
        {
            interfaceData.datasetInfo datasetInf = dataset.Value;
            //if not select, skip
            if (!datasetInf.isDatasetSelected)
                continue;
            String datasetName = dataset.Key;
            int a = datasetName.LastIndexOf(m_interfaceData.getDatasetListKeywordSeparator());
            int b = datasetName.Length;
            datasetName = datasetName.Substring(a + 1, b - a - 1);

            String datasetCondition = datasetInf.searchCondition;
            //get processed TSV format dataset data
            String datasetRow = String.Join(delimiter, getDatasetRow(datasetName, datasetCondition, isCompare));
            tsvText.Add(datasetRow);
            //to print field list
            List<Tuple<String, bool>> fieldList = datasetInf.fieldInfo;
            if (fieldList.Count == 0 && datasetInf.isFieldSelectedAll)
            {
                List<String> tempFieldList = m_interfaceData.getFieldNameList(datasetName);
                List<Tuple<String, bool>> newColumnInfo = new List<Tuple<String, bool>>();
                //create button 1 by 1
                for (int i = 0; i < tempFieldList.Count; i++)
                {
                    String fieldName = tempFieldList[i];
                    if (fieldName.Contains(".")) continue;
                    Tuple<String, bool> newTuple = new Tuple<String, bool>(fieldName, true);
                    newColumnInfo.Add(newTuple);
                }
                m_interfaceData.setDatasetFieldInfo("", "", "", datasetName, newColumnInfo);
                fieldList = newColumnInfo;
            }
            //if select none
            else if (fieldList.Count <= 0)
            {
                continue;
            }
            foreach (Tuple<String, bool> field in fieldList)
            {
                if (field.Item2 == true) //if selected
                {
                    // if the field has no domain, print only 1 line
                    bool isTable = m_interfaceData.isDatasetTable(DB1Variable, user1Variable, version1Variable, datasetName);
                    if (!m_interfaceData.m_DBComparer.hasDomain(DB1Variable, user1Variable, version1Variable, datasetName, field.Item1, isTable))
                    {
                        String fieldRow = String.Join(delimiter, getSingleFieldRow(field.Item1, isCompare));
                        tsvText.Add(fieldRow); //add field name
                    }
                    else
                    {
                        List<String> multiFieldRow = getMultiFieldRow(datasetName, datasetCondition, field.Item1, isCompare);
                        foreach (String line in multiFieldRow)
                        {
                            tsvText.Add(line);
                        }
                    }
                }

            }
        }
        File.WriteAllLines(filename, tsvText);
        //close writing messagebox
        IntPtr hWnd = FindWindowByCaption(IntPtr.Zero, "Writing");
        if (hWnd != IntPtr.Zero)
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        if (isCompare)
        {
            System.Windows.MessageBox.Show(" Successfully output the compare result to the TSV file", "Success");
        }
        else
        {
            System.Windows.MessageBox.Show(" Successfully output the result to the TSV file", "Success");
        }
        return true;
    }
    /**
    * @brief Get TSV format dataset row
    * @param dataSetName	        [in]	dataset name
    * @param datasetCondition       [in]    dataset search condition
    * @param isCompare     	        [in]    only get 1 user or compare 2 users data
    * return one line of string which is dataset data in TSV format
    */
    public List<String> getDatasetRow(String datasetName, String datasetCondition, bool isCompare)
    {
        String parentDataset = "*";
		// check if target dataset is table or featureclass
        if (!m_interfaceData.isDatasetTable("", "", "", datasetName))
        {
            //if featureclass, get parent dataset name
            parentDataset = m_interfaceData.getParentDatasetName("", "", "", datasetName);
        }
		//set all necessary data before converted into TSV line

        String code = "";
        String description = "";
        String recordCount = m_interfaceData.getRecordNumber(datasetCondition, DB1Variable, user1Variable, version1Variable, datasetName);
        String distArea = m_interfaceData.getTotalDistArea(datasetCondition, DB1Variable, user1Variable, version1Variable, datasetName);
        List<String> datasetRow;
		// if compare data between 2 users, write the second user's data too
        if (isCompare)
        {
            String recordCount2 = m_interfaceData.getRecordNumber(datasetCondition, DB2Variable, user2Variable, version2Variable, datasetName);
            String distArea2 = m_interfaceData.getTotalDistArea(datasetCondition, DB2Variable, user2Variable, version2Variable, datasetName).ToString();
            datasetRow = new List<String> { parentDataset, datasetName, datasetCondition, code, description, recordCount, distArea, recordCount2, distArea2 };
        }
        else
        {
            datasetRow = new List<String> { parentDataset, datasetName, datasetCondition, code, description, recordCount, distArea };
        }

        return datasetRow;
    }
    /**
    * @brief Get TSV format field data
    * @param fieldName	            [in]	field name
    * @param isCompare     	        [in]    only get 1 user or compare 2 users data
    * return one line of string which is field data in TSV format
    */
    public List<String> getSingleFieldRow(String fieldName, bool isCompare)
    {
		//set all necessary data before converted into TSV line
        String parentDataset = "";
        String condition = "";
        String code = "";
        String description = "";
        String recordCount = "";
        String distArea = "";
        List<String> fieldRow;
        if (isCompare)
        {
            fieldRow = new List<String> { parentDataset, fieldName, condition, code, description, recordCount, distArea, recordCount, distArea };
        }
        else
        {
            fieldRow = new List<String> { parentDataset, fieldName, condition, code, description, recordCount, distArea };
        }
        return fieldRow;
    }
    /**
    * @brief Get TSV format field data
    * @param datasetName            [in]    dataset name
    * @param datasetCondtion        [in]    search condition
    * @param fieldName	            [in]	field name
    * @param isCompare     	        [in]    only get 1 user or compare 2 users data
    * return multiple lines of string which is field and domain data in TSV format
    */
    public List<String> getMultiFieldRow(String datasetName, String datasetCondition, String fieldName, bool isCompare)
    {
        //set all necessary data before converted into TSV line
        String parentDataset = "";
        String condition = "";
        List<String> domainDetails = m_interfaceData.getDomainNameList(datasetName, fieldName);
        String code = domainDetails[0];
        String description = "";
        String totalRecordCount = m_interfaceData.getRecordNumber(datasetCondition, DB1Variable, user1Variable, version1Variable, datasetName);
        String totalDistArea = m_interfaceData.getTotalDistArea(datasetCondition, DB1Variable, user1Variable, version1Variable, datasetName);

        List<String> multiFieldRow = new List<String>();
		// if compare data between 2 users, write the second user's data too
        if (isCompare)
        {
            String totalRecordCount2 = m_interfaceData.getRecordNumber(datasetCondition, DB2Variable, user2Variable, version2Variable, datasetName);
            String totalDistArea2 = m_interfaceData.getTotalDistArea(datasetCondition, DB2Variable, user2Variable, version2Variable, datasetName).ToString();
            if (domainDetails[1] == "Range") description = "Range " + domainDetails[2] + "-" + domainDetails[3];
            List<String> fieldRow = new List<String> { parentDataset, fieldName, condition, code, description, totalRecordCount, totalDistArea, totalRecordCount2, totalDistArea2 };
            String convertedFieldRow = String.Join(delimiter, fieldRow);
            multiFieldRow.Add(convertedFieldRow);
            if (domainDetails[1] == "Coded Value")
            {
                //get multiline domain details
                for (int i = 2; i < domainDetails.Count; i += 2)
                {
                    code = domainDetails[i];
                    description = domainDetails[i + 1];
                    String SQLText = fieldName + " = " + code; //ex. where OBJECTID > 10 AND PRODUCT_C = 3
                    if (!String.IsNullOrWhiteSpace(datasetCondition))
                    {
                        SQLText += " AND " + datasetCondition;
                    }
                    String fieldRecordCount = m_interfaceData.getRecordNumber(SQLText, DB1Variable, user1Variable, version1Variable, datasetName);
                    String fieldRecordCount2 = m_interfaceData.getRecordNumber(SQLText, DB2Variable, user2Variable, version2Variable, datasetName);
                    String fieldDistArea = m_interfaceData.getTotalDistArea(SQLText, DB1Variable, user1Variable, version1Variable, datasetName);
                    String fieldDistArea2 = m_interfaceData.getTotalDistArea(SQLText, DB2Variable, user2Variable, version2Variable, datasetName);
                    fieldRow = new List<String> { parentDataset, "", condition, code, description, fieldRecordCount, fieldDistArea, fieldRecordCount2, fieldDistArea2 };
                    convertedFieldRow = String.Join(delimiter, fieldRow);
                    multiFieldRow.Add(convertedFieldRow);
                }
            }	
        }
        else
        {
            if (domainDetails[1] == "Range") description = "Range " + domainDetails[2] + "-" + domainDetails[3];
            List<String> fieldRow = new List<String> { parentDataset, fieldName, condition, code, description, totalRecordCount, totalDistArea };
            String convertedFieldRow = String.Join(delimiter, fieldRow);
            multiFieldRow.Add(convertedFieldRow);
            if (domainDetails[1] == "Coded Value")
            {
                //get multiline domain details
                for (int i = 2; i < domainDetails.Count; i += 2)
                {
                    code = domainDetails[i];
                    description = domainDetails[i + 1];
                    String SQLText = fieldName + " = " + code; //ex. where OBJECTID > 10 AND PRODUCT_C = 3
                    if (!String.IsNullOrWhiteSpace(datasetCondition))
                    {
                        SQLText += " AND " + datasetCondition;
                    }
                    String fieldRecordCount = m_interfaceData.getRecordNumber(SQLText, DB1Variable, user1Variable, version1Variable, datasetName);
                    String fieldDistArea = m_interfaceData.getTotalDistArea(SQLText, DB1Variable, user1Variable, version1Variable, datasetName);
                    fieldRow = new List<String> { parentDataset, "", condition, code, description, fieldRecordCount, fieldDistArea };
                    convertedFieldRow = String.Join(delimiter, fieldRow);
                    multiFieldRow.Add(convertedFieldRow);
                }
            }
            

        }
        return multiFieldRow;
    }
}
