/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/web_window.h"

#include <DTitlebar>
#include <DThemeManager>
#include <DPlatformWindowHandle>

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QBuffer>
#include <QWebChannel>
#include <qcef_web_page.h>
#include <qcef_web_settings.h>
#include <qcef_web_view.h>
#include <qcef_global_settings.h>

#include "base/consts.h"
#include "services/search_manager.h"
#include "services/settings_manager.h"
#include "ui/web_event_delegate.h"
#include "ui/channel/image_viewer_proxy.h"
#include "ui/channel/log_proxy.h"
#include "ui/channel/menu_proxy.h"
#include "ui/channel/search_proxy.h"
#include "ui/channel/settings_proxy.h"
#include "ui/channel/store_daemon_proxy.h"
#include "ui/channel/account_proxy.h"
#include "ui/channel/channel_proxy.h"
#include "ui/widgets/image_viewer.h"
#include "ui/widgets/search_completion_window.h"
#include "ui/widgets/title_bar.h"
#include "ui/widgets/title_bar_menu.h"

namespace dstore
{

namespace
{

const int kSearchDelay = 200;

const char kSettingsWinSize[] = "size";
const char kSettingsWinPos[] = "pos";
const char kSettingsWinMax[] = "isMaximized";

void BackupWindowState(QWidget *widget)
{
    Q_ASSERT(widget != nullptr);

    QVariantMap windowState;
    windowState.insert(kSettingsWinPos, widget->pos());
    windowState.insert(kSettingsWinSize, widget->size());
    windowState.insert(kSettingsWinMax, widget->isMaximized());

    QByteArray data;
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << windowState;

    SettingsManager::instance()->setWindowState(data);
}

void RestoreWindowState(QWidget *widget)
{
    Q_ASSERT(widget != nullptr);
    QByteArray data = SettingsManager::instance()->getWindowState();
    QBuffer readBuffer(&data);
    readBuffer.open(QIODevice::ReadOnly);
    QDataStream in(&readBuffer);
    QVariantMap settings;
    in >> settings;

    if (settings.contains(kSettingsWinSize)) {
        widget->resize(settings.value(kSettingsWinSize).toSize());
    }
    if (settings.contains(kSettingsWinPos)) {
        widget->move(settings.value(kSettingsWinPos).toPoint());
    }
    if (settings.value(kSettingsWinMax, false).toBool()) {
        widget->showMaximized();
    }
}

}  // namespace

WebWindow::WebWindow(QWidget *parent)
    : DMainWindow(parent),
      search_timer_(new QTimer(this)),
      search_re_(QRegularExpression("[\\+\\$\\.\\^!@#%&\\(\\)]"))
{
    this->setObjectName("WebWindow");

    // 使用 redirectContent 模式，用于内嵌 x11 窗口时能有正确的圆角效果
    DPlatformWindowHandle::enableDXcbForWindow(this, true);

    search_timer_->setSingleShot(true);

    this->initUI();
    this->initServices();
    this->initProxy();

    // Connect signals to slots after all of internal objects are constructed.
    this->initConnections();

    // Restore window state on init.
    RestoreWindowState(this);
}

WebWindow::~WebWindow()
{
    // Save current window state.
    BackupWindowState(this);

    if (proxy_thread_ != nullptr) {
        proxy_thread_->quit();
        proxy_thread_->deleteLater();
        proxy_thread_ = nullptr;
    }
    if (image_viewer_proxy_ != nullptr) {
        image_viewer_proxy_->deleteLater();
        image_viewer_proxy_ = nullptr;
    }
    if (log_proxy_ != nullptr) {
        log_proxy_->deleteLater();
        log_proxy_ = nullptr;
    }
    if (menu_proxy_ != nullptr) {
        menu_proxy_->deleteLater();
        menu_proxy_ = nullptr;
    }
    if (settings_proxy_ != nullptr) {
        settings_proxy_->deleteLater();
        settings_proxy_ = nullptr;
    }
    if (store_daemon_proxy_ != nullptr) {
        store_daemon_proxy_->deleteLater();
        store_daemon_proxy_ = nullptr;
    }
    if (account_proxy_ != nullptr) {
        account_proxy_->deleteLater();
        account_proxy_ = nullptr;
    }
}

void WebWindow::setQCefSettings(QCefGlobalSettings *settings)
{
    SettingsManager::instance()->setQCefSettings(settings);
}

void WebWindow::loadPage()
{
    web_view_->load(QUrl(kIndexPage));
}

void WebWindow::showWindow()
{
    const QRect geometry = qApp->desktop()->availableGeometry(this);
    if (geometry.width() > 1366) {
        this->setMinimumSize(1208, 778);
    } else {
        this->setMinimumSize(960, 716);
    }
    this->show();
}

void WebWindow::showAppDetail(const QString &app_name)
{
    // TODO(Shaohua): Make sure angular context has been initialized.
    emit search_proxy_->openApp(app_name);
}

void WebWindow::raiseWindow()
{
    this->raise();
}

void WebWindow::initConnections()
{
    connect(completion_window_, &SearchCompletionWindow::resultClicked,
            this, &WebWindow::onSearchResultClicked);
    connect(completion_window_, &SearchCompletionWindow::searchButtonClicked,
            this, &WebWindow::onSearchButtonClicked);

    connect(image_viewer_proxy_, &ImageViewerProxy::openImageFileRequested,
            image_viewer_, &ImageViewer::open);
    connect(image_viewer_proxy_, &ImageViewerProxy::openPixmapRequested,
            image_viewer_, &ImageViewer::openPixmap);
    connect(image_viewer_proxy_, &ImageViewerProxy::openOnlineImageRequest,
            image_viewer_, &ImageViewer::showIndicator);
    connect(image_viewer_, &ImageViewer::previousImageRequested,
            image_viewer_proxy_, &ImageViewerProxy::onPreviousImageRequested);
    connect(image_viewer_, &ImageViewer::nextImageRequested,
            image_viewer_proxy_, &ImageViewerProxy::onNextImageRequested);

    connect(search_manager_, &SearchManager::searchAppResult,
            this, &WebWindow::onSearchAppResult);
    connect(search_manager_, &SearchManager::completeSearchAppResult,
            this, &WebWindow::onCompleteSearchAppResult);

    connect(search_proxy_, &SearchProxy::onAppListUpdated,
            search_manager_, &SearchManager::updateAppList);
    connect(search_proxy_, &SearchProxy::onAppListUpdated,
            store_daemon_proxy_, &StoreDaemonProxy::updateAppList);

    connect(search_timer_, &QTimer::timeout,
            this, &WebWindow::onSearchTextChangedDelay);

    connect(title_bar_, &TitleBar::backwardButtonClicked,
            this, &WebWindow::webViewGoBack);
    connect(title_bar_, &TitleBar::forwardButtonClicked,
            this, &WebWindow::webViewGoForward);
    connect(title_bar_, &TitleBar::searchTextChanged,
            this, &WebWindow::onSearchTextChanged);
    connect(title_bar_, &TitleBar::downKeyPressed,
            completion_window_, &SearchCompletionWindow::goDown);
    connect(title_bar_, &TitleBar::enterPressed,
            this, &WebWindow::onTitleBarEntered);
    connect(title_bar_, &TitleBar::upKeyPressed,
            completion_window_, &SearchCompletionWindow::goUp);
    connect(title_bar_, &TitleBar::focusOut,
            this, &WebWindow::onSearchEditFocusOut);
    connect(title_bar_, &TitleBar::loginRequested,
    this, [&](bool login) {
        if (login) {
            account_proxy_->login();
        } else {
            account_proxy_->logout();
        }
    });

    connect(tool_bar_menu_, &TitleBarMenu::recommendAppRequested,
            menu_proxy_, &MenuProxy::recommendAppRequested);
    connect(tool_bar_menu_, &TitleBarMenu::privacyAgreementRequested,
            menu_proxy_, &MenuProxy::privacyAgreementRequested);
    connect(tool_bar_menu_, &TitleBarMenu::switchThemeRequested,
            menu_proxy_, &MenuProxy::switchThemeRequested);
    connect(tool_bar_menu_, &TitleBarMenu::switchThemeRequested,
            this, &WebWindow::onThemeChaged);
    connect(tool_bar_menu_, &TitleBarMenu::regionChanged,
            this, &WebWindow::onRegionChanged);
    connect(tool_bar_menu_, &TitleBarMenu::clearCacheRequested,
            store_daemon_proxy_, &StoreDaemonProxy::clearArchives);


    connect(title_bar_, &TitleBar::commentRequested,
            menu_proxy_, &MenuProxy::commentRequested);
    connect(title_bar_, &TitleBar::requestDonates,
            menu_proxy_, &MenuProxy::donateRequested);
    connect(title_bar_, &TitleBar::requestApps,
            menu_proxy_, &MenuProxy::appsRequested);
    connect(menu_proxy_, &MenuProxy::userInfoUpdated,
            title_bar_, &TitleBar::setUserInfo);

    connect(web_view_->page(), &QCefWebPage::urlChanged,
            this, &WebWindow::onWebViewUrlChanged);

    connect(web_view_->page(), &QCefWebPage::fullscreenRequested,
            this, &WebWindow::onFullscreenRequest);

    connect(web_view_->page(), &QCefWebPage::loadingStateChanged,
            this, &WebWindow::onLoadingStateChanged);

    connect(settings_proxy_, &SettingsProxy::raiseWindowRequested,
            this, &WebWindow::raiseWindow);
}

void WebWindow::initProxy()
{
#ifdef DSTORE_DISABLE_MULTI_THREAD
    bool useMultiThread = false;
#else
    bool useMultiThread = true;
#endif

    auto parent = this;
    if (useMultiThread) {
        parent = nullptr;
    }
    auto page_channel = web_view_->page()->webChannel();
    auto channel_proxy = new ChannelProxy(this);
    page_channel->registerObject("channelProxy", channel_proxy);

    auto web_channel = new QWebChannel(parent);
    web_channel->connectTo(channel_proxy->transport);
    store_daemon_proxy_ = new StoreDaemonProxy(parent);
    image_viewer_proxy_ = new ImageViewerProxy(parent);
    log_proxy_ = new LogProxy(parent);
    menu_proxy_ = new MenuProxy(parent);
    search_proxy_ = new SearchProxy(parent);
    settings_proxy_ = new SettingsProxy(parent);
    account_proxy_ = new AccountProxy(parent);

    web_channel->registerObject("imageViewer", image_viewer_proxy_);
    web_channel->registerObject("log", log_proxy_);
    web_channel->registerObject("menu", menu_proxy_);
    web_channel->registerObject("search", search_proxy_);
    web_channel->registerObject("settings", settings_proxy_);
    web_channel->registerObject("storeDaemon", store_daemon_proxy_);
    web_channel->registerObject("account", account_proxy_);

    if (useMultiThread) {
        proxy_thread_ = new QThread(parent);
        web_channel->moveToThread(proxy_thread_);
        image_viewer_proxy_->moveToThread(proxy_thread_);
        log_proxy_->moveToThread(proxy_thread_);
        menu_proxy_->moveToThread(proxy_thread_);
        search_proxy_->moveToThread(proxy_thread_);
        settings_proxy_->moveToThread(proxy_thread_);
        store_daemon_proxy_->moveToThread(proxy_thread_);
        account_proxy_->moveToThread(proxy_thread_);
        proxy_thread_->start();
    }
}

void WebWindow::initUI()
{
    Dtk::Widget::DThemeManager::instance()->registerWidget(this);

    web_view_ = new QCefWebView();
    this->setCentralWidget(web_view_);

    image_viewer_ = new ImageViewer(this);

    completion_window_ = new SearchCompletionWindow();
    completion_window_->hide();

    title_bar_ = new TitleBar(SettingsManager::instance()->supportSignIn());
    this->titlebar()->setCustomWidget(title_bar_, Qt::AlignCenter, false);
    this->titlebar()->setSeparatorVisible(true);
    tool_bar_menu_ = new TitleBarMenu(SettingsManager::instance()->supportSignIn(), this);
    this->titlebar()->setMenu(tool_bar_menu_);

    // Disable web security.
    auto settings = web_view_->page()->settings();
    settings->setMinimumFontSize(8);
    settings->setWebSecurity(QCefWebSettings::StateDisabled);

    // init default font size
    settings->setDefaultFontSize(this->fontInfo().pixelSize());

    web_event_delegate_ = new WebEventDelegate(this);
    web_view_->page()->setEventDelegate(web_event_delegate_);

    this->setFocusPolicy(Qt::ClickFocus);

    Dtk::Widget::DThemeManager::instance()->registerWidget(this->titlebar(), "DTitlebar");
}

void WebWindow::initServices()
{
    search_manager_ = new SearchManager(this);
}

bool WebWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Filters mouse press event only.
    if (event->type() == QEvent::MouseButtonPress &&
            qApp->activeWindow() == this &&
            watched->objectName() == QLatin1String("QMainWindowClassWindow")) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        switch (mouseEvent->button()) {
        case Qt::BackButton: {
            this->webViewGoBack();
            break;
        }
        case Qt::ForwardButton: {
            this->webViewGoForward();
            break;
        }
        default: {
        }
        }
    }

    if (event->type() == QEvent::FontChange && watched == this) {
        if (this->settings_proxy_) {
            auto fontInfo = this->fontInfo();
            Q_EMIT this->settings_proxy_->fontChangeRequested(fontInfo.family(), fontInfo.pixelSize());
        }
    }

    return QObject::eventFilter(watched, event);
}

void WebWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    title_bar_->setFixedWidth(event->size().width());
}

void WebWindow::focusInEvent(QFocusEvent *event)
{
    DMainWindow::focusInEvent(event);

    web_view_->setFocus();
}

void WebWindow::onSearchAppResult(const QString &keyword,
                                  const SearchMetaList &result)
{
    Q_UNUSED(keyword);
    completion_window_->setSearchResult(result);

    if (result.isEmpty()) {
        // Hide completion window if no anchor entry matches.
        completion_window_->hide();
    } else {
        completion_window_->show();
        completion_window_->raise();
        completion_window_->autoResize();
        // Move to below of search edit.
        const QPoint local_point(this->rect().width() / 2 - 94, 36);
        const QPoint global_point(this->mapToGlobal(local_point));
        completion_window_->move(global_point);
        completion_window_->setFocusPolicy(Qt::NoFocus);
        completion_window_->setFocusPolicy(Qt::StrongFocus);
    }
}

void WebWindow::onCompleteSearchAppResult(const QString &keyword,
        const SearchMetaList &result)
{
    Q_UNUSED(keyword);

    // Show search page in web.
    QStringList names;
    for (const SearchMeta &app : result) {
        names.append(app.name);
    }
    emit search_proxy_->openAppList(completion_window_->getKeyword(), names);
    completion_window_->hide();
}

