#include "DeviceMonitor.h"

#include <objbase.h>

DeviceMonitor::DeviceMonitor(QObject *parent)
    : QThread(parent)
{}

DeviceMonitor::~DeviceMonitor()
{
    if (m_stopEvent)
        SetEvent(m_stopEvent);
    wait(3000);
}

HRESULT STDMETHODCALLTYPE DeviceMonitor::OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                               LPCWSTR)
{
    if (flow == eRender && (role == eConsole || role == eMultimedia))
        emit defaultRenderDeviceChanged();
    return S_OK;
}

void DeviceMonitor::run()
{
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
        return;

    IMMDeviceEnumerator *enumerator = nullptr;
    const HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                        reinterpret_cast<void **>(&enumerator));
    if (SUCCEEDED(hr) && enumerator)
        enumerator->RegisterEndpointNotificationCallback(this);

    WaitForSingleObject(m_stopEvent, INFINITE);

    if (enumerator) {
        enumerator->UnregisterEndpointNotificationCallback(this);
        enumerator->Release();
    }
    CoUninitialize();
    CloseHandle(m_stopEvent);
    m_stopEvent = nullptr;
}
