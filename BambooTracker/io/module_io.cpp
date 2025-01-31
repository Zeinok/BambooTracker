#include "module_io.hpp"
#include <string>
#include "version.hpp"
#include "file_io_error.hpp"
#include "file_io.hpp"
#include "pitch_converter.hpp"
#include "effect.hpp"

ModuleIO::ModuleIO() {}

void ModuleIO::saveModule(BinaryContainer& ctr, std::weak_ptr<Module> mod,
						  std::weak_ptr<InstrumentsManager> instMan)
{
	std::shared_ptr<InstrumentsManager> instManLocked = instMan.lock();

	ctr.appendString("BambooTrackerMod");
	size_t eofOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy EOF offset
	uint32_t fileVersion = Version::ofModuleFileInBCD();
	ctr.appendUint32(fileVersion);


	/***** Module section *****/
	ctr.appendString("MODULE  ");
	size_t modOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy module section offset
	std::string modTitle = mod.lock()->getTitle();
	ctr.appendUint32(modTitle.length());
	if (!modTitle.empty()) ctr.appendString(modTitle);
	std::string author = mod.lock()->getAuthor();
	ctr.appendUint32(author.length());
	if (!author.empty()) ctr.appendString(author);
	std::string copyright = mod.lock()->getCopyright();
	ctr.appendUint32(copyright.length());
	if (!copyright.empty()) ctr.appendString(copyright);
	std::string comment = mod.lock()->getComment();
	ctr.appendUint32(comment.length());
	if (!comment.empty()) ctr.appendString(comment);
	ctr.appendUint32(mod.lock()->getTickFrequency());
	ctr.appendUint32(mod.lock()->getStepHighlight1Distance());
	ctr.appendUint32(mod.lock()->getStepHighlight2Distance());
	MixerType mixType = mod.lock()->getMixerType();
	ctr.appendUint8(static_cast<uint8_t>(mixType));
	if (mixType == MixerType::CUSTOM) {
		ctr.appendInt8(static_cast<int8_t>(mod.lock()->getCustomMixerFMLevel() * 10));
		ctr.appendInt8(static_cast<int8_t>(mod.lock()->getCustomMixerSSGLevel() * 10));
	}
	ctr.writeUint32(modOfs, ctr.size() - modOfs);


	/***** Instrument section *****/
	ctr.appendString("INSTRMNT");
	size_t instOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy instrument section offset
	std::vector<int> instIdcs = instManLocked->getEntriedInstrumentIndices();
	ctr.appendUint8(static_cast<uint8_t>(instIdcs.size()));
	for (auto& idx : instIdcs) {
		if (std::shared_ptr<AbstractInstrument> inst = instManLocked->getInstrumentSharedPtr(idx)) {
			ctr.appendUint8(static_cast<uint8_t>(inst->getNumber()));
			size_t iOfs = ctr.size();
			ctr.appendUint32(0);	// Dummy instrument block offset
			std::string name = inst->getName();
			ctr.appendUint32(name.length());
			if (!name.empty()) ctr.appendString(name);
			switch (inst->getSoundSource()) {
			case SoundSource::FM:
			{
				ctr.appendUint8(0x00);
				auto instFM = std::dynamic_pointer_cast<InstrumentFM>(inst);
				ctr.appendUint8(static_cast<uint8_t>(instFM->getEnvelopeNumber()));
				uint8_t tmp = static_cast<uint8_t>(instFM->getLFONumber());
				ctr.appendUint8(instFM->getLFOEnabled() ? tmp : (0x80 | tmp));
				for (auto& param : FileIO::ENV_FM_PARAMS) {
					tmp = static_cast<uint8_t>(instFM->getOperatorSequenceNumber(param));
					ctr.appendUint8(instFM->getOperatorSequenceEnabled(param) ? tmp : (0x80 | tmp));
				}
				tmp = static_cast<uint8_t>(instFM->getArpeggioNumber(FMOperatorType::All));
				ctr.appendUint8(instFM->getArpeggioEnabled(FMOperatorType::All) ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instFM->getPitchNumber(FMOperatorType::All));
				ctr.appendUint8(instFM->getPitchEnabled(FMOperatorType::All) ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instFM->getEnvelopeResetEnabled(FMOperatorType::All))
					  | static_cast<uint8_t>(instFM->getEnvelopeResetEnabled(FMOperatorType::Op1) << 1)
					  | static_cast<uint8_t>(instFM->getEnvelopeResetEnabled(FMOperatorType::Op2) << 2)
					  | static_cast<uint8_t>(instFM->getEnvelopeResetEnabled(FMOperatorType::Op3) << 3)
					  | static_cast<uint8_t>(instFM->getEnvelopeResetEnabled(FMOperatorType::Op4) << 4);
				ctr.appendUint8(tmp);
				for (auto& type : FileIO::OP_FM_TYPES) {
					tmp = static_cast<uint8_t>(instFM->getArpeggioNumber(type));
					ctr.appendUint8(instFM->getArpeggioEnabled(type) ? tmp : (0x80 | tmp));
				}
				for (auto& type : FileIO::OP_FM_TYPES) {
					tmp = static_cast<uint8_t>(instFM->getPitchNumber(type));
					ctr.appendUint8(instFM->getPitchEnabled(type) ? tmp : (0x80 | tmp));
				}
				break;
			}
			case SoundSource::SSG:
			{
				ctr.appendUint8(0x01);
				auto instSSG = std::dynamic_pointer_cast<InstrumentSSG>(inst);
				uint8_t tmp = static_cast<uint8_t>(instSSG->getWaveformNumber());
				ctr.appendUint8(instSSG->getWaveformEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instSSG->getToneNoiseNumber());
				ctr.appendUint8(instSSG->getToneNoiseEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instSSG->getEnvelopeNumber());
				ctr.appendUint8(instSSG->getEnvelopeEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instSSG->getArpeggioNumber());
				ctr.appendUint8(instSSG->getArpeggioEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instSSG->getPitchNumber());
				ctr.appendUint8(instSSG->getPitchEnabled() ? tmp : (0x80 | tmp));
				break;
			}
			case SoundSource::DRUM:
				break;
			case SoundSource::ADPCM:
			{
				ctr.appendUint8(0x02);
				auto instADPCM = std::dynamic_pointer_cast<InstrumentADPCM>(inst);
				ctr.appendUint8(static_cast<uint8_t>(instADPCM->getWaveformNumber()));
				uint8_t tmp = static_cast<uint8_t>(instADPCM->getEnvelopeNumber());
				ctr.appendUint8(instADPCM->getEnvelopeEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instADPCM->getArpeggioNumber());
				ctr.appendUint8(instADPCM->getArpeggioEnabled() ? tmp : (0x80 | tmp));
				tmp = static_cast<uint8_t>(instADPCM->getPitchNumber());
				ctr.appendUint8(instADPCM->getPitchEnabled() ? tmp : (0x80 | tmp));
				break;
			}
			}
			ctr.writeUint32(iOfs, ctr.size() - iOfs);
		}
	}
	ctr.writeUint32(instOfs, ctr.size() - instOfs);


	/***** Instrument property section *****/
	ctr.appendString("INSTPROP");
	size_t instPropOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy instrument property section offset

	// FM envelope
	std::vector<int> envFMIdcs = instManLocked->getEnvelopeFMEntriedIndices();
	if (!envFMIdcs.empty()) {
		ctr.appendUint8(0x00);
		ctr.appendUint8(static_cast<uint8_t>(envFMIdcs.size()));
		for (auto& idx : envFMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint8(0);	// Dummy offset
			uint8_t tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::AL) << 4)
						  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::FB));
			ctr.appendUint8(tmp);
			// Operator 1
			tmp = instManLocked->getEnvelopeFMOperatorEnabled(idx, 0);
			tmp = static_cast<uint8_t>((tmp << 5)) | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::AR1));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::KS1) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DR1));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DT1) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SR1));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SL1) << 4)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::RR1));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::TL1));
			ctr.appendUint8(tmp);
			int tmp2 = instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG1);
			tmp = ((tmp2 == -1) ? 0x80 : static_cast<uint8_t>(tmp2 << 4))
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::ML1));
			ctr.appendUint8(tmp);
			// Operator 2
			tmp = instManLocked->getEnvelopeFMOperatorEnabled(idx, 1);
			tmp = static_cast<uint8_t>(tmp << 5) | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::AR2));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::KS2) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DR2));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DT2) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SR2));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SL2) << 4)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::RR2));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::TL2));
			ctr.appendUint8(tmp);
			tmp2 = instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG2);
			tmp = ((tmp2 == -1) ? 0x80 : static_cast<uint8_t>(tmp2 << 4))
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::ML2));
			ctr.appendUint8(tmp);
			// Operator 3
			tmp = instManLocked->getEnvelopeFMOperatorEnabled(idx, 2);
			tmp = static_cast<uint8_t>(tmp << 5) | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::AR3));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::KS3) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DR3));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DT3) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SR3));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SL3) << 4)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::RR3));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::TL3));
			ctr.appendUint8(tmp);
			tmp2 = instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG3);
			tmp = ((tmp2 == -1) ? 0x80 : static_cast<uint8_t>(tmp2 << 4))
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::ML3));
			ctr.appendUint8(tmp);
			// Operator 4
			tmp = instManLocked->getEnvelopeFMOperatorEnabled(idx, 3);
			tmp = static_cast<uint8_t>(tmp << 5) | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::AR4));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::KS4) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DR4));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::DT4) << 5)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SR4));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SL4) << 4)
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::RR4));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::TL4));
			ctr.appendUint8(tmp);
			tmp2 = instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG4);
			tmp = ((tmp2 == -1) ? 0x80 : static_cast<uint8_t>(tmp2 << 4))
				  | static_cast<uint8_t>(instManLocked->getEnvelopeFMParameter(idx, FMEnvelopeParameter::ML4));
			ctr.appendUint8(tmp);
			ctr.writeUint8(ofs, static_cast<uint8_t>(ctr.size() - ofs));
		}
	}

	// FM LFO
	std::vector<int> lfoFMIdcs = instManLocked->getLFOFMEntriedIndices();
	if (!lfoFMIdcs.empty()) {
		ctr.appendUint8(0x01);
		ctr.appendUint8(static_cast<uint8_t>(lfoFMIdcs.size()));
		for (auto& idx : lfoFMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint8(0);	// Dummy offset
			uint8_t tmp = static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::FREQ) << 4)
						  | static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::PMS));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::AM4) << 7)
				  | static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::AM3) << 6)
				  | static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::AM2) << 5)
				  | static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::AM1) << 4)
				  | static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::AMS));
			ctr.appendUint8(tmp);
			tmp = static_cast<uint8_t>(instManLocked->getLFOFMparameter(idx, FMLFOParameter::Count));
			ctr.appendUint8(tmp);
			ctr.writeUint8(ofs, static_cast<uint8_t>(ctr.size() - ofs));
		}
	}

	// FM envelope parameter
	for (size_t i = 0; i < 38; ++i) {
		std::vector<int> idcs = instManLocked->getOperatorSequenceFMEntriedIndices(FileIO::ENV_FM_PARAMS[i]);
		if (!idcs.empty()) {
			ctr.appendUint8(0x02 + static_cast<uint8_t>(i));
			ctr.appendUint8(static_cast<uint8_t>(idcs.size()));
			for (auto& idx : idcs) {
				ctr.appendUint8(static_cast<uint8_t>(idx));
				size_t ofs = ctr.size();
				ctr.appendUint16(0);	// Dummy offset
				auto seq = instManLocked->getOperatorSequenceFMSequence(FileIO::ENV_FM_PARAMS[i], idx);
				ctr.appendUint16(static_cast<uint16_t>(seq.size()));
				for (auto& com : seq) {
					ctr.appendUint16(static_cast<uint16_t>(com.type));
				}
				auto loop = instManLocked->getOperatorSequenceFMLoops(FileIO::ENV_FM_PARAMS[i], idx);
				ctr.appendUint16(static_cast<uint16_t>(loop.size()));
				for (auto& l : loop) {
					ctr.appendUint16(static_cast<uint16_t>(l.begin));
					ctr.appendUint16(static_cast<uint16_t>(l.end));
					ctr.appendUint8(static_cast<uint8_t>(l.times));
				}
				auto release = instManLocked->getOperatorSequenceFMRelease(FileIO::ENV_FM_PARAMS[i], idx);
				switch (release.type) {
				case ReleaseType::NoRelease:
					ctr.appendUint8(0x00);
					break;
				case ReleaseType::FixedRelease:
					ctr.appendUint8(0x01);
					ctr.appendUint16(static_cast<uint16_t>(release.begin));
					break;
				case ReleaseType::AbsoluteRelease:
					ctr.appendUint8(0x02);
					ctr.appendUint16(static_cast<uint16_t>(release.begin));
					break;
				case ReleaseType::RelativeRelease:
					ctr.appendUint8(0x03);
					ctr.appendUint16(static_cast<uint16_t>(release.begin));
					break;
				}
				ctr.appendUint8(0);	// Skip sequence type
				ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
			}
		}
	}

	// FM arpeggio
	std::vector<int> arpFMIdcs = instManLocked->getArpeggioFMEntriedIndices();
	if (!arpFMIdcs.empty()) {
		ctr.appendUint8(0x28);
		ctr.appendUint8(static_cast<uint8_t>(arpFMIdcs.size()));
		for (auto& idx : arpFMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getArpeggioFMSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getArpeggioFMLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getArpeggioFMRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getArpeggioFMType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::FIXED_SEQUENCE:		ctr.appendUint8(0x01);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// FM pitch
	std::vector<int> ptFMIdcs = instManLocked->getPitchFMEntriedIndices();
	if (!ptFMIdcs.empty()) {
		ctr.appendUint8(0x29);
		ctr.appendUint8(static_cast<uint8_t>(ptFMIdcs.size()));
		for (auto& idx : ptFMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getPitchFMSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getPitchFMLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getPitchFMRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getPitchFMType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// SSG waveform
	std::vector<int> wfSSGIdcs = instManLocked->getWaveformSSGEntriedIndices();
	if (!wfSSGIdcs.empty()) {
		ctr.appendUint8(0x30);
		ctr.appendUint8(static_cast<uint8_t>(wfSSGIdcs.size()));
		for (auto& idx : wfSSGIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getWaveformSSGSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
				ctr.appendInt32(static_cast<int32_t>(com.data));
			}
			auto loop = instManLocked->getWaveformSSGLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getWaveformSSGRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			ctr.appendUint8(0);	// Skip sequence type
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// SSG tone/noise
	std::vector<int> tnSSGIdcs = instManLocked->getToneNoiseSSGEntriedIndices();
	if (!tnSSGIdcs.empty()) {
		ctr.appendUint8(0x31);
		ctr.appendUint8(static_cast<uint8_t>(tnSSGIdcs.size()));
		for (auto& idx : tnSSGIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getToneNoiseSSGSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getToneNoiseSSGLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getToneNoiseSSGRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			ctr.appendUint8(0);	// Skip sequence type
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// SSG envelope
	std::vector<int> envSSGIdcs = instManLocked->getEnvelopeSSGEntriedIndices();
	if (!envSSGIdcs.empty()) {
		ctr.appendUint8(0x32);
		ctr.appendUint8(static_cast<uint8_t>(envSSGIdcs.size()));
		for (auto& idx : envSSGIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getEnvelopeSSGSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
				ctr.appendInt32(static_cast<int32_t>(com.data));
			}
			auto loop = instManLocked->getEnvelopeSSGLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getEnvelopeSSGRelease(idx);

			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			ctr.appendUint8(0);	// Skip sequence type
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// SSG arpeggio
	std::vector<int> arpSSGIdcs = instManLocked->getArpeggioSSGEntriedIndices();
	if (!arpSSGIdcs.empty()) {
		ctr.appendUint8(0x33);
		ctr.appendUint8(static_cast<uint8_t>(arpSSGIdcs.size()));
		for (auto& idx : arpSSGIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getArpeggioSSGSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getArpeggioSSGLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getArpeggioSSGRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getArpeggioSSGType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::FIXED_SEQUENCE:		ctr.appendUint8(0x01);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// SSG pitch
	std::vector<int> ptSSGIdcs = instManLocked->getPitchSSGEntriedIndices();
	if (!ptSSGIdcs.empty()) {
		ctr.appendUint8(0x34);
		ctr.appendUint8(static_cast<uint8_t>(ptSSGIdcs.size()));
		for (auto& idx : ptSSGIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getPitchSSGSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getPitchSSGLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getPitchSSGRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getPitchSSGType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// ADPCM waveform
	std::vector<int> wfADPCMIdcs = instManLocked->getWaveformADPCMEntriedIndices();
	if (!wfADPCMIdcs.empty()) {
		ctr.appendUint8(0x40);
		ctr.appendUint8(static_cast<uint8_t>(wfADPCMIdcs.size()));
		for (auto& idx : wfADPCMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint32(0);	// Dummy offset
			ctr.appendUint8(static_cast<uint8_t>(instManLocked->getWaveformADPCMRootKeyNumber(idx)));
			ctr.appendUint16(static_cast<uint16_t>(instManLocked->getWaveformADPCMRootDeltaN(idx)));
			ctr.appendUint8(static_cast<uint8_t>(instManLocked->isWaveformADPCMRepeatable(idx)));
			std::vector<uint8_t> samples = instManLocked->getWaveformADPCMSamples(idx);
			ctr.appendUint32(samples.size());
			ctr.appendVector(std::move(samples));
			ctr.writeUint32(ofs, ctr.size() - ofs);
		}
	}

	// ADPCM envelope
	std::vector<int> envADPCMIdcs = instManLocked->getEnvelopeADPCMEntriedIndices();
	if (!envADPCMIdcs.empty()) {
		ctr.appendUint8(0x41);
		ctr.appendUint8(static_cast<uint8_t>(envADPCMIdcs.size()));
		for (auto& idx : envADPCMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getEnvelopeADPCMSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
				ctr.appendInt32(static_cast<int32_t>(com.data));
			}
			auto loop = instManLocked->getEnvelopeADPCMLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getEnvelopeADPCMRelease(idx);

			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			ctr.appendUint8(0);	// Skip sequence type
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// ADPCM arpeggio
	std::vector<int> arpADPCMIdcs = instManLocked->getArpeggioADPCMEntriedIndices();
	if (!arpADPCMIdcs.empty()) {
		ctr.appendUint8(0x42);
		ctr.appendUint8(static_cast<uint8_t>(arpADPCMIdcs.size()));
		for (auto& idx : arpADPCMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getArpeggioADPCMSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getArpeggioADPCMLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getArpeggioADPCMRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getArpeggioADPCMType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::FIXED_SEQUENCE:		ctr.appendUint8(0x01);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	// ADPCM pitch
	std::vector<int> ptADPCMIdcs = instManLocked->getPitchADPCMEntriedIndices();
	if (!ptADPCMIdcs.empty()) {
		ctr.appendUint8(0x43);
		ctr.appendUint8(static_cast<uint8_t>(ptADPCMIdcs.size()));
		for (auto& idx : ptADPCMIdcs) {
			ctr.appendUint8(static_cast<uint8_t>(idx));
			size_t ofs = ctr.size();
			ctr.appendUint16(0);	// Dummy offset
			auto seq = instManLocked->getPitchADPCMSequence(idx);
			ctr.appendUint16(static_cast<uint16_t>(seq.size()));
			for (auto& com : seq) {
				ctr.appendUint16(static_cast<uint16_t>(com.type));
			}
			auto loop = instManLocked->getPitchADPCMLoops(idx);
			ctr.appendUint16(static_cast<uint16_t>(loop.size()));
			for (auto& l : loop) {
				ctr.appendUint16(static_cast<uint16_t>(l.begin));
				ctr.appendUint16(static_cast<uint16_t>(l.end));
				ctr.appendUint8(static_cast<uint8_t>(l.times));
			}
			auto release = instManLocked->getPitchADPCMRelease(idx);
			switch (release.type) {
			case ReleaseType::NoRelease:
				ctr.appendUint8(0x00);
				// If release.type is NO_RELEASE, then release.begin == -1 so omit to save it.
				break;
			case ReleaseType::FixedRelease:
				ctr.appendUint8(0x01);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::AbsoluteRelease:
				ctr.appendUint8(0x02);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			case ReleaseType::RelativeRelease:
				ctr.appendUint8(0x03);
				ctr.appendUint16(static_cast<uint16_t>(release.begin));
				break;
			}
			switch (instManLocked->getPitchADPCMType(idx)) {
			case SequenceType::ABSOLUTE_SEQUENCE:	ctr.appendUint8(0x00);	break;
			case SequenceType::RELATIVE_SEQUENCE:	ctr.appendUint8(0x02);	break;
			default:												break;
			}
			ctr.writeUint16(ofs, static_cast<uint16_t>(ctr.size() - ofs));
		}
	}

	ctr.writeUint32(instPropOfs, ctr.size() - instPropOfs);


	/***** Groove section *****/
	ctr.appendString("GROOVE  ");
	size_t grooveOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy groove section offset
	size_t grooveCnt = mod.lock()->getGrooveCount();
	ctr.appendUint8(static_cast<uint8_t>(grooveCnt - 1));
	for (size_t i = 0; i < grooveCnt; ++i) {
		ctr.appendUint8(static_cast<uint8_t>(i));
		auto seq = mod.lock()->getGroove(static_cast<int>(i)).getSequence();
		ctr.appendUint8(static_cast<uint8_t>(seq.size()));
		for (auto& g : seq) {
			ctr.appendUint8(static_cast<uint8_t>(g));
		}
	}
	ctr.writeUint32(grooveOfs, ctr.size() - grooveOfs);


	/***** Song section *****/
	ctr.appendString("SONG    ");
	size_t songSecOfs = ctr.size();
	ctr.appendUint32(0);	// Dummy song section offset
	size_t songCnt = mod.lock()->getSongCount();
	ctr.appendUint8(static_cast<uint8_t>(songCnt));

	// Song
	for (size_t i = 0; i < songCnt; ++i) {
		ctr.appendUint8(static_cast<uint8_t>(i));
		size_t songOfs = ctr.size();
		ctr.appendUint32(0);	// Dummy song block offset
		auto& sng = mod.lock()->getSong(static_cast<int>(i));
		std::string title = sng.getTitle();
		ctr.appendUint32(title.length());
		if (!title.empty()) ctr.appendString(title);
		ctr.appendUint32(static_cast<uint32_t>(sng.getTempo()));
		uint8_t tmp = static_cast<uint8_t>(sng.getGroove());
		ctr.appendUint8(sng.isUsedTempo() ? (0x80 | tmp) : tmp);
		ctr.appendUint32(static_cast<uint32_t>(sng.getSpeed()));
		ctr.appendUint8(static_cast<uint8_t>(sng.getDefaultPatternSize()) - 1);
		auto style = sng.getStyle();
		switch (style.type) {
		case SongType::Standard:		ctr.appendUint8(0x00);	break;
		case SongType::FM3chExpanded:	ctr.appendUint8(0x01);	break;
		default:	throw FileOutputError(FileIO::FileType::Mod);
		}

		// Bookmark
		size_t bmSize = sng.getBookmarkSize();
		ctr.appendUint8(static_cast<uint8_t>(bmSize));
		for (size_t i = 0; i < bmSize; ++i) {
			Bookmark bm = sng.getBookmark(static_cast<int>(i));
			ctr.appendUint32(bm.name.length());
			ctr.appendString(bm.name);
			ctr.appendUint8(static_cast<uint8_t>(bm.order));
			ctr.appendUint8(static_cast<uint8_t>(bm.step));
		}

		// Track
		for (auto& attrib : style.trackAttribs) {
			ctr.appendUint8(static_cast<uint8_t>(attrib.number));
			size_t trackOfs = ctr.size();
			ctr.appendUint32(0);	// Dummy track subblock offset
			auto& track = sng.getTrack(attrib.number);

			// Order
			size_t odrSize = track.getOrderSize();
			ctr.appendUint8(static_cast<uint8_t>(odrSize) - 1);
			for (size_t o = 0; o < odrSize; ++o)
				ctr.appendUint8(static_cast<uint8_t>(track.getOrderData(static_cast<int>(o)).patten));
			ctr.appendUint8(static_cast<uint8_t>(track.getEffectDisplayWidth()));

			// Pattern
			for (auto& idx : track.getEditedPatternIndices()) {
				ctr.appendUint8(static_cast<uint8_t>(idx));
				size_t ptnOfs = ctr.size();
				ctr.appendUint32(0);	// Dummy pattern subblock offset
				auto& pattern = track.getPattern(idx);

				// Step
				std::vector<int> stepIdcs = pattern.getEditedStepIndices();
				for (auto& sidx : stepIdcs) {
					ctr.appendUint8(static_cast<uint8_t>(sidx));
					size_t evFlagOfs = ctr.size();
					ctr.appendUint16(0);	// Dummy set event flag
					auto& step = pattern.getStep(sidx);
					uint16_t eventFlag = 0;
					int tmp = step.getNoteNumber();
					if (tmp != -1) {
						eventFlag |= 0x0001;
						ctr.appendInt8(static_cast<int8_t>(tmp));
					}
					tmp = step.getInstrumentNumber();
					if (tmp != -1) {
						eventFlag |= 0x0002;
						ctr.appendUint8(static_cast<uint8_t>(tmp));
					}
					tmp = step.getVolume();
					if (tmp != -1) {
						eventFlag |= 0x0004;
						ctr.appendUint8(static_cast<uint8_t>(tmp));
					}
					for (int i = 0; i < 4; ++i) {
						std::string tmpstr = step.getEffectID(i);
						if (tmpstr != "--") {
							eventFlag |= (0x0008 << (i << 1));
							ctr.appendString(tmpstr);
						}
						tmp = step.getEffectValue(i);
						if (tmp != -1) {
							eventFlag |= (0x0010 << (i << 1));
							ctr.appendUint8(static_cast<uint8_t>(tmp));
						}
					}
					ctr.writeUint16(evFlagOfs, eventFlag);
				}

				ctr.writeUint32(ptnOfs, ctr.size() - ptnOfs);
			}

			ctr.writeUint32(trackOfs, ctr.size() - trackOfs);
		}
		ctr.writeUint32(songOfs, ctr.size() - songOfs);

	}
	ctr.writeUint32(songSecOfs, ctr.size() - songSecOfs);

	ctr.writeUint32(eofOfs, ctr.size() - eofOfs);
}

void ModuleIO::loadModule(BinaryContainer& ctr, std::weak_ptr<Module> mod,
						  std::weak_ptr<InstrumentsManager> instMan)
{
	size_t globCsr = 0;
	if (ctr.readString(globCsr, 16) != "BambooTrackerMod")
		throw FileCorruptionError(FileIO::FileType::Mod);
	globCsr += 16;
	size_t eofOfs = ctr.readUint32(globCsr);
	size_t eof = globCsr + eofOfs;
	globCsr += 4;
	size_t fileVersion = ctr.readUint32(globCsr);
	if (fileVersion > Version::ofModuleFileInBCD())
		throw FileVersionError(FileIO::FileType::Mod);
	globCsr += 4;

	while (globCsr < eof) {
		if (ctr.readString(globCsr, 8) == "MODULE  ")
			globCsr = loadModuleSectionInModule(mod, ctr, globCsr + 8, fileVersion);
		else if (ctr.readString(globCsr, 8) == "INSTRMNT")
			globCsr = loadInstrumentSectionInModule(instMan, ctr, globCsr + 8, fileVersion);
		else if (ctr.readString(globCsr, 8) == "INSTPROP")
			globCsr = loadInstrumentPropertySectionInModule(instMan, ctr, globCsr + 8, fileVersion);
		else if (ctr.readString(globCsr, 8) == "GROOVE  ")
			globCsr = loadGrooveSectionInModule(mod, ctr, globCsr + 8, fileVersion);
		else if (ctr.readString(globCsr, 8) == "SONG    ")
			globCsr = loadSongSectionInModule(mod, ctr, globCsr + 8, fileVersion);
		else
			throw FileCorruptionError(FileIO::FileType::Mod);
	}
}

size_t ModuleIO::loadModuleSectionInModule(std::weak_ptr<Module> mod, BinaryContainer& ctr,
										   size_t globCsr, uint32_t version)
{
	std::shared_ptr<Module> modLocked = mod.lock();

	size_t modOfs = ctr.readUint32(globCsr);
	size_t modCsr = globCsr + 4;
	size_t modTitleLen = ctr.readUint32(modCsr);
	modCsr += 4;
	if (modTitleLen > 0) {
		modLocked->setTitle(ctr.readString(modCsr, modTitleLen));
		modCsr += modTitleLen;
	}
	size_t modAuthorLen = ctr.readUint32(modCsr);
	modCsr += 4;
	if (modAuthorLen > 0) {
		modLocked->setAuthor(ctr.readString(modCsr, modAuthorLen));
		modCsr += modAuthorLen;
	}
	size_t modCopyrightLen = ctr.readUint32(modCsr);
	modCsr += 4;
	if (modCopyrightLen > 0) {
		modLocked->setCopyright(ctr.readString(modCsr, modCopyrightLen));
		modCsr += modCopyrightLen;
	}
	size_t modCommentLen = ctr.readUint32(modCsr);
	modCsr += 4;
	if (modCommentLen > 0) {
		modLocked->setComment(ctr.readString(modCsr, modCommentLen));
		modCsr += modCommentLen;
	}
	modLocked->setTickFrequency(ctr.readUint32(modCsr));
	modCsr += 4;
	modLocked->setStepHighlight1Distance(ctr.readUint32(modCsr));
	modCsr += 4;
	if (version >= Version::toBCD(1, 0, 3)) {
		modLocked->setStepHighlight2Distance(ctr.readUint32(modCsr));
		modCsr += 4;
	}
	else {
		modLocked->setStepHighlight2Distance(modLocked->getStepHighlight1Distance() * 4);
	}
	if (version >= Version::toBCD(1, 3, 0)) {
		auto mixType = static_cast<MixerType>(ctr.readUint8(modCsr++));
		modLocked->setMixerType(mixType);
		if (mixType == MixerType::CUSTOM) {
			modLocked->setCustomMixerFMLevel(ctr.readInt8(modCsr++) / 10.0);
			modLocked->setCustomMixerSSGLevel(ctr.readInt8(modCsr++) / 10.0);
		}
	}
	else {
		modLocked->setMixerType(MixerType::UNSPECIFIED);
	}

	return globCsr + modOfs;
}

size_t ModuleIO::loadInstrumentSectionInModule(std::weak_ptr<InstrumentsManager> instMan,
											   BinaryContainer& ctr, size_t globCsr, uint32_t version)
{
	(void)version;

	std::shared_ptr<InstrumentsManager> instManLocked = instMan.lock();

	size_t instOfs = ctr.readUint32(globCsr);
	size_t instCsr = globCsr + 4;
	uint8_t instCnt = ctr.readUint8(instCsr);
	instCsr += 1;
	for (uint8_t i = 0; i < instCnt; ++i) {
		uint8_t idx = ctr.readUint8(instCsr);
		instCsr += 1;
		size_t iOfs = ctr.readUint32(instCsr);
		size_t iCsr = instCsr + 4;
		size_t nameLen = ctr.readUint32(iCsr);
		iCsr += 4;
		std::string name = u8"";
		if (nameLen > 0) {
			name = ctr.readString(iCsr, nameLen);
			iCsr += nameLen;
		}
		switch (ctr.readUint8(iCsr++)) {
		case 0x00:	// FM
		{
			auto instFM = new InstrumentFM(idx, name, instManLocked.get());
			instFM->setEnvelopeNumber(ctr.readUint8(iCsr));
			iCsr += 1;
			uint8_t tmp = ctr.readUint8(iCsr);
			instFM->setLFOEnabled((0x80 & tmp) ? false : true);
			instFM->setLFONumber(0x7f & tmp);
			iCsr += 1;
			for (auto& param : FileIO::ENV_FM_PARAMS) {
				tmp = ctr.readUint8(iCsr);
				instFM->setOperatorSequenceEnabled(param, (0x80 & tmp) ? false : true);
				instFM->setOperatorSequenceNumber(param, 0x7f & tmp);
				iCsr += 1;
			}

			tmp = ctr.readUint8(iCsr);
			instFM->setArpeggioEnabled(FMOperatorType::All, (0x80 & tmp) ? false : true);
			instFM->setArpeggioNumber(FMOperatorType::All, 0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instFM->setPitchEnabled(FMOperatorType::All, (0x80 & tmp) ? false : true);
			instFM->setPitchNumber(FMOperatorType::All, 0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instFM->setEnvelopeResetEnabled(FMOperatorType::All, (tmp & 0x01));
			instFM->setEnvelopeResetEnabled(FMOperatorType::Op1, (tmp & 0x02));
			instFM->setEnvelopeResetEnabled(FMOperatorType::Op2, (tmp & 0x04));
			instFM->setEnvelopeResetEnabled(FMOperatorType::Op3, (tmp & 0x08));
			instFM->setEnvelopeResetEnabled(FMOperatorType::Op4, (tmp & 0x10));
			iCsr += 1;
			if (version >= Version::toBCD(1, 1, 0)) {
				for (auto& t : FileIO::OP_FM_TYPES) {
					tmp = ctr.readUint8(iCsr);
					instFM->setArpeggioEnabled(t, (0x80 & tmp) ? false : true);
					instFM->setArpeggioNumber(t, 0x7f & tmp);
					iCsr += 1;
				}
				for (auto& t : FileIO::OP_FM_TYPES) {
					tmp = ctr.readUint8(iCsr);
					instFM->setPitchEnabled(t, (0x80 & tmp) ? false : true);
					instFM->setPitchNumber(t, 0x7f & tmp);
					iCsr += 1;
				}
			}
			instManLocked->addInstrument(std::unique_ptr<AbstractInstrument>(instFM));
			break;
		}
		case 0x01:	// SSG
		{
			auto instSSG = new InstrumentSSG(idx, name, instManLocked.get());
			uint8_t tmp = ctr.readUint8(iCsr);
			instSSG->setWaveformEnabled((0x80 & tmp) ? false : true);
			instSSG->setWaveformNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instSSG->setToneNoiseEnabled((0x80 & tmp) ? false : true);
			instSSG->setToneNoiseNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instSSG->setEnvelopeEnabled((0x80 & tmp) ? false : true);
			instSSG->setEnvelopeNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instSSG->setArpeggioEnabled((0x80 & tmp) ? false : true);
			instSSG->setArpeggioNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instSSG->setPitchEnabled((0x80 & tmp) ? false : true);
			instSSG->setPitchNumber(0x7f & tmp);
			iCsr += 1;
			instManLocked->addInstrument(std::unique_ptr<AbstractInstrument>(instSSG));
			break;
		}
		case 0x02:	// ADPCM
		{
			auto instADPCM = new InstrumentADPCM(idx, name, instManLocked.get());
			instADPCM->setWaveformNumber(ctr.readUint8(iCsr++));
			uint8_t tmp = ctr.readUint8(iCsr);
			instADPCM->setEnvelopeEnabled((0x80 & tmp) ? false : true);
			instADPCM->setEnvelopeNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instADPCM->setArpeggioEnabled((0x80 & tmp) ? false : true);
			instADPCM->setArpeggioNumber(0x7f & tmp);
			iCsr += 1;
			tmp = ctr.readUint8(iCsr);
			instADPCM->setPitchEnabled((0x80 & tmp) ? false : true);
			instADPCM->setPitchNumber(0x7f & tmp);
			iCsr += 1;
			instManLocked->addInstrument(std::unique_ptr<AbstractInstrument>(instADPCM));
			break;
		}
		default:
			throw FileCorruptionError(FileIO::FileType::Mod);
		}
		instCsr += iOfs;
	}

	return globCsr + instOfs;
}

size_t ModuleIO::loadInstrumentPropertySectionInModule(std::weak_ptr<InstrumentsManager> instMan,
													   BinaryContainer& ctr, size_t globCsr,
													   uint32_t version)
{
	std::shared_ptr<InstrumentsManager> instManLocked = instMan.lock();

	size_t instPropOfs = ctr.readUint32(globCsr);
	size_t instPropCsr = globCsr + 4;
	globCsr += instPropOfs;
	while (instPropCsr < globCsr) {
		switch (ctr.readUint8(instPropCsr++)) {
		case 0x00:	// FM envelope
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint8_t ofs = ctr.readUint8(instPropCsr);
				size_t csr = instPropCsr + 1;
				uint8_t tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::AL, tmp >> 4);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::FB, tmp & 0x0f);
				// Operator 1
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMOperatorEnabled(idx, 0, (0x20 & tmp) ? true : false);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::AR1, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::KS1, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DR1, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DT1, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SR1, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SL1, tmp >> 4);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::RR1, tmp & 0x0f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::TL1, tmp);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::ML1, tmp & 0x0f);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG1,
													   (tmp & 0x80) ? -1 : ((tmp >> 4) & 0x07));
				// Operator 2
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMOperatorEnabled(idx, 1, (0x20 & tmp) ? true : false);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::AR2, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::KS2, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DR2, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DT2, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SR2, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SL2, tmp >> 4);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::RR2, tmp & 0x0f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::TL2, tmp);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::ML2, tmp & 0x0f);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG2,
													   (tmp & 0x80) ? -1 : ((tmp >> 4) & 0x07));
				// Operator 3
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMOperatorEnabled(idx, 2, (0x20 & tmp) ? true : false);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::AR3, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::KS3, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DR3, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DT3, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SR3, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SL3, tmp >> 4);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::RR3, tmp & 0x0f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::TL3, tmp);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::ML3, tmp & 0x0f);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG3,
													   (tmp & 0x80) ? -1 : ((tmp >> 4) & 0x07));
				// Operator 4
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMOperatorEnabled(idx, 3, (0x20 & tmp) ? true : false);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::AR4, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::KS4, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DR4, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::DT4, tmp >> 5);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SR4, tmp & 0x1f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SL4, tmp >> 4);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::RR4, tmp & 0x0f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::TL4, tmp);
				tmp = ctr.readUint8(csr++);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::ML4, tmp & 0x0f);
				instManLocked->setEnvelopeFMParameter(idx, FMEnvelopeParameter::SSGEG4,
													   (tmp & 0x80) ? -1 : ((tmp >> 4) & 0x07));
				instPropCsr += ofs;
			}
			break;
		}
		case 0x01:	// FM LFO
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint8_t ofs = ctr.readUint8(instPropCsr);
				size_t csr = instPropCsr + 1;
				uint8_t tmp = ctr.readUint8(csr++);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::FREQ, tmp >> 4);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::PMS, tmp & 0x0f);
				tmp = ctr.readUint8(csr++);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::AMS, tmp & 0x0f);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::AM1, (tmp & 0x10) ? true : false);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::AM2, (tmp & 0x20) ? true : false);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::AM3, (tmp & 0x40) ? true : false);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::AM4, (tmp & 0x80) ? true : false);
				tmp = ctr.readUint8(csr++);
				instManLocked->setLFOFMParameter(idx, FMLFOParameter::Count, tmp);
				instPropCsr += ofs;
			}
			break;
		}
		case 0x02:	// FM AL
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::AL, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x03:	// FM FB
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::FB, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x04:	// FM AR1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::AR1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x05:	// FM DR1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DR1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x06:	// FM SR1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SR1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x07:	// FM RR1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::RR1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x08:	// FM SL1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SL1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x09:	// FM TL1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::TL1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0a:	// FM KS1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::KS1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0b:	// FM ML1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::ML1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0c:	// FM DT1
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DT1, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0d:	// FM AR2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::AR2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0e:	// FM DR2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DR2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x0f:	// FM SR2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SR2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x10:	// FM RR2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::RR2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x11:	// FM SL2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SL2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x12:	// FM TL2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::TL2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x13:	// FM KS2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::KS2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x14:	// FM ML2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::ML2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x15:	// FM DT2
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DT2, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x16:	// FM AR3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::AR3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x17:	// FM DR3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DR3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x18:	// FM SR3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SR3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x19:	// FM RR3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::RR3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1a:	// FM SL3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SL3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1b:	// FM TL3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::TL3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1c:	// FM KS3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::KS3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1d:	// FM ML3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::ML3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1e:	// FM DT3
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DT3, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x1f:	// FM AR4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::AR4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x20:	// FM DR4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DR4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x21:	// FM SR4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SR4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x22:	// FM RR4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::RR4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x23:	// FM SL4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::SL4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x24:	// FM TL4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::TL4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x25:	// FM KS4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::KS4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x26:	// FM ML4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::ML4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x27:	// FM DT4
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i)
				instPropCsr += loadInstrumentPropertyOperatorSequence(
								   FMEnvelopeParameter::DT4, instPropCsr, instManLocked, ctr, version);
			break;
		}
		case 0x28:	// FM arpeggio
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 2, 0)) csr += 2;
					if (l == 0)
						instManLocked->setArpeggioFMSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addArpeggioFMSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setArpeggioFMLoops(idx, begins, ends, times);
				}
				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setArpeggioFMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setArpeggioFMRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setArpeggioFMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					switch (ctr.readUint8(csr++)) {
					case 0x00:	// Absolute
						instManLocked->setArpeggioFMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
						break;
					case 0x01:	// Fixed
						instManLocked->setArpeggioFMType(idx, SequenceType::FIXED_SEQUENCE);
						break;
					case 0x02:	// Relative
						instManLocked->setArpeggioFMType(idx, SequenceType::RELATIVE_SEQUENCE);
						break;
					default:
						if (version < Version::toBCD(1, 3, 2)) {
							// Recover deep clone bug
							// https://github.com/rerrahkr/BambooTracker/issues/170
							instManLocked->setArpeggioFMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
							break;
						}
						else {
							throw FileCorruptionError(FileIO::FileType::Mod);
						}
					}
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x29:	// FM pitch
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 2, 0)) csr += 2;
					if (l == 0)
						instManLocked->setPitchFMSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addPitchFMSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setPitchFMLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setPitchFMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setPitchFMRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setPitchFMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					switch (ctr.readUint8(csr++)) {
					case 0x00:	// Absolute
						instManLocked->setPitchFMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
						break;
					case 0x02:	// Relative
						instManLocked->setPitchFMType(idx, SequenceType::RELATIVE_SEQUENCE);
						break;
					default:
						if (version < Version::toBCD(1, 3, 2)) {
							// Recover deep clone bug
							// https://github.com/rerrahkr/BambooTracker/issues/170
							instManLocked->setPitchFMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
							break;
						}
						else {
							throw FileCorruptionError(FileIO::FileType::Mod);
						}
					}
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x30:	// SSG waveform
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 2, 0)) {
						if (data == 3) data = static_cast<int>(SSGWaveformType::SQM_TRIANGLE);
						else if (data == 4) data = static_cast<int>(SSGWaveformType::SQM_SAW);
					}
					int32_t subdata;
					if (version >= Version::toBCD(1, 2, 0)) {
						subdata = ctr.readInt32(csr);
						csr += 4;
					}
					else {
						subdata = ctr.readUint16(csr);
						csr += 2;
						if (subdata != -1)
							subdata = PitchConverter::getPitchSSGSquare(subdata);
					}
					if (l == 0)
						instManLocked->setWaveformSSGSequenceCommand(idx, 0, data, subdata);
					else
						instManLocked->addWaveformSSGSequenceCommand(idx, data, subdata);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setWaveformSSGLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setWaveformSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setWaveformSSGRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setWaveformSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					++csr;	// Skip sequence type
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x31:	// SSG tone/noise
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 3, 1)) {
						if (data > 0) {
							uint16_t tmp = data - 1;
							data = tmp / 32 * 32 + (31 - tmp % 32) + 1;
						}
					}
					if (version < Version::toBCD(1, 2, 0)) csr += 2;
					if (l == 0)
						instManLocked->setToneNoiseSSGSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addToneNoiseSSGSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setToneNoiseSSGLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setToneNoiseSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setToneNoiseSSGRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setToneNoiseSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					++csr;	// Skip sequence type
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x32:	// SSG envelope
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					int32_t subdata;
					if (version >= Version::toBCD(1, 2, 0)) {
						subdata = ctr.readInt32(csr);
						csr += 4;
					}
					else {
						subdata = ctr.readUint16(csr);
						csr += 2;
					}
					if (l == 0)
						instManLocked->setEnvelopeSSGSequenceCommand(idx, 0, data, subdata);
					else
						instManLocked->addEnvelopeSSGSequenceCommand(idx, data, subdata);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setEnvelopeSSGLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				case 0x02:	// Absolute
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::AbsoluteRelease, pos);
					else instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				case 0x03:	// Relative
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::RelativeRelease, pos);
					else instManLocked->setEnvelopeSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					++csr;	// Skip sequence type
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x33:	// SSG arpeggio
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 2, 0)) csr += 2;
					if (l == 0)
						instManLocked->setArpeggioSSGSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addArpeggioSSGSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setArpeggioSSGLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setArpeggioSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setArpeggioSSGRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setArpeggioSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					switch (ctr.readUint8(csr++)) {
					case 0x00:	// Absolute
						instManLocked->setArpeggioSSGType(idx, SequenceType::ABSOLUTE_SEQUENCE);
						break;
					case 0x01:	// Fixed
						instManLocked->setArpeggioSSGType(idx, SequenceType::FIXED_SEQUENCE);
						break;
					case 0x02:	// Relative
						instManLocked->setArpeggioSSGType(idx, SequenceType::RELATIVE_SEQUENCE);
						break;
					default:
						if (version < Version::toBCD(1, 3, 2)) {
							// Recover deep clone bug
							// https://github.com/rerrahkr/BambooTracker/issues/170
							instManLocked->setArpeggioSSGType(idx, SequenceType::ABSOLUTE_SEQUENCE);
							break;
						}
						else {
							throw FileCorruptionError(FileIO::FileType::Mod);
						}
					}
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x34:	// SSG pitch
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (version < Version::toBCD(1, 2, 0)) csr += 2;
					if (l == 0)
						instManLocked->setPitchSSGSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addPitchSSGSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setPitchSSGLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setPitchSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setPitchSSGRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setPitchSSGRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				if (version >= Version::toBCD(1, 0, 1)) {
					switch (ctr.readUint8(csr++)) {
					case 0x00:	// Absolute
						instManLocked->setPitchSSGType(idx, SequenceType::ABSOLUTE_SEQUENCE);
						break;
					case 0x02:	// Relative
						instManLocked->setPitchSSGType(idx, SequenceType::RELATIVE_SEQUENCE);
						break;
					default:
						if (version < Version::toBCD(1, 3, 2)) {
							// Recover deep clone bug
							// https://github.com/rerrahkr/BambooTracker/issues/170
							instManLocked->setPitchSSGType(idx, SequenceType::ABSOLUTE_SEQUENCE);
							break;
						}
						else {
							throw FileCorruptionError(FileIO::FileType::Mod);
						}
					}
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x40:	// ADPCM waveform
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint32_t ofs = ctr.readUint32(instPropCsr);
				size_t csr = instPropCsr + 4;

				instManLocked->setWaveformADPCMRootKeyNumber(idx, ctr.readUint8(csr++));
				instManLocked->setWaveformADPCMRootDeltaN(idx, ctr.readUint16(csr));
				csr += 2;
				instManLocked->setWaveformADPCMRepeatEnabled(idx, (ctr.readUint8(csr++) & 0x01) != 0);
				uint32_t len = ctr.readUint32(csr);
				csr += 4;
				std::vector<uint8_t> samples = ctr.getSubcontainer(csr, len).toVector();
				csr += len;
				instManLocked->storeWaveformADPCMSample(idx, std::move(samples));

				instPropCsr += ofs;
			}
			break;
		}
		case 0x41:	// ADPCM envelope
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					int32_t subdata;
					subdata = ctr.readInt32(csr);
					csr += 4;
					if (l == 0)
						instManLocked->setEnvelopeADPCMSequenceCommand(idx, 0, data, subdata);
					else
						instManLocked->addEnvelopeADPCMSequenceCommand(idx, data, subdata);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setEnvelopeADPCMLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				case 0x02:	// Absolute
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::AbsoluteRelease, pos);
					else instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				case 0x03:	// Relative
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					if (pos < seqLen) instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::RelativeRelease, pos);
					else instManLocked->setEnvelopeADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				++csr;	// Skip sequence type

				instPropCsr += ofs;
			}
			break;
		}
		case 0x42:	// ADPCM arpeggio
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (l == 0)
						instManLocked->setArpeggioADPCMSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addArpeggioADPCMSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setArpeggioADPCMLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setArpeggioADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setArpeggioADPCMRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setArpeggioADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				switch (ctr.readUint8(csr++)) {
				case 0x00:	// Absolute
					instManLocked->setArpeggioADPCMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
					break;
				case 0x01:	// Fixed
					instManLocked->setArpeggioADPCMType(idx, SequenceType::FIXED_SEQUENCE);
					break;
				case 0x02:	// Relative
					instManLocked->setArpeggioADPCMType(idx, SequenceType::RELATIVE_SEQUENCE);
					break;
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}

				instPropCsr += ofs;
			}
			break;
		}
		case 0x43:	// ADPCM pitch
		{
			uint8_t cnt = ctr.readUint8(instPropCsr++);
			for (size_t i = 0; i < cnt; ++i) {
				uint8_t idx = ctr.readUint8(instPropCsr++);
				uint16_t ofs = ctr.readUint16(instPropCsr);
				size_t csr = instPropCsr + 2;

				uint16_t seqLen = ctr.readUint16(csr);
				csr += 2;
				for (uint16_t l = 0; l < seqLen; ++l) {
					uint16_t data = ctr.readUint16(csr);
					csr += 2;
					if (l == 0)
						instManLocked->setPitchADPCMSequenceCommand(idx, 0, data, 0);
					else
						instManLocked->addPitchADPCMSequenceCommand(idx, data, 0);
				}

				uint16_t loopCnt = ctr.readUint16(csr);
				csr += 2;
				if (loopCnt > 0) {
					std::vector<int> begins, ends, times;
					for (uint16_t l = 0; l < loopCnt; ++l) {
						begins.push_back(ctr.readUint16(csr));
						csr += 2;
						ends.push_back(ctr.readUint16(csr));
						csr += 2;
						times.push_back(ctr.readUint8(csr++));
					}
					instManLocked->setPitchADPCMLoops(idx, begins, ends, times);
				}

				switch (ctr.readUint8(csr++)) {
				case 0x00:	// No release
					instManLocked->setPitchADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				case 0x01:	// Fixed
				{
					uint16_t pos = ctr.readUint16(csr);
					csr += 2;
					// Release point check (prevents a bug)
					// https://github.com/rerrahkr/BambooTracker/issues/11
					if (pos < seqLen) instManLocked->setPitchADPCMRelease(idx, ReleaseType::FixedRelease, pos);
					else instManLocked->setPitchADPCMRelease(idx, ReleaseType::NoRelease, -1);
					break;
				}
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}
				switch (ctr.readUint8(csr++)) {
				case 0x00:	// Absolute
					instManLocked->setPitchADPCMType(idx, SequenceType::ABSOLUTE_SEQUENCE);
					break;
				case 0x02:	// Relative
					instManLocked->setPitchADPCMType(idx, SequenceType::RELATIVE_SEQUENCE);
					break;
				default:
					throw FileCorruptionError(FileIO::FileType::Mod);
				}

				instPropCsr += ofs;
			}
			break;
		}
		default:
			throw FileCorruptionError(FileIO::FileType::Mod);
		}
	}

	return globCsr;
}

