#include "gui/wave_visual.hpp"
#include "gui/color_palette.hpp"
#include <limits>
#include <QPainter>
//Xcode 8.3: "no member names 'abs' in namespace 'std'
#include <cstdlib>

WaveVisual::WaveVisual(QWidget *parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_OpaquePaintEvent);
}

void WaveVisual::setColorPalette(std::shared_ptr<ColorPalette> palette)
{
	palette_ = palette;
}

void WaveVisual::setStereoSamples(const int16_t *buffer, size_t frames)
{
	// identify the highest of the 2 input signals
	int32_t sum = 0;
	for (size_t i = 0; i < frames; ++i) {
		size_t p = i << 1;
		sum += std::abs(buffer[p]);
		sum -= std::abs(buffer[p + 1]);
	}

	// use this signal as display data
	samples_.resize(frames);
	int16_t *samples = samples_.data();
	for (size_t i = 0; i < frames; ++i)
		samples[i] = buffer[(i << 1) + (sum < 0)];

	repaint();
}

void WaveVisual::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);

	if (!palette_)
		return;

	int w = width();
	int h = height();
	int centerh = h >> 1;
	painter.fillRect(0, 0, w, h, palette_->wavBackColor);

	const int16_t *samples = samples_.data();
	size_t frames = samples_.size();
	if (frames <= 0)
		return;

	painter.setPen(palette_->wavDrawColor);

	int lastY = centerh;
	const int16_t range = std::numeric_limits<int16_t>::max() >> 1;
	for (int x = 0; x < w; ++x) {
		size_t index = (size_t)(x * ((double)frames / w));

		int16_t sample = samples[index];
		int y = centerh - (centerh * sample  / range);
		painter.drawPoint(x, y);

		// draw intermediate points
		int y1 = lastY, y2 = y;
		int yM = (y1 + y2) >> 1;
		while (y1 != y2) {
			bool b = (y1 > yM) ^ (y1 > y2);
			painter.drawPoint(x - !b, y1);
			y1 += (y1 < y2) ? 1 : -1;
		}

		lastY = y;
	}
}
