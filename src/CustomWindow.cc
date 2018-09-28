// CustomWindow
// Copyright (C) 2018 Citorva
//
// Based on Adaedra QDwm
// Copyright (C) 2011 Adaedra
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "CustomWindow.hh"

#ifdef Q_OS_WIN
    #include <QSysInfo>
    #include <QApplication>
    #include <QPainter>
    #include <QStyle>
    #include <QStylePainter>
    #include <QStyleOption>
    #include <QMouseEvent>
    #include <QScreen>
	#include <QLayout>

static const long ncHitZone[3][4] = {
	{HTTOPLEFT, HTLEFT, HTLEFT, HTBOTTOMLEFT},
	{HTTOP, HTCAPTION, HTNOWHERE, HTBOTTOM},
	{HTTOPRIGHT, HTRIGHT, HTRIGHT, HTBOTTOMRIGHT}
};

#endif

#ifdef __MINGW32__
    #define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif

namespace CustomWindow {

CustomWindow::CustomWindow(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags) {
#ifdef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground, true);

    mCanMove = false;
    mFrameRemoved = false;
    mMargins = QMargins(0, 0, 0, 0);
    setTitleBarSize(-1);
    setBorderSize(-1);
    mTitleBarHover = QStyle::SC_None;
    mTitleBarState = QStyle::State_None;
    mSizingMethod = contentSizing;
	mTransluentWindow = false;

    // Patch for Windows 10 (If not, the border size is 8px).
    if (QSysInfo::productVersion() == "10")
        setBorderSize(1);
#endif
}

CustomWindow::~CustomWindow()
{
}

#ifdef Q_OS_WIN

int CustomWindow::borderSize(void) const {
	if (mBorderSize < 0)
		return style()->pixelMetric(QStyle::PM_MDIFrameWidth);

	return (mBorderSize);
}

QRect CustomWindow::clientGeometry(void) const {
	return clientGeometry(mGeometryFlags);
}

QRect CustomWindow::clientGeometry(int flags) const {
	QMargins margins = (isAeroActivated() && (flags & CALCSIZE_USE_MARGIN)) ? extraMargins() : QMargins(0, 0, 0, 0);
	int border = (mFrameRemoved && (flags & CALCSIZE_USE_BORDER)) ? borderSize() : 0;
	int titlebar = (mFrameRemoved && (flags & CALCSIZE_USE_TITLEBAR)) ? titleBarSize() : 0;

	QRect ret(margins.left() + border, margins.top() + border + titlebar, width() - margins.left() - margins.right() - 2 * border,
		height() - margins.top() - margins.bottom() - 2 * border - titlebar);

	return ret;
}

void CustomWindow::disableTransluentBackground(void)
{
	mTransluentWindow = false;
	DisableWindowBlur();
}

void CustomWindow::declareCaption(const QWidget* widget)
{
	mCaptions.push_back(widget);
}

void CustomWindow::enableTransluentBackground(QColor color, qreal opacity)
{
	mBackgroundColor = color;
	mBlurBehindOpacity = opacity;
	mTransluentWindow = true;
	EnableWindowBlur();
}

QMargins CustomWindow::extraMargins(void) const {
	return mMargins;
}

int CustomWindow::geometryFlags(void) const {
	return mGeometryFlags;
}

bool CustomWindow::hasControls(int cx, int cy) {
	QStyleOptionTitleBar title;
	title.initFrom(this);
	title.titleBarFlags = windowFlags();
	title.rect.setRect(0, 0, width(), style()->pixelMetric(QStyle::PM_TitleBarHeight));

	if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarCloseButton, nullptr).contains(cx, cy))
		return true;
	if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarMaxButton, nullptr).contains(cx, cy))
		return true;
	if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarMinButton, nullptr).contains(cx, cy))
		return true;
	if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarNormalButton, nullptr).contains(cx, cy))
		return true;

	RECT rcWin;
	GetWindowRect(reinterpret_cast<HWND>(winId()), &rcWin);

	QWidget* w = QApplication::widgetAt(cx + rcWin.left, cy + rcWin.top);
	if (w != nullptr && w != this)
		return true;

	return false;
}

bool CustomWindow::hasTransluentBackground(void) const
{
	return mTransluentWindow;
}

bool CustomWindow::haveSystemMenu(void) const
{
	return GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE) & WS_SYSMENU;
}

void CustomWindow::hideSystemMenu(void)
{
	SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE, GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE) & ~WS_SYSMENU);
}

