#include "StdAfx.h"
#include "DeviceMonitor.h"
#include "DeviceDatabase.h"

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
	m_cs.Init();
}


DeviceMonitor::~DeviceMonitor(void)
{
	m_cs.Term();
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

	UpdateDeviceList();
}

void DeviceMonitor::Unregister()
{
	if (m_hDevNotify != NULL)
	{
		::UnregisterDeviceNotification(m_hDevNotify);
		m_hDevNotify = NULL;
	}
}

/*
 * Convert the dbcc_name to the device instance id.
 * See http://www.codeproject.com/Articles/14500/Detecting-Hardware-Insertion-and-or-Removal.
 * The sample of the dbcc_name of DEV_BROADCAST_DEVICEINTERFACE is as follows:
 * \\?\USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
 *
 *   \\?\USB: USB means this is a USB device class
 *   Vid_04e8&Pid_053b: Vid/Pid is VendorID and ProductID (but this is device class specific, USB use Vid/Pid, different device classes use different naming conventions)
 *   002F9A9828E0F06: Serial number.
 *   {a5dcbf10-6530-11d2-901f-00c04fb951ed}: the device interface class GUID
 * The corresponding device instance id is:
 * USB\Vid_04e8&Pid_503b\0002F9A9828E0F06
 * 
 */
static CString dbcc_nameToDeviceInstanceId(LPCTSTR ddbc_name)
{
	CString sEmptyId = _T("");

	if (ddbc_name == NULL)
	{
		return sEmptyId;
	}

	CString sDbccName(ddbc_name);

	// Skip the "\\?\" at the beginning
	int left = 4;
	if (sDbccName.GetLength() <= 4) 
	{
		return sEmptyId;
	}

	// Remove the device class ID at the end
    int right = sDbccName.ReverseFind(_T('#'));
	if (right < 4)
	{
		return sEmptyId;
	}

	CString sDevId = sDbccName.Mid(left, right - left);
	sDevId.Replace(_T("#"), _T("\\"));
	sDevId.MakeUpper();

    return sDevId;
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

	// Get the device instance id
	PDEV_BROADCAST_DEVICEINTERFACE pDeviceInterface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(pHdr);
	CString sDevId = dbcc_nameToDeviceInstanceId(pDeviceInterface->dbcc_name);

	if (nEventType == DBT_DEVICEARRIVAL)
	{
		UpdateDeviceList();
	}

	// Check if the device is supported
	if (GetDeviceInfoById(sDevId) == NULL)
	{
		const DeviceInfo* pInfo = GetDeviceInfoBySubDeviceId(sDevId);
		if (pInfo == NULL) 
		{
			return false;
		}
		sDevId = pInfo->DeviceInstanceId;
	}
	
	if (nEventType == DBT_DEVICEREMOVECOMPLETE)
	{
		UpdateDeviceList();
	}

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
				pObserver->OnDeviceInserted(sDevId);
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			{
				// A supported device has been removed
				pObserver->OnDeviceRemoved(sDevId);
			}
			break;
		}
	}

	return true;
}

const DeviceInfo* DeviceMonitor::GetDeviceInfoByIndex(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_aDeviceList.GetCount()))
	{
		return NULL;
	}
	return m_aDeviceList[index];
}

const DeviceInfo* DeviceMonitor::GetDeviceInfoById(LPCTSTR lpcstrDeviceInstanceId) const
{
	if (lpcstrDeviceInstanceId == NULL)
	{
		return NULL;
	}

	int count = static_cast<int>(m_aDeviceList.GetCount());
	for (int i = 0; i < count; i++)
	{
		const DeviceInfo* pDevInfo = m_aDeviceList[i];
		if (pDevInfo->DeviceInstanceId == lpcstrDeviceInstanceId)
		{
			return pDevInfo;
		}
	}
	return NULL;
}

const DeviceInfo* DeviceMonitor::GetDeviceInfoBySubDeviceId(LPCTSTR lpcstrDeviceInstanceId) const
{
	if (lpcstrDeviceInstanceId == NULL)
	{
		return NULL;
	}

	int count = static_cast<int>(m_aDeviceList.GetCount());
	for (int i = 0; i < count; i++)
	{
		const DeviceInfo* pDevInfo = m_aDeviceList[i];
		if (pDevInfo->AndroidSubDeviceInstanceId == lpcstrDeviceInstanceId)
		{
			return pDevInfo;
		}
	}
	return NULL;
}

