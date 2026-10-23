#pragma once

#include <windows.h>

#include <QObject>
#include <QQuickWindow>
#include <QRect>
#include <QVector>

class ConfigStore;

/**
 * Attaches the wallpaper QQuickWindow to the Win11 WorkerW layer.
 * Fallback: normal frameless bottom-most desktop window.
 * Main thread only.
 */
class WindowHost : public QObject
{
    Q_OBJECT
public:
    explicit WindowHost(ConfigStore *config, QObject *parent = nullptr);
    ~WindowHost() override;

    /** Call once after the QML window has been created and shown. */
    void attach(QQuickWindow *window);

    bool workerwAttached() const { return m_workerwAttached; }
    void setClickThrough(bool enabled);

public slots:
    /** Re-run the WorkerW lookup / SetParent (Explorer restart handling). */
    void reattach();

private slots:
    void onConfigChanged(const QString &key);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    static HWND findWorkerW(bool allowSpawn = true);
    void performAttach(bool allowSpawnWorkerW);

    /** Mode B: top-level bottom-pinned window (no wallpaper WorkerW). */
    void pinBottomFullscreen(int w, int h);

    /** Recompute the physical screen rectangles occupied by widget content. */
    void recomputeContentRects();
    /** True when the physical screen point lies inside a content rectangle. */
    bool hitTestContent(int x, int y) const;

    /** WM_NCHITTEST subclass: passes clicks through outside content without
     *  clipping rendering (a window region would clip the full-screen wash). */
    void installSurfaceSubclass();
    void forgetSurfaceSubclass();
    static LRESULT CALLBACK surfaceSubclassProc(HWND hwnd, UINT msg,
                                                WPARAM wp, LPARAM lp);

    /** Deferred retry while the shell (Progman) is not ready yet. */
    void scheduleReattach();

    /** Periodic self-heal: verify parent / geometry / visibility. */
    void checkIntegrity();

    static LRESULT CALLBACK messageWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    ConfigStore *m_config;
    QQuickWindow *m_window = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_attachedHost = nullptr;
    HWND m_msgHwnd = nullptr;
    WNDPROC m_prevMsgProc = nullptr;
    UINT m_taskbarRestartMsg = 0;
    int m_reattachRetries = 0;
    bool m_workerwAttached = false;
    bool m_topLevelMode = false;

    HWND m_subclassedHwnd = nullptr;
    QVector<QRect> m_contentRects;
};
