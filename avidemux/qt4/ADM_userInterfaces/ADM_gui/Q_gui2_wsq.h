/***************************************************************************
    Avidemux WsQ UI additions (theme, card panel, timeline, navigation bar,
    VU column, icon rail, remembered window geometry)
 ***************************************************************************/
#ifndef Q_gui2_wsq_h
#define Q_gui2_wsq_h

#include <functional>
#include <QAction>
#include <QDockWidget>
#include <QLayout>
#include <QList>
#include <QObject>
#include <QPair>
#include <QRect>
#include <QVector>
#include <QWidget>

#define ADM_QT_THEME_WSQ 3

/* Accent colour used by custom painted widgets */
#define WSQ_ACCENT QColor(0x4a, 0x9b, 0xff)

/**
    \class WsqVuBars
    \brief thin vertical level bars (one per active audio channel)
*/
class WsqVuBars : public QWidget
{
  public:
    explicit WsqVuBars(QWidget *parent) : QWidget(parent)
    {
    }
    void setVolumes(const int32_t *volume);

  protected:
    void paintEvent(QPaintEvent *ev) override;

  private:
    QVector<float> levels;
    QVector<float> peaks;
};

/**
    \class WsqHelper
    \brief event filter: navigation bar re-layout, enabled-state mirroring, misc.
*/
class WsqHelper : public QObject
{
  public:
    explicit WsqHelper(QObject *parent) : QObject(parent)
    {
    }
    bool eventFilter(QObject *watched, QEvent *event) override;

    QWidget *navContents = nullptr;
    QWidget *timeField = nullptr;
    QWidget *volumeButton = nullptr;
    std::function<void(void)> onNavResize;
    std::function<void(void)> onTimeDoubleClick;
    std::function<void(int)> onVolumeWheel;
    QList<QPair<QObject *, QAction *>> mirrors;
    QList<QPair<QObject *, QWidget *>> widgetMirrors;

  private:
    bool pending = false;
};

#endif
