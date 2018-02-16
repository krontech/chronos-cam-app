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

#define HANDLE_HEIGHT 30

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
	if(!QSlider::invertedAppearance())//slider is not inverted, and position 0 is at top of screen
	{
		start	= (groove_rect.height() - HANDLE_HEIGHT)	* (double)(QSlider::maximum() - highlightEnd)		/ QSlider::maximum();
		end		= (groove_rect.height() - HANDLE_HEIGHT)	* (double)(QSlider::maximum() - highlightStart)		/ QSlider::maximum();
		//Subtract the handle height because the start or end region should line up with the middle of the handle,
		//and the middle if the handle cannot be moved to the very top or bottom of the screen
	}
	else
	{
		start = groove_rect.height()* (double)highlightStart / QSlider::maximum();
		end = groove_rect.height()	* (double)highlightEnd / QSlider::maximum();
	}

	//specify (left, top, width, height) of the rectangle to highlight
	QRect rect(groove_rect.left(), HANDLE_HEIGHT/2 + end, groove_rect.width() + 2, start - end);
	QPainter painter(this);
	painter.fillRect(rect, QBrush(Qt::red));

	QSlider::paintEvent(ev);
}
