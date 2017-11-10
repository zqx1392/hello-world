inline IWorkspaceName2Ptr create_at_style_workspace_string_name([SA_Pre(Null = SA_No, NullTerminated = SA_Yes)] LPCTSTR lpszConnection)
{
	LPCTSTR p1, p2, p3;
	if (!((p1 = ::_tcschr(lpszConnection, _T('@'))) && (p2 = ::_tcschr(p1 + 1, _T('('))) && (p3 = ::_tcsrchr(p2 + 1, _T(')'))) && (*(p3 + 1) == 0)))
		return 0;

	size_t nUserLen = p1 - lpszConnection;
	size_t nServerLen = p2 - (p1 + 1);
	size_t nVersionLen = p3 - (p2 + 1);
	if (nUserLen <= 0 || nServerLen <= 0 || nVersionLen <= 0)
		return 0;

	// OracleDC接続のサブルーチンを呼び出す
	return create_workspace_name_oracle_dc(
		CComVariant(CComBSTR(static_cast<int>(nServerLen), p1 + 1)),
		CComVariant(CComBSTR(static_cast<int>(nUserLen), lpszConnection)),
		CComVariant(CComBSTR(static_cast<int>(nUserLen), lpszConnection)),
		CComVariant(CComBSTR(static_cast<int>(nVersionLen), p2 + 1))
	);
}

inline IWorkspaceName2Ptr create_workspace_name_oracle_dc(const CComVariant& tnsName, const CComVariant& user, const CComVariant& password, const CComVariant& version)
{
	// SDE用のWorkspaceNameを生成し、接続プロパティを設定する。
	const IPropertySetPtr ipPropertySet(__uuidof(PropertySet));

	// DBCLIENT, AUTHENTICATION_MODEプロパティは決め打ち
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("DBCLIENT")), CComVariant(_T("oracle"))));
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("AUTHENTICATION_MODE")), CComVariant(_T("DBMS"))));

	// 各引数の値をプロパティにセット
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("DB_CONNECTION_PROPERTIES")), tnsName));
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("USER")), user));
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("PASSWORD")), password));
	atl2::valid(ipPropertySet->SetProperty(CComBSTR(_T("VERSION")), version));

	// IWorkspaceNameを生成して返す
	const IWorkspaceName2Ptr ipWorkspaceName(__uuidof(WorkspaceName));
	atl2::valid(ipWorkspaceName->put_WorkspaceFactoryProgID(CComBSTR(_T("esriDataSourcesGDB.SdeWorkspaceFactory"))));
	atl2::valid(ipWorkspaceName->put_ConnectionProperties(ipPropertySet));
	return ipWorkspaceName;
}

- install RHEL
- find 2 tools bugs

sell on shopee, facebookmarket


1. upgrade facebook to add shop and service (also advert?)
2. post to other facebook page
3. post guitar to pantip, kaidee
4. copy website, get their weakness.


ぎみ（気味） につれて　に加える　てから→て以来

When you do pullups, try to imagine yourself pulling your elbows down, not pulling your body up. It's a mental trick that can make them feel easier, because it forces you to use the muscles in your back more.

For now.
- call thaiairways for royal orchid account
- start clannad. again.
- clear ear wax

Japan
- write more
- jdrama

Exercise
- running or self weight

Programming
- Soon

stardew valley, 
NieR:Automata,
E3 games (shit I need ps4...)