void DeviceMonitor::UpdateDeviceList()
{
	m_cs.Lock();

	m_aDeviceList.RemoveAll();

	// Prepare to enumerate all the USB devices
    HDEVINFO hDeviceInfo = ::SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (hDeviceInfo == INVALID_HANDLE_VALUE)
    {
		m_cs.Unlock();
		return;
	}

	// Enumerate all the USB devices to find the supported devices
	SP_DEVINFO_DATA spDevInfoData;
	spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(int i = 0; SetupDiEnumDeviceInfo(hDeviceInfo, i, &spDevInfoData); i++)
    {
        CAutoPtr<DeviceInfo> pNode(new DeviceInfo());

		// Get the device instance ID
		TCHAR szBuffer[MAX_PATH];
		if (!::SetupDiGetDeviceInstanceId(hDeviceInfo,
                                        &spDevInfoData,
                                        szBuffer,
                                        MAX_PATH, NULL))
		{
			break;
		}
		pNode->DeviceInstanceId = szBuffer;
		pNode->DeviceSerialNumber = DeviceInfo::GetSerialNumber(pNode->DeviceInstanceId);

		if (FilterDevice(pNode))
		{
			if (!GetAndroidSubDeviceInfo(spDevInfoData.DevInst, pNode))
			{
				pNode->InstallState = 4;
				TRACE(_T("Failed to get android sub device!\n"));
			}
			m_aDeviceList.Add(pNode);
			pNode.Detach();
		}
    }

	::SetupDiDestroyDeviceInfoList(hDeviceInfo);

	m_cs.Unlock();
}

/*
 * Check if the USB is supported.
 * @return true if the device is supported. Otherwise false.
 */
bool DeviceMonitor::FilterDevice(const DeviceInfo* pDevInfo)
{
	return DeviceDatabase::Instance()->FindDriverByDeviceInstanceID(pDevInfo->DeviceInstanceId) != NULL;
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

bool DeviceMonitor::GetAndroidSubDeviceInfo(DEVINST dnDevInst, DeviceInfo* pDevInfo)
{
	ASSERT(pDevInfo);

	// Enumerate the sub-devices to find the android sub-device

	DEVINST dnChild = NULL;
	if (CM_Get_Child(&dnChild, dnDevInst, 0) != CR_SUCCESS)
	{
		return false;
	}

	do
	{
		/**
		 * If the driver of the android device is not installed, the device description should be "Android" or "Android Device",
		 * and the device class is empty.
		 * Otherwise the device description is specified by the driver, and the class GUID is {f72fe0d4-cbcb-407d-8814-9ed673d0dd6b}.
		 * As a result, we can find the android device by checking either its decription or device class GUID.
		 */
		CString sDeviceDescription = GetDevNodePropertyString(dnChild, CM_DRP_DEVICEDESC);
		CString sLowercaseDesc = sDeviceDescription;
		sLowercaseDesc.MakeLower();

		CString sClassGUID = GetDevNodePropertyString(dnChild, CM_DRP_CLASSGUID);
		
		// Check if the sub device is the android.
		if (sLowercaseDesc.Find(_T("android")) == -1 && sClassGUID != _T("{f72fe0d4-cbcb-407d-8814-9ed673d0dd6b}"))
		{
			continue;
		}

		// Get device info.
		TCHAR szDeviceID[MAX_DEVICE_ID_LEN];
		if (CM_Get_Device_ID(dnChild, szDeviceID, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS)
		{
			pDevInfo->AndroidSubDeviceInstanceId = szDeviceID;
		}
		pDevInfo->DeviceDescription = sDeviceDescription;
		pDevInfo->AndroidHardwareID = GetDevNodePropertyString(dnChild, CM_DRP_HARDWAREID);
		pDevInfo->InstallState = _tcstol((LPCTSTR)GetDevNodePropertyString(dnChild, CM_DRP_INSTALL_STATE), NULL, 16);
		// Sometimes InstallState shows the driver is installed, but no driver exits. We need to check the CM_DRP_DRIVER property to ensure the driver is installed correctly.
		if (pDevInfo->InstallState == CM_INSTALL_STATE_INSTALLED && GetDevNodePropertyString(dnChild, CM_DRP_DRIVER).IsEmpty())
		{
			pDevInfo->InstallState = CM_INSTALL_STATE_FAILED_INSTALL;
		}
		TRACE(_T("Android device found!\nDevice ID:%s\nDescription: %s\nHardware ID: %s\nDriver install state: 0x%lx\n\n"), szDeviceID, (LPCTSTR)pDevInfo->DeviceDescription, (LPCTSTR)pDevInfo->AndroidHardwareID, pDevInfo->InstallState);

		return true;
	}
	while(CM_Get_Sibling(&dnChild, dnChild, 0) == CR_SUCCESS);

	return false;
}