bool CustomWindow::isAeroActivated(void) {
	BOOL isDwmEnabled;
	HRESULT hr;

	if (QSysInfo::windowsVersion() < QSysInfo::WV_VISTA)
		return false;

	hr = DwmIsCompositionEnabled(&isDwmEnabled);
	if (hr < 0)
		return false;
	return isDwmEnabled == TRUE;
}

bool CustomWindow::isCaption(int cx, int cy) const
{
	for (const auto* widget : mCaptions) {
		QPoint wpos = widget->mapToGlobal(QPoint(0, 0)) - pos();
		int minX = wpos.x();
		int minY = wpos.y();
		int maxX = minX + widget->size().width();
		int maxY = minY + widget->size().height();

		if (minX <= cx && cx <= maxX && minY <= cy && cy <= maxY)
			return true;
	}
	return false;
}

bool CustomWindow::isFrameRemoved(void) const {
	return mFrameRemoved;
}

bool CustomWindow::isGeometryFlagsActivated(int flag) const {
	return mFrameRemoved && (mGeometryFlags & flag);
}

bool CustomWindow::isThemeActivated(void) {
	if (QSysInfo::windowsVersion() < QSysInfo::WV_XP)
		return (false);

	return IsThemeActive() == TRUE;
}

void CustomWindow::mouseMoveEvent(QMouseEvent* eve) {
	RECT realWin;
	GetWindowRect(reinterpret_cast<HWND>(winId()), &realWin);

	ncMouseMove(eve->globalX() - realWin.left,
		eve->globalY() - realWin.top);
}

void CustomWindow::mousePressEvent(QMouseEvent* eve) {
	RECT realWin;
	GetWindowRect(reinterpret_cast<HWND>(winId()), &realWin);

	ncMousePress(eve->globalX() - realWin.left,
		eve->globalY() - realWin.top);
}

void CustomWindow::mouseReleaseEvent(QMouseEvent* eve) {
	RECT realWin;
	GetWindowRect(reinterpret_cast<HWND>(winId()), &realWin);

	ncMouseRelease(eve->globalX() - realWin.left,
		eve->globalY() - realWin.top);
}

bool CustomWindow::nativeEvent(const QByteArray &eventType, void* message, long* result) {
    Q_UNUSED(eventType);
    MSG* wMsg = reinterpret_cast<MSG*>(message);
    UINT wMessage = wMsg->message;
    bool hasHandled = false;
    long res = 0;


	if (isAeroActivated()) {
		hasHandled = DwmDefWindowProc(wMsg->hwnd, wMsg->message,
			wMsg->wParam, wMsg->lParam, reinterpret_cast<LRESULT*>(&res));
	}

    if (wMessage == WM_NCCALCSIZE && wMsg->wParam == TRUE && mFrameRemoved) {
        hasHandled = true;
        res = 0;
    }

    if (wMessage == WM_NCHITTEST && res == 0 && mFrameRemoved) {
        if (!(res == HTCLOSE || res == HTMAXBUTTON || res == HTMINBUTTON || res == HTHELP))
            res = ncHitTest(wMsg);

        if (res != HTNOWHERE)
            hasHandled = true;
    }

    if (wMessage == WM_DWMCOMPOSITIONCHANGED
        || wMessage == WM_THEMECHANGED) {
        updateFrame();
        if (isAeroActivated()) {
            updateMargins();
        }

        if (wMessage == WM_DWMCOMPOSITIONCHANGED) {
            emit compositionChanged();
        }
        emit themeChanged();

        repaint();
    }

    if (wMessage == WM_NCPAINT && !isThemeActivated()) {
        hasHandled = true;
    }

    if (wMessage == WM_NCMOUSEMOVE) {
        int cx, cy;
        RECT realWin;

        GetWindowRect(wMsg->hwnd, &realWin);

        cx = GET_X_LPARAM(wMsg->lParam) - realWin.left;
        cy = GET_Y_LPARAM(wMsg->lParam) - realWin.top;

        ncMouseMove(cx, cy);
    }

    if (hasHandled)
        *result = res;
    return hasHandled;
}

