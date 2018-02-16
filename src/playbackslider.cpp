/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include "playbackslider.h"
#include <QStyleOption>
#include <QPainter>
#include <QApplication>
#include <QDebug>

PlaybackSlider::PlaybackSlider(QWidget *parent) :
	QSlider(parent)
{

}

PlaybackSlider::~PlaybackSlider()
{

}

void PlaybackSlider::setHighlightRegion(int start, int end)
{
	highlightStart = start;
	highlightEnd = end;
	repaint();
}

void PlaybackSlider::paintEvent(QPaintEvent *ev) {
	QStyleOptionSlider opt;
	int start;
	int end;
	initStyleOption(&opt);

	opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
	if (tickPosition() != NoTicks) {
		opt.subControls |= QStyle::SC_SliderTickmarks;
	}

	QRect groove_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);

	//Reverse the bar if the slider is set to reverse direction
	if(QSlider::invertedAppearance())
	{
		start = groove_rect.width() * (double)(QSlider::maximum() - highlightEnd) / QSlider::maximum();
		end = groove_rect.width() * (double)(QSlider::maximum() - highlightStart) / QSlider::maximum();
	}
	else
	{
		start = groove_rect.width() * (double)highlightStart / QSlider::maximum();
		end = groove_rect.width() * (double)highlightEnd / QSlider::maximum();
	}

	QRect rect(groove_rect.left() + start, groove_rect.top(), end, groove_rect.height());
	QPainter painter(this);
	painter.fillRect(rect, QBrush(Qt::red));

	QSlider::paintEvent(ev);
}
