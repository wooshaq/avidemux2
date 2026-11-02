/***************************************************************************
    Avidemux WsQ: modern dark theme, collapsible codec panel with icon rail,
    wrapping navigation bar and remembered window geometry.
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QStyleFactory>
#include <QToolBar>
#include <QToolButton>
#include <QResizeEvent>
#include <QTimer>

#include "Q_gui2.h"
#include "Q_gui2_wsq.h"
#include "ADM_QSettings.h"

#define WSQ_ICON(x) QIcon(":/new/prefix1/pics/wsq_" #x ".png")
#define PL(x) QString::fromUtf8(x)

/* ----------------------------------------------------------------------
 *  WsqFlowLayout: lays out items left to right and wraps them to the next
 *  row when there is not enough width (after the Qt "Flow Layout" example).
 * ---------------------------------------------------------------------- */
WsqFlowLayout::WsqFlowLayout(QWidget *parent, int hSpacing, int vSpacing)
    : QLayout(parent), hSpace(hSpacing), vSpace(vSpacing)
{
    setContentsMargins(0, 0, 0, 0);
}

WsqFlowLayout::~WsqFlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void WsqFlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

int WsqFlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *WsqFlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *WsqFlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    return nullptr;
}

Qt::Orientations WsqFlowLayout::expandingDirections() const
{
    return Qt::Orientations();
}

bool WsqFlowLayout::hasHeightForWidth() const
{
    return true;
}

int WsqFlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

void WsqFlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize WsqFlowLayout::sizeHint() const
{
    // Preferred size: everything on a single row.
    int w = 0, h = 0;
    for (QLayoutItem *item : itemList)
    {
        if (item->isEmpty())
            continue;
        QSize s = item->sizeHint();
        if (w)
            w += hSpace;
        w += s.width();
        h = qMax(h, s.height());
    }
    const QMargins m = contentsMargins();
    return QSize(w + m.left() + m.right(), h + m.top() + m.bottom());
}

QSize WsqFlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem *item : itemList)
        if (!item->isEmpty())
            size = size.expandedTo(item->minimumSize());
    const QMargins m = contentsMargins();
    size += QSize(m.left() + m.right(), m.top() + m.bottom());
    return size;
}

int WsqFlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QMargins m = contentsMargins();
    QRect effective = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;

    // First pass: split into rows, so that each row can be vertically centered.
    QList<QList<QLayoutItem *>> rows;
    QList<QLayoutItem *> row;
    for (QLayoutItem *item : itemList)
    {
        if (item->isEmpty())
            continue;
        int nextX = x + item->sizeHint().width() + hSpace;
        if (nextX - hSpace > effective.right() + 1 && !row.isEmpty())
        {
            rows.append(row);
            row.clear();
            x = effective.x();
            nextX = x + item->sizeHint().width() + hSpace;
        }
        row.append(item);
        x = nextX;
    }
    if (!row.isEmpty())
        rows.append(row);

    for (const QList<QLayoutItem *> &r : rows)
    {
        lineHeight = 0;
        for (QLayoutItem *item : r)
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        x = effective.x();
        for (QLayoutItem *item : r)
        {
            QSize s = item->sizeHint();
            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y + (lineHeight - s.height()) / 2), s));
            x += s.width() + hSpace;
        }
        y += lineHeight + vSpace;
    }
    if (!rows.isEmpty())
        y -= vSpace;
    return y - rect.y() + m.bottom();
}

/* ----------------------------------------------------------------------
 *  Helper object: keeps the navigation dock height in sync with the
 *  wrapping layout and mirrors push button enabled state onto rail actions.
 * ---------------------------------------------------------------------- */
bool WsqHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize && watched == navContents)
    {
        if (!pending)
        {
            pending = true;
            QTimer::singleShot(0, this, [this]() { updateNavHeight(); });
        }
    }
    else if (event->type() == QEvent::EnabledChange)
    {
        for (auto &p : mirrors)
            if (p.first == watched)
                p.second->setEnabled(static_cast<QWidget *>(watched)->isEnabled());
    }
    return QObject::eventFilter(watched, event);
}

