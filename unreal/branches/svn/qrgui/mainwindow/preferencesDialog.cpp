#include "preferencesDialog.h"
#include "ui_preferencesDialog.h"

#include <QSettings>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PreferencesDialog)
{
	ui->setupUi(this);
	initPreferences();
}

PreferencesDialog::~PreferencesDialog()
{
	delete ui;
}

void PreferencesDialog::initPreferences()
{
	QSettings settings("SPbSU", "QReal");
	ui->embeddedLinkerIndentSlider->setValue(settings.value("EmbeddedLinkerIndent", 8).toInt());
	ui->embeddedLinkerSizeSlider->setValue(settings.value("EmbeddedLinkerSize", 6).toInt());

	ui->chooseDiagramsToSaveCheckBox->setChecked(
		settings.value("ChooseDiagramsToSave", true).toBool());
	ui->diagramCreateCheckBox->setChecked(settings.value("DiagramCreateSuggestion", true).toBool());
	ui->paletteTabCheckBox->setChecked(settings.value("PaletteTabSwitching", true).toBool());
	ui->chaoticEditionCheckBox->setChecked(settings.value("ChaoticEdition", false).toBool());
	ui->saveExitCheckBox->setChecked(settings.value("SaveExitSuggestion", true).toBool());
	ui->activateGridCheckBox->setChecked(settings.value("ActivateGrid", true).toBool());
	ui->antialiasingCheckBox->setChecked(settings.value("Antialiasing", true).toBool());
	ui->splashScreenCheckBox->setChecked(settings.value("Splashscreen", true).toBool());
	ui->showGridCheckBox->setChecked(settings.value("ShowGrid", true).toBool());
	ui->openGLCheckBox->setChecked(settings.value("OpenGL", true).toBool());

	ui->pathToQmake->setText(settings.value("pathToQmake", "qmake").toString());
	ui->pathToMake->setText(settings.value("pathToMake", "mingw32-make").toString());
	ui->pluginExtension->setText(settings.value("pluginExtension", "dll").toString());
}

void PreferencesDialog::applyChanges()
{
	QSettings settings("SPbSU", "QReal");
	settings.setValue("EmbeddedLinkerIndent", ui->embeddedLinkerIndentSlider->value());
	settings.setValue("EmbeddedLinkerSize", ui->embeddedLinkerSizeSlider->value());

	settings.setValue("ChooseDiagramsToSave", ui->chooseDiagramsToSaveCheckBox->isChecked());
	settings.setValue("DiagramCreateSuggestion", ui->diagramCreateCheckBox->isChecked());
	settings.setValue("PaletteTabSwitching", ui->paletteTabCheckBox->isChecked());
	settings.setValue("ChaoticEdition", ui->chaoticEditionCheckBox->isChecked());
	settings.setValue("SaveExitSuggestion", ui->saveExitCheckBox->isChecked());
	settings.setValue("Splashscreen", ui->splashScreenCheckBox->isChecked());
	settings.setValue("ActivateGrid", ui->activateGridCheckBox->isChecked());
	settings.setValue("Antialiasing", ui->antialiasingCheckBox->isChecked());
	settings.setValue("ShowGrid", ui->showGridCheckBox->isChecked());
	settings.setValue("OpenGL", ui->openGLCheckBox->isChecked());

	settings.setValue("pathToQmake", ui->pathToQmake->text());
	settings.setValue("pathToMake", ui->pathToMake->text());
	settings.setValue("pluginExtension", ui->pluginExtension->text());
}

void PreferencesDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void PreferencesDialog::on_okButton_clicked()
{
	applyChanges();
	close();
}

void PreferencesDialog::on_applyButton_clicked()
{
	applyChanges();
}

void PreferencesDialog::on_cancelButton_clicked()
{
	close();
}
