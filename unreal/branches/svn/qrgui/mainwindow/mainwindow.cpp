#include "mainwindow.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSvg/QSvgGenerator>
#include <QtCore/QPluginLoader>
#include <QtGui/QProgressBar>
#include <QtCore/QSettings>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtCore/QDebug>

#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

#include "../dialogs/plugindialog.h"
#include "editorInterface.h"
#include "../model/model.h"
#include "../view/editorview.h"
#include "../umllib/uml_element.h"
#include "../generators/xmi/xmiHandler.h"
#include "../generators/metaGenerator/metaGenerator.h"
#include "../generators/java/javaHandler.h"
#include "../generators/hascol/hascolGenerator.h"
#include "../dialogs/editorGeneratorDialog.h"
#include "../dialogs/checkoutdialog.h"
#include "../parsers/hascol/hascolParser.h"
#include "../parsers/xml/xmlParser.h"
#include "errorReporter.h"
#include "../editorManager/listenerManager.h"
#include "shapeEdit/shapeEdit.h"
#include "gesturesShow/gestureswidget.h"
#include "preferencesDialog.h"
#include "openShapeEditorButton.h"
#include "../qrrepo/svnClient.h"


using namespace qReal;

MainWindow::MainWindow()
	: mListenerManager(NULL), mPropertyModel(mEditorManager)
{
	QSettings settings("SPbSU", "QReal");
	bool showSplash = settings.value("Splashscreen", true).toBool();
	QSplashScreen* splash =
			new QSplashScreen(QPixmap(":/icons/kroki3.PNG"), Qt::SplashScreen | Qt::WindowStaysOnTopHint);

	QProgressBar *progress = new QProgressBar((QWidget*) splash);
	progress->move(20,270);
	progress->setFixedWidth(600);
	progress->setFixedHeight(15);
	progress->setRange(0, 100);
	progress->setValue(5);
	if (showSplash)
	{
		splash->show();
		QApplication::processEvents();
	}

	ui.setupUi(this);
	ui.tabs->setTabsClosable(true);
	ui.tabs->setMovable(true);

	if (!showSplash)
		ui.actionShowSplash->setChecked(false);

	ui.minimapView->setRenderHint(QPainter::Antialiasing, true);

	progress->setValue(20);

	connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));

	connect(ui.actionShowSplash, SIGNAL(toggled(bool)), this, SLOT (toggleShowSplash(bool)));

	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.actionSave_as, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
	connect(ui.actionMakeSvg, SIGNAL(triggered()), this, SLOT(makeSvg()));

	connect(ui.actionDeleteFromDiagram, SIGNAL(triggered()), this, SLOT(deleteFromDiagram()));

	connect(ui.tabs, SIGNAL(currentChanged(int)), this, SLOT(changeMiniMapSource(int)));
	connect(ui.tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

	connect(ui.actionCheckout, SIGNAL(triggered()), this, SLOT(doCheckout()));
	connect(ui.actionCommit, SIGNAL(triggered()), this, SLOT(doCommit()));
	connect(ui.actionExport_to_XMI, SIGNAL(triggered()), this, SLOT(exportToXmi()));
	connect(ui.actionGenerate_to_Java, SIGNAL(triggered()), this, SLOT(generateToJava()));
	connect(ui.actionGenerate_editor, SIGNAL(triggered()), this, SLOT(generateEditor()));
	connect(ui.actionGenerate_to_Hascol, SIGNAL(triggered()), this, SLOT(generateToHascol()));
	connect(ui.actionShape_Edit, SIGNAL(triggered()), this, SLOT(openNewEmptyTab()));
	connect(ui.actionGenerate_Editor, SIGNAL(triggered()), this, SLOT(newGenerateEditor()));
	connect(ui.actionParse_Editor_xml, SIGNAL(triggered()), this, SLOT(parseEditorXml()));
	connect(ui.actionPreferences, SIGNAL(triggered()), this, SLOT(showPreferencesDialog()));

	connect(ui.actionParse_Hascol_sources, SIGNAL(triggered()), this, SLOT(parseHascol()));
	connect(ui.actionParse_Java_Libraries, SIGNAL(triggered()), this, SLOT(parseJavaLibraries()));

	connect(ui.actionPlugins, SIGNAL(triggered()), this, SLOT(settingsPlugins()));
	connect(ui.actionShow_grid, SIGNAL(triggered()), this, SLOT(showGrid()));
	connect(ui.actionSwitch_on_grid, SIGNAL(toggled(bool)), this, SLOT(switchGrid(bool)));

	connect(ui.actionHelp, SIGNAL(triggered()), this, SLOT(showHelp()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
	connect(ui.actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	connect(ui.actionShow, SIGNAL(triggered()), this, SLOT(showGestures()));

	connect(ui.minimapZoomSlider, SIGNAL(valueChanged(int)), this, SLOT(adjustMinimapZoom(int)));

	adjustMinimapZoom(ui.minimapZoomSlider->value());

	progress->setValue(40);

	// XXX: kludge... don't know how to do it in designer
	ui.diagramDock->setWidget(ui.diagramExplorer);

	ui.paletteDock->setWidget(ui.paletteToolbox);

	ui.propertyEditor->setModel(&mPropertyModel);
	ui.propertyEditor->verticalHeader()->hide();
	ui.propertyEditor->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	ui.propertyEditor->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	ui.propertyEditor->setItemDelegate(&mDelegate);
	mDelegate.setMainWindow(this);

	connect(ui.diagramExplorer,SIGNAL(clicked(QModelIndex const &)),this,SLOT(diagramExplorerClicked(QModelIndex)));
	ui.diagramExplorer->addAction(ui.actionDeleteFromDiagram);

	progress->setValue(60);
	loadPlugins();
	showMaximized();
	progress->setValue(70);

	settings.beginGroup("MainWindow");
	if (!settings.value("maximized", true).toBool()) {
		showNormal();
		resize(settings.value("size", QSize(1024, 800)).toSize());
		move(settings.value("pos", QPoint(0, 0)).toPoint());
	}
	settings.endGroup();

	QString workingDir = settings.value("workingDir", "./save").toString();

	mModel = new model::Model(mEditorManager, workingDir);
	IdList missingPlugins = mEditorManager.checkNeededPlugins(mModel->api());
	if (!missingPlugins.isEmpty()) {
		QString text = "These plugins are not present, but needed to load the save:\n";
		foreach (Id const id, missingPlugins) {
			text += id.editor() + "\n";
		}
		QMessageBox::warning(this, tr("Some plugins are missing"), text);
		close();
		return;
	}

	mListenerManager = new ListenerManager(mEditorManager.listeners(), &mModel->assistApi());
	this->mGesturesWidget = new GesturesWidget();

	connect(ui.actionClear, SIGNAL(triggered()), this, SLOT(exterminate()));

	progress->setValue(80);

	mPropertyModel.setSourceModel(mModel);
	ui.diagramExplorer->setModel(mModel);
	ui.diagramExplorer->setRootIndex(mModel->rootIndex());

	connect(mModel, SIGNAL(nameChanged(QModelIndex const &)), this, SLOT(updateTab(QModelIndex const &)));

	if (mModel->rowCount() > 0)
		openNewTab(mModel->index(0, 0, QModelIndex()));

	progress->setValue(100);
	if (showSplash)
		splash->close();
	delete splash;

	//choosing diagrams to save isn't implemented yet
	settings.setValue("ChooseDiagramsToSave", false);
	//so it is turned off

	if (settings.value("diagramCreateSuggestion", true).toBool())
		suggestToCreateDiagram();
	mModel->resetChangedDiagrams();
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
	if (keyEvent->modifiers() == Qt::AltModifier && keyEvent->key() == Qt::Key_X){
		close();
	} else if ( keyEvent->key() == Qt::Key_F2 ){
		save();
	}
}

MainWindow::~MainWindow()
{
	delete mModel;
	delete mListenerManager;
}

EditorManager* MainWindow::manager() {
	return &mEditorManager;
}

void MainWindow::finalClose()
{
	clEvent->accept();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	clEvent = event;
	QSettings settings("SPbSU", "QReal");
	if ((mModel->isChanged()) && (settings.value("SaveExitSuggestion", true).toBool())) {
		event->ignore();
		suggestToSave();
	} else {
		settings.beginGroup("MainWindow");
		settings.setValue("maximized", isMaximized());
		settings.setValue("size", size());
		settings.setValue("pos", pos());
		settings.endGroup();
	}
}

void MainWindow::loadPlugins()
{
	foreach (Id const editor, mEditorManager.editors()) {
		foreach (Id const diagram, mEditorManager.diagrams(editor)) {
			ui.paletteToolbox->addDiagramType(diagram, mEditorManager.friendlyName(diagram) );

			foreach (Id const element, mEditorManager.elements(diagram)) {
				ui.paletteToolbox->addItemType(element, mEditorManager.friendlyName(element), mEditorManager.icon(element));
			}
		}
	}
	ui.paletteToolbox->initDone();
}

void MainWindow::adjustMinimapZoom(int zoom)
{
	ui.minimapView->resetMatrix();
	ui.minimapView->scale(0.01*zoom,0.01*zoom);
}

void MainWindow::activateItemOrDiagram(const QModelIndex &idx, bool bl, bool isSetSel)
{
	QModelIndex parent = idx.parent();

	if (ui.tabs->isEnabled())
	{
		if (parent == mModel->rootIndex()) {
			getCurrentTab()->mvIface()->setRootIndex(idx);
		} else {
			getCurrentTab()->mvIface()->setRootIndex(parent);
			// select this item on diagram
			getCurrentTab()->scene()->clearSelection();
			UML::Element *e = (static_cast<EditorViewScene *>(getCurrentTab()->scene()))->getElemByModelIndex(idx);
			if (e)
			{
				e->setColorRect(bl);
				if (isSetSel)
					e->setSelected(true);
			}
			else
				qDebug() << "shit happened!!!\n";
		}
	}
}

void MainWindow::activateItemOrDiagram(Id const &id, bool bl, bool isSetSel) {
	activateItemOrDiagram(mModel->indexById(id), bl, isSetSel);
}

void MainWindow::activateSubdiagram(QModelIndex const &idx) {
	// end-to-end links: if there's a first-level diagram with the same name as
	// this element, show it
	QString targetName = mModel->data(idx, Qt::DisplayRole).toString();
	int rows = mModel->rowCount(mModel->rootIndex());
	for (int i = 0; i < rows; ++i) {
		QModelIndex child = mModel->index(i, 0, mModel->rootIndex());
		if (mModel->data(child, Qt::DisplayRole).toString() == targetName)
		{
			activateItemOrDiagram(child);
			return;
		}
	}

	QModelIndex diagramToActivate = idx;
	while (diagramToActivate.isValid() && diagramToActivate.parent().isValid()
		&& diagramToActivate.parent() != getCurrentTab()->mvIface()->rootIndex())
		{
		diagramToActivate = diagramToActivate.parent();
	}

	if (diagramToActivate.model()->rowCount(diagramToActivate) > 0) {
		QModelIndex childIndex = diagramToActivate.model()->index(0, 0, diagramToActivate);
		activateItemOrDiagram(childIndex);
	}
}

void MainWindow::sceneSelectionChanged()
{
	if (!getCurrentTab())
		return;
	QList<QGraphicsItem*> graphicsItems = getCurrentTab()->scene()->selectedItems();
	int length = graphicsItems.size();
	if (length == 1) {
		QGraphicsItem *item = graphicsItems[0];
		if (UML::Element *elem = dynamic_cast<UML::Element *>(item)) {
			if (elem->index().isValid()) {
				ui.diagramExplorer->setCurrentIndex(elem->index());
				mPropertyModel.setIndex(elem->index());
			}
		} else {
			ui.diagramExplorer->setCurrentIndex(QModelIndex());
			mPropertyModel.setIndex(QModelIndex());

			foreach(QGraphicsItem* item, graphicsItems) {
				UML::EdgeElement* edge = dynamic_cast<UML::EdgeElement*>(item);
				if (edge) {
					length--;
					graphicsItems.removeOne(edge);
				}
			}
			if (length > 1) {
				foreach(QGraphicsItem* item, graphicsItems) {
					UML::NodeElement* node = dynamic_cast<UML::NodeElement*>(item);
					if (node)
						node->hideEmbeddedLinkers();
				}
			}
		}
	}
}

QString MainWindow::getWorkingDir(QString const &dialogWindowTitle)
{
	QString const dirName = QFileDialog::getExistingDirectory(this, dialogWindowTitle,".", QFileDialog::ShowDirsOnly);

	if (dirName.isEmpty())
		return "";

	QSettings settings("SPbSU", "QReal");
	settings.setValue("workingDir", dirName);

	return dirName;
}

void MainWindow::open()
{
	QString const dirName = getWorkingDir(tr("Select directory with a save to open"));

	if (dirName.isEmpty())
		return;

	mModel->open(dirName);
}

void MainWindow::setShape(const QString &data, const QPersistentModelIndex &index, const int &role)
{
	mModel->setData(index, data, role);
}

void MainWindow::print()
{
	QPrinter printer(QPrinter::HighResolution);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() == QDialog::Accepted) {
		QPainter painter(&printer);
		//		QRect allScene = pieChart->mapFromScene(pieChart->scene()->sceneRect()).boundingRect();
		getCurrentTab()->scene()->render(&painter);
	}
}

void MainWindow::makeSvg()
{
	QSvgGenerator newSvg;

	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty())
		return;

	newSvg.setFileName(fileName);
	newSvg.setSize(QSize(800,600));

	QPainter painter(&newSvg);
	getCurrentTab()->scene()->render(&painter);
}

void MainWindow::settingsPlugins()
{
	PluginDialog dialog(mEditorManager , this);
	dialog.exec();
}

void MainWindow::deleteFromExplorer()
{
	QModelIndex idx = ui.diagramExplorer->currentIndex();
	closeTab(idx);
	if (idx.isValid())
		mModel->removeRow(idx.row(), idx.parent());
}

void MainWindow::deleteFromScene()
{
	foreach (QGraphicsItem *item, getCurrentTab()->scene()->selectedItems())
		deleteFromScene(item);
}

void MainWindow::deleteFromScene(QGraphicsItem *target)
{

	if (UML::Element *elem = dynamic_cast<UML::Element *>(target))
	{
		qDebug() << "Deleting object, uuid: " << elem->uuid().toString();
		if (elem->index().isValid())
			mModel->removeRow(elem->index().row(), elem->index().parent());
	}
}

void MainWindow::deleteFromDiagram()
{
	if (mModel) {
		if (ui.diagramExplorer->hasFocus()) {
			deleteFromExplorer();
		} else if (getCurrentTab()->hasFocus()) {
			deleteFromScene();
		}
	}
}

void MainWindow::showAbout()
{
	QMessageBox::about(this, tr("About QReal"),
					   tr("<center>This is <b>QReal</b><br>"
						  "Just another CASE tool</center>"));
}

void MainWindow::showHelp()
{
	QMessageBox::about(this, tr("Help"),
					   tr("To begin:\n"
						  "1. To add items to diagrams, drag & drop them from Palette to editor\n"
						  "2. Get more help from author :)"));
}

void MainWindow::toggleShowSplash(bool show)
{
	QSettings settings("SPbSU", "QReal");
	settings.setValue("Splashscreen", show);
}

void MainWindow::checkoutDialogOk()
{
	//qDebug() << sender();
	CheckoutDialog *dialog = (CheckoutDialog*)sender();
	QString dir = dialog->getDir();
	QString url = dialog->getUrl();
	if (dir.isEmpty() || url.isEmpty())
	{
		QMessageBox::warning(this, "Error", "Url and directory should be valid non-empty paths");
		return;
	}
	SvnClient *client = new SvnClient(dir.toAscii().data());
	QString res = client->checkout(url.toAscii().data(), svn::Path(dir.toAscii().data()));
	QMessageBox::information(this, "Result", res);
}
void MainWindow::checkoutDialogCancel()
{
}

void MainWindow::doCheckout()
{
	QString path;
	QString url;
	CheckoutDialog *dialog = new CheckoutDialog(this);
	connect(dialog, SIGNAL(accepted()), this, SLOT(checkoutDialogOk()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(checkoutDialogCancel()));
	dialog->show();
}

void MainWindow::doCommit()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("Select directory to commit"));
	if (path.isEmpty())
		return;

	SvnClient client(path.toAscii().data());
	QString *message = new QString;
	int revision = client.commit(*message);
	if (revision > 0)
		QMessageBox::information(this, tr("Success"), tr("Committed successfully to revision ")+
								QString::number(revision));
	else if (!message->isNull())
		QMessageBox::warning(this, tr("Error"), *message);
	else
		QMessageBox::information(this, tr("Info"), tr("Nothing to commit"));
}