void WsqHelper::updateNavHeight(void)
{
    pending = false;
    if (!navDock || !navContents || !navContents->layout())
        return;
    QLayout *l = navContents->layout();
    int w = navContents->width();
    if (w <= 0)
        return;
    int h = l->hasHeightForWidth() ? l->heightForWidth(w) : l->sizeHint().height();
    int extra = navDock->height() - navContents->height();
    if (extra < 0)
        extra = 0;
    int target = h + extra;
    if (navDock->minimumHeight() != target || navDock->maximumHeight() != target)
        navDock->setFixedHeight(target);
}

/* ----------------------------------------------------------------------
 *  Stylesheet
 * ---------------------------------------------------------------------- */
static const char *wsqStyleSheet = R"QSS(
QMainWindow, QDialog { background: #1b1e24; }
QWidget { color: #d6dbe3; }
QMainWindow::separator { background: #262b33; width: 1px; height: 1px; }
QWidget#centralwidget { background: #121418; }
QWidget#dockWidgetContents, QWidget#dockWidgetContents_2, QWidget#dockWidgetContents_3,
QWidget#dockWidgetContents_5, QWidget#dockWidgetContents_7 { background: #1f232a; }

QMenuBar { background: #16191e; color: #aeb6c3; border-bottom: 1px solid #262b33; padding: 2px 4px; }
QMenuBar::item { background: transparent; padding: 4px 10px; border-radius: 4px; }
QMenuBar::item:selected { background: #2c323b; color: #ffffff; }
QMenuBar::item:pressed { background: #1e3557; color: #ffffff; }
QMenu { background: #22262e; border: 1px solid #333a44; padding: 4px; }
QMenu::item { padding: 5px 28px 5px 26px; border-radius: 4px; color: #d6dbe3; }
QMenu::item:selected { background: #1e3557; color: #ffffff; }
QMenu::item:disabled { color: #5d6573; }
QMenu::separator { height: 1px; background: #333a44; margin: 4px 8px; }

QToolBar { background: #1b1e24; border: none; border-bottom: 1px solid #262b33; spacing: 2px; padding: 3px 6px; }
QToolBar#wsqRail { background: #1f232a; border: none; border-right: 1px solid #262b33; padding: 6px 4px; spacing: 4px; }
QToolBar::separator { background: #2e343d; width: 1px; height: 1px; margin: 4px 6px; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 3px; }
QToolButton:hover { background: #2c323b; border-color: #3a414c; }
QToolButton:pressed, QToolButton:checked { background: #1e3557; border-color: #2b4f80; }
QToolButton:disabled { background: transparent; border-color: transparent; }
QToolButton#toolButtonPlay { background: #24344d; border-color: #2b4f80; }
QToolButton#toolButtonPlay:hover { background: #2b4f80; }

QWidget#wsqNavGroup { background: #16191e; border: 1px solid #2a2f37; border-radius: 8px; }
QWidget#wsqNavPlain { background: transparent; }

QPushButton { background: #2c323b; border: 1px solid #3a414c; border-radius: 6px; padding: 4px 12px; min-height: 18px; color: #d6dbe3; }
QPushButton:hover { background: #343b45; border-color: #4a525e; }
QPushButton:pressed { background: #1e3557; border-color: #2b4f80; }
QPushButton:disabled { color: #5d6573; background: #23272e; border-color: #2c323b; }
QPushButton:default { border-color: #4a9bff; }
QPushButton#wsqSaveButton { background: #4a9bff; border: none; color: #0e1420; font-weight: bold; min-height: 30px; border-radius: 8px; }
QPushButton#wsqSaveButton:hover { background: #6aafff; }
QPushButton#wsqSaveButton:disabled { background: #2c323b; color: #5d6573; }
QPushButton#pushButtonJumpToMarkerA, QPushButton#pushButtonJumpToMarkerB { background: #1b2638; border-color: #2b4466; color: #9cc2ff; }
QPushButton#pushButtonJumpToMarkerA:hover, QPushButton#pushButtonJumpToMarkerB:hover { background: #22324a; }

QLabel#label_3, QLabel#label_4, QLabel#label_5, QLabel#wsqPanelTitle { color: #8b94a3; font-weight: bold; }
QLabel#TrackCountLabel { color: #8b94a3; }

QLineEdit, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit, QDateTimeEdit, QTimeEdit {
    background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; padding: 3px 6px;
    selection-background-color: #2a5aa0; selection-color: #ffffff; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QTextEdit:focus, QPlainTextEdit:focus { border-color: #4a9bff; }
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled { color: #5d6573; background: #1f232a; }
QLineEdit#currentTime { background: #2a1f24; border: 1px solid #4a2c33; color: #ff8a8a; padding: 1px 4px; }
QSpinBox::up-button, QDoubleSpinBox::up-button, QTimeEdit::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 16px; border: none; border-top-right-radius: 6px; }
QSpinBox::down-button, QDoubleSpinBox::down-button, QTimeEdit::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 16px; border: none; border-bottom-right-radius: 6px; }
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: #2c323b; }
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow, QTimeEdit::up-arrow { image: url(:/new/prefix1/pics/wsq_arrow_up.png); width: 8px; height: 6px; }
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow, QTimeEdit::down-arrow { image: url(:/new/prefix1/pics/wsq_arrow_down.png); width: 8px; height: 6px; }

QComboBox { background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; padding: 3px 8px; min-height: 18px; }
QComboBox:hover { border-color: #4a525e; }
QComboBox:focus { border-color: #4a9bff; }
QComboBox:disabled { color: #5d6573; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow { image: url(:/new/prefix1/pics/wsq_arrow_down.png); width: 10px; height: 7px; }
QComboBox QAbstractItemView { background: #22262e; border: 1px solid #333a44; selection-background-color: #1e3557; selection-color: #ffffff; outline: 0; padding: 2px; }

QCheckBox, QRadioButton { spacing: 7px; }
QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #4a525e; border-radius: 3px; background: #1b1e24; }
QCheckBox::indicator:hover { border-color: #4a9bff; }
QCheckBox::indicator:checked { background: #4a9bff; border-color: #4a9bff; image: url(:/new/prefix1/pics/wsq_check.png); }
QCheckBox::indicator:disabled { background: #23272e; border-color: #2c323b; }
QRadioButton::indicator { width: 14px; height: 14px; border: 1px solid #4a525e; border-radius: 8px; background: #1b1e24; }
QRadioButton::indicator:checked { background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #4a9bff, stop:0.45 #4a9bff, stop:0.55 #1b1e24, stop:1 #1b1e24); border-color: #4a9bff; }

QGroupBox { border: 1px solid #333a44; border-radius: 8px; margin-top: 14px; padding-top: 8px; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #8b94a3; }

QTabWidget::pane { border: 1px solid #333a44; border-radius: 6px; top: -1px; background: #1f232a; }
QTabBar::tab { background: #22262e; border: 1px solid #333a44; padding: 5px 12px; margin-right: 2px; color: #aeb6c3;
               border-top-left-radius: 6px; border-top-right-radius: 6px; }
QTabBar::tab:selected { background: #2c323b; color: #ffffff; border-bottom-color: #2c323b; }
QTabBar::tab:hover:!selected { background: #2a2f37; }

QListView, QTreeView, QTableView, QListWidget, QTreeWidget, QTableWidget {
    background: #1b1e24; border: 1px solid #333a44; border-radius: 6px; alternate-background-color: #20242b;
    selection-background-color: #1e3557; selection-color: #ffffff; outline: 0; }
QHeaderView::section { background: #22262e; color: #aeb6c3; border: none; border-right: 1px solid #333a44; border-bottom: 1px solid #333a44; padding: 4px 8px; }

QScrollBar:vertical { background: transparent; width: 11px; margin: 0; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }
QScrollBar::handle:vertical { background: #3a414c; border-radius: 4px; min-height: 28px; margin: 2px; }
QScrollBar::handle:horizontal { background: #3a414c; border-radius: 4px; min-width: 28px; margin: 2px; }
QScrollBar::handle:hover { background: #4a525e; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QSlider#horizontalSlider_2::groove:vertical { width: 4px; background: #2c323b; border-radius: 2px; }
QSlider#horizontalSlider_2::handle:vertical { background: #4a9bff; height: 12px; margin: 0 -5px; border-radius: 6px; }
QSlider#horizontalSlider_2::add-page:vertical { background: #4a9bff; border-radius: 2px; }
QSlider#horizontalSlider_2::groove:horizontal { height: 4px; background: #2c323b; border-radius: 2px; }
QSlider#horizontalSlider_2::handle:horizontal { background: #4a9bff; width: 12px; margin: -5px 0; border-radius: 6px; }
QSlider#horizontalSlider_2::sub-page:horizontal { background: #4a9bff; border-radius: 2px; }

QProgressBar { background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; text-align: center; min-height: 16px; }
QProgressBar::chunk { background: #4a9bff; border-radius: 5px; }
QToolTip { background: #22262e; color: #d6dbe3; border: 1px solid #3a414c; padding: 4px; }
QStatusBar { background: #16191e; color: #8b94a3; border-top: 1px solid #262b33; }
QStatusBar QLabel { color: #8b94a3; }
QStatusBar::item { border: none; }
)QSS";

/* ----------------------------------------------------------------------
 *  MainWindow additions
 * ---------------------------------------------------------------------- */

static QWidget *wsqGroup(const QList<QWidget *> &widgets, bool framed)
{
    QWidget *g = new QWidget();
    g->setObjectName(framed ? "wsqNavGroup" : "wsqNavPlain");
    g->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *h = new QHBoxLayout(g);
    h->setContentsMargins(framed ? 3 : 0, framed ? 2 : 0, framed ? 3 : 0, framed ? 2 : 0);
    h->setSpacing(framed ? 1 : 6);
    for (QWidget *w : widgets)
        if (w)
            h->addWidget(w);
    return g;
}

static void wsqClearLayout(QLayout *l)
{
    if (!l)
        return;
    QLayoutItem *item;
    while ((item = l->takeAt(0)))
    {
        if (item->layout())
        {
            wsqClearLayout(item->layout());
            delete item->layout();
            continue;
        }
        delete item; // widgets were already moved elsewhere, spacers die here
    }
}

/**
    \fn wsqSetup
    \brief Build the WsQ UI additions. Called once from the constructor.
*/
void MainWindow::wsqSetup(void)
{
    wsqHelper = new WsqHelper(this);

    /* --- Navigation bar: grouped buttons in a wrapping layout --- */
    QWidget *flowHost = new QWidget(ui.dockWidgetContents_2);
    flowHost->setObjectName("wsqNavFlow");
    WsqFlowLayout *flow = new WsqFlowLayout(flowHost, 8, 6);
    flow->setContentsMargins(0, 6, 0, 0);

    flow->addWidget(wsqGroup({ui.toolButtonPlay, ui.toolButtonPreviousFrame, ui.toolButtonNextFrame,
                              ui.toolButtonPreviousIntraFrame, ui.toolButtonNextIntraFrame},
                             true));
    flow->addWidget(wsqGroup({ui.toolButtonSetMarkerA, ui.toolButtonDeleteSelection, ui.toolButtonSetMarkerB}, true));
    flow->addWidget(wsqGroup({ui.toolButtonPreviousCutPoint, ui.toolButtonNextCutPoint,
                              ui.toolButtonPreviousBlackFrame, ui.toolButtonNextBlackFrame},
                             true));
    flow->addWidget(wsqGroup({ui.toolButtonFirstFrame, ui.toolButtonLastFrame, ui.toolButtonBackOneMinute,
                              ui.toolButtonForwardOneMinute},
                             true));
    flow->addWidget(wsqGroup({ui.pushButtonTime, ui.currentTime}, false));
    flow->addWidget(wsqGroup({ui.totalTime}, false));
    flow->addWidget(wsqGroup({ui.label_10, ui.label_8}, false));

    // The original two rows (buttons, time) are now empty: remove them.
    wsqClearLayout(ui.verticalLayout);
    ui.verticalLayout->addWidget(flowHost);

    // Let the dock shrink and grow with the wrapping layout.
    ui.navigationWidget->setMinimumWidth(0);
    ui.dockWidgetContents_2->setMinimumWidth(0);
    ui.navigationWidget->setMinimumHeight(0);
    ui.navigationWidget->setMaximumHeight(QWIDGETSIZE_MAX);
    wsqHelper->navDock = ui.navigationWidget;
    wsqHelper->navContents = ui.dockWidgetContents_2;
    ui.dockWidgetContents_2->installEventFilter(wsqHelper);

    /* --- Codec panel: header with collapse button, big save button --- */
    QWidget *header = new QWidget(ui.dockWidgetContents);
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(0, 0, 0, 4);
    QLabel *title = new QLabel(PL("USTAWIENIA"), header);
    title->setObjectName("wsqPanelTitle");
    QToolButton *collapse = new QToolButton(header);
    collapse->setObjectName("wsqCollapseButton");
    collapse->setIcon(WSQ_ICON(collapse));
    collapse->setIconSize(QSize(16, 16));
    collapse->setToolTip(PL("Zwiń panel (F9)"));
    collapse->setAutoRaise(true);
    hl->addWidget(title);
    hl->addStretch(1);
    hl->addWidget(collapse);
    ui.verticalLayout_9->insertWidget(0, header);
    connect(collapse, SIGNAL(clicked()), this, SLOT(wsqTogglePanel()));

    QPushButton *save = new QPushButton(WSQ_ICON(save_dark), PL("  Zapisz wideo"), ui.dockWidgetContents);
    save->setObjectName("wsqSaveButton");
    save->setIconSize(QSize(18, 18));
    save->setCursor(Qt::PointingHandCursor);
    ui.verticalLayout_9->addWidget(save);
    connect(save, &QPushButton::clicked, this, [this]() { sendAction(ACT_SAVE_VIDEO); });
    wsqSaveButton = save;

    /* --- Icon rail shown while the codec panel is collapsed --- */
    wsqRail = new QToolBar(PL("Panel"), this);
    wsqRail->setObjectName("wsqRail");
    wsqRail->setMovable(false);
    wsqRail->setFloatable(false);
    wsqRail->setOrientation(Qt::Vertical);
    wsqRail->setIconSize(QSize(20, 20));
    wsqRail->toggleViewAction()->setVisible(false);
    addToolBar(Qt::LeftToolBarArea, wsqRail);

    QAction *a = wsqRail->addAction(WSQ_ICON(expand), PL("Rozwiń panel (F9)"));
    connect(a, SIGNAL(triggered()), this, SLOT(wsqTogglePanel()));
    wsqRail->addSeparator();

    struct
    {
        const char *icon;
        const char *tip;
        QPushButton *button;
    } railItems[] = {
        {"video", "Wyjście wideo – konfiguruj", ui.pushButtonVideoConf},
        {"vfilter", "Filtry wideo", ui.pushButtonVideoFilter},
        {"audio", "Wyjście audio – konfiguruj", ui.pushButtonAudioConf},
        {"afilter", "Filtry audio", ui.pushButtonAudioFilter},
        {"format", "Format wyjściowy – konfiguruj", ui.pushButtonFormatConfigure},
    };
    for (auto &it : railItems)
    {
        QAction *ra = wsqRail->addAction(QIcon(QString(":/new/prefix1/pics/wsq_%1.png").arg(it.icon)), PL(it.tip));
        QPushButton *b = it.button;
        connect(ra, &QAction::triggered, b, [b]() { b->click(); });
        ra->setEnabled(b->isEnabled());
        wsqHelper->mirrors.append(qMakePair((QObject *)b, ra));
        b->installEventFilter(wsqHelper);
    }
    QWidget *stretch = new QWidget(wsqRail);
    stretch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    stretch->setStyleSheet("background: transparent;");
    wsqRail->addWidget(stretch);
    QAction *railSave = wsqRail->addAction(WSQ_ICON(save), PL("Zapisz wideo"));
    connect(railSave, &QAction::triggered, this, [this]() { sendAction(ACT_SAVE_VIDEO); });
    wsqRail->hide();

    connect(ui.codecWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(wsqPanelVisibilityChanged(bool)));

    QAction *toggle = new QAction(PL("Zwiń / rozwiń panel"), this);
    toggle->setShortcut(QKeySequence(Qt::Key_F9));
    toggle->setShortcutContext(Qt::WindowShortcut);
    addAction(toggle);
    connect(toggle, SIGNAL(triggered()), this, SLOT(wsqTogglePanel()));

    /* --- Theme menu entry --- */
    wsqThemeAction = new QAction(PL("WsQ – ciemny (nowoczesny)"), this);
    wsqThemeAction->setCheckable(true);
    ui.menuThemes->insertAction(ui.menuThemes->actions().isEmpty() ? nullptr : ui.menuThemes->actions().first(),
                                wsqThemeAction);
    connect(wsqThemeAction, SIGNAL(triggered(bool)), this, SLOT(setWsqThemeSlot(bool)));

    /* --- Keep window size option --- */
    wsqKeepSizeAction = new QAction(PL("Zachowuj rozmiar okna przy otwieraniu plików"), this);
    wsqKeepSizeAction->setCheckable(true);
    wsqKeepSizeAction->setChecked(true);
    ui.menuView->addSeparator();
    ui.menuView->addAction(wsqKeepSizeAction);
    connect(wsqKeepSizeAction, &QAction::toggled, this, [](bool on) {
        QSettings *qset = qtSettingsCreate();
        if (qset)
        {
            qset->beginGroup("MainWindow");
            qset->setValue("wsqKeepWindowSize", on);
            qset->endGroup();
            delete qset;
        }
    });
}

/**
    \fn wsqKeepWindowSize
*/
bool MainWindow::wsqKeepWindowSize(void)
{
    return wsqKeepSizeAction && wsqKeepSizeAction->isChecked();
}

/**
    \fn wsqSetKeepWindowSize
*/
void MainWindow::wsqSetKeepWindowSize(bool keep)
{
    if (wsqKeepSizeAction)
        wsqKeepSizeAction->setChecked(keep);
}

/**
    \fn wsqRailWidth
    \brief width taken by the icon rail, for zoom calculations
*/
uint32_t MainWindow::wsqRailWidth(void)
{
    if (wsqRail && wsqRail->isVisible())
        return wsqRail->frameSize().width();
    return 0;
}

/**
    \fn wsqTogglePanel
*/
void MainWindow::wsqTogglePanel(void)
{
    ui.codecWidget->setVisible(ui.codecWidget->isHidden());
    wsqPanelVisibilityChanged(true);
    syncToolbarsMenu();
}

/**
    \fn wsqPanelVisibilityChanged
    \brief show the rail exactly when the codec panel is explicitly hidden
*/
void MainWindow::wsqPanelVisibilityChanged(bool)
{
    if (!wsqRail)
        return;
    if (isMinimized())
        return;
    bool collapsed = ui.codecWidget->isHidden();
    if (wsqRail->isVisible() != collapsed)
        wsqRail->setVisible(collapsed);
    if (wsqSaveButton)
        wsqSaveButton->setEnabled(avifileinfo != NULL);
}

/**
    \fn wsqFileStateChanged
    \brief called when a file is loaded/closed to update WsQ-only controls
*/
void MainWindow::wsqFileStateChanged(bool loaded)
{
    if (wsqSaveButton)
        wsqSaveButton->setEnabled(loaded);
    if (wsqRail)
    {
        QList<QAction *> acts = wsqRail->actions();
        if (!acts.isEmpty())
            acts.last()->setEnabled(loaded);
    }
}

/**
    \fn setWsqTheme
*/
void MainWindow::setWsqTheme(void)
{
    QApplication::setStyle("fusion");
    QPalette p;
    p.setColor(QPalette::Window, QColor(0x1f, 0x23, 0x2a));
    p.setColor(QPalette::WindowText, QColor(0xd6, 0xdb, 0xe3));
    p.setColor(QPalette::Base, QColor(0x1b, 0x1e, 0x24));
    p.setColor(QPalette::AlternateBase, QColor(0x22, 0x26, 0x2e));
    p.setColor(QPalette::ToolTipBase, QColor(0x22, 0x26, 0x2e));
    p.setColor(QPalette::ToolTipText, QColor(0xd6, 0xdb, 0xe3));
    p.setColor(QPalette::Text, QColor(0xd6, 0xdb, 0xe3));
    p.setColor(QPalette::Button, QColor(0x2c, 0x32, 0x3b));
    p.setColor(QPalette::ButtonText, QColor(0xd6, 0xdb, 0xe3));
    p.setColor(QPalette::BrightText, QColor(0xff, 0x7b, 0x7b));
    p.setColor(QPalette::Link, QColor(0x8f, 0xb8, 0xff));
    p.setColor(QPalette::Highlight, QColor(0x2a, 0x5a, 0xa0));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, QColor(0x8b, 0x94, 0xa3));
    p.setColor(QPalette::Light, QColor(0x3a, 0x41, 0x4c));
    p.setColor(QPalette::Midlight, QColor(0x33, 0x3a, 0x44));
    p.setColor(QPalette::Mid, QColor(0x2a, 0x2f, 0x37));
    p.setColor(QPalette::Dark, QColor(0x16, 0x19, 0x1e));
    p.setColor(QPalette::Shadow, QColor(0x0a, 0x0b, 0x0d));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x5d, 0x65, 0x73));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x5d, 0x65, 0x73));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x5d, 0x65, 0x73));
    p.setColor(QPalette::Disabled, QPalette::Light, QColor(0x1f, 0x23, 0x2a));
    qApp->setPalette(p);
    qApp->setStyleSheet(QString::fromUtf8(wsqStyleSheet));
    setTimeDisplaySize();

    defaultThemeAction->setChecked(false);
    lightThemeAction->setChecked(false);
    darkThemeAction->setChecked(false);
    wsqThemeAction->setChecked(true);
}

/**
    \fn setWsqThemeSlot
*/
void MainWindow::setWsqThemeSlot(bool b)
{
    UNUSED_ARG(b);
    setWsqTheme();
    QSettings *qset = qtSettingsCreate();
    if (qset)
    {
        qset->beginGroup("MainWindow");
        qset->setValue("theme", ADM_QT_THEME_WSQ);
        qset->endGroup();
        delete qset;
    }
}

/**
    \fn wsqLeaveTheme
    \brief called by the other themes: drop the WsQ stylesheet
*/
void MainWindow::wsqLeaveTheme(void)
{
    if (!qApp->styleSheet().isEmpty())
        qApp->setStyleSheet(QString());
    if (wsqThemeAction)
        wsqThemeAction->setChecked(false);
}

/**
    \fn wsqRestoreGeometry
    \brief restore the last normal window geometry, or pick a sensible default
*/
void MainWindow::wsqRestoreGeometry(QSettings *qset)
{
    wsqPanelVisibilityChanged(true); // dock state was just restored
    QRect r = qset ? qset->value("wsqGeometry", QRect()).toRect() : QRect();
    wsqSetKeepWindowSize(qset ? qset->value("wsqKeepWindowSize", true).toBool() : true);

    QScreen *screen = nullptr;
    if (r.isValid())
        screen = QGuiApplication::screenAt(r.center());
    if (r.isValid() && screen)
    {
        QRect avail = screen->availableGeometry();
        r.setWidth(qMin(r.width(), avail.width()));
        r.setHeight(qMin(r.height(), avail.height()));
        resize(r.size());
        move(r.topLeft());
        return;
    }
    screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    QRect avail = screen->availableGeometry();
    QSize s(qMin(1280, avail.width() * 9 / 10), qMin(800, avail.height() * 9 / 10));
    resize(s);
    move(avail.center() - QPoint(s.width() / 2, s.height() / 2));
}

/**
    \fn wsqSaveGeometry
*/
void MainWindow::wsqSaveGeometry(QSettings *qset)
{
    if (!qset)
        return;
    // Store the normal (un-maximized) geometry, position of the client area.
    QRect r = (isMaximized() || isFullScreen()) ? normalGeometry() : geometry();
    if (r.isValid())
        qset->setValue("wsqGeometry", r);
    qset->setValue("wsqKeepWindowSize", wsqKeepWindowSize());
}