long CustomWindow::ncHitTest(MSG* wMsg) {
	QPoint cur(GET_X_LPARAM(wMsg->lParam), GET_Y_LPARAM(wMsg->lParam));
	UINT8 xPos = 1;
	UINT8 yPos = 2;

	RECT rcWin;
	GetWindowRect(wMsg->hwnd, &rcWin);

	if (hasControls(cur.x() - rcWin.left, cur.y() - rcWin.top))
	{
		if (isCaption(cur.x() - rcWin.left, cur.y() - rcWin.top))
			return HTCAPTION;
		else
			return HTCLIENT;
	}

	RECT rcFrame = { 0 };
	AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

	if (cur.y() >= rcWin.top && cur.y() <= rcWin.top + borderSize())
		yPos = 0;
	else if (cur.y() >= rcWin.top + borderSize() && cur.y() <= rcWin.top + borderSize() + titleBarSize())
		yPos = 1;
	else if (cur.y() >= rcWin.bottom - borderSize() && cur.y() < rcWin.bottom)
		yPos = 3;

	if (cur.x() >= rcWin.left && cur.x() < rcWin.left + borderSize())
		xPos = 0;
	else if (cur.x() >= rcWin.right - borderSize() && cur.x() < rcWin.right)
		xPos = 2;

	return ncHitZone[xPos][yPos];
}

void CustomWindow::ncMouseMove(int cx, int cy) {
	if (!mFrameRemoved || isAeroActivated()) {
		mTitleBarHover = QStyle::SC_None;
		mTitleBarState = QStyle::State_None;
		return;
	}

	QStyleOptionTitleBar title;
	title.initFrom(this);
	title.titleBarFlags = windowFlags();
	if (isThemeActivated())
		title.rect.setRect(0, 0, width(), style()->pixelMetric(QStyle::PM_TitleBarHeight));
	else
		title.rect.setRect(borderSize(), borderSize(), width() - 2 * borderSize(),
			style()->pixelMetric(QStyle::PM_TitleBarHeight));

	if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarCloseButton, nullptr).contains(cx, cy)) {
		if (mTitleBarHover != QStyle::SC_TitleBarCloseButton) {
			mTitleBarHover = QStyle::SC_TitleBarCloseButton;
			mTitleBarState = QStyle::State_MouseOver;
			repaint();
		}
	}
	else if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarMaxButton, nullptr).contains(cx, cy) && !isMaximized()) {
		if (mTitleBarHover != QStyle::SC_TitleBarMaxButton) {
			mTitleBarHover = QStyle::SC_TitleBarMaxButton;
			mTitleBarState = QStyle::State_MouseOver;
			repaint();
		}
	}
	else if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarMinButton, nullptr).contains(cx, cy)) {
		if (mTitleBarHover != QStyle::SC_TitleBarMinButton) {
			mTitleBarHover = QStyle::SC_TitleBarMinButton;
			mTitleBarState = QStyle::State_MouseOver;
			repaint();
		}
	}
	else if (style()->subControlRect(QStyle::CC_TitleBar, &title, QStyle::SC_TitleBarMaxButton, nullptr).contains(cx, cy) && isMaximized()) {
		if (mTitleBarHover != QStyle::SC_TitleBarNormalButton) {
			mTitleBarHover = QStyle::SC_TitleBarNormalButton;
			mTitleBarState = QStyle::State_MouseOver;
			repaint();
		}
	}
	else {
		if (mTitleBarHover != QStyle::SC_None) {
			mTitleBarHover = QStyle::SC_None;
			mTitleBarState = QStyle::State_None;
			repaint();
		}
	}
}

void CustomWindow::ncMousePress(int, int) {
	if (mTitleBarHover != QStyle::SC_None) {
		mTitleBarState = QStyle::State_Sunken;
		repaint();
	}
}

void CustomWindow::ncMouseRelease(int, int) {
	if (mTitleBarState == QStyle::State_Sunken) {
		mTitleBarState = QStyle::State_MouseOver;
		repaint();

		if (mTitleBarHover == QStyle::SC_TitleBarCloseButton)
			close();
		else if (mTitleBarHover == QStyle::SC_TitleBarMaxButton)
			showMaximized();
		else if (mTitleBarHover == QStyle::SC_TitleBarMinButton)
			showMinimized();
		else if (mTitleBarHover == QStyle::SC_TitleBarNormalButton)
			showNormal();
	}
}