void MainWindow::exportToXmi()
{
	generators::XmiHandler xmi(mModel->api());

	QString const fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty())
		return;

	QString const errors = xmi.exportToXmi(fileName);

	if (!errors.isEmpty()) {
		QMessageBox::warning(this, tr("errors"), "Some errors occured. Export may be incorrect. Errors list: \n" + errors);
	} else {
		QMessageBox::information(this, tr("finished"), "Export is finished");
	}

	qDebug() << "Done.";
}

void MainWindow::generateToJava()
{
	generators::JavaHandler java(mModel->api());

	QString const dirName = QFileDialog::getExistingDirectory(this);
	if (dirName.isEmpty())
		return;

	QString const errors = java.generateToJava(dirName);

	if (!errors.isEmpty()) {
		QMessageBox::warning(this, tr("errors"), "Some errors occured. Export may be incorrect. Errors list: \n" + errors);
	} else {
		QMessageBox::information(this, tr("finished"), "Export is finished");
	}

	qDebug() << "Done.";
}

void MainWindow::parseJavaLibraries()
{
	generators::JavaHandler java(mModel->api());

	QString const dirName = QFileDialog::getExistingDirectory(this);
	if (dirName.isEmpty())
		return;

	QString const errors = java.parseJavaLibraries(dirName);

	if (!errors.isEmpty()) {
		QMessageBox::warning(this, tr("errors"), "Some errors occured. Export may be incorrect. Errors list: \n" + errors);
	} else {
		QMessageBox::information(this, tr("finished"), "Parsing is finished");
	}

	qDebug() << "Done.";
}