void WebWindow::onSearchEditFocusOut()
{
    QTimer::singleShot(20, [ = ]() {
        this->completion_window_->hide();
    });
}

void WebWindow::onSearchButtonClicked()
{
    this->prepareSearch(true);
}

void WebWindow::onSearchTextChangedDelay()
{
    this->prepareSearch(false);
}

void WebWindow::prepareSearch(bool entered)
{
    const QString text = title_bar_->getSearchText();
    // Filters special chars.
    if (text.size() <= 1 || text.contains(search_re_)) {
        qCritical() << "Invalid regexp:" << text;
        return;
    }

    completion_window_->setKeyword(text);

    // Do real search.
    if (entered) {
        search_manager_->completeSearchApp(text);
    } else {
        search_manager_->searchApp(text);
    }
}

void WebWindow::onSearchResultClicked(const SearchMeta &result)
{
    // Emit signal to web page.
    emit search_proxy_->openApp(result.name);
}

void WebWindow::onSearchTextChanged(const QString &text)
{
    if (text.size() > 1) {
        search_timer_->stop();
        search_timer_->start(kSearchDelay);
    } else {
        this->onSearchEditFocusOut();
    }
}

void WebWindow::onTitleBarEntered()
{
    const QString text = title_bar_->getSearchText();
    if (text.size() > 1) {
        completion_window_->onEnterPressed();
    }
}

void WebWindow::onThemeChaged(const QString theme_name)
{
    Dtk::Widget::DThemeManager::instance()->setTheme(theme_name);
}

void WebWindow::onWebViewUrlChanged(const QUrl &url)
{
    Q_UNUSED(url);
}

void WebWindow::onLoadingStateChanged(bool,
                                      bool can_go_back,
                                      bool can_go_forward)
{
    title_bar_->setBackwardButtonActive(can_go_back);
    title_bar_->setForwardButtonActive(can_go_forward);
}

void WebWindow::onRegionChanged()
{
    this->loadPage();
}

void WebWindow::webViewGoBack()
{
    auto page = web_view_->page();
    if (page->canGoBack()) {
        page->back();
    }
}

void WebWindow::webViewGoForward()
{
    auto page = web_view_->page();
    if (page->canGoForward()) {
        page->forward();
    }
}
void WebWindow::onFullscreenRequest(bool fullscreen) {
    if (fullscreen) {
        this->titlebar()->hide();
        this->showFullScreen();
    } else {
        this->titlebar()->show();
        this->showNormal();
    }
}

}  // namespace dstore
