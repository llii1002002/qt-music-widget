#pragma once

#include <windows.h>
#include <mmdeviceapi.h>

#include <QThread>

/**
 * Watches the default render endpoint on its own MTA thread.
 * Emits defaultRenderDeviceChanged() (value-less; consumers restart capture).
 */
class DeviceMonitor : public QThread, public IMMNotificationClient
{
    Q_OBJECT
public:
    explicit DeviceMonitor(QObject *parent = nullptr);
    ~DeviceMonitor() override;

    // IUnknown (object lifetime is managed by QThread; no COM ref counting)
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
    {
        if (!ppv)
            return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
            *ppv = static_cast<IMMNotificationClient *>(this);
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // IMMNotificationClient
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                     LPCWSTR) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        return S_OK;
    }

signals:
    void defaultRenderDeviceChanged();

protected:
    void run() override;

private:
    HANDLE m_stopEvent = nullptr;
};
