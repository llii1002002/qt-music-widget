#include "WindowHost.h"

#include "../core/ConfigStore.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMetaObject>
#include <QLoggingCategory>
#include <QEvent>
#include <QWindow>
#include <QTimer>
#include <QPlatformSurfaceEvent>
#include <QRegion>
#include <QMap>
#include <QPair>

#include <windowsx.h>

// Surface HWND -> (owner, previous wndproc) for WM_NCHITTEST subclassing.
static QMap<HWND, QPair<class WindowHost *, WNDPROC>> s_surfaceSubclasses;

namespace {
constexpr UINT WM_MD_REATTACH_PRIVATE = WM_APP + 0x51;
}

WindowHost::WindowHost(ConfigStore *config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    connect(m_config, &ConfigStore::changed, this, &WindowHost::onConfigChanged);
}

WindowHost::~WindowHost()
{
    if (m_subclassedHwnd) {
        auto it = s_surfaceSubclasses.constFind(m_subclassedHwnd);
        if (it != s_surfaceSubclasses.cend() && IsWindow(m_subclassedHwnd)) {
            SetWindowLongPtrW(m_subclassedHwnd, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(it->second));
        }
        s_surfaceSubclasses.remove(m_subclassedHwnd);
        m_subclassedHwnd = nullptr;
    }
    if (m_msgHwnd) {
        if (m_prevMsgProc)
            SetWindowLongPtrW(m_msgHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_prevMsgProc));
        DestroyWindow(m_msgHwnd);
    }
}

void WindowHost::attach(QQuickWindow *window)
{
    m_window = window;
    // Catch controlled HWND recreations (flag toggles) and re-parent the
    // fresh surface back onto the wallpaper host.
    m_window->installEventFilter(this);

    // ALL extended styles are expressed through Qt flags so Qt's internal
    // style cache stays authoritative:
    //   Qt::Tool                       -> WS_EX_TOOLWINDOW (no taskbar/Alt+Tab)
    //   Qt::WindowDoesNotAcceptFocus   -> WS_EX_NOACTIVATE
    //   Qt::WindowTransparentForInput  -> WS_EX_LAYERED | WS_EX_TRANSPARENT
    // Any external SetWindowLong(GWL_EXSTYLE) races Qt's window manager and
    // causes silent HWND recreation with detached/frozen surfaces.
    Qt::WindowFlags f = Qt::FramelessWindowHint
                      | Qt::Tool
                      | Qt::WindowDoesNotAcceptFocus;
    if (m_config->boolValue(CfgKey::WindowClickThrough))
        f |= Qt::WindowTransparentForInput;
    m_window->setFlags(f);

    m_hwnd = reinterpret_cast<HWND>(window->winId());

    // Hidden TOP-LEVEL tool window used to observe Explorer restarts
    // (TaskbarCreated) and display changes. It MUST be a real top-level
    // window: HWND_BROADCAST messages like TaskbarCreated are never
    // delivered to HWND_MESSAGE message-only windows.
    m_taskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");
    m_msgHwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                L"Static", L"MusicDesktopHostMsg",
                                WS_POPUP, 0, 0, 0, 0,
                                nullptr, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
    if (m_msgHwnd) {
        SetWindowLongPtrW(m_msgHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_prevMsgProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            m_msgHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowHost::messageWndProc)));
    }

    performAttach(/*allowSpawnWorkerW=*/true);
    installSurfaceSubclass();

    // Self-healing watchdog: Explorer restarts destroy our parent Progman
    // (taking our HWND with it) and broadcast-based notifications can be
    // missed. Independently verify parent/geometry/visibility every 2 s.
    auto *watchdog = new QTimer(this);
    connect(watchdog, &QTimer::timeout, this, &WindowHost::checkIntegrity);
    watchdog->start(2000);
}

// Locate Qt's (re)created top-level surface for this process WITHOUT calling
// QWindow::winId(): forcing creation during a shell restart blocks the main
// thread indefinitely in CTF/IME initialisation. Qt recreates destroyed
// surfaces itself; we simply adopt the new HWND.
static HWND findOwnQtSurface()
{
    struct Ctx { HWND found; DWORD pid; } ctx{ nullptr, GetCurrentProcessId() };
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto *c = reinterpret_cast<Ctx *>(lp);
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        if (pid == c->pid) {
            wchar_t cls[64] = {0};
            GetClassNameW(h, cls, 63);
            if (wcsncmp(cls, L"Qt5QWindow", 10) == 0) {
                c->found = h;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));
    return ctx.found;
}

