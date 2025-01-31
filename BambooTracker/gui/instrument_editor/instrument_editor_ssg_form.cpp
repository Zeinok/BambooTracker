#include "instrument_editor_ssg_form.hpp"
#include "ui_instrument_editor_ssg_form.h"
#include <vector>
#include <utility>
#include <stdexcept>
#include "gui/event_guard.hpp"
#include "pitch_converter.hpp"
#include "command_sequence.hpp"
#include "gui/jam_layout.hpp"
#include "misc.hpp"

InstrumentEditorSSGForm::InstrumentEditorSSGForm(int num, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::InstrumentEditorSSGForm),
	instNum_(num)
{
	ui->setupUi(this);

	//========== Waveform ==========//
	ui->waveEditor->setMaximumDisplayedRowCount(7);
	ui->waveEditor->setDefaultRow(0);
	ui->waveEditor->AddRow(tr("Sq"), false);
	ui->waveEditor->AddRow(tr("Tri"), false);
	ui->waveEditor->AddRow(tr("Saw"), false);
	ui->waveEditor->AddRow(tr("InvSaw"), false);
	ui->waveEditor->AddRow(tr("SMTri"), false);
	ui->waveEditor->AddRow(tr("SMSaw"), false);
	ui->waveEditor->AddRow(tr("SMInvSaw"), false);
	ui->waveEditor->autoFitLabelWidth();

	QObject::connect(ui->waveEditor, &VisualizedInstrumentMacroEditor::sequenceCommandAdded,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			if (isModulatedWaveformSSG(row)) setWaveformSequenceColumn(col);	// Set square-mask frequency
			bt_.lock()->addWaveformSSGSequenceCommand(
						ui->waveNumSpinBox->value(), row, ui->waveEditor->getSequenceDataAt(col));
			emit waveformParameterChanged(ui->waveNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->waveEditor, &VisualizedInstrumentMacroEditor::sequenceCommandRemoved,
					 this, [&]() {
		if (!isIgnoreEvent_) {
			bt_.lock()->removeWaveformSSGSequenceCommand(ui->waveNumSpinBox->value());
			emit waveformParameterChanged(ui->waveNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->waveEditor, &VisualizedInstrumentMacroEditor::sequenceCommandChanged,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			if (isModulatedWaveformSSG(row)) setWaveformSequenceColumn(col);	// Set square-mask frequency
			bt_.lock()->setWaveformSSGSequenceCommand(
						ui->waveNumSpinBox->value(), col, row, ui->waveEditor->getSequenceDataAt(col));
			emit waveformParameterChanged(ui->waveNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->waveEditor, &VisualizedInstrumentMacroEditor::loopChanged,
					 this, [&](std::vector<int> begins, std::vector<int> ends, std::vector<int> times) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setWaveformSSGLoops(
						ui->waveNumSpinBox->value(), std::move(begins), std::move(ends), std::move(times));
			emit waveformParameterChanged(ui->waveNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->waveEditor, &VisualizedInstrumentMacroEditor::releaseChanged,
					 this, [&](VisualizedInstrumentMacroEditor::ReleaseType type, int point) {
		if (!isIgnoreEvent_) {
			ReleaseType t = convertReleaseTypeForData(type);
			bt_.lock()->setWaveformSSGRelease(ui->waveNumSpinBox->value(), t, point);
			emit waveformParameterChanged(ui->waveNumSpinBox->value(), instNum_);
			emit modified();
		}
	});

	//========== Tone/Noise ==========//
	QObject::connect(ui->tnEditor, &VisualizedInstrumentMacroEditor::sequenceCommandAdded,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->addToneNoiseSSGSequenceCommand(
						ui->tnNumSpinBox->value(), row, ui->tnEditor->getSequenceDataAt(col));
			emit toneNoiseParameterChanged(ui->tnNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->tnEditor, &VisualizedInstrumentMacroEditor::sequenceCommandRemoved,
					 this, [&]() {
		if (!isIgnoreEvent_) {
			bt_.lock()->removeToneNoiseSSGSequenceCommand(ui->tnNumSpinBox->value());
			emit toneNoiseParameterChanged(ui->tnNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->tnEditor, &VisualizedInstrumentMacroEditor::sequenceCommandChanged,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setToneNoiseSSGSequenceCommand(
						ui->tnNumSpinBox->value(), col, row, ui->tnEditor->getSequenceDataAt(col));
			emit toneNoiseParameterChanged(ui->tnNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->tnEditor, &VisualizedInstrumentMacroEditor::loopChanged,
					 this, [&](std::vector<int> begins, std::vector<int> ends, std::vector<int> times) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setToneNoiseSSGLoops(
						ui->tnNumSpinBox->value(), std::move(begins), std::move(ends), std::move(times));
			emit toneNoiseParameterChanged(ui->tnNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->tnEditor, &VisualizedInstrumentMacroEditor::releaseChanged,
					 this, [&](VisualizedInstrumentMacroEditor::ReleaseType type, int point) {
		if (!isIgnoreEvent_) {
			ReleaseType t = convertReleaseTypeForData(type);
			bt_.lock()->setToneNoiseSSGRelease(ui->tnNumSpinBox->value(), t, point);
			emit toneNoiseParameterChanged(ui->tnNumSpinBox->value(), instNum_);
			emit modified();
		}
	});

	//========== Envelope ==========//
	ui->envEditor->setMaximumDisplayedRowCount(16);
	ui->envEditor->setDefaultRow(15);
	for (int i = 0; i < 16; ++i) {
		ui->envEditor->AddRow(QString::number(i), false);
	}
	for (int i = 0; i < 8; ++i) {
		ui->envEditor->AddRow(tr("HEnv %1").arg(i), false);
	}
	ui->envEditor->autoFitLabelWidth();
	ui->envEditor->setMultipleReleaseState(true);
	ui->envEditor->setPermittedReleaseTypes(
				VisualizedInstrumentMacroEditor::ReleaseType::ABSOLUTE_RELEASE
				| VisualizedInstrumentMacroEditor::ReleaseType::RELATIVE_RELEASE
				| VisualizedInstrumentMacroEditor::ReleaseType::FIXED_RELEASE);

	QObject::connect(ui->envEditor, &VisualizedInstrumentMacroEditor::sequenceCommandAdded,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			if (row >= 16) setEnvelopeSequenceColumn(col);	// Set hard frequency
			bt_.lock()->addEnvelopeSSGSequenceCommand(
						ui->envNumSpinBox->value(), row, ui->envEditor->getSequenceDataAt(col));
			emit envelopeParameterChanged(ui->envNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->envEditor, &VisualizedInstrumentMacroEditor::sequenceCommandRemoved,
					 this, [&]() {
		if (!isIgnoreEvent_) {
			bt_.lock()->removeEnvelopeSSGSequenceCommand(ui->envNumSpinBox->value());
			emit envelopeParameterChanged(ui->envNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->envEditor, &VisualizedInstrumentMacroEditor::sequenceCommandChanged,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			if (row >= 16) setEnvelopeSequenceColumn(col);	// Set hard frequency
			bt_.lock()->setEnvelopeSSGSequenceCommand(
						ui->envNumSpinBox->value(), col, row, ui->envEditor->getSequenceDataAt(col));
			emit envelopeParameterChanged(ui->envNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->envEditor, &VisualizedInstrumentMacroEditor::loopChanged,
					 this, [&](std::vector<int> begins, std::vector<int> ends, std::vector<int> times) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setEnvelopeSSGLoops(
						ui->envNumSpinBox->value(), std::move(begins), std::move(ends), std::move(times));
			emit envelopeParameterChanged(ui->envNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->envEditor, &VisualizedInstrumentMacroEditor::releaseChanged,
					 this, [&](VisualizedInstrumentMacroEditor::ReleaseType type, int point) {
		if (!isIgnoreEvent_) {
			ReleaseType t = convertReleaseTypeForData(type);
			bt_.lock()->setEnvelopeSSGRelease(ui->envNumSpinBox->value(), t, point);
			emit envelopeParameterChanged(ui->envNumSpinBox->value(), instNum_);
			emit modified();
		}
	});

	//========== Arpeggio ==========//
	ui->arpTypeComboBox->addItem(tr("Absolute"), VisualizedInstrumentMacroEditor::SequenceType::AbsoluteSequence);
	ui->arpTypeComboBox->addItem(tr("Fixed"), VisualizedInstrumentMacroEditor::SequenceType::FixedSequence);
	ui->arpTypeComboBox->addItem(tr("Relative"), VisualizedInstrumentMacroEditor::SequenceType::RelativeSequence);

	QObject::connect(ui->arpEditor, &VisualizedInstrumentMacroEditor::sequenceCommandAdded,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->addArpeggioSSGSequenceCommand(
						ui->arpNumSpinBox->value(), row, ui->arpEditor->getSequenceDataAt(col));
			emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->arpEditor, &VisualizedInstrumentMacroEditor::sequenceCommandRemoved,
					 this, [&]() {
		if (!isIgnoreEvent_) {
			bt_.lock()->removeArpeggioSSGSequenceCommand(ui->arpNumSpinBox->value());
			emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->arpEditor, &VisualizedInstrumentMacroEditor::sequenceCommandChanged,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setArpeggioSSGSequenceCommand(
						ui->arpNumSpinBox->value(), col, row, ui->arpEditor->getSequenceDataAt(col));
			emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->arpEditor, &VisualizedInstrumentMacroEditor::loopChanged,
					 this, [&](std::vector<int> begins, std::vector<int> ends, std::vector<int> times) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setArpeggioSSGLoops(
						ui->arpNumSpinBox->value(), std::move(begins), std::move(ends), std::move(times));
			emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->arpEditor, &VisualizedInstrumentMacroEditor::releaseChanged,
					 this, [&](VisualizedInstrumentMacroEditor::ReleaseType type, int point) {
		if (!isIgnoreEvent_) {
			ReleaseType t = convertReleaseTypeForData(type);
			bt_.lock()->setArpeggioSSGRelease(ui->arpNumSpinBox->value(), t, point);
			emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	// Leave Before Qt5.7.0 style due to windows xp
	QObject::connect(ui->arpTypeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
					 this, &InstrumentEditorSSGForm::onArpeggioTypeChanged);

	//========== Pitch ==========//
	ui->ptEditor->setMaximumDisplayedRowCount(15);
	ui->ptEditor->setDefaultRow(127);
	ui->ptEditor->setLabelDiaplayMode(true);
	for (int i = 0; i < 255; ++i) {
		ui->ptEditor->AddRow(QString::asprintf("%+d", i - 127), false);
	}
	ui->ptEditor->autoFitLabelWidth();
	ui->ptEditor->setUpperRow(134);
	ui->ptEditor->setMMLDisplay0As(-127);

	ui->ptTypeComboBox->addItem(tr("Absolute"), VisualizedInstrumentMacroEditor::SequenceType::AbsoluteSequence);
	ui->ptTypeComboBox->addItem(tr("Relative"), VisualizedInstrumentMacroEditor::SequenceType::RelativeSequence);

	QObject::connect(ui->ptEditor, &VisualizedInstrumentMacroEditor::sequenceCommandAdded,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->addPitchSSGSequenceCommand(
						ui->ptNumSpinBox->value(), row, ui->ptEditor->getSequenceDataAt(col));
			emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->ptEditor, &VisualizedInstrumentMacroEditor::sequenceCommandRemoved,
					 this, [&]() {
		if (!isIgnoreEvent_) {
			bt_.lock()->removePitchSSGSequenceCommand(ui->ptNumSpinBox->value());
			emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->ptEditor, &VisualizedInstrumentMacroEditor::sequenceCommandChanged,
					 this, [&](int row, int col) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setPitchSSGSequenceCommand(
						ui->ptNumSpinBox->value(), col, row, ui->ptEditor->getSequenceDataAt(col));
			emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->ptEditor, &VisualizedInstrumentMacroEditor::loopChanged,
					 this, [&](std::vector<int> begins, std::vector<int> ends, std::vector<int> times) {
		if (!isIgnoreEvent_) {
			bt_.lock()->setPitchSSGLoops(
						ui->ptNumSpinBox->value(), std::move(begins), std::move(ends), std::move(times));
			emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	QObject::connect(ui->ptEditor, &VisualizedInstrumentMacroEditor::releaseChanged,
					 this, [&](VisualizedInstrumentMacroEditor::ReleaseType type, int point) {
		if (!isIgnoreEvent_) {
			ReleaseType t = convertReleaseTypeForData(type);
			bt_.lock()->setPitchSSGRelease(ui->ptNumSpinBox->value(), t, point);
			emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
			emit modified();
		}
	});
	// Leave Before Qt5.7.0 style due to windows xp
	QObject::connect(ui->ptTypeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
					 this, &InstrumentEditorSSGForm::onPitchTypeChanged);
}

InstrumentEditorSSGForm::~InstrumentEditorSSGForm()
{
	delete ui;
}

int InstrumentEditorSSGForm::getInstrumentNumber() const
{
	return instNum_;
}

void InstrumentEditorSSGForm::setCore(std::weak_ptr<BambooTracker> core)
{
	bt_ = core;
	updateInstrumentParameters();
}

void InstrumentEditorSSGForm::setConfiguration(std::weak_ptr<Configuration> config)
{
	config_ = config;
}

void InstrumentEditorSSGForm::setColorPalette(std::shared_ptr<ColorPalette> palette)
{
	ui->waveEditor->setColorPalette(palette);
	ui->tnEditor->setColorPalette(palette);
	ui->envEditor->setColorPalette(palette);
	ui->arpEditor->setColorPalette(palette);
	ui->ptEditor->setColorPalette(palette);
}

SequenceType InstrumentEditorSSGForm::convertSequenceTypeForData(VisualizedInstrumentMacroEditor::SequenceType type)
{
	switch (type) {
	case VisualizedInstrumentMacroEditor::SequenceType::NoType:
		return SequenceType::NO_SEQUENCE_TYPE;
	case VisualizedInstrumentMacroEditor::SequenceType::FixedSequence:
		return SequenceType::FIXED_SEQUENCE;
	case VisualizedInstrumentMacroEditor::SequenceType::AbsoluteSequence:
		return SequenceType::ABSOLUTE_SEQUENCE;
	case VisualizedInstrumentMacroEditor::SequenceType::RelativeSequence:
		return SequenceType::RELATIVE_SEQUENCE;
	default:
		throw std::invalid_argument("Unexpected SequenceType.");
	}
}

VisualizedInstrumentMacroEditor::SequenceType InstrumentEditorSSGForm::convertSequenceTypeForUI(SequenceType type)
{
	switch (type) {
	case SequenceType::NO_SEQUENCE_TYPE:
		return VisualizedInstrumentMacroEditor::SequenceType::NoType;
	case SequenceType::FIXED_SEQUENCE:
		return VisualizedInstrumentMacroEditor::SequenceType::FixedSequence;
	case SequenceType::ABSOLUTE_SEQUENCE:
		return VisualizedInstrumentMacroEditor::SequenceType::AbsoluteSequence;
	case SequenceType::RELATIVE_SEQUENCE:
		return VisualizedInstrumentMacroEditor::SequenceType::RelativeSequence;
	default:
		throw std::invalid_argument("Unexpected SequenceType.");
	}
}

ReleaseType InstrumentEditorSSGForm::convertReleaseTypeForData(VisualizedInstrumentMacroEditor::ReleaseType type)
{
	switch (type) {
	case VisualizedInstrumentMacroEditor::ReleaseType::NO_RELEASE:
		return ReleaseType::NoRelease;
	case VisualizedInstrumentMacroEditor::ReleaseType::FIXED_RELEASE:
		return ReleaseType::FixedRelease;
	case VisualizedInstrumentMacroEditor::ReleaseType::ABSOLUTE_RELEASE:
		return ReleaseType::AbsoluteRelease;
	case VisualizedInstrumentMacroEditor::ReleaseType::RELATIVE_RELEASE:
		return ReleaseType::RelativeRelease;
	default:
		throw std::invalid_argument("Unexpected ReleaseType.");
	}
}

VisualizedInstrumentMacroEditor::ReleaseType InstrumentEditorSSGForm::convertReleaseTypeForUI(ReleaseType type)
{
	switch (type) {
	case ReleaseType::NoRelease:
		return VisualizedInstrumentMacroEditor::ReleaseType::NO_RELEASE;
	case ReleaseType::FixedRelease:
		return VisualizedInstrumentMacroEditor::ReleaseType::FIXED_RELEASE;
	case ReleaseType::AbsoluteRelease:
		return VisualizedInstrumentMacroEditor::ReleaseType::ABSOLUTE_RELEASE;
	case ReleaseType::RelativeRelease:
		return VisualizedInstrumentMacroEditor::ReleaseType::RELATIVE_RELEASE;
	default:
		throw std::invalid_argument("Unexpected ReleaseType.");
	}
}

void InstrumentEditorSSGForm::updateInstrumentParameters()
{
	Ui::EventGuard eg(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());
	auto name = QString::fromUtf8(instSSG->getName().c_str(), static_cast<int>(instSSG->getName().length()));
	setWindowTitle(QString("%1: %2").arg(instNum_, 2, 16, QChar('0')).toUpper().arg(name));

	setInstrumentWaveformParameters();
	setInstrumentToneNoiseParameters();
	setInstrumentEnvelopeParameters();
	setInstrumentArpeggioParameters();
	setInstrumentPitchParameters();
}

/********** Events **********/
// MUST DIRECT CONNECTION
void InstrumentEditorSSGForm::keyPressEvent(QKeyEvent *event)
{
	// General keys
	switch (event->key()) {
	case Qt::Key_Escape:
		close();
		break;
	default:
		// For jam key on
		if (!event->isAutoRepeat()) {
			// Musical keyboard
			Qt::Key qtKey = static_cast<Qt::Key>(event->key());
			try {
				JamKey jk = getJamKeyFromLayoutMapping(qtKey, config_);
				emit jamKeyOnEvent(jk);
			} catch (std::invalid_argument&) {}
		}
		break;
	}
}

// MUST DIRECT CONNECTION
void InstrumentEditorSSGForm::keyReleaseEvent(QKeyEvent *event)
{
	// For jam key off
	if (!event->isAutoRepeat()) {
		Qt::Key qtKey = static_cast<Qt::Key>(event->key());
		try {
			JamKey jk = getJamKeyFromLayoutMapping(qtKey, config_);
			emit jamKeyOffEvent(jk);
		} catch (std::invalid_argument&) {}
	}
}

//--- Waveform
void InstrumentEditorSSGForm::setInstrumentWaveformParameters()
{
	Ui::EventGuard ev(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());

	ui->waveNumSpinBox->setValue(instSSG->getWaveformNumber());
	ui->waveEditor->clearData();
	for (auto& com : instSSG->getWaveformSequence()) {
		QString str("");
		if (isModulatedWaveformSSG(com.type)) {
			if (CommandSequenceUnit::checkDataType(com.data) == CommandSequenceUnit::RATIO) {
				auto ratio = CommandSequenceUnit::data2ratio(com.data);
				str = QString("%1/%2").arg(ratio.first).arg(ratio.second);
			}
			else {
				str = QString::number(com.data);
			}
		}
		ui->waveEditor->addSequenceCommand(com.type, str, com.data);
	}
	for (auto& l : instSSG->getWaveformLoops()) {
		ui->waveEditor->addLoop(l.begin, l.end, l.times);
	}
	ui->waveEditor->setRelease(convertReleaseTypeForUI(instSSG->getWaveformRelease().type),
							   instSSG->getWaveformRelease().begin);
	if (instSSG->getWaveformEnabled()) {
		ui->waveEditGroupBox->setChecked(true);
		onWaveformNumberChanged();
	}
	else {
		ui->waveEditGroupBox->setChecked(false);
	}
}

void InstrumentEditorSSGForm::setWaveformSequenceColumn(int col)
{
	auto button = ui->squareMaskButtonGroup->checkedButton();
	if (button == ui->squareMaskRawRadioButton) {
		ui->waveEditor->setText(col, QString::number(ui->squareMaskRawSpinBox->value()));
		ui->waveEditor->setData(col, ui->squareMaskRawSpinBox->value());
	}
	else {
		ui->waveEditor->setText(col, QString::number(ui->squareMaskToneSpinBox->value()) + "/"
								+ QString::number(ui->squareMaskMaskSpinBox->value()));

		ui->waveEditor->setData(col, CommandSequenceUnit::ratio2data(
									ui->squareMaskToneSpinBox->value(),
									ui->squareMaskMaskSpinBox->value()));
	}
}

/********** Slots **********/
void InstrumentEditorSSGForm::onWaveformNumberChanged()
{
	// Change users view
	std::vector<int> users = bt_.lock()->getWaveformSSGUsers(ui->waveNumSpinBox->value());
	QStringList l;
	std::transform(users.begin(), users.end(), std::back_inserter(l), [](int n) {
		return QString("%1").arg(n, 2, 16, QChar('0')).toUpper();
	});
	ui->waveUsersLineEdit->setText(l.join(","));
}

void InstrumentEditorSSGForm::onWaveformParameterChanged(int wfNum)
{
	if (ui->waveNumSpinBox->value() == wfNum) {
		Ui::EventGuard eg(isIgnoreEvent_);
		setInstrumentWaveformParameters();
	}
}

void InstrumentEditorSSGForm::on_waveEditGroupBox_toggled(bool arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGWaveformEnabled(instNum_, arg1);
		setInstrumentWaveformParameters();
		emit waveformNumberChanged();
		emit modified();
	}

	onWaveformNumberChanged();
}

void InstrumentEditorSSGForm::on_waveNumSpinBox_valueChanged(int arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGWaveform(instNum_, arg1);
		setInstrumentWaveformParameters();
		emit waveformNumberChanged();
		emit modified();
	}

	onWaveformNumberChanged();
}

void InstrumentEditorSSGForm::on_squareMaskRawSpinBox_valueChanged(int arg1)
{
	ui->squareMaskRawSpinBox->setSuffix(
				QString(" (0x") + QString("%1 | ").arg(arg1, 3, 16, QChar('0')).toUpper()
				+ QString("%1Hz)").arg(arg1 ? QString::number(124800.0 / arg1, 'f', 4) : "-")
				);
}

//--- Tone/Noise
void InstrumentEditorSSGForm::setInstrumentToneNoiseParameters()
{
	Ui::EventGuard ev(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());

	ui->tnNumSpinBox->setValue(instSSG->getToneNoiseNumber());
	ui->tnEditor->clearData();
	for (auto& com : instSSG->getToneNoiseSequence()) {
		ui->tnEditor->addSequenceCommand(com.type);
	}
	for (auto& l : instSSG->getToneNoiseLoops()) {
		ui->tnEditor->addLoop(l.begin, l.end, l.times);
	}
	ui->tnEditor->setRelease(convertReleaseTypeForUI(instSSG->getToneNoiseRelease().type),
							 instSSG->getToneNoiseRelease().begin);
	if (instSSG->getToneNoiseEnabled()) {
		ui->tnEditGroupBox->setChecked(true);
		onToneNoiseNumberChanged();
	}
	else {
		ui->tnEditGroupBox->setChecked(false);
	}
}

/********** Slots **********/
void InstrumentEditorSSGForm::onToneNoiseNumberChanged()
{
	// Change users view
	std::vector<int> users = bt_.lock()->getToneNoiseSSGUsers(ui->tnNumSpinBox->value());
	QStringList l;
	std::transform(users.begin(), users.end(), std::back_inserter(l), [](int n) {
		return QString("%1").arg(n, 2, 16, QChar('0')).toUpper();
	});
	ui->tnUsersLineEdit->setText(l.join((",")));
}

void InstrumentEditorSSGForm::onToneNoiseParameterChanged(int tnNum)
{
	if (ui->tnNumSpinBox->value() == tnNum) {
		Ui::EventGuard eg(isIgnoreEvent_);
		setInstrumentToneNoiseParameters();
	}
}

void InstrumentEditorSSGForm::on_tnEditGroupBox_toggled(bool arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGToneNoiseEnabled(instNum_, arg1);
		setInstrumentToneNoiseParameters();
		emit toneNoiseNumberChanged();
		emit modified();
	}

	onToneNoiseNumberChanged();
}

void InstrumentEditorSSGForm::on_tnNumSpinBox_valueChanged(int arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGToneNoise(instNum_, arg1);
		setInstrumentToneNoiseParameters();
		emit toneNoiseNumberChanged();
		emit modified();
	}

	onToneNoiseNumberChanged();
}

//--- Envelope
void InstrumentEditorSSGForm::setInstrumentEnvelopeParameters()
{
	Ui::EventGuard ev(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());

	ui->envNumSpinBox->setValue(instSSG->getEnvelopeNumber());
	ui->envEditor->clearData();
	for (auto& com : instSSG->getEnvelopeSequence()) {
		QString str("");
		if (com.type >= 16) {
			if (CommandSequenceUnit::checkDataType(com.data) == CommandSequenceUnit::RATIO) {
				auto ratio = CommandSequenceUnit::data2ratio(com.data);
				str = QString("%1/%2").arg(ratio.first).arg(ratio.second);
			}
			else {
				str = QString::number(com.data);
			}
		}
		ui->envEditor->addSequenceCommand(com.type, str, com.data);
	}
	for (auto& l : instSSG->getEnvelopeLoops()) {
		ui->envEditor->addLoop(l.begin, l.end, l.times);
	}
	ui->envEditor->setRelease(convertReleaseTypeForUI(instSSG->getEnvelopeRelease().type),
							  instSSG->getEnvelopeRelease().begin);
	if (instSSG->getEnvelopeEnabled()) {
		ui->envEditGroupBox->setChecked(true);
		onEnvelopeNumberChanged();
	}
	else {
		ui->envEditGroupBox->setChecked(false);
	}
}

void InstrumentEditorSSGForm::setEnvelopeSequenceColumn(int col)
{
	if (ui->hardFreqButtonGroup->checkedButton() == ui->hardFreqRawRadioButton) {
		ui->envEditor->setText(col, QString::number(ui->hardFreqRawSpinBox->value()));
		ui->envEditor->setData(col, ui->hardFreqRawSpinBox->value());
	}
	else {
		ui->envEditor->setText(col, QString::number(ui->hardFreqToneSpinBox->value()) + "/"
							   + QString::number(ui->hardFreqHardSpinBox->value()));

		ui->envEditor->setData(col, CommandSequenceUnit::ratio2data(
								   ui->hardFreqToneSpinBox->value(),
								   ui->hardFreqHardSpinBox->value()));
	}
}

/********** Slots **********/
void InstrumentEditorSSGForm::onEnvelopeNumberChanged()
{
	// Change users view
	std::vector<int> users = bt_.lock()->getEnvelopeSSGUsers(ui->envNumSpinBox->value());
	QStringList l;
	std::transform(users.begin(), users.end(), std::back_inserter(l), [](int n) {
		return QString("%1").arg(n, 2, 16, QChar('0')).toUpper();
	});
	ui->envUsersLineEdit->setText(l.join((",")));
}

void InstrumentEditorSSGForm::onEnvelopeParameterChanged(int envNum)
{
	if (ui->envNumSpinBox->value() == envNum) {
		Ui::EventGuard eg(isIgnoreEvent_);
		setInstrumentEnvelopeParameters();
	}
}

void InstrumentEditorSSGForm::on_envEditGroupBox_toggled(bool arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGEnvelopeEnabled(instNum_, arg1);
		setInstrumentEnvelopeParameters();
		emit envelopeNumberChanged();
		emit modified();
	}

	onEnvelopeNumberChanged();
}

void InstrumentEditorSSGForm::on_envNumSpinBox_valueChanged(int arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGEnvelope(instNum_, arg1);
		setInstrumentEnvelopeParameters();
		emit envelopeNumberChanged();
		emit modified();
	}

	onEnvelopeNumberChanged();
}

void InstrumentEditorSSGForm::on_hardFreqRawSpinBox_valueChanged(int arg1)
{
	ui->hardFreqRawSpinBox->setSuffix(
				QString(" (0x") + QString("%1 | ").arg(arg1, 4, 16, QChar('0')).toUpper()
				+ QString("%1Hz").arg(arg1 ? QString::number(7800.0 / arg1, 'f', 4) : "-"));
}

//--- Arpeggio
void InstrumentEditorSSGForm::setInstrumentArpeggioParameters()
{
	Ui::EventGuard ev(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());

	ui->arpNumSpinBox->setValue(instSSG->getArpeggioNumber());
	ui->arpEditor->clearData();
	for (auto& com : instSSG->getArpeggioSequence()) {
		ui->arpEditor->addSequenceCommand(com.type);
	}
	for (auto& l : instSSG->getArpeggioLoops()) {
		ui->arpEditor->addLoop(l.begin, l.end, l.times);
	}
	ui->arpEditor->setRelease(convertReleaseTypeForUI(instSSG->getArpeggioRelease().type),
							  instSSG->getArpeggioRelease().begin);
	for (int i = 0; i < ui->arpTypeComboBox->count(); ++i) {
		if (instSSG->getArpeggioType() == convertSequenceTypeForData(
					static_cast<VisualizedInstrumentMacroEditor::SequenceType>(ui->arpTypeComboBox->itemData(i).toInt()))) {
			ui->arpTypeComboBox->setCurrentIndex(i);
			break;
		}
	}
	if (instSSG->getArpeggioEnabled()) {
		ui->arpEditGroupBox->setChecked(true);
		onArpeggioNumberChanged();
	}
	else {
		ui->arpEditGroupBox->setChecked(false);
	}
}

/********** Slots **********/
void InstrumentEditorSSGForm::onArpeggioNumberChanged()
{
	// Change users view
	std::vector<int> users = bt_.lock()->getArpeggioSSGUsers(ui->arpNumSpinBox->value());
	QStringList l;
	std::transform(users.begin(), users.end(), std::back_inserter(l), [](int n) {
		return QString("%1").arg(n, 2, 16, QChar('0')).toUpper();
	});
	ui->arpUsersLineEdit->setText(l.join(","));
}

void InstrumentEditorSSGForm::onArpeggioParameterChanged(int tnNum)
{
	if (ui->arpNumSpinBox->value() == tnNum) {
		Ui::EventGuard eg(isIgnoreEvent_);
		setInstrumentArpeggioParameters();
	}
}

void InstrumentEditorSSGForm::onArpeggioTypeChanged(int index)
{
	Q_UNUSED(index)

	auto type = static_cast<VisualizedInstrumentMacroEditor::SequenceType>(
					ui->arpTypeComboBox->currentData(Qt::UserRole).toInt());
	if (!isIgnoreEvent_) {
		bt_.lock()->setArpeggioSSGType(ui->arpNumSpinBox->value(), convertSequenceTypeForData(type));
		emit arpeggioParameterChanged(ui->arpNumSpinBox->value(), instNum_);
		emit modified();
	}

	ui->arpEditor->setSequenceType(type);
}

void InstrumentEditorSSGForm::on_arpEditGroupBox_toggled(bool arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGArpeggioEnabled(instNum_, arg1);
		setInstrumentArpeggioParameters();
		emit arpeggioNumberChanged();
		emit modified();
	}

	onArpeggioNumberChanged();
}

void InstrumentEditorSSGForm::on_arpNumSpinBox_valueChanged(int arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGArpeggio(instNum_, arg1);
		setInstrumentArpeggioParameters();
		emit arpeggioNumberChanged();
		emit modified();
	}

	onArpeggioNumberChanged();
}

//--- Pitch
void InstrumentEditorSSGForm::setInstrumentPitchParameters()
{
	Ui::EventGuard ev(isIgnoreEvent_);

	std::unique_ptr<AbstractInstrument> inst = bt_.lock()->getInstrument(instNum_);
	auto instSSG = dynamic_cast<InstrumentSSG*>(inst.get());

	ui->ptNumSpinBox->setValue(instSSG->getPitchNumber());
	ui->ptEditor->clearData();
	for (auto& com : instSSG->getPitchSequence()) {
		ui->ptEditor->addSequenceCommand(com.type);
	}
	for (auto& l : instSSG->getPitchLoops()) {
		ui->ptEditor->addLoop(l.begin, l.end, l.times);
	}
	ui->ptEditor->setRelease(convertReleaseTypeForUI(instSSG->getPitchRelease().type),
							 instSSG->getPitchRelease().begin);
	for (int i = 0; i < ui->ptTypeComboBox->count(); ++i) {
		if (instSSG->getPitchType() == convertSequenceTypeForData(
					static_cast<VisualizedInstrumentMacroEditor::SequenceType>(ui->ptTypeComboBox->itemData(i).toInt()))) {
			ui->ptTypeComboBox->setCurrentIndex(i);
			break;
		}
	}
	if (instSSG->getPitchEnabled()) {
		ui->ptEditGroupBox->setChecked(true);
		onPitchNumberChanged();
	}
	else {
		ui->ptEditGroupBox->setChecked(false);
	}
}

/********** Slots **********/
void InstrumentEditorSSGForm::onPitchNumberChanged()
{
	// Change users view
	std::vector<int> users = bt_.lock()->getPitchSSGUsers(ui->ptNumSpinBox->value());
	QStringList l;
	std::transform(users.begin(), users.end(), std::back_inserter(l), [](int n) {
		return QString("%1").arg(n, 2, 16, QChar('0')).toUpper();
	});
	ui->ptUsersLineEdit->setText(l.join(","));
}

void InstrumentEditorSSGForm::onPitchParameterChanged(int tnNum)
{
	if (ui->ptNumSpinBox->value() == tnNum) {
		Ui::EventGuard eg(isIgnoreEvent_);
		setInstrumentPitchParameters();
	}
}

void InstrumentEditorSSGForm::onPitchTypeChanged(int index)
{
	Q_UNUSED(index)

	auto type = static_cast<VisualizedInstrumentMacroEditor::SequenceType>(ui->ptTypeComboBox->currentData(Qt::UserRole).toInt());
	if (!isIgnoreEvent_) {
		bt_.lock()->setPitchSSGType(ui->ptNumSpinBox->value(), convertSequenceTypeForData(type));
		emit pitchParameterChanged(ui->ptNumSpinBox->value(), instNum_);
		emit modified();
	}

	ui->ptEditor->setSequenceType(type);
}

void InstrumentEditorSSGForm::on_ptEditGroupBox_toggled(bool arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGPitchEnabled(instNum_, arg1);
		setInstrumentPitchParameters();
		emit pitchNumberChanged();
		emit modified();
	}

	onPitchNumberChanged();
}

void InstrumentEditorSSGForm::on_ptNumSpinBox_valueChanged(int arg1)
{
	if (!isIgnoreEvent_) {
		bt_.lock()->setInstrumentSSGPitch(instNum_, arg1);
		setInstrumentPitchParameters();
		emit pitchNumberChanged();
		emit modified();
	}

	onPitchNumberChanged();
}
