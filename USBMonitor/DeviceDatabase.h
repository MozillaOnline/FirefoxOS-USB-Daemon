#pragma once

enum InstallType
{
	DPINST,
	EXE
};

struct DriverInfo
{
	CString DeviceInstanceId;
	CString AndroidHardwareID;
	CString DriverDownlaodURL;
	InstallType InstallType;
};

/**
 * Database to store the supported device list.
 */
class DeviceDatabase
{
public:
	/**
	 * Get a database instance and create once if not exists.
	 */
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
	 * Free the Device database.
	 */
	static void Close()
	{
		if (s_pInstance)
		{
			delete s_pInstance;
		}
	}

	/**
	 * Load dabase from JSON file
	 */
	bool Load(LPCTSTR strFileName);

	const DriverInfo* FindDriverByDeviceInstanceID(const CString& id) const;

	const DriverInfo* FindDriverByAndroidHardwareID(const CString& id) const;

private:
	DeviceDatabase(void);
	~DeviceDatabase(void);

	static DeviceDatabase* s_pInstance;

	CAtlMap<CString, CAutoPtr<DriverInfo>, CElementTraits<CString> > m_instanceIDMap;
	CAtlMap<CString, CAutoPtr<DriverInfo>, CElementTraits<CString> > m_hardwareIDMap;
};

