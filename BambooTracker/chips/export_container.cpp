#include "export_container.hpp"
#include "export_handler.hpp"
#include <algorithm>

namespace chip
{
	ExportContainerInterface::~ExportContainerInterface() {}

	//******************************//
	WavExportContainer::WavExportContainer()
	{
	}

	void WavExportContainer::recordRegisterChange(uint32_t offset, uint8_t value)
	{
		(void)offset;
		(void)value;
	}

	void WavExportContainer::recordStream(int16_t* stream, size_t nSamples)
	{
		std::copy(stream, stream + (nSamples << 1), std::back_inserter(samples_));
	}

	bool WavExportContainer::empty() const
	{
		return samples_.empty();
	}

	void WavExportContainer::clear()
	{
		samples_.clear();
	}

	std::vector<int16_t> WavExportContainer::getStream() const
	{
		return samples_;
	}

	//******************************//
	VgmExportContainer::VgmExportContainer(int target, uint32_t intrRate)
		: target_(target),
		  lastWait_(0),
		  totalSampCnt_(0),
		  intrRate_(intrRate),
		  isSetLoop_(false),
		  loopPoint_(0)
	{
	}

	void VgmExportContainer::recordRegisterChange(uint32_t offset, uint8_t value)
	{
		if (lastWait_) setWait();

		const int fm = target_ & Export_FmMask;
		const int ssg = target_ & Export_SsgMask;

		const uint8_t cmdSsg =
			(ssg != Export_InternalSsg) ? 0xa0 : (fm == Export_YM2608) ? 0x56 :
			(fm == Export_YM2203) ? 0x55 : 0x00;
		const uint8_t cmdFmPortA =
			(fm == Export_YM2608) ? 0x56 : (fm == Export_YM2612) ? 0x52 :
			(fm == Export_YM2203) ? 0x55 : 0x00;
		const uint8_t cmdFmPortB =
			(fm == Export_YM2608) ? 0x57 : (fm == Export_YM2612) ? 0x53 :
			0x00;

		if (cmdSsg && offset < 0x10) {
			buf_.push_back(cmdSsg);
			buf_.push_back(offset);
			buf_.push_back(value);
		}
		else if (cmdFmPortA && (offset & 0x100) == 0) {
			bool compatible = true;

			if (offset == 0x28) { // Key register
				if (fm == Export_YM2203 && (value & 7) >= 3)
					compatible = false;
			}
			else if (offset == 0x29) // Mode register
				compatible = fm == Export_YM2608;
			else if ((offset & 0xf0) == 0x10) // Rhythm section
				compatible = fm == Export_YM2608;

			if (compatible) {
				buf_.push_back(cmdFmPortA);
				buf_.push_back(offset & 0xff);
				buf_.push_back(value);
			}
		}
		else if (cmdFmPortB && (offset & 0x100) != 0) {
			bool compatible = true;

			if (offset < 0x10) // ADPCM section
				compatible = fm == Export_YM2608;

			if (compatible) {
				buf_.push_back(cmdFmPortB);
				buf_.push_back(offset & 0xff);
				buf_.push_back(value);
			}
		}
	}

	void VgmExportContainer::recordStream(int16_t* stream, size_t nSamples)
	{
		(void)stream;
		lastWait_ += nSamples;
		totalSampCnt_ += nSamples;
	}

	void VgmExportContainer::clear()
	{
		buf_.clear();
		lastWait_ = 0;
		totalSampCnt_ = 0;
		isSetLoop_ = false;
		loopPoint_ = 0;
	}

	bool VgmExportContainer::empty() const
	{
		return (buf_.empty() || lastWait_ != 0);
	}

	std::vector<uint8_t> VgmExportContainer::getData()
	{
		if (lastWait_) setWait();
		return buf_;
	}

	size_t VgmExportContainer::getSampleLength() const
	{
		return totalSampCnt_;
	}

	size_t VgmExportContainer::setLoopPoint()
	{
		if (lastWait_) setWait();
		isSetLoop_ = true;
		return loopPoint_;
	}

	size_t VgmExportContainer::forceMoveLoopPoint()
	{
		loopPoint_ = buf_.size();
		return loopPoint_;
	}

