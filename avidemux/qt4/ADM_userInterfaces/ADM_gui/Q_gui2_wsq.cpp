/***************************************************************************
    Avidemux WsQ: main window layout and look following the WsQ design
    (card settings panel, custom timeline, navigation bar that splits into
    two rows when narrow, VU column, icon rail, toolbar zoom/info, remembered
    window geometry, dark theme).
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <cmath>
#include <QApplication>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScreen>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "ADM_default.h"
#include "Q_gui2.h"
#include "Q_gui2_wsq.h"
#include "ADM_QSettings.h"
#include "avi_vars.h"
#include "ADM_coreVideoFilter.h"
#include "ADM_coreAudio.h"

#define WSQ_ICON(x) QIcon(":/new/prefix1/pics/wsq_" #x ".png")
#define PL(x) QString::fromUtf8(x)

/* VU meter feed (see T_vumeter.cpp) */
extern void (*wsqVuHook)(const int32_t *volume);
extern bool wsqFlexibleVideoFrame;
static MainWindow *wsqWindow = nullptr;
static void wsqVuFeed(const int32_t *volume)
{
    if (wsqWindow)
        wsqWindow->wsqVuUpdate(volume);
}

/* ======================================================================
 *  WsqVuBars
 * ====================================================================== */
void WsqVuBars::setVolumes(const int32_t *volume)
{
    QVector<float> l;
    for (int i = 0; i < 8; i++)
    {
        if (volume[i] > 3)
            continue; // inactive channel
        float v = (volume[i] + 60.f) / 63.f;
        if (v < 0)
            v = 0;
        if (v > 1)
            v = 1;
        l.append(v);
    }
    if (peaks.size() != l.size())
        peaks = QVector<float>(l.size(), 0.f);
    for (int i = 0; i < l.size(); i++)
        peaks[i] = qMax(l[i], peaks[i] - 0.015f);
    levels = l;
    update();
}

void WsqVuBars::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    int n = levels.size();
    if (n < 2)
        n = 2; // idle: two empty tracks like the design
    const int barW = n > 2 ? qMax(3, (width() - (n - 1) * 3) / n) : 6;
    const int gap = n > 2 ? 3 : 4;
    const int total = n * barW + (n - 1) * gap;
    int x = (width() - total) / 2;
    const int h = height();
    for (int i = 0; i < n; i++)
    {
        QRectF track(x, 0, barW, h);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x26, 0x2b, 0x33));
        p.drawRoundedRect(track, barW / 2.0, barW / 2.0);
        if (i < levels.size())
        {
            float lv = levels[i];
            QRectF fill(x, h - h * lv, barW, h * lv);
            QColor c = WSQ_ACCENT;
            if (lv > 60.f / 63.f)
                c = QColor(0xff, 0x7b, 0x7b);
            else if (lv > 42.f / 63.f)
                c = QColor(0x8f, 0xc8, 0xff);
            p.setBrush(c);
            p.drawRoundedRect(fill, barW / 2.0, barW / 2.0);
            float pk = peaks[i];
            p.setBrush(QColor(0xd6, 0xdb, 0xe3));
            p.drawRect(QRectF(x, h - h * pk, barW, 2));
        }
        x += barW + gap;
    }
}

/* ======================================================================
 *  WsqHelper
 * ====================================================================== */
