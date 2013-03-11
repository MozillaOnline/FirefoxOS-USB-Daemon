#include "StdAfx.h"
#include "DeviceMonitor.h"

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
}


DeviceMonitor::~DeviceMonitor(void)
{
	ClearDeviceList();
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
        DEVICE_NOTIFY_WINDOW_HANDLE);

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
static CDuiString dbcc_nameToDeviceInstanceId(LPCTSTR ddbc_name)
{
	CDuiString sEmptyId = _T("");

	if (ddbc_name == NULL)
	{
		return sEmptyId;
	}

	CDuiString sDbccName(ddbc_name);

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

	CDuiString sDevId = sDbccName.Mid(left, right - left);
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
	CDuiString sDevId = dbcc_nameToDeviceInstanceId(pDeviceInterface->dbcc_name);

	if (nEventType == DBT_DEVICEARRIVAL)
	{
		UpdateDeviceList();
	}

	// Check if the device is supported
	if (GetDeviceInfoById(sDevId) == NULL)
	{
		return false;
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
	if (index < 0 || index >= static_cast<int>(m_aDeviceList.size()))
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

	int count = static_cast<int>(m_aDeviceList.size());
	for (int i = 0; i < count; i++)
	{
		DeviceInfo* pDevInfo = m_aDeviceList[i];
		if (pDevInfo->DeviceInstanceId == lpcstrDeviceInstanceId)
		{
			return pDevInfo;
		}
	}
	return NULL;
}

void DeviceMonitor::ClearDeviceList()
{
	while(m_aDeviceList.size() > 0)
	{
		DeviceInfo* pNode = m_aDeviceList.back();
		m_aDeviceList.pop_back();
		if (pNode)
		{
			delete pNode;
		}
	}
}


void DeviceMonitor::UpdateDeviceList()
{
	ClearDeviceList();

	// Prepare to enumerate all the USB devices
    HDEVINFO hDeviceInfo = ::SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (hDeviceInfo == INVALID_HANDLE_VALUE)
    {
		return;
	}

	// Enumerate all the USB devices to find the supported devices
	SP_DEVINFO_DATA spDevInfoData;
	spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(int i = 0; SetupDiEnumDeviceInfo(hDeviceInfo, i, &spDevInfoData); i++)
    {
        DeviceInfo* pNode = new DeviceInfo();

		// Get the device instance ID
		TCHAR szBuffer[MAX_PATH];
		if (!::SetupDiGetDeviceInstanceId(hDeviceInfo,
                                        &spDevInfoData,
                                        szBuffer,
                                        MAX_PATH, NULL))
		{
			delete pNode;
			break;
		}
		pNode->DeviceInstanceId = szBuffer;

		if (FilterDevice(pNode))
		{
			m_aDeviceList.push_back(pNode);
		}
		else
		{
			delete pNode;
		}
    }

	::SetupDiDestroyDeviceInfoList(hDeviceInfo);
}

/*
 * Check if the USB is supported.
 * @return true if the device is supported. Otherwise false.
 */
bool DeviceMonitor::FilterDevice(DeviceInfo* pDevInfo)
{
	static LPCTSTR const arSupportedDeviceIds[] = {
		_T("USB\\VID_19D2&PID_1350\\FULL_UNAGI"),
		_T("USB\\VID_19D2&PID_1350\\FULL_OTORO")
	};

	int count = sizeof(arSupportedDeviceIds) / sizeof(LPCTSTR);
	for (int i = 0; i < count; i++)
	{
		if (pDevInfo->DeviceInstanceId == arSupportedDeviceIds[i])
			return true;
	}

	return false;
}