void WindowHost::checkIntegrity()
{
    if (!m_window)
        return;
    bool surfaceRecreated = false;
    if (!m_hwnd || !IsWindow(m_hwnd)) {
        // Old surface gone with the shell; adopt Qt's fresh replacement.
        m_hwnd = findOwnQtSurface();
        if (!m_hwnd)
            return; // Qt has not recreated it yet; try again next tick
        m_attachedHost = nullptr;
        surfaceRecreated = true;
    }

    RECT rc;
    const bool geometryOk = GetWindowRect(m_hwnd, &rc)
        && (rc.right - rc.left) >= 1000 && (rc.bottom - rc.top) >= 500;

    if (m_topLevelMode) {
        // Bottom-most z-order is established once at attach. We do NOT fight
        // other self-bottoming wallpaper peers (e.g. vendor CEF desktop
        // widgets); rising one notch still keeps us below every normal app.
        // The watchdog only repairs real faults: minimisation (Win+D),
        // geometry/visibility loss, or a recreated surface.
        bool needHeal = !geometryOk || !IsWindowVisible(m_hwnd)
                        || IsIconic(m_hwnd) || surfaceRecreated;
        if (!needHeal)
            return;
        m_reattachRetries = 0;
        // A fresh surface on a WorkerW-capable system may be allowed to spawn.
        performAttach(surfaceRecreated);
        return;
    }

    const HWND parent = GetAncestor(m_hwnd, GA_PARENT);
    const bool parentOk = m_attachedHost && IsWindow(m_attachedHost)
        && parent == m_attachedHost;

    if (parentOk && geometryOk && IsWindowVisible(m_hwnd) && !surfaceRecreated)
        return; // healthy

    m_attachedHost = nullptr;
    m_reattachRetries = 0;
    performAttach(surfaceRecreated);
}

void WindowHost::reattach()
{
    // TaskbarCreated arrives while the shell is still initialising; give it
    // a moment, then let the watchdog / performAttach converge. Never block
    // this slot on cross-process shell calls.
    QTimer::singleShot(2500, this, [this] {
        m_attachedHost = nullptr;
        m_reattachRetries = 0;
        if (m_hwnd && IsWindow(m_hwnd))
            performAttach(/*allowSpawnWorkerW=*/true);
    });
}

void WindowHost::scheduleReattach()
{
    if (m_reattachRetries >= 30) // ~30s max (30 * 1000ms)
        return;
    ++m_reattachRetries;
    QTimer::singleShot(1000, this, [this] {
        if (m_hwnd && IsWindow(m_hwnd))
            performAttach(/*allowSpawnWorkerW=*/false);
    });
}

bool WindowHost::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_window && event->type() == QEvent::PlatformSurface) {
        auto *ev = static_cast<QPlatformSurfaceEvent *>(event);
        if (ev->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            // Our parent (Progman) is being destroyed and takes this HWND
            // with it. Qt will recreate the surface later; just forget it.
            // Calling winId() here would CreateWindow during the shell
            // restart window and BLOCK indefinitely in CTF initialisation.
            forgetSurfaceSubclass();
            m_hwnd = nullptr;
            m_attachedHost = nullptr;
            m_workerwAttached = false;
        } else if (ev->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            m_hwnd = reinterpret_cast<HWND>(m_window->winId());
            // Reparent slightly later: right after creation Qt itself still
            // applies geometry/show state; let it settle first.
            QTimer::singleShot(300, this, [this] {
                m_attachedHost = nullptr;
                m_reattachRetries = 0;
                if (m_hwnd && IsWindow(m_hwnd)) {
                    performAttach(/*allowSpawnWorkerW=*/false);
                    installSurfaceSubclass();
                }
            });
        }
    }
    return QObject::eventFilter(obj, event);
}

void WindowHost::setClickThrough(bool enabled)
{
    if (!m_window)
        return;
    // Qt recreates the platform window; eventFilter re-attaches afterwards.
    m_window->setFlag(Qt::WindowTransparentForInput, enabled);
}

void WindowHost::onConfigChanged(const QString &key)
{
    if (key == QLatin1String(CfgKey::WindowClickThrough)) {
        // Hit-testing reads the live config, so nothing else to refresh;
        // the Qt flag toggle below recreates the HWND (SurfaceCreated
        // re-runs the attach and reinstalls the subclass).
        setClickThrough(m_config->boolValue(CfgKey::WindowClickThrough));
    } else if (m_topLevelMode
               && (key == QLatin1String(CfgKey::CoverSize)
                   || key == QLatin1String(CfgKey::LyricOffsetX)
                   || key == QLatin1String(CfgKey::LyricOffsetY)
                   || key == QLatin1String(CfgKey::CoverOffsetX)
                   || key == QLatin1String(CfgKey::CoverOffsetY))) {
        recomputeContentRects();
    }
}