void MainWindow::generateToHascol()
{
	generators::HascolGenerator hascolGenerator(mModel->api());

	gui::ErrorReporter const errors = hascolGenerator.generate();
	errors.showErrors("Generation finished successfully");

	qDebug() << "Done.";
}

void MainWindow::newGenerateEditor()
{
	generators::MetaGenerator metaGenerator(mModel->api());

	QString directoryName;
	QFileInfo directoryXml;
	gui::ErrorReporter errors;
	const QHash<Id, QString> metamodelList = metaGenerator.getMetamodelList();
	QDir dir(".");
	bool found = false;
	while (dir.cdUp() && !found) {
		QFileInfoList infoList = dir.entryInfoList(QDir::Dirs);
		foreach (QFileInfo const directory, infoList){
			if (directory.baseName() == "qrxml") {
				found = true;
				directoryXml = directory;
				directoryName = directory.absolutePath();
			}
		}
	}
	if (!found) {
		QMessageBox::warning(this, tr("error"), "Cannot find the directory for saving");
		return;
	}
	foreach (Id const key, metamodelList.keys()) {
		dir.mkdir(directoryXml.absolutePath() + "/qrxml/" + metamodelList[key]);
		errors = metaGenerator.generateEditor(key, directoryName + "/qrxml/" + metamodelList[key] + "/" + metamodelList[key]);

		if (errors.showErrors("Generation finished successfully")) {
			QSettings settings("SPbSU", "QReal");
			loadingNewEditor(directoryName, metamodelList[key], settings.value("pathToQmake", "qmake").toString(),
					settings.value("pathToMake", "mingw32-make").toString(), settings.value("pluginExtension", "dll").toString());
		}
	}
}

