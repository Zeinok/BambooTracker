#ifndef INSTRUMENT_EDITOR_ADPCM_FORM_HPP
#define INSTRUMENT_EDITOR_ADPCM_FORM_HPP

#include <memory>
#include <QWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QPixmap>
#include "bamboo_tracker.hpp"
#include "configuration.hpp"
#include "jam_manager.hpp"
#include "gui/color_palette.hpp"
#include "gui/instrument_editor/visualized_instrument_macro_editor.hpp"

namespace Ui {
	class InstrumentEditorADPCMForm;
}

class InstrumentEditorADPCMForm : public QWidget
{
	Q_OBJECT

public:
	explicit InstrumentEditorADPCMForm(int num, QWidget *parent = nullptr);
	~InstrumentEditorADPCMForm() override;
	int getInstrumentNumber() const;
	void setCore(std::weak_ptr<BambooTracker> core);
	void setConfiguration(std::weak_ptr<Configuration> config);
	void setColorPalette(std::shared_ptr<ColorPalette> palette);

signals:
	void jamKeyOnEvent(JamKey key);
	void jamKeyOffEvent(JamKey key);
	void modified();

protected:
	bool eventFilter(QObject* obj, QEvent* ev) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

private:
	Ui::InstrumentEditorADPCMForm *ui;
	int instNum_;
	bool isIgnoreEvent_;

	std::weak_ptr<BambooTracker> bt_;
	std::weak_ptr<Configuration> config_;
	std::shared_ptr<ColorPalette> palette_;

	SequenceType convertSequenceTypeForData(VisualizedInstrumentMacroEditor::SequenceType type);
	VisualizedInstrumentMacroEditor::SequenceType convertSequenceTypeForUI(SequenceType type);

	ReleaseType convertReleaseTypeForData(VisualizedInstrumentMacroEditor::ReleaseType type);
	VisualizedInstrumentMacroEditor::ReleaseType convertReleaseTypeForUI(ReleaseType type);

	void updateInstrumentParameters();

	//========== Waveform ==========//
signals:
	void waveformNumberChanged();
	void waveformParameterChanged(int wfNum, int fromInstNum);
	void waveformAssignRequested();

public slots:
	void onWaveformNumberChanged();
	void onWaveformParameterChanged(int wfNum);
	void onWaveformSampleMemoryUpdated();

private:
	std::unique_ptr<QPixmap> wavMemPixmap_;
	std::unique_ptr<QPixmap> wavViewPixmap_;

	void setInstrumentWaveformParameters();
	void importSampleFrom(const QString file);
	void updateSampleMemoryBar();
	void updateSampleView();

private slots:
	void on_waveNumSpinBox_valueChanged(int arg1);
	void on_rootRateSpinBox_valueChanged(int arg1);
	void on_waveRepeatCheckBox_toggled(bool checked);
	void on_waveImportPushButton_clicked();
	void on_waveClearPushButton_clicked();

	//========== Envelope ==========//
signals:
	void envelopeNumberChanged();
	void envelopeParameterChanged(int wfNum, int fromInstNum);

public slots:
	void onEnvelopeNumberChanged();
	void onEnvelopeParameterChanged(int envNum);

private:
	void setInstrumentEnvelopeParameters();

private slots:
	void on_envEditGroupBox_toggled(bool arg1);
	void on_envNumSpinBox_valueChanged(int arg1);

	//========== Arpeggio ==========//
signals:
	void arpeggioNumberChanged();
	void arpeggioParameterChanged(int arpNum, int fromInstNum);

public slots:
	void onArpeggioNumberChanged();
	void onArpeggioParameterChanged(int arpNum);

private:
	void setInstrumentArpeggioParameters();

private slots:
	void onArpeggioTypeChanged(int index);
	void on_arpEditGroupBox_toggled(bool arg1);
	void on_arpNumSpinBox_valueChanged(int arg1);

	//========== Pitch ==========//
signals:
	void pitchNumberChanged();
	void pitchParameterChanged(int ptNum, int fromInstNum);

public slots:
	void onPitchNumberChanged();
	void onPitchParameterChanged(int arpNum);

private:
	void setInstrumentPitchParameters();

private slots:
	void onPitchTypeChanged(int index);
	void on_ptEditGroupBox_toggled(bool arg1);
	void on_ptNumSpinBox_valueChanged(int arg1);
};

#endif // INSTRUMENT_EDITOR_ADPCM_FORM_HPP
