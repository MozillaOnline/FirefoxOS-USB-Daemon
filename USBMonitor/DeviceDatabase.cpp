#include "StdAfx.h"
#include "DeviceDatabase.h"
#include "App.h"

DeviceDatabase::DeviceDatabase(void)
{
}

DeviceDatabase::~DeviceDatabase(void)
{
}

DeviceDatabase* DeviceDatabase::s_pInstance = NULL;

bool DeviceDatabase::Load(LPCTSTR strFileName)
{
	USES_CONVERSION;

	if (strFileName == NULL)
	{
		return true;
	}

	std::ifstream fs(T2A(strFileName));
	if (!fs)
	{
		TRACE(_T("Failed to open %s\n"), strFileName);
		return false;
	}
	Json::Value root;
	Json::Reader reader;
	bool success = reader.parse(fs, root, false);
	fs.close();
	if (!success)
	{
		CString strMsg;
		strMsg = reader.getFormattedErrorMessages().c_str();
		TRACE(_T("%s\n"), (LPCTSTR)strMsg);
		return false;
	}
	Json::Value devices = root["devices"];
	int nDevice = devices.size();
	for (int i = 0; i < nDevice; i++)
	{
		Json::Value device = devices[i];
		CAutoPtr<DriverInfo> pDriverInfo(new DriverInfo());
		pDriverInfo->DeviceInstanceId = UTF8ToCString(device["device_instance_id"].asCString());
		pDriverInfo->AndroidHardwareID = UTF8ToCString(device["android_hardware_id"].asCString());
		pDriverInfo->DriverDownlaodURL = UTF8ToCString(device["driver_download_url"].asCString());
		pDriverInfo->ShowWindow = device["driver_download_url"].asBool();
		m_driverMap[pDriverInfo->DeviceInstanceId] = pDriverInfo;
	}
	return true;
}

const DriverInfo* DeviceDatabase::FindDriverByDeviceInstanceID(const CString& id) const
{

	auto pair = m_driverMap.Lookup(id);
	if (pair == NULL)
	{
		return NULL;
	}
	return pair->m_value;
}