void MainWindow::loadingNewEditor(const QString &directoryName, const QString &metamodelName,
		QString const &commandFirst, QString const &commandSecond, QString const &file)
{
	if ((commandFirst == "") || (commandSecond == "") || (file == "")) {
		QMessageBox::warning(this, tr("error"), "please, fill compiler settings");
		return;
	}

	QString normalizeDirName = metamodelName.at(0).toUpper() + metamodelName.mid(1);

	QProgressBar *progress = new QProgressBar(this);
	progress->show();

	QApplication::processEvents();

	progress->move(530, 335);
	progress->setFixedWidth(240);
	progress->setFixedHeight(20);
	progress->setRange(0, 100);
	progress->setValue(5);

	if (mEditorManager.editors().contains(Id(normalizeDirName))) {
		foreach (Id const diagram, mEditorManager.diagrams(Id(normalizeDirName)))
			ui.paletteToolbox->deleteDiagramType(diagram);

		if (!mEditorManager.unloadPlugin(metamodelName + "." + file)) {
			QMessageBox::warning(this, "error", "cannot unload plugin");
			progress->close();
			delete progress;
			return;
		}
	}

	progress->setValue(20);

	QProcess builder;
	builder.setWorkingDirectory(directoryName + "/qrxml/" + metamodelName);
	builder.start(commandFirst);
	if ((builder.waitForFinished()) && (builder.exitCode() == 0)) {
		progress->setValue(60);
		builder.start(commandSecond);
		if (builder.waitForFinished() && (builder.exitCode() == 0)) {
			progress->setValue(80);

			if (mEditorManager.loadPlugin(metamodelName + "." + file)) {

				foreach (Id const diagram, mEditorManager.diagrams(Id(normalizeDirName))) {
					ui.paletteToolbox->addDiagramType(diagram, mEditorManager.friendlyName(diagram));

					foreach (Id const element, mEditorManager.elements(diagram))
						ui.paletteToolbox->addItemType(element, mEditorManager.friendlyName(element), mEditorManager.icon(element));
				}
				ui.paletteToolbox->initDone();
				progress->setValue(100);
			}
		}
	}
	if (progress->value() != 100)
		QMessageBox::warning(this, tr("error"), "cannot load new editor");
	progress->close();
	delete progress;

}

