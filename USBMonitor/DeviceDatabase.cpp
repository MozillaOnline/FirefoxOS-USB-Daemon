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

	CString s_version;//{32bit: "x86","x86.6.1","x86.6.2"}, {64bit:"amd64","amd64.6.1","amd64.6.2"}
	OSVERSIONINFOEX versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof versionInfo;
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&versionInfo));
	if (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion == 2)
    {
		s_version = Is64BitWindows() ? "amd64.6.2" : "x86.6.2";
	} else {
		s_version = Is64BitWindows() ? "amd64" : "x86";
	}

	Json::Value devices = root["devices"];
	int nDevice = devices.size();
	for (int i = 0; i < nDevice; i++)
	{
		Json::Value device = devices[i];
		CAutoPtr<DriverInfo> pDriverInfo(new DriverInfo());
		CAutoPtr<DriverInfo> pDriverInfoClone(new DriverInfo());
		pDriverInfo->DeviceInstanceId = UTF8ToCString(device["device_instance_id"].asCString());
		pDriverInfoClone->DeviceInstanceId = pDriverInfo->DeviceInstanceId;
		pDriverInfo->AndroidHardwareID = UTF8ToCString(device["android_hardware_id"].asCString());
		pDriverInfoClone->AndroidHardwareID = pDriverInfo->AndroidHardwareID;

		Json::Value drivers = device["drivers"];
		int nDrivers = drivers.size();
		for (int j = 0; j < nDrivers; j++)
		{
			Json::Value driver = drivers[j];
			CString os = UTF8ToCString(driver["OS"].asCString());
			if(os == "all" || os == s_version) {
				pDriverInfo->DriverDownlaodURL = UTF8ToCString(driver["download_url"].asCString());
				pDriverInfoClone->DriverDownlaodURL = pDriverInfo->DriverDownlaodURL;
				CString installType = UTF8ToCString(driver["install_type"].asCString());
				pDriverInfo->InstallType = installType == _T("dpinst") ? DPINST : EXE;
				pDriverInfoClone->InstallType = pDriverInfo->InstallType;
				m_instanceIDMap[pDriverInfo->DeviceInstanceId] = pDriverInfo;
				m_hardwareIDMap[pDriverInfoClone->AndroidHardwareID] = pDriverInfoClone;
				break;
			}
		}
	}
	return true;
}

const DriverInfo* DeviceDatabase::FindDriverByDeviceInstanceID(const CString& id) const
{

	auto pair = m_instanceIDMap.Lookup(id);
	if (pair == NULL)
	{
		return NULL;
	}
	return pair->m_value;
}

const DriverInfo* DeviceDatabase::FindDriverByAndroidHardwareID(const CString& id) const
{

	auto pair = m_hardwareIDMap.Lookup(id);
	if (pair == NULL)
	{
		return NULL;
	}
	return pair->m_value;
}