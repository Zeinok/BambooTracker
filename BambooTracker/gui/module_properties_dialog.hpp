#ifndef MODULE_PROPERTIES_DIALOG_HPP
#define MODULE_PROPERTIES_DIALOG_HPP

#include <QDialog>
#include <QString>
#include <QTreeWidgetItem>
#include <memory>
#include "bamboo_tracker.hpp"
#include "misc.hpp"

namespace Ui {
	class ModulePropertiesDialog;
}

class ModulePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	ModulePropertiesDialog(std::weak_ptr<BambooTracker> core, double configFmMixer,
						   double configSsgMixer, QWidget *parent = nullptr);
	~ModulePropertiesDialog() override;

public slots:
	void onAccepted();

private slots:
	void on_upToolButton_clicked();
	void on_downToolButton_clicked();
	void on_removePushButton_clicked();
	void on_insertPushButton_clicked();
	void on_songTreeWidget_itemSelectionChanged();
	void on_editTitleLineEdit_textEdited(const QString &arg1);

	void on_mixerTypeComboBox_currentIndexChanged(int index);

	void on_customMixerSetPushButton_clicked();

private:
	Ui::ModulePropertiesDialog *ui;
	std::weak_ptr<BambooTracker> bt_;

	double fmMixer_, ssgMixer_;
	double configFmMixer_, configSsgMixer_;

	void insertSong(int row, QString title, SongType type, int prevNum = -1);
	void checkButtonsEnabled();
	void swapset(int aboveRow, int belowRow);

	void setCustomMixerLevels(double fm, double ssg);
};

#endif // MODULE_PROPERTIES_DIALOG_HPP