void MainWindow::parseEditorXml()
{
	if (!mEditorManager.editors().contains(Id("Meta_editor"))) {
		QMessageBox::warning(this, tr("error"), "required plugin is not loaded");
		return;
	}
	QDir dir(".");
	QString directoryName = ".";
	while (dir.cdUp()) {
		QFileInfoList infoList = dir.entryInfoList(QDir::Dirs);
		foreach (QFileInfo const directory, infoList){
			if (directory.baseName() == "qrxml") {
				directoryName = directory.absolutePath() + "/qrxml";
			}
		}
	}
	QString const fileName = QFileDialog::getOpenFileName(this, tr("Select xml file to parse"), directoryName, "XML files (*.xml)");
	if (fileName == "")
		return;


	parsers::XmlParser parser(mModel->mutableApi(), mEditorManager);

	parser.parseFile(fileName);
	if (QMessageBox::question(this, tr("loading.."),"Do you want to load connected metamodels?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		parser.loadIncludeList(fileName);

	mModel->reinit();
}

EditorView * MainWindow::getCurrentTab()
{
	return dynamic_cast<EditorView *>(ui.tabs->currentWidget());
}

void MainWindow::changeMiniMapSource( int index )
{
	if (index != -1) {
		ui.tabs->setEnabled(true);
		EditorView *editorView = getCurrentTab();
		if (editorView != NULL)
			ui.minimapView->setScene(editorView->scene());
	} else
	{
		ui.tabs->setEnabled(false);
		ui.minimapView->setScene(0);;
	}
}

void qReal::MainWindow::closeTab( int index )
{
	QWidget *widget = ui.tabs->widget(index);
	ui.tabs->removeTab(index);
	delete widget;
}

void MainWindow::exterminate()
{
	int tabCount = ui.tabs->count();
	for (int i = 0; i < tabCount; i++)
		closeTab(i);
	mModel->exterminate();
}

void MainWindow::generateEditor()
{
	EditorGeneratorDialog editorGeneratorDialog(mModel->api());
	editorGeneratorDialog.exec();
}

void MainWindow::parseHascol()
{
	QStringList const fileNames = QFileDialog::getOpenFileNames(this, tr("Select Hascol files to parse"), ".", "*.md;;*.*");
	if (fileNames.empty())
		return;

	parsers::HascolParser parser(mModel->mutableApi(), mEditorManager);
	gui::ErrorReporter errors = parser.parse(fileNames);

	errors.showErrors("Parsing is finished");

	mModel->reinit();
}

void MainWindow::showPreferencesDialog()
{
	PreferencesDialog preferencesDialog;
	preferencesDialog.exec();
}

void MainWindow::openNewEmptyTab()
{
	QObject *object = sender();
	OpenShapeEditorButton *button = dynamic_cast<OpenShapeEditorButton*>(object);
	QString text = "Shape Edit";
	ShapeEdit *shapeEdit = NULL;
	if (button != NULL) {
		QPersistentModelIndex index = button->getIndex();
		int role = button->getRole();
		QString propertyValue = button->getPropertyValue();
		shapeEdit = new ShapeEdit(index, role);
		if (propertyValue != "")
			shapeEdit->load(propertyValue);
		mModel->setData(index, propertyValue, role);
		connect(shapeEdit, SIGNAL(shapeSaved(QString, QPersistentModelIndex const &, int const &)), this, SLOT(setShape(QString, QPersistentModelIndex const &, int const &)));
	}
	else {
		shapeEdit = new ShapeEdit();
	}
	ui.tabs->addTab(shapeEdit, text);
	ui.tabs->setCurrentWidget(shapeEdit);
}

void MainWindow::centerOn(const QModelIndex &index)
{
	Id itemId = mModel->idByIndex(index);
	if (itemId.element() == mModel->assistApi().editorManager().getEditorInterface(itemId.editor())->diagramNodeName(itemId.diagram()))
		return;
	EditorView* view = getCurrentTab();
	EditorViewScene* scene = dynamic_cast<EditorViewScene*>(view->scene());
	UML::Element* element = scene->getElem(itemId);

	scene->clearSelection();
	element->setSelected(true);

	float widthTab = ui.tabs->size().width();
	float heightTab = ui.tabs->size().height();
	float widthEl = element->boundingRect().width();
	float heightEl = element->boundingRect().height();
	view->ensureVisible(element, (widthTab - widthEl)/2, (heightTab - heightEl)/2);
}

void MainWindow::diagramExplorerClicked(const QModelIndex &index)
{
	mPropertyModel.setIndex(index);
	openNewTab(index);
	centerOn(index);
}

void MainWindow::openNewTab(const QModelIndex &arg)
{
//	if( index.parent() != QModelIndex() ) // only first-level diagrams are opened in new tabs
//		return;

	QModelIndex index = arg;
	while (index.parent() != QModelIndex())
		index = index.parent();

	mModel->setRootIndex(index);
	int tabNumber = -1;
	for (int i = 0; i < ui.tabs->count(); i++) {
		EditorView *tab = (static_cast<EditorView *>(ui.tabs->widget(i)));
		if (tab->mvIface()->rootIndex() == index) {
			tabNumber = i;
			break;
		}
	}
	if (tabNumber != -1) {
		ui.tabs->setCurrentIndex(tabNumber);
	} else {
		EditorView *view = new EditorView();
		ui.tabs->addTab(view, mModel->data(index, Qt::EditRole).toString());
		ui.tabs->setCurrentWidget(view);
		//		if (!index.isValid())
		//			index = mModel->rootIndex();
		initCurrentTab(index);
	}

	//changing of palette active editor:
	QSettings settings("SPbSU", "QReal");
	if (settings.value("PaletteTabSwitching", true).toBool())
	{
		int i = 0;
		bool diagram = false;
		foreach(QString name, ui.paletteToolbox->getTabNames()) {
			//this conditions are not good because of strings comparing
			QString tabName = name.trimmed().remove(" ");
			QString diagramName = mModel->idByIndex(index).diagram().remove("_");
			if (diagramName.contains(tabName)) {
				ui.paletteToolbox->getComboBox()->setCurrentIndex(i);
				diagram = true;
			}
			if (diagram)
				continue;
			QString editorName = mModel->idByIndex(index).diagram().remove("_");
			if (editorName.contains(tabName))
				ui.paletteToolbox->getComboBox()->setCurrentIndex(i);
			i++;
		}
	}
}

void MainWindow::initCurrentTab(const QModelIndex &rootIndex)
{
	getCurrentTab()->setMainWindow(this);
	QModelIndex index = rootIndex;

	changeMiniMapSource(ui.tabs->currentIndex());

	connect(getCurrentTab()->scene(), SIGNAL(selectionChanged()), SLOT(sceneSelectionChanged()));
	connect(ui.actionZoom_In, SIGNAL(triggered()), getCurrentTab(), SLOT(zoomIn()));
	connect(ui.actionZoom_Out, SIGNAL(triggered()), getCurrentTab(), SLOT(zoomOut()));
	connect(ui.actionAntialiasing, SIGNAL(toggled(bool)), getCurrentTab(), SLOT(toggleAntialiasing(bool)));
	connect(ui.actionOpenGL_Renderer, SIGNAL(toggled(bool)), getCurrentTab(), SLOT(toggleOpenGL(bool)));

	getCurrentTab()->mvIface()->setModel(mModel);
	getCurrentTab()->mvIface()->setRootIndex(index);
	connect(mModel, SIGNAL(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int))
			, getCurrentTab()->mvIface(), SLOT(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int)));
	connect(mModel, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int))
			, getCurrentTab()->mvIface(), SLOT(rowsMoved(QModelIndex, int, int, QModelIndex, int)));

	emit tabOpened();
}