bool WsqHelper::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::Resize:
        if (watched == navContents && onNavResize && !pending)
        {
            pending = true;
            QTimer::singleShot(0, this, [this]() {
                pending = false;
                onNavResize();
            });
        }
        break;
    case QEvent::EnabledChange:
        for (auto &p : mirrors)
            if (p.first == watched)
                p.second->setEnabled(static_cast<QWidget *>(watched)->isEnabled());
        for (auto &p : widgetMirrors)
            if (p.first == watched)
                p.second->setEnabled(static_cast<QWidget *>(watched)->isEnabled());
        break;
    case QEvent::MouseButtonDblClick:
        if (watched == timeField && onTimeDoubleClick)
        {
            onTimeDoubleClick();
            return true;
        }
        break;
    case QEvent::Wheel:
        if (watched == volumeButton && onVolumeWheel)
        {
            QWheelEvent *w = static_cast<QWheelEvent *>(event);
            onVolumeWheel(w->angleDelta().y() > 0 ? 5 : -5);
            return true;
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

/* ======================================================================
 *  Stylesheet
 * ====================================================================== */
static const char *wsqStyleSheet = R"QSS(
QMainWindow, QDialog { background: #1b1e24; }
QWidget { color: #d6dbe3; }
QMainWindow::separator { background: #262b33; width: 1px; height: 1px; }
QWidget#centralwidget { background: #121418; }
QWidget#dockWidgetContents, QWidget#dockWidgetContents_2, QWidget#dockWidgetContents_3,
QWidget#dockWidgetContents_5, QWidget#dockWidgetContents_7 { background: #1f232a; }
QWidget#dockWidgetContents_2 { border-top: 1px solid #262b33; }
QScrollArea#wsqPanelScroll, QScrollArea#wsqPanelScroll > QWidget > QWidget { background: #1f232a; }
QWidget#wsqVuColumn { background: #1f232a; border-left: 1px solid #262b33; }

QMenuBar { background: #1b1e24; color: #aeb6c3; padding: 2px 6px; }
QMenuBar::item { background: transparent; padding: 5px 9px; border-radius: 4px; }
QMenuBar::item:selected { background: #2c323b; color: #ffffff; }
QMenuBar::item:pressed { background: #1e3557; color: #ffffff; }
QMenu { background: #22262e; border: 1px solid #333a44; padding: 4px; }
QMenu::item { padding: 5px 28px 5px 26px; border-radius: 4px; color: #d6dbe3; }
QMenu::item:selected { background: #1e3557; color: #ffffff; }
QMenu::item:disabled { color: #5d6573; }
QMenu::separator { height: 1px; background: #333a44; margin: 4px 8px; }

QToolBar { background: #1b1e24; border: none; border-bottom: 1px solid #262b33; spacing: 4px; padding: 4px 10px; }
QToolBar#wsqRail { background: #1f232a; border: none; border-right: 1px solid #262b33; padding: 8px 6px; spacing: 6px; }
QToolBar::separator { background: #2e343d; width: 1px; height: 20px; margin: 6px 6px; }
QToolBar#wsqRail::separator { width: 24px; height: 1px; margin: 2px 4px; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 4px; }
QToolButton:hover { background: #2c323b; border-color: #3a414c; }
QToolButton:pressed, QToolButton:checked { background: #1e3557; border-color: #2b4f80; }
QToolButton:disabled { background: transparent; border-color: transparent; }
QToolBar#wsqRail QToolButton#wsqRailSave { background: #4a9bff; border: none; border-radius: 8px; }
QToolBar#wsqRail QToolButton#wsqRailSave:hover { background: #6aafff; }
QToolBar#wsqRail QToolButton#wsqRailSave:disabled { background: #2c323b; }
QToolBar#wsqRail QToolButton#wsqRailExpand { background: #252a32; border: 1px solid #333a44; }

QToolButton#wsqZoomButton { background: #252a32; border: 1px solid #333a44; border-radius: 6px; padding: 3px 10px; color: #aeb6c3; font-size: 12px; }
QToolButton#wsqZoomButton:hover { border-color: #4a525e; }
QToolButton#wsqZoomButton::menu-indicator { image: url(:/new/prefix1/pics/wsq_arrow_down.png); subcontrol-position: right center; width: 8px; right: 6px; }
QLabel#wsqInfoLabel { color: #8b94a3; font-size: 12px; padding-right: 6px; }

QFrame#wsqCard { background: #252a32; border: 1px solid #2f353f; border-radius: 8px; }
QFrame#wsqCard QLabel { background: transparent; }
QLabel#wsqCardTitle, QLabel#wsqPanelTitle { color: #8b94a3; font-size: 11px; font-weight: normal; }
QLabel#wsqCardMuted { color: #8b94a3; }
QPushButton#wsqCollapseButton { background: transparent; border: 1px solid #333a44; border-radius: 6px; padding: 2px 8px; color: #aeb6c3; font-size: 12px; min-height: 20px; }
QPushButton#wsqCollapseButton:hover { background: #2c323b; }
QFrame#wsqCard QLabel#wsqBadge, QLabel#wsqBadge { background: #4a9bff; color: #0e1420; border-radius: 9px; font-size: 11px; font-weight: bold; padding: 0 5px; }
QLabel#wsqRailBadge { background: #4a9bff; color: #0e1420; border-radius: 7px; font-size: 9px; font-weight: bold; }

QWidget#wsqNavGroup { background: #16191e; border: 1px solid #2a2f37; border-radius: 8px; }
QWidget#wsqNavGroup QToolButton { min-width: 26px; min-height: 26px; }
QToolButton#toolButtonPlay { background: #4a9bff; border: none; border-radius: 6px; min-width: 30px; }
QToolButton#toolButtonPlay:hover { background: #6aafff; }
QToolButton#toolButtonPlay:disabled { background: #2c323b; }
QToolButton#toolButtonSetMarkerA, QToolButton#toolButtonSetMarkerB { color: #4a9bff; font-weight: bold; padding: 4px 10px; }
QToolButton#toolButtonSetMarkerA:disabled, QToolButton#toolButtonSetMarkerB:disabled { color: #3d5f8c; }
QLabel#wsqMuted { color: #8b94a3; }
QLabel#totalTime { color: #aeb6c3; font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 14px; }
QLabel#label_8 { background: #2c323b; border-radius: 4px; padding: 3px 8px; font-size: 11px; font-weight: bold; color: #d6dbe3; }
QLabel#selectionDuration { color: #d6dbe3; font-family: Consolas, "DejaVu Sans Mono", monospace; }

QPushButton { background: #2c323b; border: 1px solid #3a414c; border-radius: 6px; padding: 4px 12px; min-height: 18px; color: #d6dbe3; }
QPushButton:hover { background: #343b45; border-color: #4a525e; }
QPushButton:pressed { background: #1e3557; border-color: #2b4f80; }
QPushButton:disabled { color: #5d6573; background: #23272e; border-color: #2c323b; }
QPushButton:default { border-color: #4a9bff; }
QPushButton#wsqSaveButton, QPushButton#wsqAccentButton { background: #4a9bff; border: none; color: #0e1420; font-weight: bold; min-height: 32px; border-radius: 8px; font-size: 14px; }
QPushButton#wsqSaveButton:hover, QPushButton#wsqAccentButton:hover { background: #6aafff; }
QPushButton#wsqSaveButton:disabled, QPushButton#wsqAccentButton:disabled { background: #2c323b; color: #5d6573; }
QPushButton#pushButtonJumpToMarkerA, QPushButton#pushButtonJumpToMarkerB {
    background: #1b2638; border: 1px solid #2b4466; color: #9cc2ff; padding: 3px 10px; min-height: 20px;
    font-family: Consolas, "DejaVu Sans Mono", monospace; }
QPushButton#pushButtonJumpToMarkerA:hover, QPushButton#pushButtonJumpToMarkerB:hover { background: #22324a; }
QPushButton#pushButtonJumpToMarkerA:disabled, QPushButton#pushButtonJumpToMarkerB:disabled { color: #4d6a92; background: #1a212c; border-color: #233247; }
QPushButton#wsqDangerButton { color: #ff8a8a; }

QLineEdit, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit, QDateTimeEdit, QTimeEdit {
    background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; padding: 3px 6px;
    selection-background-color: #2a5aa0; selection-color: #ffffff; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QTextEdit:focus, QPlainTextEdit:focus { border-color: #4a9bff; }
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled { color: #5d6573; background: #1f232a; }
QLineEdit#currentTime { background: #2a1f24; border: 1px solid #4a2c33; color: #ff8a8a; padding: 2px 8px; }
QSpinBox::up-button, QDoubleSpinBox::up-button, QTimeEdit::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 16px; border: none; border-top-right-radius: 6px; }
QSpinBox::down-button, QDoubleSpinBox::down-button, QTimeEdit::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 16px; border: none; border-bottom-right-radius: 6px; }
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: #2c323b; }
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow, QTimeEdit::up-arrow { image: url(:/new/prefix1/pics/wsq_arrow_up.png); width: 8px; height: 6px; }
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow, QTimeEdit::down-arrow { image: url(:/new/prefix1/pics/wsq_arrow_down.png); width: 8px; height: 6px; }
QSpinBox#spinBox_TimeValue { font-family: Consolas, "DejaVu Sans Mono", monospace; color: #8b94a3; }

QComboBox { background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; padding: 4px 10px; min-height: 20px; }
QComboBox:hover { border-color: #4a525e; }
QComboBox:focus { border-color: #4a9bff; }
QComboBox:disabled { color: #5d6573; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox::down-arrow { image: url(:/new/prefix1/pics/wsq_arrow_down.png); width: 10px; height: 7px; }
QComboBox QAbstractItemView { background: #22262e; border: 1px solid #333a44; selection-background-color: #1e3557; selection-color: #ffffff; outline: 0; padding: 2px; }

QCheckBox, QRadioButton { spacing: 7px; background: transparent; }
QCheckBox { color: #aeb6c3; }
QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #4a525e; border-radius: 3px; background: transparent; }
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

/* video filter manager (WsQ layout) */
QWidget#wsqFilterCategories { background: #1f232a; border-right: 1px solid #262b33; }
QListWidget#listFilterCategory { background: transparent; border: none; }
QListWidget#listFilterCategory::item { padding: 8px 10px; border-radius: 6px; color: #aeb6c3; margin: 1px 0; }
QListWidget#listFilterCategory::item:selected { background: #1e3557; color: #ffffff; border-left: 2px solid #4a9bff; }
QListWidget#listFilterCategory::item:hover:!selected { background: #252a32; }
QListWidget#listWidgetAvailable, QListWidget#listWidgetActive { background: transparent; border: none; }
QWidget#wsqFilterMiddle { border-right: 1px solid #262b33; }
QWidget#wsqDialogFooter { background: #1f232a; border-top: 1px solid #262b33; }
QWidget#wsqDialogHeader { background: #16191e; border-bottom: 1px solid #262b33; }

QScrollBar:vertical { background: transparent; width: 11px; margin: 0; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }
QScrollBar::handle:vertical { background: #3a414c; border-radius: 4px; min-height: 28px; margin: 2px; }
QScrollBar::handle:horizontal { background: #3a414c; border-radius: 4px; min-width: 28px; margin: 2px; }
QScrollBar::handle:hover { background: #4a525e; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QProgressBar { background: #1b1e24; border: 1px solid #3a414c; border-radius: 6px; text-align: center; min-height: 16px; }
QProgressBar::chunk { background: #4a9bff; border-radius: 5px; }
QToolTip { background: #22262e; color: #d6dbe3; border: 1px solid #3a414c; padding: 4px; }
QStatusBar { background: #16191e; color: #8b94a3; border-top: 1px solid #262b33; min-height: 22px; }
QStatusBar QLabel { color: #8b94a3; font-size: 11px; padding: 0 6px; }
QStatusBar::item { border: none; }
)QSS";

/* ======================================================================
 *  Small helpers
 * ====================================================================== */
static QFrame *wsqCard(QWidget *parent, const QString &title, QLabel **titleLabel = nullptr)
{
    QFrame *card = new QFrame(parent);
    card->setObjectName("wsqCard");
    QVBoxLayout *v = new QVBoxLayout(card);
    v->setContentsMargins(12, 12, 12, 12);
    v->setSpacing(8);
    QLabel *t = new QLabel(title, card);
    t->setObjectName("wsqCardTitle");
    v->addWidget(t);
    if (titleLabel)
        *titleLabel = t;
    return card;
}

static QWidget *wsqGroup(const QList<QWidget *> &widgets, bool framed, int spacing)
{
    QWidget *g = new QWidget();
    g->setObjectName(framed ? "wsqNavGroup" : "wsqNavPlain");
    g->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *h = new QHBoxLayout(g);
    h->setContentsMargins(framed ? 3 : 0, framed ? 3 : 0, framed ? 3 : 0, framed ? 3 : 0);
    h->setSpacing(spacing);
    for (QWidget *w : widgets)
        if (w)
            h->addWidget(w);
    return g;
}

static void wsqTakeAll(QLayout *l)
{
    if (!l)
        return;
    QLayoutItem *item;
    while ((item = l->takeAt(0)))
    {
        if (item->layout())
        {
            wsqTakeAll(item->layout());
            delete item->layout();
            continue;
        }
        delete item; // widgets are re-parented elsewhere, spacers die here
    }
}

static QString wsqUpperTitle(const QString &s)
{
    QString t = s;
    t.remove('&');
    t.remove(':');
    return t.trimmed().toUpper();
}

static QString wsqGroupDigits(uint64_t v)
{
    QString s = QString::number((qulonglong)v);
    for (int i = s.size() - 3; i > 0; i -= 3)
        s.insert(i, QChar(0x2009)); // thin space like "60 238"
    return s;
}

/* ======================================================================
 *  MainWindow additions
 * ====================================================================== */

/**
    \fn wsqBuildPanel
    \brief settings panel: header, cards, save button
*/
void MainWindow::wsqBuildPanel(void)
{
    QWidget *host = ui.dockWidgetContents;
    // pull everything out of the original panel layout
    wsqTakeAll(ui.verticalLayout_9);
    QVBoxLayout *v = ui.verticalLayout_9;
    v->setContentsMargins(14, 14, 14, 14);
    v->setSpacing(12);

    // header
    QHBoxLayout *hl = new QHBoxLayout();
    hl->setContentsMargins(0, 0, 0, 0);
    QLabel *title = new QLabel(PL("USTAWIENIA"), host);
    title->setObjectName("wsqPanelTitle");
    QPushButton *collapse = new QPushButton(WSQ_ICON(collapse), PL("Zwiń"), host);
    collapse->setObjectName("wsqCollapseButton");
    collapse->setIconSize(QSize(14, 14));
    collapse->setToolTip(PL("Zwiń panel (F9)"));
    collapse->setCursor(Qt::PointingHandCursor);
    connect(collapse, SIGNAL(clicked()), this, SLOT(wsqTogglePanel()));
    hl->addWidget(title);
    hl->addStretch(1);
    hl->addWidget(collapse);
    v->addLayout(hl);

    // decoder card
    QFrame *dec = wsqCard(host, PL("DEKODER WIDEO"));
    QHBoxLayout *dr = new QHBoxLayout();
    dr->setSpacing(8);
    wsqDecoderLabel = new QLabel(PL("—"), dec);
    QPushButton *decConf = new QPushButton(ui.pushButtonVideoConf->text(), dec);
    decConf->setEnabled(false);
    connect(decConf, &QPushButton::clicked, this, [this]() { sendAction(ACT_DecoderOption); });
    wsqDecoderLabel->setProperty("wsqDecConf", QVariant::fromValue((void *)decConf));
    dr->addWidget(wsqDecoderLabel, 1);
    dr->addWidget(decConf);
    static_cast<QVBoxLayout *>(dec->layout())->addLayout(dr);
    v->addWidget(dec);

    // video output card
    QFrame *vid = wsqCard(host, wsqUpperTitle(ui.label_3->text()));
    QVBoxLayout *vl = static_cast<QVBoxLayout *>(vid->layout());
    ui.comboBoxVideo->setMaximumWidth(QWIDGETSIZE_MAX);
    ui.comboBoxVideo->setParent(vid);
    vl->addWidget(ui.comboBoxVideo);
    QHBoxLayout *vr = new QHBoxLayout();
    vr->setSpacing(8);
    ui.pushButtonVideoConf->setParent(vid);
    ui.pushButtonVideoFilter->setParent(vid);
    // "Filters" button with a counter badge
    {
        QString txt = ui.pushButtonVideoFilter->text();
        ui.pushButtonVideoFilter->setText(QString());
        QHBoxLayout *bl = new QHBoxLayout(ui.pushButtonVideoFilter);
        bl->setContentsMargins(8, 0, 8, 0);
        bl->setSpacing(6);
        QLabel *t = new QLabel(txt, ui.pushButtonVideoFilter);
        t->setAttribute(Qt::WA_TransparentForMouseEvents);
        t->setStyleSheet("background: transparent; color: palette(button-text);");
        t->setProperty("wsqFilterText", true);
        wsqFilterBadge = new QLabel("0", ui.pushButtonVideoFilter);
        wsqFilterBadge->setObjectName("wsqBadge");
        wsqFilterBadge->setAlignment(Qt::AlignCenter);
        wsqFilterBadge->setMinimumSize(18, 18);
        wsqFilterBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
        wsqFilterBadge->hide();
        bl->addStretch(1);
        bl->addWidget(t);
        bl->addWidget(wsqFilterBadge);
        bl->addStretch(1);
        ui.pushButtonVideoFilter->setMinimumWidth(t->sizeHint().width() + 46);
    }
    vr->addWidget(ui.pushButtonVideoConf, 1);
    vr->addWidget(ui.pushButtonVideoFilter, 1);
    vl->addLayout(vr);
    v->addWidget(vid);

    // audio output card
    QLabel *aTitle = nullptr;
    QFrame *aud = wsqCard(host, wsqUpperTitle(ui.label_5->text()), &aTitle);
    QVBoxLayout *al = static_cast<QVBoxLayout *>(aud->layout());
    {
        // title row: "WYJŚCIE AUDIO" + track count
        al->removeWidget(aTitle);
        QHBoxLayout *tr = new QHBoxLayout();
        tr->setSpacing(4);
        tr->addWidget(aTitle);
        ui.TrackCountLabel->setParent(aud);
        ui.TrackCountLabel->setObjectName("wsqCardTitle");
        tr->addWidget(ui.TrackCountLabel);
        tr->addStretch(1);
        al->insertLayout(0, tr);
    }
    ui.comboBoxAudio->setMaximumWidth(QWIDGETSIZE_MAX);
    ui.comboBoxAudio->setParent(aud);
    al->addWidget(ui.comboBoxAudio);
    QHBoxLayout *ar = new QHBoxLayout();
    ar->setSpacing(8);
    ui.pushButtonAudioConf->setParent(aud);
    ui.pushButtonAudioFilter->setParent(aud);
    ar->addWidget(ui.pushButtonAudioConf, 1);
    ar->addWidget(ui.pushButtonAudioFilter, 1);
    al->addLayout(ar);
    QHBoxLayout *sr = new QHBoxLayout();
    sr->setSpacing(8);
    ui.checkBox_TimeShift->setParent(aud);
    ui.spinBox_TimeValue->setParent(aud);
    ui.label_11->setParent(aud);
    ui.label_11->setObjectName("wsqCardMuted");
    {
        QString t = ui.checkBox_TimeShift->text();
        t.remove(':');
        ui.checkBox_TimeShift->setText(t);
    }
    ui.spinBox_TimeValue->setFixedWidth(84);
    ui.spinBox_TimeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sr->addWidget(ui.checkBox_TimeShift, 1);
    sr->addWidget(ui.spinBox_TimeValue);
    sr->addWidget(ui.label_11);
    al->addLayout(sr);
    v->addWidget(aud);

    // output format card
    QFrame *fmt = wsqCard(host, wsqUpperTitle(ui.label_4->text()));
    QVBoxLayout *fl = static_cast<QVBoxLayout *>(fmt->layout());
    ui.comboBoxFormat->setMaximumWidth(QWIDGETSIZE_MAX);
    ui.comboBoxFormat->setParent(fmt);
    ui.pushButtonFormatConfigure->setParent(fmt);
    fl->addWidget(ui.comboBoxFormat);
    fl->addWidget(ui.pushButtonFormatConfigure);
    v->addWidget(fmt);

    // the original section labels are no longer used
    ui.label_3->hide();
    ui.label_4->hide();
    ui.label_5->hide();

    v->addStretch(1);

    QPushButton *save = new QPushButton(WSQ_ICON(save_dark), PL("Zapisz wideo"), host);
    save->setObjectName("wsqSaveButton");
    save->setIconSize(QSize(18, 18));
    save->setMinimumHeight(44);
    save->setCursor(Qt::PointingHandCursor);
    save->setEnabled(false);
    connect(save, &QPushButton::clicked, this, [this]() { sendAction(ACT_SAVE_VIDEO); });
    v->addWidget(save);
    wsqSaveButton = save;

    host->setMinimumWidth(252);
    // the panel scrolls instead of dictating the window's minimum height
    QScrollArea *scroll = new QScrollArea(ui.codecWidget);
    scroll->setObjectName("wsqPanelScroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setMinimumHeight(0);
    ui.codecWidget->setWidget(scroll); // takes the contents widget out of the dock first
    scroll->setWidget(host);
    ui.codecWidget->setMinimumWidth(280);
    ui.codecWidget->setMaximumWidth(280);
    ui.centralwidget->setMinimumSize(120, 80);
}

/**
    \fn wsqBuildNav
    \brief custom timeline + navigation bar (one row wide, two rows narrow)
*/
void MainWindow::wsqBuildNav(void)
{
    QWidget *host = ui.dockWidgetContents_2;
    ADM_mwNavSlider::wsqStyle = true;
    ui.horizontalSlider->setFixedHeight(34);

    // empty the original button/time rows
    wsqTakeAll(ui.verticalLayout);
    ui.verticalLayout_8->setContentsMargins(16, 10, 16, 12);
    ui.verticalLayout_8->setSpacing(10);

    auto iconOnly = [](QToolButton *b, int px) {
        b->setIconSize(QSize(px, px));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setMinimumSize(34, 34);
        b->setMaximumSize(34, 34);
    };
    // transport group: play, start, prev key, prev, next, next key, end (+ more when narrow)
    ui.toolButtonPlay->setIcon(WSQ_ICON(play_dark));
    ui.toolButtonPlay->setMinimumSize(38, 34);
    ui.toolButtonPlay->setMaximumSize(38, 34);
    ui.toolButtonPlay->setIconSize(QSize(16, 16));
    for (QToolButton *b : {ui.toolButtonFirstFrame, ui.toolButtonPreviousIntraFrame, ui.toolButtonPreviousFrame,
                           ui.toolButtonNextFrame, ui.toolButtonNextIntraFrame, ui.toolButtonLastFrame,
                           ui.toolButtonPreviousBlackFrame, ui.toolButtonNextBlackFrame})
        iconOnly(b, 16);

    wsqMoreButton = new QToolButton(host);
    wsqMoreButton->setObjectName("wsqMoreButton");
    wsqMoreButton->setIcon(WSQ_ICON(more));
    wsqMoreButton->setToolTip(PL("Więcej przycisków nawigacji"));
    wsqMoreButton->setPopupMode(QToolButton::InstantPopup);
    wsqMoreButton->setStyleSheet("QToolButton::menu-indicator { image: none; width: 0; }");
    iconOnly(wsqMoreButton, 16);
    {
        QMenu *m = new QMenu(wsqMoreButton);
        struct
        {
            QToolButton *b;
        } items[] = {{ui.toolButtonFirstFrame},          {ui.toolButtonLastFrame},
                     {ui.toolButtonPreviousBlackFrame}, {ui.toolButtonNextBlackFrame},
                     {ui.toolButtonPreviousCutPoint},   {ui.toolButtonNextCutPoint},
                     {ui.toolButtonDeleteSelection}};
        for (auto &it : items)
        {
            QToolButton *b = it.b;
            QString t = b->toolTip();
            if (t.isEmpty())
                t = b->objectName();
            QAction *a = m->addAction(b->icon(), t);
            connect(a, &QAction::triggered, b, [b]() { b->click(); });
        }
        wsqMoreButton->setMenu(m);
    }
    wsqG1 = wsqGroup({ui.toolButtonPlay, ui.toolButtonFirstFrame, ui.toolButtonPreviousIntraFrame,
                      ui.toolButtonPreviousFrame, ui.toolButtonNextFrame, ui.toolButtonNextIntraFrame,
                      ui.toolButtonLastFrame, wsqMoreButton},
                     true, 2);

    // marker group: "[ A" "B ]" + black frame search
    ui.toolButtonSetMarkerA->setIcon(QIcon());
    ui.toolButtonSetMarkerB->setIcon(QIcon());
    ui.toolButtonSetMarkerA->setText("[ A");
    ui.toolButtonSetMarkerB->setText("B ]");
    for (QToolButton *b : {ui.toolButtonSetMarkerA, ui.toolButtonSetMarkerB})
    {
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setMinimumSize(40, 34);
        b->setMaximumSize(60, 34);
    }
    wsqG2 = wsqGroup({ui.toolButtonSetMarkerA, ui.toolButtonSetMarkerB, ui.toolButtonPreviousBlackFrame,
                      ui.toolButtonNextBlackFrame},
                     true, 2);

    // time group: current time pill "/" total
    ui.currentTime->setMinimumHeight(32);
    {
        QFont mono(PL("Consolas"));
        mono.setStyleHint(QFont::Monospace);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        mono.setFamilies({PL("Consolas"), PL("DejaVu Sans Mono"), PL("Courier New")});
#endif
        mono.setPixelSize(15);
        ui.currentTime->setFont(mono);
        QFontMetrics fm(mono);
        ui.currentTime->setFixedSize(fm.horizontalAdvance(PL("00:00:00.000")) + 26, 32);
    }
    ui.totalTime->setText(ui.totalTime->text());
    wsqG3a = wsqGroup({ui.currentTime, ui.totalTime}, false, 8);
    // frame type pill
    ui.label_8->setAlignment(Qt::AlignCenter);
    wsqG3b = wsqGroup({ui.label_8}, false, 0);

    // selection group: A [..] B [..] Długość ..
    QLabel *la = new QLabel("A", host);
    QLabel *lb = new QLabel("B", host);
    wsqDurTitle = new QLabel(PL("Długość"), host);
    for (QLabel *l : {la, lb, wsqDurTitle})
        l->setObjectName("wsqMuted");
    ui.pushButtonJumpToMarkerA->setParent(host);
    ui.pushButtonJumpToMarkerB->setParent(host);
    ui.selectionDuration->setParent(host);
    ui.selectionDuration->setMinimumWidth(0);
    wsqG4 = wsqGroup({la, ui.pushButtonJumpToMarkerA, lb, ui.pushButtonJumpToMarkerB, wsqDurTitle,
                      ui.selectionDuration},
                     false, 10);

    // buttons not shown in the bar (still reachable from the "more" menu / Go menu / shortcuts)
    for (QWidget *w : {(QWidget *)ui.toolButtonDeleteSelection, (QWidget *)ui.toolButtonPreviousCutPoint,
                       (QWidget *)ui.toolButtonNextCutPoint, (QWidget *)ui.toolButtonBackOneMinute,
                       (QWidget *)ui.toolButtonForwardOneMinute, (QWidget *)ui.pushButtonTime, (QWidget *)ui.label_10})
    {
        w->setParent(host);
        w->hide();
    }

    QWidget *rows = new QWidget(host);
    rows->setObjectName("wsqNavRows");
    // never let the single-row layout dictate the window's minimum width: it re-arranges instead
    rows->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QVBoxLayout *rv = new QVBoxLayout(rows);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(8);
    wsqRow1 = new QHBoxLayout();
    wsqRow1->setSpacing(14);
    wsqRow2Host = new QWidget(rows);
    wsqRow2 = new QHBoxLayout(wsqRow2Host);
    wsqRow2->setContentsMargins(0, 0, 0, 0);
    wsqRow2->setSpacing(14);
    rv->addLayout(wsqRow1);
    rv->addWidget(wsqRow2Host);
    ui.verticalLayout->addWidget(rows);

    // the separate selection (jog) dock is replaced by the selection group
    ui.selectionWidget->hide();

    ui.navigationWidget->setMinimumWidth(0);
    ui.dockWidgetContents_2->setMinimumWidth(0);
    ui.navigationWidget->setMinimumHeight(0);
    ui.navigationWidget->setMaximumHeight(QWIDGETSIZE_MAX);

    wsqHelper->navContents = host;
    wsqHelper->onNavResize = [this]() { wsqArrangeNav(); };
    host->installEventFilter(wsqHelper);
    wsqHelper->timeField = ui.currentTime;
    wsqHelper->onTimeDoubleClick = [this]() {
        if (ui.pushButtonTime->isEnabled())
            ui.pushButtonTime->click();
    };
    ui.currentTime->installEventFilter(wsqHelper);
    ui.currentTime->setToolTip(PL("Kliknij dwukrotnie, aby przejść do czasu"));
    wsqArrangeNav(true);
}

/**
    \fn wsqArrangeNav
    \brief place the groups for the current width: one row (wide) or two rows (narrow)
*/
void MainWindow::wsqArrangeNav(bool force)
{
    if (!wsqRow1)
        return;
    QWidget *host = ui.dockWidgetContents_2;
    int avail = host->width() - 32;
    // width needed for the single-row layout (all buttons visible)
    ui.toolButtonFirstFrame->setVisible(true);
    ui.toolButtonLastFrame->setVisible(true);
    ui.toolButtonPreviousBlackFrame->setVisible(true);
    ui.toolButtonNextBlackFrame->setVisible(true);
    wsqMoreButton->setVisible(false);
    wsqG1->adjustSize();
    wsqG2->adjustSize();
    int need = wsqG1->sizeHint().width() + wsqG2->sizeHint().width() + wsqG3a->sizeHint().width() +
               wsqG3b->sizeHint().width() + wsqG4->sizeHint().width() + 14 * 5;
    int mode = (avail >= need) ? 1 : 0;
    if (mode == 0)
    {
        ui.toolButtonFirstFrame->setVisible(false);
        ui.toolButtonLastFrame->setVisible(false);
        ui.toolButtonPreviousBlackFrame->setVisible(false);
        ui.toolButtonNextBlackFrame->setVisible(false);
        wsqMoreButton->setVisible(true);
    }
    if (mode != wsqMode || force)
    {
        wsqMode = mode;
        wsqTakeAll(wsqRow1);
        wsqTakeAll(wsqRow2);
        if (mode == 1)
        {
            wsqRow1->addWidget(wsqG1);
            wsqRow1->addWidget(wsqG2);
            wsqRow1->addWidget(wsqG3a);
            wsqRow1->addWidget(wsqG3b);
            wsqRow1->addStretch(1);
            wsqRow1->addWidget(wsqG4);
            wsqRow2Host->hide();
            wsqDurTitle->setText(PL("Długość"));
        }
        else
        {
            wsqRow1->addWidget(wsqG1);
            wsqRow1->addWidget(wsqG2);
            wsqRow1->addStretch(1);
            wsqRow1->addWidget(wsqG3a);
            wsqRow2->addWidget(wsqG3b);
            wsqRow2->addStretch(1);
            wsqRow2->addWidget(wsqG4);
            wsqRow2Host->show();
            wsqDurTitle->setText(PL("Dł."));
        }
        for (QWidget *w : {wsqG1, wsqG2, wsqG3a, wsqG3b, wsqG4})
            w->show();
    }
    // fixed dock height matching the content
    int h = host->layout()->sizeHint().height();
    int extra = ui.navigationWidget->height() - host->height();
    if (extra < 0)
        extra = 0;
    if (ui.navigationWidget->minimumHeight() != h + extra || ui.navigationWidget->maximumHeight() != h + extra)
        ui.navigationWidget->setFixedHeight(h + extra);
}

/**
    \fn wsqBuildToolbar
*/
void MainWindow::wsqBuildToolbar(void)
{
    QToolBar *tb = ui.toolBar;
    tb->setMovable(false);
    tb->setIconSize(QSize(18, 18));

    // Rebuild the order of the design: open, append, save | info, queue | zoom ... media info
    QList<QAction *> keep = tb->actions(); // includes HDR + script actions, kept after the zoom pill
    for (QAction *a : keep)
        tb->removeAction(a);

    QAction *append = new QAction(WSQ_ICON(append), PL("Dołącz plik"), this);
    append->setObjectName("wsqActionAppend");
    QAction *queue = new QAction(WSQ_ICON(queue), PL("Kolejka zadań"), this);
    queue->setObjectName("wsqActionQueue");

    tb->addAction(ui.actionOpen);
    tb->addAction(append);
    tb->addAction(ui.actionSave_video);
    tb->addSeparator();
    tb->addAction(ui.actionProperties);
    tb->addAction(queue);
    tb->addSeparator();

    wsqZoomButton = new QToolButton(tb);
    wsqZoomButton->setObjectName("wsqZoomButton");
    wsqZoomButton->setText(PL("Zoom: 100%"));
    wsqZoomButton->setPopupMode(QToolButton::InstantPopup);
    wsqZoomButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QMenu *zm = new QMenu(wsqZoomButton);
    struct
    {
        const char *t;
        Action a;
    } zooms[] = {{"25%", ACT_ZOOM_1_4},
                 {"50%", ACT_ZOOM_1_2},
                 {"100%", ACT_ZOOM_1_1},
                 {"200%", ACT_ZOOM_2_1},
                 {"Dopasuj do okna", ACT_ZOOM_FIT_IN}};
    for (auto &z : zooms)
    {
        Action act = z.a;
        QAction *a = zm->addAction(PL(z.t));
        connect(a, &QAction::triggered, this, [this, act]() { sendAction(act); });
    }
    wsqZoomButton->setMenu(zm);
    tb->addWidget(wsqZoomButton);

    // HDR settings (only visible for HDR content) and user script buttons (only when configured)
    for (QAction *a : keep)
    {
        if (a == ui.actionOpen || a == ui.actionSave_video || a == ui.actionProperties || a == ui.actionPlayFiltered)
            continue;
        if (a->isSeparator() && a != actionHDRSeparator)
            continue;
        tb->addAction(a);
    }
    // the toggle for filtered preview is not part of the design (kept in the Video menu)
    ui.actionPlayFiltered->setVisible(false);

    QWidget *spacer = new QWidget(tb);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;");
    tb->addWidget(spacer);
    wsqInfoLabel = new QLabel(tb);
    wsqInfoLabel->setObjectName("wsqInfoLabel");
    tb->addWidget(wsqInfoLabel);
}

/**
    \fn wsqBuildCentral
    \brief center the preview, add the VU column, hide replaced docks
*/
void MainWindow::wsqBuildCentral(void)
{
    // center the video frame: spacers left/right (in _10) and above/below (new column).
    // The frame may shrink below the video size; the zoom follows on resize.
    ui.frame_video->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui.frame_video->setMinimumSize(0, 0);
    int fi = ui._10->indexOf(ui.frame_video);
    ui._10->removeWidget(ui.frame_video);
    QVBoxLayout *col = new QVBoxLayout();
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);
    col->addStretch(1);
    col->addWidget(ui.frame_video, 1000);
    col->addStretch(1);
    ui._10->insertLayout(fi < 0 ? 0 : fi, col, 1000);
    ui._10->insertStretch(0, 1);
    ui._10->setStretch(ui._10->count() - 1, 1); // the trailing spacer from the .ui
    wsqFlexibleVideoFrame = true;

    // VU column on the right of the preview
    wsqVuColumn = new QWidget(ui.centralwidget);
    wsqVuColumn->setObjectName("wsqVuColumn");
    wsqVuColumn->setAttribute(Qt::WA_StyledBackground, true);
    wsqVuColumn->setFixedWidth(44);
    QVBoxLayout *vc = new QVBoxLayout(wsqVuColumn);
    vc->setContentsMargins(0, 14, 0, 10);
    vc->setSpacing(8);
    wsqVu = new WsqVuBars(wsqVuColumn);
    vc->addWidget(wsqVu, 1);
    ui.toolButtonAudioToggle->setParent(wsqVuColumn);
    ui.toolButtonAudioToggle->setIconSize(QSize(14, 14));
    vc->addWidget(ui.toolButtonAudioToggle, 0, Qt::AlignHCenter);
    ui.horizontalLayout_3->setContentsMargins(9, 9, 0, 0);
    ui.horizontalLayout_3->addWidget(wsqVuColumn);
    wsqHelper->volumeButton = ui.toolButtonAudioToggle;
    wsqHelper->onVolumeWheel = [this](int d) {
        QSlider *s = ui.horizontalSlider_2;
        s->setValue(qBound(s->minimum(), s->value() + d, s->maximum()));
        ui.toolButtonAudioToggle->setToolTip(PL("Głośność: %1% (kółko myszy)").arg(s->value()));
    };
    ui.toolButtonAudioToggle->installEventFilter(wsqHelper);
    ui.toolButtonAudioToggle->setToolTip(PL("Głośność: %1% (kółko myszy)").arg(ui.horizontalSlider_2->value()));

    wsqWindow = this;
    wsqVuHook = wsqVuFeed;

    // docks replaced by the VU column / selection group
    ui.audioMetreWidget->hide();
    ui.volumeWidget->hide();
}

/**
    \fn wsqSetup
    \brief Build the WsQ UI. Called once from the constructor.
*/
void MainWindow::wsqSetup(void)
{
    wsqHelper = new WsqHelper(this);

    wsqBuildPanel();
    wsqBuildNav();
    wsqBuildToolbar();
    wsqBuildCentral();

    /* --- Icon rail shown while the settings panel is collapsed --- */
    wsqRail = new QToolBar(PL("Panel"), this);
    wsqRail->setObjectName("wsqRail");
    wsqRail->setMovable(false);
    wsqRail->setFloatable(false);
    wsqRail->setOrientation(Qt::Vertical);
    wsqRail->setIconSize(QSize(18, 18));
    wsqRail->toggleViewAction()->setVisible(false);
    addToolBar(Qt::LeftToolBarArea, wsqRail);

    QAction *a = wsqRail->addAction(WSQ_ICON(expand), PL("Rozwiń panel (F9)"));
    connect(a, SIGNAL(triggered()), this, SLOT(wsqTogglePanel()));
    if (QWidget *w = wsqRail->widgetForAction(a))
        w->setObjectName("wsqRailExpand");
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
        if (b == ui.pushButtonVideoFilter)
        {
            if (QWidget *w = wsqRail->widgetForAction(ra))
            {
                wsqRailBadge = new QLabel("0", w);
                wsqRailBadge->setObjectName("wsqRailBadge");
                wsqRailBadge->setAlignment(Qt::AlignCenter);
                wsqRailBadge->setFixedSize(14, 14);
                wsqRailBadge->move(w->sizeHint().width() - 14, 0);
                wsqRailBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
                wsqRailBadge->hide();
            }
        }
    }
    QWidget *stretch = new QWidget(wsqRail);
    stretch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    stretch->setStyleSheet("background: transparent;");
    wsqRail->addWidget(stretch);
    QAction *railSave = wsqRail->addAction(WSQ_ICON(save_dark), PL("Zapisz wideo"));
    connect(railSave, &QAction::triggered, this, [this]() { sendAction(ACT_SAVE_VIDEO); });
    if (QWidget *w = wsqRail->widgetForAction(railSave))
        w->setObjectName("wsqRailSave");
    railSave->setEnabled(false);
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

    /* --- poll the filter chain size for the badges --- */
    QTimer *t = new QTimer(this);
    t->setInterval(700);
    connect(t, &QTimer::timeout, this, [this]() { wsqPollFilters(); });
    t->start();

    wsqUpdateFrameLabels();
}

/**
    \fn wsqPollFilters
*/
void MainWindow::wsqPollFilters(void)
{
    int n = avifileinfo ? (int)ADM_VideoFilters.size() : 0;
    if (n == wsqLastFilterCount)
        return;
    wsqLastFilterCount = n;
    for (QLabel *b : {wsqFilterBadge, wsqRailBadge})
    {
        if (!b)
            continue;
        b->setText(QString::number(n));
        b->setVisible(n > 0);
    }
}

/**
    \fn wsqVuUpdate
*/
void MainWindow::wsqVuUpdate(const int32_t *volume)
{
    if (wsqVu)
        wsqVu->setVolumes(volume);
}

/**
    \fn wsqSetCurrentPts
*/
void MainWindow::wsqSetCurrentPts(uint64_t pts)
{
    wsqCurPts = pts;
    wsqUpdateFrameLabels();
}

/**
    \fn wsqSetTotalPts
*/
void MainWindow::wsqSetTotalPts(uint64_t pts)
{
    wsqTotalPts = pts;
    wsqUpdateFrameLabels();
}

/**
    \fn wsqSetMarkers
*/
void MainWindow::wsqSetMarkers(uint64_t a, uint64_t b)
{
    wsqMarkA = a;
    wsqMarkB = b;
    wsqUpdateFrameLabels();
}

/**
    \fn wsqUpdateFrameLabels
    \brief "Klatka N / T" and "Wybrano K klatek" in the status bar
*/
void MainWindow::wsqUpdateFrameLabels(void)
{
    if (!wsqFrameLabel || !wsqSelLabel)
        return;
    if (!avifileinfo || !avifileinfo->fps1000)
    {
        wsqFrameLabel->hide();
        wsqSelLabel->hide();
        return;
    }
    double fps = avifileinfo->fps1000 / 1000.0;
    uint64_t cur = (uint64_t)llround(wsqCurPts / 1000000.0 * fps);
    uint64_t tot = (uint64_t)llround(wsqTotalPts / 1000000.0 * fps);
    uint64_t sel = wsqMarkB > wsqMarkA ? (uint64_t)llround((wsqMarkB - wsqMarkA) / 1000000.0 * fps) : 0;
    wsqFrameLabel->setText(PL("Klatka %1 / %2").arg(wsqGroupDigits(cur), wsqGroupDigits(tot)));
    wsqSelLabel->setText(PL("Wybrano %1 klatek").arg(wsqGroupDigits(sel)));
    wsqFrameLabel->show();
    wsqSelLabel->show();
}

/**
    \fn wsqUpdateInfo
    \brief toolbar media info, zoom pill, decoder card; called from updateStatusBarInfo
*/
void MainWindow::wsqUpdateInfo(void)
{
    if (wsqZoomButton)
        wsqZoomButton->setText(PL("Zoom: %1%").arg(statusBarInfo_Zoom));
    QPushButton *decConf = wsqDecoderLabel ? (QPushButton *)wsqDecoderLabel->property("wsqDecConf").value<void *>()
                                           : nullptr;
    if (!avifileinfo)
    {
        if (wsqInfoLabel)
            wsqInfoLabel->clear();
        if (wsqDecoderLabel)
            wsqDecoderLabel->setText(PL("—"));
        if (decConf)
            decConf->setEnabled(false);
        wsqUpdateFrameLabels();
        return;
    }
    QString vcodec;
    if (video_body)
    {
        char *c = video_body->getVideoCodec();
        if (c)
        {
            vcodec = QString::fromLatin1(c).trimmed();
            ADM_dealloc(c);
        }
    }
    QString info = PL("%1×%2 · %3 fps").arg(avifileinfo->width).arg(avifileinfo->height)
                       .arg(QString::number(avifileinfo->fps1000 / 1000.0, 'f', avifileinfo->fps1000 % 1000 ? 3 : 0));
    if (!vcodec.isEmpty())
        info += PL(" · ") + vcodec;
    ADM_audioStream *as = nullptr;
    if (video_body && video_body->getDefaultAudioTrack(&as) && as)
    {
        WAVHeader *h = as->getInfo();
        if (h)
        {
            QString ch = h->channels == 6 ? "5.1" : (h->channels == 8 ? "7.1" : QString("%1.0").arg(h->channels));
            info += PL(" · %1 %2").arg(QString::fromUtf8(getStrFromAudioCodec(h->encoding)), ch);
        }
    }
    if (wsqInfoLabel)
        wsqInfoLabel->setText(info);
    if (wsqDecoderLabel)
    {
        QString d = statusBarInfo_Decoder.isEmpty() ? PL("—") : statusBarInfo_Decoder.toLower();
        if (!vcodec.isEmpty())
            d += PL(" · ") + vcodec;
        wsqDecoderLabel->setText(d);
    }
    if (decConf)
        decConf->setEnabled(!strcmp(video_body->getVideoDecoderName(), "Lavcodec"));
    wsqUpdateFrameLabels();
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
    \fn wsqExtraWidth
    \brief width taken by the icon rail and the VU column, for zoom calculations
*/
uint32_t MainWindow::wsqExtraWidth(void)
{
    uint32_t w = 0;
    if (wsqRail && wsqRail->isVisible())
        w += wsqRail->frameSize().width();
    if (wsqVuColumn && wsqVuColumn->isVisible())
        w += wsqVuColumn->width();
    return w;
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
    \brief show the rail exactly when the settings panel is explicitly hidden
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
    if (!loaded)
    {
        wsqCurPts = wsqTotalPts = wsqMarkA = wsqMarkB = 0;
        if (wsqVu)
        {
            int32_t silent[8] = {255, 255, 255, 255, 255, 255, 255, 255};
            wsqVu->setVolumes(silent);
        }
    }
    wsqPollFilters();
    wsqUpdateInfo();
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
    p.setColor(QPalette::Highlight, QColor(0x1e, 0x35, 0x57));
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
    {
        QFontMetrics fm(ui.currentTime->font());
        ui.currentTime->setFixedSize(fm.horizontalAdvance(PL("00:00:00.000")) + 26, 32);
    }

    defaultThemeAction->setChecked(false);
    lightThemeAction->setChecked(false);
    darkThemeAction->setChecked(false);
    wsqThemeAction->setChecked(true);
    wsqArrangeNav(true);
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
    \fn wsqApplySavedGeometry
    \brief restore the saved normal geometry (Qt's own format, handles the window frame and DPI)
*/
static bool wsqApplySavedGeometry(QMainWindow *w, const QByteArray &g)
{
    if (g.isEmpty())
        return false;
    if (!w->restoreGeometry(g))
        return false;
    // Avidemux maximizes the window itself (see UI_RunApp), keep only the normal geometry here
    if (w->windowState() & Qt::WindowMaximized)
        w->setWindowState(w->windowState() & ~Qt::WindowMaximized);
    return true;
}

/**
    \fn wsqRestoreGeometry
    \brief restore the last normal window geometry, or pick a sensible default
*/
void MainWindow::wsqRestoreGeometry(QSettings *qset)
{
    // docks replaced by the WsQ layout stay hidden even if an old state shows them
    ui.selectionWidget->hide();
    ui.audioMetreWidget->hide();
    ui.volumeWidget->hide();
    wsqPanelVisibilityChanged(true); // dock state was just restored
    wsqSetKeepWindowSize(qset ? qset->value("wsqKeepWindowSize", true).toBool() : true);

    wsqSavedGeometry = qset ? qset->value("wsqGeometryBytes", QByteArray()).toByteArray() : QByteArray();
    if (wsqApplySavedGeometry(this, wsqSavedGeometry))
        return;

    // older WsQ builds stored a plain rectangle
    QRect r = qset ? qset->value("wsqGeometry", QRect()).toRect() : QRect();
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
        wsqSavedGeometry = saveGeometry();
        return;
    }
    screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    QRect avail = screen->availableGeometry();
    QSize s(qMin(1440, avail.width() * 9 / 10), qMin(900, avail.height() * 9 / 10));
    resize(s);
    move(avail.center() - QPoint(s.width() / 2, s.height() / 2));
}

/**
    \fn wsqReapplyGeometry
    \brief start-up code (DXVA probing, settings, first layout) may still resize the window
           after it was restored; put the saved size back once everything has settled
*/
void MainWindow::wsqReapplyGeometry(void)
{
    if (wsqSavedGeometry.isEmpty())
        return;
    auto apply = [this]() {
        if (isMaximized() || isFullScreen() || isMinimized())
            return;
        wsqApplySavedGeometry(this, wsqSavedGeometry);
    };
    QTimer::singleShot(0, this, apply);
    QTimer::singleShot(400, this, apply);
}

/**
    \fn wsqSaveGeometry
*/
void MainWindow::wsqSaveGeometry(QSettings *qset)
{
    if (!qset)
        return;
    qset->setValue("wsqGeometryBytes", saveGeometry());
    qset->remove("wsqGeometry");
    qset->setValue("wsqKeepWindowSize", wsqKeepWindowSize());
}