void CustomWindow::paintEvent(QPaintEvent*) {
	if (isAeroActivated()) {
		if (!(mTransluentWindow && mBlurBehindOpacity <= 0.0))
		{
			QPainter p(this);
			QRect r;
			
			if (mFrameRemoved) {
				r.setLeft(borderSize() + mMargins.left());
				r.setWidth(width() - (2 * borderSize() + mMargins.left() + mMargins.right()));
				r.setTop(borderSize() + titleBarSize() + mMargins.top());
				r.setHeight(height() - (2 * borderSize() + titleBarSize()
					+ mMargins.top() + mMargins.bottom()));
			}
			else {
				r.setLeft(mMargins.left());
				r.setWidth(width() - mMargins.left() - mMargins.right());
				r.setTop(mMargins.top());
				r.setHeight(height() - mMargins.top() - mMargins.bottom());
			}

			if (mMargins.top() != -1 || (mFrameRemoved && (borderSize() != 0 || titleBarSize() != 0)))
			{
				if (mTransluentWindow) {
					if (mBlurBehindOpacity > 1.0) mBlurBehindOpacity = 1.0;
					p.setOpacity(mBlurBehindOpacity);
					p.fillRect(r, mBackgroundColor);
				}
				else {
					p.setBrush(palette().window());
					p.setPen(Qt::NoPen);
				}
			}
		}
	}
	else {
		if (mFrameRemoved) {
			paintWinFrame();
		}
		else {
			QPainter p(this);

			p.setBrush(palette().window());
			p.setPen(Qt::NoPen);

			p.drawRect(0, 0, width(), height());
		}
	}
}

void CustomWindow::paintWinFrame(void) {
	QStylePainter p;
	p.begin(this);

	p.setPen(Qt::NoPen);
	p.setBrush(palette().window());
	p.drawRect(0, 0, width(), height());

	QPalette pal = palette();
	if (isActiveWindow())
		pal.setCurrentColorGroup(QPalette::Active);
	else
		pal.setCurrentColorGroup(QPalette::Inactive);

	QStyleOptionFrame frame;
	frame.initFrom(this);
	frame.lineWidth = borderSize();

	QStyleOptionTitleBar title;
	title.initFrom(this);
	if (isThemeActivated())
		title.rect.setRect(0, 0, width(), borderSize() + titleBarSize());
	else
		title.rect.setRect(borderSize(), borderSize(), width() - 2 * borderSize(),
			titleBarSize() + style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth));
	title.state = isActiveWindow() ? QStyle::State_Active : QStyle::State_None;
	title.palette = pal;
	title.titleBarState = title.state;
	if (isMaximized())
		title.titleBarState |= Qt::WindowMaximized;
	title.subControls = title.subControls & ~QStyle::SC_TitleBarSysMenu;
	title.activeSubControls = nullptr;

	p.drawPrimitive(QStyle::PE_FrameWindow, frame);
	p.drawComplexControl(QStyle::CC_TitleBar, title);

	title.subControls &= ~QStyle::SC_TitleBarLabel;
	title.titleBarFlags = windowFlags();
	title.activeSubControls = mTitleBarHover;
	title.state = mTitleBarState;
	if (isThemeActivated())
		title.rect.setRect(0, 0 - style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth), width(),
			style()->pixelMetric(QStyle::PM_TitleBarHeight) + style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth));
	else
		title.rect.setRect(borderSize(), borderSize(), width() - 2 * borderSize(),
			style()->pixelMetric(QStyle::PM_TitleBarHeight));

	p.drawComplexControl(QStyle::CC_TitleBar, title);

	p.end();
}

void CustomWindow::removeCaption(const QWidget* widget)
{
	for (int i = 0; i < mCaptions.size(); i++) {
		if (mCaptions[i] == widget) {
			mCaptions.erase(mCaptions.begin() + i);
		}
	}
}

void CustomWindow::resize(int w, int h, Sizing method)
{
	if (method == defaultSizing) method = mSizingMethod;
	int modifier = 0;
	if (method == contentSizing) modifier = 2 * mBorderSize;

	QWidget::resize(w + modifier, h + modifier);
}

void CustomWindow::resize(const QSize &size, Sizing method)
{
	resize(size.width(), size.height(), method);
}

void CustomWindow::setBorderSize(int size) {
	if (size < 0)
		mBorderSize = -1;
	else
		mBorderSize = size;

	if (mFrameRemoved) {
		updateFrame();
		updateMargins();
		updateLayoutMargins();
	}
}

void CustomWindow::setExtraMargins(const QMargins& margins) {
	mMargins = margins;
	updateMargins();
	updateLayoutMargins();
}

void CustomWindow::setFrameRemoved(bool isRemoved) {
	mFrameRemoved = isRemoved;
	updateFrame();
	updateMargins();
	updateLayoutMargins();
	setMouseTracking(isRemoved);
}

void CustomWindow::setGeometryFlags(int flags) {
	mGeometryFlags = flags;
}

void CustomWindow::setLayout(QLayout *layout)
{
	mLayoutMargins = layout->contentsMargins();
	QWidget::setLayout(layout);
}

void CustomWindow::setSizing(Sizing method)
{
	if (method != defaultSizing)
	{
		mSizingMethod = method;
	}
}