size_t ModuleIO::loadInstrumentPropertyOperatorSequence(FMEnvelopeParameter param,
														size_t instMemCsr,
														std::shared_ptr<InstrumentsManager>& instManLocked,
														BinaryContainer& ctr, uint32_t version)
{
	uint8_t idx = ctr.readUint8(instMemCsr++);
	uint16_t ofs = ctr.readUint16(instMemCsr);
	size_t csr = instMemCsr + 2;

	uint16_t seqLen = ctr.readUint16(csr);
	csr += 2;
	for (uint16_t l = 0; l < seqLen; ++l) {
		uint16_t data = ctr.readUint16(csr);
		csr += 2;
		if (version < Version::toBCD(1, 2, 2)) csr += 2;
		if (l == 0)
			instManLocked->setOperatorSequenceFMSequenceCommand(param, idx, 0, data, 0);
		else
			instManLocked->addOperatorSequenceFMSequenceCommand(param, idx, data, 0);
	}

	uint16_t loopCnt = ctr.readUint16(csr);
	csr += 2;
	if (loopCnt > 0) {
		std::vector<int> begins, ends, times;
		for (uint16_t l = 0; l < loopCnt; ++l) {
			begins.push_back(ctr.readUint16(csr));
			csr += 2;
			ends.push_back(ctr.readUint16(csr));
			csr += 2;
			times.push_back(ctr.readUint8(csr++));
		}
		instManLocked->setOperatorSequenceFMLoops(param, idx, begins, ends, times);
	}

	switch (ctr.readUint8(csr++)) {
	case 0x00:	// No release
		instManLocked->setOperatorSequenceFMRelease(param, idx, ReleaseType::NoRelease, -1);
		break;
	case 0x01:	// Fixed
	{
		uint16_t pos = ctr.readUint16(csr);
		csr += 2;
		// Release point check (prevents a bug; see rerrahkr/BambooTracker issue #11)
		if (pos < seqLen) instManLocked->setOperatorSequenceFMRelease(param, idx, ReleaseType::FixedRelease, pos);
		else instManLocked->setOperatorSequenceFMRelease(param, idx, ReleaseType::NoRelease, -1);
		break;
	}
	default:
		throw FileCorruptionError(FileIO::FileType::Mod);
	}

	if (version >= Version::toBCD(1, 0, 1)) {
		++csr;	// Skip sequence type
	}

	return ofs + 1;
}