	void VgmExportContainer::setDataBlock(std::vector<uint8_t> data)
	{
		buf_.push_back(0x67);
		buf_.push_back(0x66);
		buf_.push_back(0x81);
		size_t blockSize = data.size() + 8;
		buf_.push_back(blockSize & 0xff);
		buf_.push_back((blockSize >> 8) & 0xff);
		buf_.push_back((blockSize >> 16) & 0xff);
		buf_.push_back(blockSize >> 24);
		buf_.push_back(data.size() & 0xff);
		buf_.push_back((data.size() >> 8) & 0xff);
		buf_.push_back((data.size() >> 16) & 0xff);
		buf_.push_back(data.size() >> 24);
		buf_.resize(buf_.size() + 4);	// Start address is 0
		std::copy(data.begin(), data.end(), std::back_inserter(buf_));
	}

	void VgmExportContainer::setWait()
	{
		while (lastWait_) {
			uint32_t sub;

			if (intrRate_ == 50) {
				if (lastWait_ > 65535) {
					uint32_t tmp = lastWait_ - 65535;
					if (tmp <= 882) {
						//65535 - (882 - tmp)
						sub = 64653 + tmp;
					}
					else if (tmp <= 1764) {
						//65535 - (1764 - tmp)
						sub = 63771 + tmp;
					}
					else if (tmp <= 2646) {
						//65535 - (2646 - tmp)
						sub = 62889 + tmp;
					}
					else {
						sub = 65535;
					}
					buf_.push_back(0x61);
					buf_.push_back(sub & 0x00ff);
					buf_.push_back(sub >> 8);
				}
				else {
					if (lastWait_ <= 16) {
						buf_.push_back(0x70 | (lastWait_ - 1));
					}
					else if (lastWait_ > 2646) {
						buf_.push_back(0x61);
						buf_.push_back(lastWait_ & 0x00ff);
						buf_.push_back(lastWait_ >> 8);
					}
					else if (lastWait_ == 2646) {
						buf_.push_back(0x63);
						buf_.push_back(0x63);
						buf_.push_back(0x63);
					}
					else if (1764 <= lastWait_ && lastWait_ <= 1780) {
						uint32_t tmp = lastWait_ - 1764;
						buf_.push_back(0x63);
						buf_.push_back(0x63);
						if (tmp) buf_.push_back(0x70 | (tmp - 1));
					}
					else if (882 <= lastWait_ && lastWait_ <= 898) {
						uint32_t tmp = lastWait_ - 882;
						buf_.push_back(0x63);
						if (tmp) buf_.push_back(0x70 | (tmp - 1));
					}
					else {
						buf_.push_back(0x61);
						buf_.push_back(lastWait_ & 0x00ff);
						buf_.push_back(lastWait_ >> 8);
					}
					sub = lastWait_;
				}
			}
			else if (intrRate_ == 60) {
				if (lastWait_ > 65535) {
					uint32_t tmp = lastWait_ - 65535;
					if (tmp <= 735) {
						//65535 - (735 - tmp)
						sub = 64800 + tmp;
					}
					else if (tmp <= 1470) {
						//65535 - (1470 - tmp)
						sub = 64065 + tmp;
					}
					else if (tmp <= 2205) {
						//65535 - (2205 - tmp)
						sub = 63330 + tmp;
					}
					else {
						sub = 65535;
					}
					buf_.push_back(0x61);
					buf_.push_back(sub & 0x00ff);
					buf_.push_back(sub >> 8);
				}
				else {
					if (lastWait_ <= 16) {
						buf_.push_back(0x70 | (lastWait_ - 1));
					}
					else if (lastWait_ > 2205) {
						buf_.push_back(0x61);
						buf_.push_back(lastWait_ & 0x00ff);
						buf_.push_back(lastWait_ >> 8);
					}
					else if (lastWait_ == 2205) {
						buf_.push_back(0x62);
						buf_.push_back(0x62);
						buf_.push_back(0x62);
					}
					else if (1470 <= lastWait_ && lastWait_ <= 1486) {
						uint32_t tmp = lastWait_ - 1470;
						buf_.push_back(0x62);
						buf_.push_back(0x62);
						if (tmp) buf_.push_back(0x70 | (tmp - 1));
					}
					else if (735 <= lastWait_ && lastWait_ <= 751) {
						uint32_t tmp = lastWait_ - 735;
						buf_.push_back(0x62);
						if (tmp) buf_.push_back(0x70 | (tmp - 1));
					}
					else {
						buf_.push_back(0x61);
						buf_.push_back(lastWait_ & 0x00ff);
						buf_.push_back(lastWait_ >> 8);
					}
					sub = lastWait_;
				}
			}
			else {
				if (lastWait_ > 65535) {
					sub = 65535;
					buf_.push_back(0x61);
					buf_.push_back(sub & 0x00ff);
					buf_.push_back(sub >> 8);
				}
				else {
					buf_.push_back(0x61);
					buf_.push_back(lastWait_ & 0x00ff);
					buf_.push_back(lastWait_ >> 8);
				}
				sub = lastWait_;
			}

			lastWait_ -= sub;
		}

		if (!isSetLoop_) loopPoint_ = buf_.size();
	}