void CustomWindow::setSystemMenu(bool hide)
{
	if (hide)
		hideSystemMenu();
	else
		showSystemMenu();
}

void CustomWindow::setTitleBarSize(int size) {
	if (size < 0)
		mTitleBarSize = -1;
	else
		mTitleBarSize = size;

	if (mFrameRemoved) {
		updateFrame();
		updateMargins();
		updateLayoutMargins();
	}
}

void CustomWindow::showSystemMenu(void)
{
	SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE, GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE) | WS_SYSMENU);
}

QSize CustomWindow::size(Sizing method) const
{
	if (method == defaultSizing) method = mSizingMethod;
	int modifier = 0;
	if (method == contentSizing) modifier = 2 * mBorderSize;

	QSize size = QWidget::size();
	return QSize(size.width() - modifier, size.height() - modifier);
}

Sizing CustomWindow::sizing(void) const
{
	return mSizingMethod;
}

int CustomWindow::titleBarSize(void) const {
	if (mTitleBarSize < 0)
		// if (isThemeActivated()) {
		return style()->pixelMetric(QStyle::PM_TitleBarHeight)
		- style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);
	// } else {
	//	return style()->pixelMetric(QStyle::PM_TitleBarHeight);
	//}

	return (mTitleBarSize);
}

qreal CustomWindow::transluentBackgroundOpacity(void) const {
	return mBlurBehindOpacity;
}

void CustomWindow::updateFrame(HWND hWnd) {
    if (hWnd == nullptr)
        hWnd = reinterpret_cast<HWND>(winId());

    RECT rcClient;
    GetWindowRect(hWnd, &rcClient);

    SetWindowPos(hWnd, nullptr, rcClient.left, rcClient.top,
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top, SWP_FRAMECHANGED);
}

void CustomWindow::updateLayoutMargins(void)
{
	if (layout() == 0)
		return;
	layout()->setContentsMargins(clientGeometry().left() + mLayoutMargins.left(),
		clientGeometry().top() + mLayoutMargins.top(),
		(width() - clientGeometry().left() - clientGeometry().width()) + mLayoutMargins.right(),
		(height() - clientGeometry().top() - clientGeometry().height()) + mLayoutMargins.bottom());
}

void CustomWindow::updateMargins(HWND hWnd) {
	if (hWnd == nullptr)
		hWnd = reinterpret_cast<HWND>(winId());

	MARGINS mar;
	mar.cxLeftWidth = mMargins.left();
	mar.cxRightWidth = mMargins.right();
	mar.cyBottomHeight = mMargins.bottom();
	mar.cyTopHeight = mMargins.top();

	if (mFrameRemoved) {
		mar.cxLeftWidth += borderSize();
		mar.cxRightWidth += borderSize();
		mar.cyBottomHeight += borderSize();
		mar.cyTopHeight += titleBarSize() + borderSize();
	}

	DwmExtendFrameIntoClientArea(hWnd, &mar);
}

void CustomWindow::EnableWindowBlur(void)
{
	const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
	if (hModule)
	{
		struct ACCENTPOLICY
		{
			int nAccentState;
			int nFlags;
			int nColor;
			int nAnimationId;
		};

		struct WINCOMPATTRDATA
		{
			DWORD nAttribute;
			PVOID pData;
			ULONG ulDataSize;
		};
		
		typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
		const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = { 3, 0, 0, 0 }; // ACCENT_ENABLE_BLURBEHIND=3...
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // =19
			SetWindowCompositionAttribute(reinterpret_cast<HWND>(winId()), &data);
		}
		FreeLibrary(hModule);
	}
}

void CustomWindow::DisableWindowBlur(void)
{
	struct ACCENTPOLICY
	{
		int nAccentState;
		int nFlags;
		int nColor;
		int nAnimationId;
	};

	struct WINCOMPATTRDATA
	{
		DWORD nAttribute;
		PVOID pData;
		ULONG ulDataSize;
	};

	const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
	if (hModule)
	{
		typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
		const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = { 0, 0, 0, 0 }; // ACCENT_ENABLE_BLURBEHIND=3...
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // =19
			SetWindowCompositionAttribute(reinterpret_cast<HWND>(winId()), &data);
		}
		FreeLibrary(hModule);
	}
}

void CustomWindow::setResizable(bool resizable)
{
	if (!resizable)
		SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE, GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE) | WS_THICKFRAME | WS_OVERLAPPED);
	else
		SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE, GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_STYLE) & ~(WS_THICKFRAME | WS_OVERLAPPED));
}

#endif
    
}