// Quick non-blocking check that Progman exists AND pumps messages. A freshly
// restarted shell can have its window created while its UI thread is still
// deep in initialisation; sending anything to it before that point blocks.
static HWND responsiveProgman()
{
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman)
        return nullptr;
    DWORD_PTR result = 0;
    if (!SendMessageTimeoutW(progman, WM_NULL, 0, 0,
                             SMTO_ABORTIFHUNG | SMTO_BLOCK, 200, &result))
        return nullptr;
    return progman;
}

HWND WindowHost::findWorkerW(bool allowSpawn)
{
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman)
        return nullptr;

    // Startup-only path: ask Progman to spawn the wallpaper WorkerW and poll
    // for it briefly. Never used on the Explorer-restart path (there a
    // half-initialised shell could block the SendMessage; Progman itself is
    // the fallback host anyway).
    if (allowSpawn) {
        DWORD_PTR result = 0;
        SendMessageTimeoutW(progman, 0x052C, 0, 0,
                            SMTO_ABORTIFHUNG, 500, &result);
        for (int attempt = 0; attempt < 5; ++attempt) {
            HWND target = nullptr;
            EnumWindows([](HWND top, LPARAM lp) -> BOOL {
                HWND defView = FindWindowExW(top, nullptr, L"SHELLDLL_DefView", nullptr);
                if (defView && !*reinterpret_cast<HWND *>(lp))
                    *reinterpret_cast<HWND *>(lp) =
                        FindWindowExW(nullptr, top, L"WorkerW", nullptr);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&target));
            if (target)
                return target;
            Sleep(120);
        }
    }

    // Immediate, non-blocking lookups only.
    HWND target = nullptr;
    EnumWindows([](HWND top, LPARAM lp) -> BOOL {
        HWND defView = FindWindowExW(top, nullptr, L"SHELLDLL_DefView", nullptr);
        if (defView && !*reinterpret_cast<HWND *>(lp))
            *reinterpret_cast<HWND *>(lp) =
                FindWindowExW(nullptr, top, L"WorkerW", nullptr);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&target));
    if (!target)
        target = FindWindowExW(nullptr, progman, L"WorkerW", nullptr);
    return target;
}

void WindowHost::performAttach(bool allowSpawnWorkerW)
{
    if (!m_hwnd || !IsWindow(m_hwnd))
        m_hwnd = findOwnQtSurface();
    if (!m_hwnd)
        return;

    // Verify the shell is responsive before touching any of its windows.
    HWND progman = responsiveProgman();
    if (!progman) {
        scheduleReattach();
        return;
    }

    const int w = GetSystemMetrics(SM_CXSCREEN);
    const int h = GetSystemMetrics(SM_CYSCREEN);

    // Reuse the known wallpaper host when it is still alive (avoids
    // re-sending 0x052C, which would spawn an extra WorkerW on some builds).
    HWND host = m_attachedHost;
    if (host && !IsWindow(host))
        host = nullptr;
    if (!host)
        host = findWorkerW(allowSpawnWorkerW);

    if (host) {
        // ---- Mode A: live as a child of the wallpaper WorkerW ----
        // The classic layer: we paint behind SHELLDLL_DefView (icons stay
        // visible and interactive) and survive "Show desktop" / Win+D.
        m_topLevelMode = false;
        if (m_window)
            m_window->setMask(QRegion()); // never clip in child mode
        const HWND currentParent = GetAncestor(m_hwnd, GA_PARENT);
        if (currentParent != host) {
            SetLastError(0);
            SetParent(m_hwnd, host);
            DWORD gle = GetLastError();
            if (gle != 0)
                qWarning("[Host] SetParent to WorkerW failed, gle=%lu", gle);
        }
        m_attachedHost = host;
        HWND actual = GetAncestor(m_hwnd, GA_PARENT);
        m_workerwAttached = (actual == host);
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, w, h,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        m_reattachRetries = 0;
        return;
    }

    // ---- Mode B: top-level bottom-pinned overlay ----
    // On newer Windows 11 builds 0x052C never spawns a wallpaper WorkerW,
    // and a WS_EX_LAYERED Qt/ANGLE surface does not composite when parented
    // to Progman (it is covered by the shell's own wallpaper painting).
    // Stay top-level: DWM composites us correctly and HWND_BOTTOM keeps us
    // below every normal application window and above the wallpaper layer.
    m_topLevelMode = true;
    m_attachedHost = nullptr;
    m_workerwAttached = false;
    {
        // GA_PARENT of a top-level window reports the desktop (#32769);
        // only detach if something actually parented us (a stale host).
        HWND anc = GetAncestor(m_hwnd, GA_PARENT);
        wchar_t acls[32] = {0};
        if (anc)
            GetClassNameW(anc, acls, 31);
        if (anc && wcscmp(acls, L"#32769") != 0)
            SetParent(m_hwnd, nullptr);
    }
    // A stale window region would clip the full-screen gradient wash; input
    // shaping is done via WM_NCHITTEST instead.
    if (m_window)
        m_window->setMask(QRegion());
    pinBottomFullscreen(w, h);
    recomputeContentRects();
    m_reattachRetries = 0;
}

