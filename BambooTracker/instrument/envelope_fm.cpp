#include "envelope_fm.hpp"

constexpr EnvelopeFM::FMOperator EnvelopeFM::DEF_OP[4];

EnvelopeFM::EnvelopeFM(int num)
	: AbstractInstrumentProperty(num)
{
	clearParameters();
	initParamMap();
}

void EnvelopeFM::initParamMap()
{
	paramMap_ = {
		{ FMEnvelopeParameter::AL, al_ },
		{ FMEnvelopeParameter::FB, fb_ },
		{ FMEnvelopeParameter::AR1, op_[0].ar_ },
		{ FMEnvelopeParameter::DR1, op_[0].dr_ },
		{ FMEnvelopeParameter::SR1, op_[0].sr_ },
		{ FMEnvelopeParameter::RR1, op_[0].rr_ },
		{ FMEnvelopeParameter::SL1, op_[0].sl_ },
		{ FMEnvelopeParameter::TL1, op_[0].tl_ },
		{ FMEnvelopeParameter::KS1, op_[0].ks_ },
		{ FMEnvelopeParameter::ML1, op_[0].ml_ },
		{ FMEnvelopeParameter::DT1, op_[0].dt_ },
		{ FMEnvelopeParameter::SSGEG1, op_[0].ssgeg_ },
		{ FMEnvelopeParameter::AR2, op_[1].ar_ },
		{ FMEnvelopeParameter::DR2, op_[1].dr_ },
		{ FMEnvelopeParameter::SR2, op_[1].sr_ },
		{ FMEnvelopeParameter::RR2, op_[1].rr_ },
		{ FMEnvelopeParameter::SL2, op_[1].sl_ },
		{ FMEnvelopeParameter::TL2, op_[1].tl_ },
		{ FMEnvelopeParameter::KS2, op_[1].ks_ },
		{ FMEnvelopeParameter::ML2, op_[1].ml_ },
		{ FMEnvelopeParameter::DT2, op_[1].dt_ },
		{ FMEnvelopeParameter::SSGEG2, op_[1].ssgeg_ },
		{ FMEnvelopeParameter::AR3, op_[2].ar_ },
		{ FMEnvelopeParameter::DR3, op_[2].dr_ },
		{ FMEnvelopeParameter::SR3, op_[2].sr_ },
		{ FMEnvelopeParameter::RR3, op_[2].rr_ },
		{ FMEnvelopeParameter::SL3, op_[2].sl_ },
		{ FMEnvelopeParameter::TL3, op_[2].tl_ },
		{ FMEnvelopeParameter::KS3, op_[2].ks_ },
		{ FMEnvelopeParameter::ML3, op_[2].ml_ },
		{ FMEnvelopeParameter::DT3, op_[2].dt_ },
		{ FMEnvelopeParameter::SSGEG3, op_[2].ssgeg_ },
		{ FMEnvelopeParameter::AR4, op_[3].ar_ },
		{ FMEnvelopeParameter::DR4, op_[3].dr_ },
		{ FMEnvelopeParameter::SR4, op_[3].sr_ },
		{ FMEnvelopeParameter::RR4, op_[3].rr_ },
		{ FMEnvelopeParameter::SL4, op_[3].sl_ },
		{ FMEnvelopeParameter::TL4, op_[3].tl_ },
		{ FMEnvelopeParameter::KS4, op_[3].ks_ },
		{ FMEnvelopeParameter::ML4, op_[3].ml_ },
		{ FMEnvelopeParameter::DT4, op_[3].dt_ },
		{ FMEnvelopeParameter::SSGEG4, op_[3].ssgeg_ }
	};
}

EnvelopeFM::EnvelopeFM(const EnvelopeFM& other)
	: AbstractInstrumentProperty(other)
{
	al_ = other.al_;
	fb_ = other.fb_;

	for (int i = 0; i < 4; ++i)
		op_[i] = other.op_[i];

	initParamMap();
}

bool operator==(const EnvelopeFM& a, const EnvelopeFM& b) {
	if (a.al_ != b.al_ || a.fb_ != b.fb_) return false;
	for (int i = 0; i< 4; ++i) {
		if (a.op_[0] != b.op_[0]) return false;
	}
	return true;
}

std::unique_ptr<EnvelopeFM> EnvelopeFM::clone()
{
	std::unique_ptr<EnvelopeFM> clone = std::make_unique<EnvelopeFM>(*this);
	clone->clearUserInstruments();
	return clone;
}

bool EnvelopeFM::getOperatorEnabled(int num) const
{
	return op_[num].enabled_;
}

void EnvelopeFM::setOperatorEnabled(int num, bool enabled)
{
	op_[num].enabled_ = enabled;
}

int EnvelopeFM::getParameterValue(FMEnvelopeParameter param) const
{
	return paramMap_.at(param);
}

void EnvelopeFM::setParameterValue(FMEnvelopeParameter param, int value)
{
	paramMap_.at(param) = value;
}

bool EnvelopeFM::isEdited() const
{
	if (al_ != DEF_AL || fb_ != DEF_FB) return true;
	for (int i = 0; i < 4; ++i) {
		if (op_[i].enabled_ != DEF_OP[i].enabled_
				|| op_[i].ar_ != DEF_OP[i].ar_
				|| op_[i].dr_ != DEF_OP[i].dr_
				|| op_[i].sr_ != DEF_OP[i].sr_
				|| op_[i].rr_ != DEF_OP[i].rr_
				|| op_[i].sl_ != DEF_OP[i].sl_
				|| op_[i].tl_ != DEF_OP[i].tl_
				|| op_[i].ks_ != DEF_OP[i].ks_
				|| op_[i].ml_ != DEF_OP[i].ml_
				|| op_[i].dt_ != DEF_OP[i].dt_)
			return true;
	}
	return false;
}

void EnvelopeFM::clearParameters()
{
	al_ = DEF_AL;
	fb_ = DEF_FB;
	for (int i = 0; i < 4; ++i) {
		op_[i] = DEF_OP[i];
	}
}
