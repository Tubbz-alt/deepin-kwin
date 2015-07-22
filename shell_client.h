/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2015 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef KWIN_SHELL_CLIENT_H
#define KWIN_SHELL_CLIENT_H

#include "abstract_client.h"

namespace KWayland
{
namespace Server
{
class ShellSurfaceInterface;
class PlasmaShellSurfaceInterface;
class QtExtendedSurfaceInterface;
}
}

namespace KWin
{

class ShellClient : public AbstractClient
{
    Q_OBJECT
public:
    ShellClient(KWayland::Server::ShellSurfaceInterface *surface);
    virtual ~ShellClient();

    QStringList activities() const override;
    QPoint clientPos() const override;
    QSize clientSize() const override;
    Layer layer() const override;
    QRect transparentRect() const override;
    bool shouldUnredirect() const override;
    NET::WindowType windowType(bool direct = false, int supported_types = 0) const override;
    void debug(QDebug &stream) const override;
    double opacity() const override;
    void setOpacity(double opacity) override;
    QByteArray windowRole() const override;

    KWayland::Server::ShellSurfaceInterface *shellSurface() const {
        return m_shellSurface;
    }

    void blockActivityUpdates(bool b = true) override;
    QString caption(bool full = true, bool stripped = false) const override;
    void checkWorkspacePosition(QRect oldGeometry = QRect(), int oldDesktop = -2, QRect oldClientGeometry = QRect()) override;
    void closeWindow() override;
    AbstractClient *findModal(bool allow_itself = false) override;
    bool isCloseable() const override;
    bool isFullScreenable() const override;
    bool isFullScreen() const override;
    bool isMaximizable() const override;
    bool isMinimizable() const override;
    bool isMovable() const override;
    bool isMovableAcrossScreens() const override;
    bool isResizable() const override;
    bool isShown(bool shaded_is_shown) const override;
    void hideClient(bool hide) override;
    void maximize(MaximizeMode) override;
    MaximizeMode maximizeMode() const override;
    bool noBorder() const override;
    const WindowRules *rules() const override;
    void sendToScreen(int screen) override;
    void setFullScreen(bool set, bool user = true) override;
    void setNoBorder(bool set) override;
    void setOnAllActivities(bool set) override;
    void setQuickTileMode(QuickTileMode mode, bool keyboard = false) override;
    void setShortcut(const QString &cut) override;
    const QKeySequence &shortcut() const override;
    void takeFocus() override;
    void updateWindowRules(Rules::Types selection) override;
    bool userCanSetFullScreen() const override;
    bool userCanSetNoBorder() const override;
    bool wantsInput() const override;
    xcb_window_t window() const override;
    using AbstractClient::move;
    void move(int x, int y, ForceGeometry_t force = NormalGeometrySet) override;
    using AbstractClient::resizeWithChecks;
    void resizeWithChecks(int w, int h, ForceGeometry_t force = NormalGeometrySet) override;
    bool hasStrut() const override;

    quint32 windowId() const {
        return m_windowId;
    }
    bool isInternal() const;
    QWindow *internalWindow() const {
        return m_internalWindow;
    }

    void installPlasmaShellSurface(KWayland::Server::PlasmaShellSurfaceInterface *surface);
    void installQtExtendedSurface(KWayland::Server::QtExtendedSurfaceInterface *surface);

    bool isInitialPositionSet() const;

protected:
    void addDamage(const QRegion &damage) override;
    bool belongsToSameApplication(const AbstractClient *other, bool active_hack) const override;
    void doSetActive() override;

private Q_SLOTS:
    void clientFullScreenChanged(bool fullScreen);

private:
    void requestGeometry(const QRect &rect);
    void setGeometry(const QRect &rect);
    void destroyClient();
    void unmap();
    void createWindowId();
    void findInternalWindow();
    void updateInternalWindowGeometry();
    void updateIcon();
    void markAsMapped();
    static void deleteClient(ShellClient *c);

    KWayland::Server::ShellSurfaceInterface *m_shellSurface;
    QSize m_clientSize;

    ClearablePoint m_positionAfterResize; // co-ordinates saved from a requestGeometry call, real geometry will be updated after the next damage event when the client has resized
    QRect m_geomFsRestore; //size and position of the window before it was set to fullscreen
    bool m_closing = false;
    quint32 m_windowId = 0;
    QWindow *m_internalWindow = nullptr;
    bool m_unmapped = true;
    MaximizeMode m_maximizeMode = MaximizeRestore;
    QRect m_geomMaximizeRestore; // size and position of the window before it was set to maximize
    NET::WindowType m_windowType = NET::Normal;
    QPointer<KWayland::Server::PlasmaShellSurfaceInterface> m_plasmaShellSurface;
    QPointer<KWayland::Server::QtExtendedSurfaceInterface> m_qtExtendedSurface;
};

}

#endif