void WindowHost::pinBottomFullscreen(int w, int h)
{
    if (IsIconic(m_hwnd))
        ShowWindow(m_hwnd, SW_RESTORE);
    SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, w, h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
}

void WindowHost::recomputeContentRects()
{
    m_contentRects.clear();
    if (!m_window || !m_topLevelMode)
        return;

    // Geometry mirrors the content blocks in MainWindow.qml. Coordinates are
    // QWindow geometry units, which equal physical screen pixels here
    // (the window is a 1:1 full-screen PER_MONITOR_AWARE surface at 0,0).
    // Lyrics and cover are now two independently positioned blocks, each
    // anchored at the screen centre with its own X/Y offsets.
    const int winW = m_window->width();
    const int winH = m_window->height();
    const int cover = m_config->value(CfgKey::CoverSize).toInt();
    const int lyricOffX = m_config->value(CfgKey::LyricOffsetX).toInt();
    const int lyricOffY = m_config->value(CfgKey::LyricOffsetY).toInt();
    const int coverOffX = m_config->value(CfgKey::CoverOffsetX).toInt();
    const int coverOffY = m_config->value(CfgKey::CoverOffsetY).toInt();

    const int colW = 920;
    const int lyricH = 230;
    const int lyricX = (winW - colW) / 2 + lyricOffX;
    const int lyricY = winH / 2 - lyricH / 2 + lyricOffY;
    m_contentRects.append(QRect(lyricX, lyricY, colW, lyricH));

    const int coverX = (winW - colW) / 2 + coverOffX;
    const int coverY = winH / 2 - cover / 2 + coverOffY;
    m_contentRects.append(QRect(coverX, coverY, colW, cover));
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int taskbarGap = winH - workArea.bottom; // taskbar height at bottom
    m_contentRects.append(QRect(70, winH - (taskbarGap + 12) - 120,
                                winW - 140, 120));
}

bool WindowHost::hitTestContent(int x, int y) const
{
    for (const QRect &r : m_contentRects) {
        if (r.contains(x, y))
            return true;
    }
    return false;
}

void WindowHost::installSurfaceSubclass()
{
    if (!m_hwnd || m_subclassedHwnd == m_hwnd)
        return;
    if (s_surfaceSubclasses.contains(m_hwnd)) {
        m_subclassedHwnd = m_hwnd;
        return;
    }
    auto *prev = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        m_hwnd, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(&WindowHost::surfaceSubclassProc)));
    if (!prev) {
        qWarning("[Host] subclass failed for hwnd=0x%p", m_hwnd);
        return;
    }
    s_surfaceSubclasses.insert(m_hwnd, qMakePair(this, prev));
    m_subclassedHwnd = m_hwnd;
}

void WindowHost::forgetSurfaceSubclass()
{
    if (!m_subclassedHwnd)
        return;
    // The HWND is being destroyed by Qt; do not swap its wndproc back.
    s_surfaceSubclasses.remove(m_subclassedHwnd);
    m_subclassedHwnd = nullptr;
}

LRESULT CALLBACK WindowHost::surfaceSubclassProc(HWND hwnd, UINT msg,
                                                 WPARAM wp, LPARAM lp)
{
    auto it = s_surfaceSubclasses.constFind(hwnd);
    const WNDPROC prev = (it != s_surfaceSubclasses.cend()) ? it->second : nullptr;

    if (msg == WM_NCHITTEST && prev) {
        WindowHost *self = it->first;
        if (self->m_topLevelMode) {
            const int mx = GET_X_LPARAM(lp);
            const int my = GET_Y_LPARAM(lp);
            const bool passThrough =
                self->m_config->boolValue(CfgKey::WindowClickThrough)
                || !self->hitTestContent(mx, my);
            if (passThrough)
                return HTTRANSPARENT;
        }
    }
    return prev ? CallWindowProcW(prev, hwnd, msg, wp, lp)
                : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK WindowHost::messageWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    WindowHost *self = reinterpret_cast<WindowHost *>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (self) {
        if (msg == self->m_taskbarRestartMsg || msg == WM_DISPLAYCHANGE) {
            QMetaObject::invokeMethod(self, "reattach", Qt::QueuedConnection);
            return 0;
        }
        if (msg == WM_MD_REATTACH_PRIVATE) {
            QMetaObject::invokeMethod(self, "reattach", Qt::QueuedConnection);
            return 0;
        }
    }
    if (self && self->m_prevMsgProc)
        return CallWindowProcW(self->m_prevMsgProc, hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}
