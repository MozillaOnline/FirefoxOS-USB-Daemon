#pragma once

struct DriverInfo
{
	CString DeviceInstanceId;
	CString AndroidHardwareID;
	CString DriverDownlaodURL;
};

/**
 * Database to store the supported device list.
 */
class DeviceDatabase
{
public:
	static DeviceDatabase* Instance() 
	{
		if (s_pInstance == NULL)
		{
			s_pInstance = new DeviceDatabase();
			s_pInstance->Load(CPaintManagerUI::GetInstancePath() + _T("drivers\\driver_list.json"));
		}
		return s_pInstance;
	}

	/**
	 * Load dabase from JSON file
	 */
	bool Load(LPCTSTR strFileName);

	const DriverInfo* FindDriverByDeviceInstanceID(const CString& id) const;

private:
	DeviceDatabase(void);
	~DeviceDatabase(void);

	static DeviceDatabase* s_pInstance;

	CAtlMap<CString, CAutoPtr<DriverInfo>, CElementTraits<CString> > m_driverMap;
};