void MainWindow::updateTab(QModelIndex const &index)
{
	for (int i = 0; i < ui.tabs->count(); i++) {
		EditorView *tab = (static_cast<EditorView *>(ui.tabs->widget(i)));
		if (tab->mvIface()->rootIndex() == index) {
			ui.tabs->setTabText(i, mModel->data(index, Qt::EditRole).toString());
			return;
		}
	}
}

void MainWindow::closeTab(QModelIndex const &index)
{
	for (int i = 0; i < ui.tabs->count(); i++) {
		EditorView *tab = (static_cast<EditorView *>(ui.tabs->widget(i)));
		if (tab->mvIface()->rootIndex() == index) {
			closeTab(i);
			return;
		}
	}
}

ListenerManager *MainWindow::listenerManager()
{
	return mListenerManager;
}

void MainWindow::showGrid()
{
	EditorView *tmpView = getCurrentTab();
	if (tmpView != NULL)
		tmpView->changeSceneGrid();
}

void MainWindow::switchGrid(bool isChecked)
{
	EditorView *tmpView = getCurrentTab();
	if (tmpView != NULL) {
		QList<QGraphicsItem *> list = tmpView->scene()->items();
		foreach (QGraphicsItem *item, list) {
			NodeElement* nodeItem = dynamic_cast<NodeElement*>(item);
			if (nodeItem != NULL) {
				if (isChecked)
					nodeItem->switchOnGrid();
				else
					nodeItem->switchOffGrid();
			}
		}
	}
}

