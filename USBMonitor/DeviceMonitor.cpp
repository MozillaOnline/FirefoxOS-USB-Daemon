#include "StdAfx.h"
#include "DeviceMonitor.h"
#include "App.h"

/*
 * The GUID_DEVINTERFACE_USB_DEVICE device interface class is defined for USB devices that are attached to a USB hub.
 * It is copied from usbiodef.h from DDK.
 * Refer to MSDN for details.
 * A5DCBF10-6530-11D2-901F-00C04FB951ED 
 */
const GUID GUID_DEVINTERFACE_USB_DEVICE \
                = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F,  0x00,  0xC0,  0x4F,  0xB9,  0x51,  0xED } };

DeviceMonitor::DeviceMonitor(void)
	: m_hDevNotify(NULL)
{
	isLoaded = false;
}


DeviceMonitor::~DeviceMonitor(void)
{
}

void DeviceMonitor::AddObserver(DeviceMonitorObserver* pObserver)
{
	// Don't add duplicated observer.
	if (FindObserver(pObserver) == -1)
	{
		m_aObservers.push_back(pObserver);
	}
}

void DeviceMonitor::RemoveObserver(DeviceMonitorObserver* pObserver)
{
	int index = FindObserver(pObserver);
	if (index == -1)
	{
		return;
	}

	// Move the item to be deleted to the end of the array and then remove it.
	if (index != static_cast<int>(m_aObservers.size()))
	{
		swap(m_aObservers[0], m_aObservers[index]);
	}
	m_aObservers.pop_back();
}

int DeviceMonitor::FindObserver(DeviceMonitorObserver* pObserver)
{
	// Check if we have already added the observer.
	int count = static_cast<int>(m_aObservers.size());
	for (int i = 0; i < count; i++)
	{
		if (m_aObservers[i] == pObserver)
		{
			return i;
		}
	}

	return -1;
}

void DeviceMonitor::RegisterToWindow(HWND hWnd)
{
	// Check if we have already registered.
	if (m_hDevNotify != NULL)
	{
		return;
	}
    DEV_BROADCAST_DEVICEINTERFACE   broadcastInterface;

    // Register to receive notification when a USB device is plugged in.
    broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    memcpy(&(broadcastInterface.dbcc_classguid),
        &(GUID_DEVINTERFACE_USB_DEVICE),
        sizeof(struct _GUID));

    m_hDevNotify = ::RegisterDeviceNotification(hWnd,
        &broadcastInterface,
        DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

	m_aDeviceList = GetDevicesList();
}

void DeviceMonitor::Unregister()
{
	if (m_hDevNotify != NULL)
	{
		::UnregisterDeviceNotification(m_hDevNotify);
		m_hDevNotify = NULL;
	}
}

/**
 * The wrapper function of CM_Get_DevNode_Registry_Property.
 * @param dnInst A caller-supplied device instance handle that is bound to the local machine.
 * @param ulProperty A CM_DRP_-prefixed constant value that identifies the device property to be obtained from the registry. These constants are defined in Cfgmgr32.h.
 * @return The string representation of the property of the following formats:
 *         REG_DWORD - The hexadecimal number is converted to a string of ten characters and starts with "0x", such as "0x00000000"
 *         REG_SZ - A single string.
 *         REG_MULTI_SZ - The concatenation of multiple strings separated by ",".
 */
static CString GetDevNodePropertyString(DEVINST dnDevInst, ULONG ulProperty)
{
	CString result;
	ULONG type = 0;

	// Get the property data length
	ULONG length = 0;
	CM_Get_DevNode_Registry_Property(dnDevInst, ulProperty, &type, NULL, &length, 0);
	if (length > 0)
	{
		// Get the property data value
		TCHAR* pBuffer = new TCHAR[length];
		if (CM_Get_DevNode_Registry_Property(dnDevInst, ulProperty, &type, pBuffer, &length, 0) == CR_SUCCESS)
		{
			switch (type)
			{
			case REG_DWORD:
				{
					DWORD dwValue = *reinterpret_cast<DWORD *>(pBuffer);
					result.Format(_T("0x%08x"), dwValue);
				}
				break;
			case REG_MULTI_SZ:
				{
					LPTSTR p = pBuffer;
					while (*p != _T('\0'))
					{
						p++;
						if (*p == _T('\0'))
						{
							*p = _T(',');
							p++;
						}
					}
					result = pBuffer;
					if (result.Right(1) == _T(","))
					{
						result = result.Left(result.GetLength() - 1);
					}
				}
				break;
			case REG_SZ:
				{
					result = pBuffer;
				}
				break;
			default:
				{
					TRACE(_T("Unkown property type: %d\n"), type);
				}
				break;
			}
		}
		delete[] pBuffer;
	}

	return result;
}

bool DeviceMonitor::GetFirefoxOSSubDeviceInfo(DEVINST dnDevInst, Json::Value &deviceInfo)
{
	// Enumerate the sub-devices to find the android sub-device
	Json::Reader reader;
	reader.parse("{\"AndroidHardwareID\": \"\",\"InstallState\": \"\"}", deviceInfo);
	DEVINST dnChild = NULL;

	if (CM_Get_Child(&dnChild, dnDevInst, 0) != CR_SUCCESS)
	{
		return false;
	}

	do
	{
		// Get device info.
		deviceInfo["AndroidHardwareID"] = Json::Value(CStringToUTF8String(GetDevNodePropertyString(dnChild, CM_DRP_HARDWAREID)));
		int nDevice = m_aDevices.size();
		for (int i = 0; i < nDevice; i++) {
			Json::Value device = m_aDevices[i];
			if(!strcmp(deviceInfo["AndroidHardwareID"].asCString(), device.asCString()))
			{
				deviceInfo["InstallState"] = Json::Value(_tcstol((LPCTSTR)GetDevNodePropertyString(dnChild, CM_DRP_INSTALL_STATE), NULL, 16));
				// Sometimes InstallState shows the driver is installed, but no driver exits. We need to check the CM_DRP_DRIVER property to ensure the driver is installed correctly.
				if (deviceInfo["InstallState"].asInt() == CM_INSTALL_STATE_INSTALLED && GetDevNodePropertyString(dnChild, CM_DRP_DRIVER).IsEmpty())
				{
					deviceInfo["InstallState"] = Json::Value(CM_INSTALL_STATE_FAILED_INSTALL);
				}
				TRACE(_T("Firefox OS device found!\nHardware ID: %s\nDriver install state: 0x%lx\n\n"), deviceInfo["AndroidHardwareID"], deviceInfo["InstallState"]);
				return true;
			}
		}
	}
	while(CM_Get_Sibling(&dnChild, dnChild, 0) == CR_SUCCESS);
	return false;
}

bool DeviceMonitor::Load(LPCTSTR strFileName)
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
	m_aDevices = root["devices"];
	return true;
}

