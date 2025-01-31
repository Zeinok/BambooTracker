#pragma once

#include <utility>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#if !defined(DECL_MAYBE_UNUSED) && defined(__GNUC__)
#define DECL_MAYBE_UNUSED __attribute__((unused))
#elif !defined(DECL_MAYBE_UNUSED)
#define DECL_MAYBE_UNUSED
#endif

enum class SoundSource : int
{
	FM = 1,
	SSG = 2,
	DRUM = 4,
	ADPCM = 8
};

enum class SongType : int
{
	Standard,
	FM3chExpanded
};

enum class Note : int
{
	C	= 0,
	CS	= 32,
	D	= 64,
	DS	= 96,
	E	= 128,
	F	= 160,
	FS	= 192,
	G	= 224,
	GS	= 256,
	A	= 288,
	AS	= 320,
	B	= 352
};

enum class FMEnvelopeTextType : int
{
	Skip, AL, FB,
	AR1, DR1, SR1, RR1, SL1, TL1, KS1, ML1, DT1,
	AR2, DR2, SR2, RR2, SL2, TL2, KS2, ML2, DT2,
	AR3, DR3, SR3, RR3, SL3, TL3, KS3, ML3, DT3,
	AR4, DR4, SR4, RR4, SL4, TL4, KS4, ML4, DT4
};

enum class FMOperatorType : int
{
	All, Op1, Op2, Op3, Op4
};

enum class SSGWaveformType : int
{
	UNSET = -1,
	SQUARE = 0,
	TRIANGLE = 1,
	SAW = 2,
	INVSAW = 3,
	SQM_TRIANGLE = 4,
	SQM_SAW = 5,
	SQM_INVSAW = 6
};

enum class EffectDisplayControl
{
	Unset, ReverseFMVolumeDelay, ReverseFMBrightness
};

enum class RealChipInterface : int
{
	NONE = 0,
	SCCI = 1,
	C86CTL = 2
};

DECL_MAYBE_UNUSED
static std::pair<int, Note> noteNumberToOctaveAndNote(int num)
{
	if (num < 0) return std::make_pair(0, Note::C);

	int oct = num / 12;
	if (oct > 7) return std::make_pair(7, Note::B);

	Note note;
	switch (num % 12) {
	case 0:		note = Note::C;		break;
	case 1:		note = Note::CS;	break;
	case 2:		note = Note::D;		break;
	case 3:		note = Note::DS;	break;
	case 4:		note = Note::E;		break;
	case 5:		note = Note::F;		break;
	case 6:		note = Note::FS;	break;
	case 7:		note = Note::G;		break;
	case 8:		note = Note::GS;	break;
	case 9:		note = Note::A;		break;
	case 10:	note = Note::AS;	break;
	case 11:	note = Note::B;		break;
	}

	return std::make_pair(oct, note);
}

DECL_MAYBE_UNUSED
static int octaveAndNoteToNoteNumber(int octave, Note note)
{
	int ret = 12 * octave;

	switch (note) {
	case Note::C:	ret += 0;	break;
	case Note::CS:	ret += 1;	break;
	case Note::D:	ret += 2;	break;
	case Note::DS:	ret += 3;	break;
	case Note::E:	ret += 4;	break;
	case Note::F:	ret += 5;	break;
	case Note::FS:	ret += 6;	break;
	case Note::G:	ret += 7;	break;
	case Note::GS:	ret += 8;	break;
	case Note::A:	ret += 9;	break;
	case Note::AS:	ret += 10;	break;
	case Note::B:	ret += 11;	break;
	}

	return ret;
}

DECL_MAYBE_UNUSED
static int ctohex(const char c)
{
	if (c == '0')		return 0;
	else if (c == '1')	return 1;
	else if (c == '2')	return 2;
	else if (c == '3')	return 3;
	else if (c == '4')	return 4;
	else if (c == '5')	return 5;
	else if (c == '6')	return 6;
	else if (c == '7')	return 7;
	else if (c == '8')	return 8;
	else if (c == '9')	return 9;
	else if (c == 'A')	return 10;
	else if (c == 'B')	return 11;
	else if (c == 'C')	return 12;
	else if (c == 'D')	return 13;
	else if (c == 'E')	return 14;
	else if (c == 'F')	return 15;
	else				return -1;
}

DECL_MAYBE_UNUSED
static uint8_t uitobcd(const uint8_t v)
{
	if (v > 99) throw std::out_of_range("Out of range.");

	uint8_t high = v / 10;
	uint8_t low = v % 10;
	return static_cast<uint8_t>(high << 4) + low;
}

DECL_MAYBE_UNUSED
static uint8_t bcdtoui(const uint8_t v)
{
	uint8_t high = v >> 4;
	uint8_t low = v & 0x0f;
	return high * 10 + low;
}

DECL_MAYBE_UNUSED
inline static size_t getFMChannelCount(SongType type)
{
	switch (type) {
	case SongType::Standard:		return 6;
	case SongType::FM3chExpanded:	return 9;
	default:	throw std::invalid_argument("Invalid SongType.");
	}
}

DECL_MAYBE_UNUSED
inline static bool isModulatedWaveformSSG(SSGWaveformType type)
{
	switch (type) {
	case SSGWaveformType::SQUARE:
	case SSGWaveformType::TRIANGLE:
	case SSGWaveformType::SAW:
	case SSGWaveformType::INVSAW:
		return false;
	case SSGWaveformType::SQM_TRIANGLE:
	case SSGWaveformType::SQM_SAW:
	case SSGWaveformType::SQM_INVSAW:
		return true;
	default:
		throw std::invalid_argument("Invalid SSGWaveformType");
	}
}

DECL_MAYBE_UNUSED
inline static bool isModulatedWaveformSSG(int id)
{
	try {
		return isModulatedWaveformSSG(static_cast<SSGWaveformType>(id));
	} catch (...) {
		throw std::invalid_argument("Invalid id");
	}
}

template <typename T>
DECL_MAYBE_UNUSED
inline const T& clamp(const T& value, const T& low, const T& high)
{
	return std::min(std::max(value, low), high);
}

DECL_MAYBE_UNUSED
inline static int calcADPCMDeltaN(unsigned int rate)
{
	return static_cast<int>(std::round((rate << 16) / 55500.));
}
