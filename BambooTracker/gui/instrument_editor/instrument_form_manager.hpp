#ifndef INSTRUMENT_FORM_MANAGER_HPP
#define INSTRUMENT_FORM_MANAGER_HPP

#include <QObject>
#include <QWidget>
#include <QString>
#include <memory>
#include <unordered_map>
#include "instrument/envelope_fm.hpp"
#include "misc.hpp"

class InstrumentFormManager : public QObject
{
	Q_OBJECT

public:
	InstrumentFormManager();
	void updateByConfiguration();

	const std::shared_ptr<QWidget> getForm(int n) const;
	void remove(int n);
	void add(int n, std::shared_ptr<QWidget> form, QString instName, SoundSource instSrc);

	void showForm(int n);
	void closeAll();
	void clearAll();

	SoundSource getFormInstrumentSoundSource(int n) const;

	int checkActivatedFormNumber() const;

public slots:
	void onInstrumentFMEnvelopeParameterChanged(int envNum, int fromInstNum);
	void onInstrumentFMEnvelopeNumberChanged();
	void onInstrumentFMLFOParameterChanged(int lfoNum, int fromInstNum);
	void onInstrumentFMLFONumberChanged();
	void onInstrumentFMOperatorSequenceParameterChanged(FMEnvelopeParameter param, int opSeqNum, int fromInstNum);
	void onInstrumentFMOperatorSequenceNumberChanged();
	void onInstrumentFMArpeggioParameterChanged(int arpNum, int fromInstNum);
	void onInstrumentFMArpeggioNumberChanged();
	void onInstrumentFMPitchParameterChanged(int ptNum, int fromInstNum);
	void onInstrumentFMPitchNumberChanged();

	void onInstrumentSSGWaveformParameterChanged(int wfNum, int fromInstNum);
	void onInstrumentSSGWaveformNumberChanged();
	void onInstrumentSSGToneNoiseParameterChanged(int tnNum, int fromInstNum);
	void onInstrumentSSGToneNoiseNumberChanged();
	void onInstrumentSSGEnvelopeParameterChanged(int envNum, int fromInstNum);
	void onInstrumentSSGEnvelopeNumberChanged();
	void onInstrumentSSGArpeggioParameterChanged(int arpNum, int fromInstNum);
	void onInstrumentSSGArpeggioNumberChanged();
	void onInstrumentSSGPitchParameterChanged(int ptNum, int fromInstNum);
	void onInstrumentSSGPitchNumberChanged();

	void onInstrumentADPCMWaveformParameterChanged(int wfNum, int fromInstNum);
	void onInstrumentADPCMWaveformNumberChanged();
	void onInstrumentADPCMSampleMemoryUpdated();
	void onInstrumentADPCMEnvelopeParameterChanged(int envNum, int fromInstNum);
	void onInstrumentADPCMEnvelopeNumberChanged();
	void onInstrumentADPCMArpeggioParameterChanged(int arpNum, int fromInstNum);
	void onInstrumentADPCMArpeggioNumberChanged();
	void onInstrumentADPCMPitchParameterChanged(int ptNum, int fromInstNum);
	void onInstrumentADPCMPitchNumberChanged();

private:
	std::unordered_map<int, std::shared_ptr<QWidget>> map_;
};


#endif // INSTRUMENT_FORM_MANAGER_HPP
