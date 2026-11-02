
/***************************************************************************
Custom slider
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "ADM_inttype.h"
#include <QPainter>
#include <QPainterPath>
#include <QSlider>
#include <QWheelEvent>
#include "ADM_mwNavSlider.h"
#include "ADM_assert.h"
#include "ADM_default.h"
bool ADM_mwNavSlider::wsqStyle = false;
/**
    \fn ADM_QSlider
*/
ADM_mwNavSlider::ADM_mwNavSlider(QWidget *parent) : ADM_flyNavSlider(parent)
{
    segments = NULL;
    numOfSegments = 0;
}
/**
    \fn dtor
*/
ADM_mwNavSlider::~ADM_mwNavSlider()
{
    if (segments)
        delete [] segments;
    segments = NULL;
}
/**
    \fn drawCutPoints
*/
void ADM_mwNavSlider::drawCutPoints(void)
{
    if (!segments || numOfSegments < 2 || !totalDuration)
        return;

    int pos, prevpos;
    prevpos = -1;
    QPainter painter(this);
    for (uint32_t i=1; i<numOfSegments; i++)
    {
        if (segments[i] == ADM_NO_PTS)
            continue;
        pos = (int)(((double)segments[i] * width()) / (double)totalDuration);
        if (pos < 1) pos = 1;
        if (pos > width() - 1) pos = width() - 1;
        if (pos == prevpos)
            painter.setPen(Qt::darkMagenta);
        else
            painter.setPen(Qt::red);
        prevpos = pos;

        if(layoutDirection() == Qt::LeftToRight)
            painter.drawLine(pos, 1, pos, height() - 3);
        else
            painter.drawLine(width() - pos, 1, width() - pos, height() - 3);
    }
}

/**
    \fn paintEvent
*/
void ADM_mwNavSlider::paintEvent(QPaintEvent *event)
{
    if (wsqStyle)
    {
        wsqPaint();
        return;
    }
    drawCutPoints();

    QSlider::paintEvent(event);

    drawSelection();
}
/**
    \fn setMarkerA
*/
void ADM_mwNavSlider::setMarkerA(uint64_t frameIndex)
{
    setMarkers(frameIndex, markerBTime);
}
/**
    \fn setMarkerB
*/
void ADM_mwNavSlider::setMarkerB(uint64_t frameIndex)
{
    setMarkers(markerATime, frameIndex);
}
/**
    \fn setMarkers
*/
void ADM_mwNavSlider::setMarkers(uint64_t frameIndexA, uint64_t frameIndexB)
{
    if (frameIndexA > totalDuration)
        printf("[ADM_QSlider] Marker A is out of bounds (%" PRIu64", %" PRIu64")\n", markerATime, totalDuration);
    else if (frameIndexB > totalDuration)
        printf("[ADM_QSlider] Marker B is out of bounds (%" PRIu64", %" PRIu64")\n", markerBTime, totalDuration);
    else
    {
        markerATime = frameIndexA;
        markerBTime = frameIndexB;

        repaint();
    }
}
/**
    \fn setTotalDuration
*/
void ADM_mwNavSlider::setTotalDuration(uint64_t duration)
{
    totalDuration = duration;
    repaint();
}
/**
    \fn setSegments
*/
void ADM_mwNavSlider::setSegments(uint32_t numOfSegs, uint64_t * segPts)
{
    if (segments)
        delete [] segments;
    segments = new uint64_t[numOfSegs];
    for (uint32_t i=0; i<numOfSegs; i++)
        segments[i] = segPts[i];
    numOfSegments = numOfSegs;
}
/*
    \fn wheelEvent
 */
void ADM_mwNavSlider::wheelEvent(QWheelEvent *e)
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    //printf("Wheel : %d\n",e->delta());
    emit sliderAction(e->delta());
#else
    emit sliderAction(e->angleDelta().ry());
#endif
}
//EOF

/**
    \fn wsqPaint
    \brief WsQ timeline: dark rounded bar, time grid ticks, cut points, A-B range, playhead
*/
void ADM_mwNavSlider::wsqPaint(void)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal W = width();
    const qreal H = height();
    const bool ltr = layoutDirection() == Qt::LeftToRight;
    auto X = [&](qreal frac) -> qreal { return ltr ? frac * W : W - frac * W; };

    QPainterPath clip;
    clip.addRoundedRect(r, 6, 6);
    p.fillPath(clip, QColor(0x16, 0x19, 0x1e));
    p.setClipPath(clip);

    // A-B range (only when the selection is not the whole video)
    if (totalDuration && markerBTime > markerATime && (markerATime > 0 || markerBTime < totalDuration))
    {
        qreal a = X((qreal)markerATime / totalDuration);
        qreal b = X((qreal)markerBTime / totalDuration);
        if (a > b)
            qSwap(a, b);
        QColor acc(0x4a, 0x9b, 0xff);
        QColor fill = acc;
        fill.setAlpha(46);
        p.fillRect(QRectF(a, 0, b - a, H), fill);
        p.fillRect(QRectF(a, 0, 2, H), acc);
        p.fillRect(QRectF(b - 2, 0, 2, H), acc);
        QFont f = font();
        f.setPixelSize(11);
        f.setBold(true);
        p.setFont(f);
        p.setPen(acc);
        p.drawText(QRectF(a + 5, 2, 20, 14), Qt::AlignLeft | Qt::AlignTop, "A");
        p.drawText(QRectF(b - 25, 2, 20, 14), Qt::AlignRight | Qt::AlignTop, "B");
    }

    // time grid ticks
    p.setPen(QPen(QColor(0x4a, 0x52, 0x5e), 1));
    for (int i = 1; i < 12; i++)
    {
        qreal x = qRound(W * i / 12.0) + 0.5;
        p.drawLine(QPointF(x, H - 9), QPointF(x, H - 2));
    }

    // cut points between segments
    if (segments && numOfSegments > 1 && totalDuration)
    {
        p.setPen(QPen(QColor(0xff, 0x9f, 0x43), 1));
        for (uint32_t i = 1; i < numOfSegments; i++)
        {
            if (segments[i] == ADM_NO_PTS)
                continue;
            qreal x = qRound(X((qreal)segments[i] / totalDuration)) + 0.5;
            p.drawLine(QPointF(x, 3), QPointF(x, H - 3));
        }
    }

    // playhead
    if (maximum() > minimum())
    {
        qreal frac = (qreal)(value() - minimum()) / (qreal)(maximum() - minimum());
        qreal x = X(frac);
        if (x < 1)
            x = 1;
        if (x > W - 2)
            x = W - 2;
        QColor ph = isEnabled() ? Qt::white : QColor(0x5d, 0x65, 0x73);
        p.fillRect(QRectF(x - 1, 0, 2, H), ph);
        p.setBrush(ph);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x - 6, -3, 12, 9), 3, 3);
    }
    p.setClipping(false);
    p.setPen(QPen(QColor(0x2a, 0x2f, 0x37), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 6, 6);
}