size_t ModuleIO::loadGrooveSectionInModule(std::weak_ptr<Module> mod, BinaryContainer& ctr,
										   size_t globCsr, uint32_t version)
{
	(void)version;

	std::shared_ptr<Module> modLocked = mod.lock();

	size_t grvOfs = ctr.readUint32(globCsr);
	size_t grvCsr = globCsr + 4;
	uint8_t cnt = ctr.readUint8(grvCsr++) + 1;
	for (uint8_t i = 0; i < cnt; ++i) {
		uint8_t idx = ctr.readUint8(grvCsr++);
		uint8_t seqLen = ctr.readUint8(grvCsr++);
		std::vector<int> seq;
		for (uint8_t l = 0; l < seqLen; ++l) {
			seq.push_back(ctr.readUint8(grvCsr++));
		}

		if (idx > 0) modLocked->addGroove();
		modLocked->setGroove(idx, seq);
	}

	return globCsr + grvOfs;
}

size_t ModuleIO::loadSongSectionInModule(std::weak_ptr<Module> mod, BinaryContainer& ctr,
										 size_t globCsr, uint32_t version)
{
	std::shared_ptr<Module> modLocked = mod.lock();

	size_t songOfs = ctr.readUint32(globCsr);
	size_t songCsr = globCsr + 4;
	uint8_t cnt = ctr.readUint8(songCsr++);
	for (uint8_t i = 0; i < cnt; ++i) {
		uint8_t idx = ctr.readUint8(songCsr++);
		size_t sOfs = ctr.readUint32(songCsr);
		size_t scsr = songCsr + 4;
		songCsr += sOfs;
		size_t titleLen = ctr.readUint32(scsr);
		scsr += 4;
		std::string title = u8"";
		if (titleLen > 0) title = ctr.readString(scsr, titleLen);
		scsr += titleLen;
		uint32_t tempo = ctr.readUint32(scsr);
		scsr += 4;
		uint8_t groove = ctr.readUint8(scsr);
		scsr += 1;
		bool isTempo = (groove & 0x80) ? true : false;
		groove &= 0x7f;
		uint32_t speed = ctr.readUint32(scsr);
		scsr += 4;
		size_t ptnSize = ctr.readUint8(scsr) + 1;
		scsr += 1;
		SongType songType;
		switch (ctr.readUint8(scsr++)) {
		case 0x00:	songType = SongType::Standard;	break;
		case 0x01:	songType = SongType::FM3chExpanded;	break;
		default:
			throw FileCorruptionError(FileIO::FileType::Mod);
		}
		modLocked->addSong(idx, songType, title, isTempo,
							static_cast<int>(tempo), groove, static_cast<int>(speed), ptnSize);
		auto& song = modLocked->getSong(idx);

		// Bookmark
		if (Version::toBCD(1, 4, 1) <= version) {
			int bmSize = ctr.readUint8(scsr++);
			for (int i = 0; i < bmSize; ++i) {
				size_t len = ctr.readUint32(scsr);
				scsr += 4;
				std::string name = ctr.readString(scsr, len);
				scsr += len;
				int order = ctr.readUint8(scsr++);
				int step = ctr.readUint8(scsr++);
				song.addBookmark(name, order, step);
			}
		}

		while (scsr < songCsr) {
			// Track
			uint8_t trackIdx = ctr.readUint8(scsr++);
			auto& track = song.getTrack(trackIdx);
			size_t trackOfs = ctr.readUint32(scsr);
			size_t trackEnd = scsr + trackOfs;
			size_t tcsr = scsr + 4;
			uint8_t odrLen = ctr.readUint8(tcsr++) + 1;
			for (uint8_t oi = 0; oi < odrLen; ++oi) {
				if (!oi)
					track.registerPatternToOrder(oi, ctr.readUint8(tcsr++));
				else {
					track.insertOrderBelow(oi - 1);
					track.registerPatternToOrder(oi, ctr.readUint8(tcsr++));
				}
			}
			if (version >= Version::toBCD(1, 2, 1)) {
				track.setEffectDisplayWidth(ctr.readUint8(tcsr++));
			}
			SoundSource sndsrc = track.getAttribute().source;

			// Pattern
			while (tcsr < trackEnd) {
				uint8_t ptnIdx = ctr.readUint8(tcsr++);
				auto& pattern = track.getPattern(ptnIdx);
				size_t ptnOfs = ctr.readUint32(tcsr);
				size_t pcsr = tcsr + 4;
				tcsr += ptnOfs;

				// Step
				while (pcsr < tcsr) {
					uint32_t stepIdx = ctr.readUint8(pcsr++);
					auto& step = pattern.getStep(static_cast<int>(stepIdx));
					uint16_t eventFlag = ctr.readUint16(pcsr);
					pcsr += 2;
					if (eventFlag & 0x0001)	{
						if (version >= Version::toBCD(1, 0, 2)) {
							step.setNoteNumber(ctr.readInt8(pcsr++));
						}
						else {
							// Change FM octave (song type is only 0x00 before v1.0.2)
							int8_t nn = ctr.readInt8(pcsr++);
							if (trackIdx < 6 && 0 <= nn && nn < 84)
								step.setNoteNumber(nn + 12);
							else
								step.setNoteNumber(nn);
						}
					}
					if (eventFlag & 0x0002)	step.setInstrumentNumber(ctr.readUint8(pcsr++));
					if (eventFlag & 0x0004)	step.setVolume(ctr.readUint8(pcsr++));
					EffectType efftype = EffectType::NoEffect;
					if (eventFlag & 0x0008)	{
						std::string id = ctr.readString(pcsr, 2);
						step.setEffectID(0, id);
						efftype = Effect::toEffectType(sndsrc, id);
						pcsr += 2;
					}
					if (eventFlag & 0x0010)	{
						int v = ctr.readUint8(pcsr++);
						if (version < Version::toBCD(1, 3, 1) && efftype == EffectType::NoisePitch && v < 32)
							v = 31 - v;
						step.setEffectValue(0, v);
					}
					efftype = EffectType::NoEffect;
					if (eventFlag & 0x0020)	{
						std::string id = ctr.readString(pcsr, 2);
						step.setEffectID(1, id);
						efftype = Effect::toEffectType(sndsrc, id);
						pcsr += 2;
					}
					if (eventFlag & 0x0040)	{
						int v = ctr.readUint8(pcsr++);
						if (version < Version::toBCD(1, 3, 1) && efftype == EffectType::NoisePitch && v < 32)
							v = 31 - v;
						step.setEffectValue(1, v);
					}
					efftype = EffectType::NoEffect;
					if (eventFlag & 0x0080)	{
						std::string id = ctr.readString(pcsr, 2);
						step.setEffectID(2, id);
						efftype = Effect::toEffectType(sndsrc, id);
						pcsr += 2;
					}
					if (eventFlag & 0x0100)	{
						int v = ctr.readUint8(pcsr++);
						if (version < Version::toBCD(1, 3, 1) && efftype == EffectType::NoisePitch && v < 32)
							v = 31 - v;
						step.setEffectValue(2, v);
					}
					efftype = EffectType::NoEffect;
					if (eventFlag & 0x0200)	{
						std::string id = ctr.readString(pcsr, 2);
						step.setEffectID(3, id);
						efftype = Effect::toEffectType(sndsrc, id);
						pcsr += 2;
					}
					if (eventFlag & 0x0400)	{
						int v = ctr.readUint8(pcsr++);
						if (version < Version::toBCD(1, 3, 1) && efftype == EffectType::NoisePitch && v < 32)
							v = 31 - v;
						step.setEffectValue(3, v);
					}
				}
			}

			scsr += trackOfs;
		}

		if (version < Version::toBCD(1, 4, 0)) {	// ADPCM track
			int odrLen = static_cast<int>(song.getOrderSize());
			int trackNum;
			switch (songType) {
			case SongType::Standard:		trackNum = 15;	break;
			case SongType::FM3chExpanded:	trackNum = 18;	break;
			}
			auto& track = song.getTrack(trackNum);
			for (int oi = 0; oi < odrLen; ++oi) {
				if (oi) track.insertOrderBelow(oi - 1);
				track.registerPatternToOrder(oi, 0);
			}
			track.setEffectDisplayWidth(0);
		}
	}

	return globCsr + songOfs;
}
