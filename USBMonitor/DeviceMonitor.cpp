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

    memcpy( &(broadcastInterface.dbcc_classguid),
        &(GUID_DEVINTERFACE_USB_DEVICE),
        sizeof(struct _GUID));

    m_hDevNotify = ::RegisterDeviceNotification(hWnd,
        &broadcastInterface,
        DEVICE_NOTIFY_WINDOW_HANDLE);
}

void DeviceMonitor::Unregister()
{
	if (m_hDevNotify != NULL)
	{
		::UnregisterDeviceNotification(m_hDevNotify);
		m_hDevNotify = NULL;
	}
}