	//******************************//
	S98ExportContainer::S98ExportContainer(int target)
		: target_(target),
		  lastWait_(0),
		  totalSampCnt_(0),
		  isSetLoop_(false),
		  loopPoint_(0)
	{
	}

	void S98ExportContainer::recordRegisterChange(uint32_t offset, uint8_t value)
	{
		if (lastWait_) setWait();

		const int fm = target_ & Export_FmMask;
		const int ssg = target_ & Export_SsgMask;

		const uint8_t cmdSsg =
			(ssg != Export_InternalSsg) ? 0x02 : (fm == Export_YM2608) ? 0x00 :
			(fm == Export_YM2203) ? 0x00 : 0xff;
		const uint8_t cmdFmPortA = 0x00;
		const uint8_t cmdFmPortB =
			(fm == Export_YM2608 || fm == Export_YM2612) ? 0x01 : 0xff;

		if (cmdSsg != 0xff && offset < 0x10) {
			buf_.push_back(cmdSsg);
			buf_.push_back(offset);
			buf_.push_back(value);
		}
		else if (cmdFmPortA != 0xff && (offset & 0x100) == 0) {
			bool compatible = true;

			if (offset == 0x28) { // Key register
				if (fm == Export_YM2203 && (value & 7) >= 3)
					compatible = false;
			}
			else if (offset == 0x29) // Mode register
				compatible = fm == Export_YM2608;
			else if ((offset & 0xf0) == 0x10) // Rhythm section
				compatible = fm == Export_YM2608;

			if (compatible) {
				buf_.push_back(cmdFmPortA);
				buf_.push_back(offset & 0xff);
				buf_.push_back(value);
			}
		}
		else if (cmdFmPortB != 0xff && (offset & 0x100) != 0) {
			bool compatible = true;

			if (offset < 0x10) // ADPCM section
				compatible = fm == Export_YM2608;

			if (compatible) {
				buf_.push_back(cmdFmPortB);
				buf_.push_back(offset & 0xff);
				buf_.push_back(value);
			}
		}
	}

	void S98ExportContainer::recordStream(int16_t* stream, size_t nSamples)
	{
		(void)stream;
		lastWait_ += nSamples;
		totalSampCnt_ += nSamples;
	}

	void S98ExportContainer::clear()
	{
		buf_.clear();
		lastWait_ = 0;
		totalSampCnt_ = 0;
		isSetLoop_ = false;
		loopPoint_ = 0;
	}

	bool S98ExportContainer::empty() const
	{
		return (buf_.empty() || lastWait_ != 0);
	}

	std::vector<uint8_t> S98ExportContainer::getData()
	{
		if (lastWait_) setWait();
		return buf_;
	}

	size_t S98ExportContainer::getSampleLength() const
	{
		return totalSampCnt_;
	}

	size_t S98ExportContainer::setLoopPoint()
	{
		if (lastWait_) setWait();
		isSetLoop_ = true;
		return loopPoint_;
	}

	size_t S98ExportContainer::forceMoveLoopPoint()
	{
		loopPoint_ = buf_.size();
		return loopPoint_;
	}

	void S98ExportContainer::setWait()
	{
		if (lastWait_ == 1) {
			buf_.push_back(0xff);
		}
		else {
			buf_.push_back(0xfe);
			lastWait_ -= 2;
			do {
				uint8_t b = lastWait_ & 0x7f;
				lastWait_ >>= 7;
				if (lastWait_ > 0) b |= 0x80;
				buf_.push_back(b);
			} while (lastWait_ > 0);
		}
		if (!isSetLoop_) loopPoint_ = buf_.size();
		lastWait_ = 0;
	}
}