void MainWindow::showGestures()
{
	QString text = "Gestures Show";
	mGesturesWidget = new GesturesWidget();
	ui.tabs->addTab(mGesturesWidget, text);
	ui.tabs->setCurrentWidget(mGesturesWidget);
	connect(this->mGesturesWidget, SIGNAL(currentElementChanged()), this, SIGNAL(currentIdealGestureChanged()));
	emit gesturesShowed();
}

IGesturesPainter * MainWindow::gesturesPainter()
{
	return this->mGesturesWidget;
}



void MainWindow::suggestToCreateDiagram()
{
	if (getCurrentTab())
		return;
	QDialog dialog;
	QVBoxLayout vLayout;
	QHBoxLayout hLayout;
	dialog.setLayout(&vLayout);
	dialog.setMinimumSize(320,240);
	dialog.setMaximumSize(320,240);
	dialog.setWindowTitle("Choose new diagram");

	QLabel label;
	label.setText(QString("There is no existing diagram,\n choose diagram you want work with:"));
	QListWidget diagramsListWidget;
	diagramsListWidget.setParent(&dialog);

	int i = 0;
	foreach(Id editor, manager()->editors()) {
		foreach(Id diagram, manager()->diagrams(Id::loadFromString("qrm:/"+editor.editor()))) {
			const QString diagramName = mModel->assistApi().editorManager().getEditorInterface(editor.editor())->diagramName(diagram.diagram());
			const QString diagramNodeName = mModel->assistApi().editorManager().getEditorInterface(editor.editor())->diagramNodeName(diagram.diagram());
			if (diagramNodeName == " ")
				continue;
			diagramsList.append("qrm:/"+editor.editor()+"/"+diagram.diagram()+"/"+diagramNodeName);
			diagramsListWidget.addItem(diagramName);
			i++;
		}
	}

	QPushButton cancelButton;
	cancelButton.setText("Cancel");
	QPushButton okButton;
	okButton.setText("Done");

	QObject::connect(&diagramsListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(diagramInCreateListSelected(int)));
	QObject::connect(&diagramsListWidget,SIGNAL(itemDoubleClicked(QListWidgetItem*)),&dialog,SLOT(close()));
	QObject::connect(&cancelButton,SIGNAL(clicked()),this,SLOT(diagramInCreateListDeselect()));
	QObject::connect(&cancelButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	QObject::connect(&okButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	diagramsListWidget.setCurrentRow(0);

	vLayout.addWidget(&label);
	vLayout.addWidget(&diagramsListWidget);

	hLayout.addWidget(&okButton);
	hLayout.addWidget(&cancelButton);

	vLayout.addLayout(&hLayout);

	dialog.exec();
}

void MainWindow::diagramInCreateListDeselect()
{
	deleteFromExplorer();
}

void MainWindow::diagramInCreateListSelected(int num)
{
	deleteFromExplorer();
	createDiagram(diagramsList.at(num));
}

void MainWindow::createDiagram(const QString &idString)
{
	Id created = mModel->assistApi().createElement(ROOT_ID,Id::loadFromString(idString));
	QModelIndex index = mModel->indexById(created);
	ui.diagramExplorer->setCurrentIndex(index);
	openNewTab(index);
}

void MainWindow::save()
{
	QSettings settings("SPbSU", "QReal");
	if (!settings.value("ChooseDiagramsToSave", true).toBool()) {
		saveAll();
		return;
	}

	QDialog dialog;
	QVBoxLayout vLayout;
	QHBoxLayout hLayout;
	QPushButton saveButton;
	QPushButton cancelButton;
	saveButton.setText("Save");
	cancelButton.setText("Cancel");
	QWidget* saveListWidget = createSaveListWidget();
	vLayout.addWidget(saveListWidget);

	QObject::connect(&saveButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	QObject::connect(&cancelButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	QObject::connect(&saveButton,SIGNAL(clicked()),this,SLOT(saveListClosed()));

	hLayout.addWidget(&saveButton);
	hLayout.addWidget(&cancelButton);

	vLayout.addLayout(&hLayout);
	dialog.setLayout(&vLayout);
	saveListWidget->show();
	dialog.exec();
}

void MainWindow::saveAll()
{
	mModel->api().saveAll();
	mModel->resetChangedDiagrams();
}

void MainWindow::saveIds(IdList const &toSave, IdList const &toRemove)
{
	//not implemented
	//TODO:
	//create structure to save deleted objects
	//(look Client::exist(), remove methods in repoapi, model, client, serializer; addChangedDiagrams method)
	//add choosing of just created diagrams

	mModel->api().save(toSave);
	mModel->api().remove(toRemove);
	mModel->resetChangedDiagrams(toSave);
	mModel->resetChangedDiagrams(toRemove);
}

void MainWindow::saveAs()	//TODO: change
{
	QString const dirName = getWorkingDir(tr("Select directory to save current model to"));
	if (dirName.isEmpty())
		return;

	mModel->saveTo(dirName);
}

QListWidget* MainWindow::createSaveListWidget()
{
	saveListChecked = new bool[mModel->api().getOpenedDiagrams().size()];
	QListWidget *listWidget = new QListWidget();

	int i =0;
	foreach(Id id, mModel->api().getOpenedDiagrams()) {
		listWidget->addItem(id.diagram());
		if (mModel->api().getChangedDiagrams().contains(id.diagramId())) {
			saveListChecked[i] = true;
			listWidget->item(i)->setCheckState(Qt::Checked);
			qDebug() << "checked: " << id.toString() << " at row: " << i;
		} else {
			listWidget->item(i)->setCheckState(Qt::Unchecked);
			saveListChecked[i] = false;
		}
		i++;
	}

	QObject::connect(listWidget,SIGNAL(itemChanged(QListWidgetItem*)),
					 this,SLOT(diagramInSaveListChanged(QListWidgetItem*)));
	return listWidget;
}

void MainWindow::diagramInSaveListChanged(QListWidgetItem* diagram)
{
	QListWidget* listWidget = diagram->listWidget();
	if (diagram->checkState() == Qt::Unchecked)
		saveListChecked[listWidget->row(diagram)] = false;
	else if (diagram->checkState() == Qt::Checked)
		saveListChecked[listWidget->row(diagram)] = true;
}

void MainWindow::saveListClosed()
{
	IdList toSave;
	IdList toRemove;
	IdList current = mModel->api().children(ROOT_ID);
	IdList opened = mModel->api().getOpenedDiagrams();

	int i = 0;
	foreach(Id id, opened) {
		qDebug() << "Was opened: " << id.diagram() << " / " << id.element();

		if (!saveListChecked[i]) {
			if (!current.contains(id))
				mModel->addDiagram(id);
			else
				continue;
		}
		if (current.contains(id))
			toSave.append(id);
		else
			toRemove.append(id);
		i++;
	}

	toSave.append(ROOT_ID);
	saveIds(toSave, toRemove);
}

void MainWindow::suggestToSave()
{
	QDialog dialog;
	QVBoxLayout vLayout;
	QHBoxLayout hLayout;
	QPushButton saveButton;
	QPushButton cancelButton;
	QPushButton discardButton;
	saveButton.setText("Save");
	cancelButton.setText("Cancel");
	discardButton.setText("Discard");
	hLayout.addWidget(&saveButton);
	hLayout.addWidget(&cancelButton);
	hLayout.addWidget(&discardButton);
	QString text = QString("The document has been modified.\n");
	text += QString("Do you want to save your changes?");
	QLabel label(text);
	vLayout.addWidget(&label);

	QWidget* saveListWidget = createSaveListWidget();
	vLayout.addWidget(saveListWidget);
	QSettings settings("SPbSU", "QReal");
	if (!settings.value("ChooseDiagramsToSave", true).toBool()) {
		saveListWidget->hide();
		vLayout.removeWidget(saveListWidget);
		QObject::connect(&saveButton,SIGNAL(clicked()),this,SLOT(saveAll()));
	} else
		QObject::connect(&saveButton,SIGNAL(clicked()),this,SLOT(saveListClosed()));

	vLayout.addLayout(&hLayout);
	dialog.setLayout(&vLayout);
	if (settings.value("ChooseDiagramsToSave", true).toBool())
		saveListWidget->show();

	QObject::connect(&saveButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	QObject::connect(&saveButton,SIGNAL(clicked()),this,SLOT(finalClose()));
	QObject::connect(&discardButton,SIGNAL(clicked()),&dialog,SLOT(close()));
	QObject::connect(&discardButton,SIGNAL(clicked()),this,SLOT(finalClose()));
	QObject::connect(&cancelButton,SIGNAL(clicked()),&dialog,SLOT(close()));

	dialog.setWindowTitle(QString("Saving"));
	dialog.exec();
}