Json::Value DeviceMonitor::GetDevicesList()
{
	if(!isLoaded)
	{
		CString fileName = CPaintManagerUI::GetInstancePath() + _T("devices.json");
		Load(fileName);
		isLoaded = true;
	}
	m_cs.Enter();

	Json::Value deviceList;
	Json::Reader reader;
	reader.parse("[]", deviceList);
	// Prepare to enumerate all the USB devices
    HDEVINFO hDeviceInfo = ::SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (hDeviceInfo == INVALID_HANDLE_VALUE)
    {
		m_cs.Leave();
		return deviceList;
	}

	// Enumerate all the USB devices to find the supported devices
	SP_DEVINFO_DATA spDevInfoData;
	spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(int i = 0; SetupDiEnumDeviceInfo(hDeviceInfo, i, &spDevInfoData); i++)
    {
		Json::Value pNode;
		// Get the device instance ID
		TCHAR szBuffer[MAX_PATH];
		if (!::SetupDiGetDeviceInstanceId(hDeviceInfo,
                                        &spDevInfoData,
                                        szBuffer,
                                        MAX_PATH, NULL))
		{
			break;
		}
		// Try to match the device instance ID first.
		if (GetFirefoxOSSubDeviceInfo(spDevInfoData.DevInst, pNode))
		{
			deviceList.append(pNode);
		}
    }
	::SetupDiDestroyDeviceInfoList(hDeviceInfo);
	m_cs.Leave();
	return deviceList;
}

bool DeviceMonitor::OnDeviceChange(UINT nEventType, PDEV_BROADCAST_HDR pHdr)
{
	// Check parameters
	if (nEventType != DBT_DEVICEARRIVAL && nEventType != DBT_DEVICEREMOVECOMPLETE)
	{
		return false;
	}
	if (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
	{
		return false;
	}
	Json::Value cur_deviceList;
	cur_deviceList = GetDevicesList();
	int nNeedUpdate = m_aDeviceList.compare(cur_deviceList);
	if(!nNeedUpdate)
	{
		return false;
	}
	m_aDeviceList = cur_deviceList;
	// Notify the observers that an supported device was changed.
	int oberverNumber = static_cast<int>(m_aObservers.size());
	for (int i = 0; i < oberverNumber; i++)
	{
		DeviceMonitorObserver* pObserver = m_aObservers[i];
		if (pObserver == NULL)
		{
			continue;
		}
		switch(nEventType)
		{
		case DBT_DEVICEARRIVAL:
			{
				// A supported device has been inserted.
				pObserver->OnDeviceChanged(m_aDeviceList, true);
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			{
				// A supported device has been removed
				pObserver->OnDeviceChanged(m_aDeviceList, false);
			}
			break;
		}
	}

	return true;
}