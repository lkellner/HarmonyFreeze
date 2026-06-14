#include "freezeManager.h"

#include <Util/pluginmanager/PLUG_Scripting.h>
#include <SDKExtension/SDK_Selection.h>

#include <QSettings>

FreezeManager::FreezeManager()
	: m_projection(PROJECTION::ORTHOGRAPHIC)
	, m_isExperimentalMode(false)
	, m_isPassOnOgl(false)
	, m_isMultithreadingMode(false)
	, m_isMoveUnusedPivots(false)
	, m_isDebugMode(true)
{
	SDK_Selection::getSelectedFrameRange(m_frameRange.start, m_frameRange.end);
	m_selFrame = m_frameRange.start;

	readSettings();
}

void FreezeManager::readSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, QLatin1String("HarmonyFreeze"), QLatin1String("HarmonyFreeze"));

	if (!settings.contains(QLatin1String("2dMode")))
		settings.setValue(QLatin1String("2dMode"), true);

	if (settings.value(QLatin1String("2dMode")).toBool())
		m_projection = PROJECTION::ORTHOGRAPHIC;
	else
		m_projection = PROJECTION::NONE;


	if (!settings.contains(QLatin1String("ExperimentalMode")))
		settings.setValue(QLatin1String("ExperimentalMode"), false);

	m_isExperimentalMode = settings.value(QLatin1String("ExperimentalMode")).toBool();



	if (!settings.contains(QLatin1String("PassOnOglControllerTransformation")))
		settings.setValue(QLatin1String("PassOnOglControllerTransformation"), true);

	m_isPassOnOgl = settings.value(QLatin1String("PassOnOglControllerTransformation")).toBool();



	if (!settings.contains(QLatin1String("UseMultithreading")))
		settings.setValue(QLatin1String("UseMultithreading"), false);

	m_isMultithreadingMode = settings.value(QLatin1String("UseMultithreading")).toBool();



	if (!settings.contains(QLatin1String("MoveUnusedPivots")))
		settings.setValue(QLatin1String("MoveUnusedPivots"), false);

	m_isMoveUnusedPivots = settings.value(QLatin1String("MoveUnusedPivots")).toBool();



	if (!settings.contains(QLatin1String("DebugMode")))
		settings.setValue(QLatin1String("DebugMode"), false);

	m_isDebugMode = settings.value(QLatin1String("DebugMode")).toBool();


	if (!settings.contains(QLatin1String("SetInbetweenKeyframesMode")))
		settings.setValue(QLatin1String("SetInbetweenKeyframesMode"), false);

	m_isSetInbetweenKfMode = settings.value(QLatin1String("SetInbetweenKeyframesMode")).toBool();
}


void FreezeManager::updateDrawingPivotStatus(int curId, const QString& layerAttr, bool hasUsedDrawingPivots)
{
	const auto it = std::find_if(m_processedElements.begin(), m_processedElements.end(),
		[curId](const auto& val) { return val.m_elementId == curId; });

	//element not included yet
	if (it == m_processedElements.end())
		return;

	ElementRegistry& registry = *it;

	//element has already been included
	auto regIt = std::find_if(registry.m_layerAttrInfoVec.begin(), registry.m_layerAttrInfoVec.end(),
		[&layerAttr](const auto& val) { return val.m_layerAttr == layerAttr; });

	if (regIt == registry.m_layerAttrInfoVec.end())
		return;

	(*regIt).m_isUseDrawingPivot = (*regIt).m_isUseDrawingPivot || hasUsedDrawingPivots;

	return;
}


bool FreezeManager::getDrawingPivotStatus(int curId, const QString& layerAttr)
{
	const auto it = std::find_if(m_processedElements.begin(), m_processedElements.end(),
		[curId](const auto& val) { return val.m_elementId == curId; });

	//element not included yet
	if (it == m_processedElements.end())
		return false;

	ElementRegistry& registry = *it;

	//element has already been included
	auto regIt = std::find_if(registry.m_layerAttrInfoVec.begin(), registry.m_layerAttrInfoVec.end(),
		[&layerAttr](const auto& val) { return val.m_layerAttr == layerAttr; });

	if (regIt == registry.m_layerAttrInfoVec.end())
		return false;

	return (*regIt).m_isUseDrawingPivot;
}


bool FreezeManager::isProcessedElement(int curId, const QString& layerAttr) const
{
	const auto it = std::find_if(m_processedElements.begin(), m_processedElements.end(),
							 [curId](const auto& val) { return val.m_elementId == curId; });

	//element not included yet
	if (it == m_processedElements.end())
		return false;

	const ElementRegistry& registry = *it;

	//element has already been included
	auto regIt = std::find_if(registry.m_layerAttrInfoVec.begin(), registry.m_layerAttrInfoVec.end(),
							[&layerAttr](const auto& val) { return val.m_layerAttr == layerAttr; });

	return regIt != registry.m_layerAttrInfoVec.end();
}


void FreezeManager::addElement(int curId, const QString& layerAttr, bool hasUsedDrawingPivots)
{
	const auto it = std::find_if(m_processedElements.begin(), m_processedElements.end(),
						   [curId](const auto& val) { return val.m_elementId == curId; });

	if (it == m_processedElements.end())
	{
		m_processedElements.emplace_back(curId, std::vector<LayerAttrInfo>{{layerAttr, hasUsedDrawingPivots}});
		return;
	}

	ElementRegistry& registry = *it;

	const auto regIt = std::find_if(registry.m_layerAttrInfoVec.begin(), registry.m_layerAttrInfoVec.end(),
							[&layerAttr](const auto& val) { return val.m_layerAttr == layerAttr; });

	if (regIt == registry.m_layerAttrInfoVec.end())
		registry.m_layerAttrInfoVec.emplace_back(layerAttr, hasUsedDrawingPivots);
}


void FreezeManager::writeAttr(QTextStream &out, const QString& moduleName, const StaticAttrData& attr)
{
	out << "\t\tnode.getAttr(\"" << moduleName << "\", 1, \"" << attr.name <<"\").setValue(" << attr.value << ");\n";
}

void FreezeManager::writeAttr(QTextStream& out, const QString& moduleName, const TextAttrData& attr)
{
	if (attr.isKeyframe)
		out << "\t\tnode.setTextAttr(\"" << moduleName << "\", \"" << attr.name << "\", " << attr.frameNo << "," << attr.value << "); \n";
}


void FreezeManager::writeAttr(QTextStream& out, const QString& moduleName, const AttrData& attr)
{
	if (attr.isKeyframe)
		out << "\t\tnode.getAttr(\"" << moduleName << "\", 1, \"" << attr.name << "\").setValueAt(" << attr.value << "," << attr.frameNo << ");\n";
}


void FreezeManager::writeAttr(QTextStream& out, const QString& moduleName, const QuaternionAttrData& attr)
{
	if (attr.isKeyframe)
	{
		//TODO: should put this column check somewhere where it only gets called once...
		//Need to manually create a quarternion column if it doesn't exist yet as, unlike
		//Bezier, it isn't being generated automatically
		out << "\t\tif(node.linkedColumn(\"" << moduleName << "\", \"" << attr.name << "\")==\"\")\n";
		out << "\t\t{\n";
		out << "\t\t\tvar newColumnName = column.generateAnonymousName();\n";
		out << "\t\t\tcolumn.add(newColumnName, \"QUATERNIONPATH\");\n";
		out << "\t\t\tnode.linkAttr(\"" << moduleName << "\", \"" << attr.name << "\", newColumnName);\n";
		out << "\t\t}\n";
		out << "\t\tnode.getAttr(\"" << moduleName << "\", 1, \"" << attr.name << "\").setValueAt(Point3d(" << attr.value.x() << "," << attr.value.y() << "," << attr.value.z() << "), " << attr.frameNo << "); \n";
	}
}


void FreezeManager::writeAttr(QTextStream& out,const QString&, const std::vector<CelInfo>& pivots)
{

	for (const auto& pivot : pivots)
	{
		out << "\t\tvar curDrawing = \n";
		out << "\t\t{\n";
		out << "\t\t\t\"drawingId\" : \"" << pivot.drawingId << "\",\n";
		out << "\t\t\t\"elementId\" : " << pivot.elementId << ",\n";

		if (pivot.layerAttr != QLatin1String(""))
			out << "\t\t\"layer\" : \"" << pivot.layerAttr << "\"\n";

		out << "\t\t};\n";
		out << "\t\tvar drawingKey = Drawing.Key(curDrawing);\n";

		out << "\t\tvar config = \n";
		out << "\t\t{\n";
		out << "\t\t\tdrawing  : drawingKey \n";
		out << "\t\t};\n";

		out << "\t\tvar pivot = Drawing.getPivot(config);\n";

		out << "\t\tpivot.x = " << pivot.pivot.x() << "; \n";
		out << "\t\tpivot.y = " << pivot.pivot.y() << "; \n";
		out << "\t\tconfig.pivot = pivot;\n";
		out << "\t\tDrawing.setPivot(config);\n";
	}
}


void FreezeManager::initializeFileJS()
{
	PLUG_ScriptingInterface * scriptUI = PLUG_Services::getScriptingInterface();

	if (!scriptUI)
		return;

	QTextStream out;

	if (m_isDebugMode)
	{
		QString path = scriptUI->projectScriptFileDir();
		QDir dir(path);

		if (!dir.exists())
		{
			if (dir.mkpath(QLatin1String(".")))
				qDebug() << "Directory created";
			else
				qDebug() << "Failed to create directory";
		}

		m_file.setFileName(dir.filePath(QLatin1String("HarmonyFreezeDebug.js")));

		if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
			return;

		out.setDevice(&m_file);
	}
	else
	{
		if (!m_tempFile.open())
			return;

		out.setDevice(&m_tempFile);
	}

	out << "function setAttr()\n{\n";
	out << "\tscene.beginUndoRedoAccum(\"freeze pegs\"); \n";
}


void FreezeManager::finalizeFileJS()
{
	PLUG_ScriptingInterface* scriptUI = PLUG_Services::getScriptingInterface();

	if (!scriptUI)
		return;

	QTextStream out;

	if (m_isDebugMode)
		out.setDevice(&m_file);
	else
		out.setDevice(&m_tempFile);

	out << "\tMessageLog.trace(\"Harmony Freeze Completed\");\n";
	out << "\tscene.endUndoRedoAccum();\n";
	out << "}\n";

	QString filepath;

	if (m_isDebugMode)
	{
		filepath = QDir::toNativeSeparators(m_file.fileName());
		m_file.close();
	}
	else
	{
		filepath = QDir::toNativeSeparators(m_tempFile.fileName());
		m_tempFile.close();
	}

	scriptUI->callFunctionInFile(filepath, QLatin1String("setAttr"), false);
}


void FreezeManager::setMatrices(const Math::Matrix4x4 matrix, SC_SceneMetrics* sceneMetrics)
{
	switch (m_projection)
	{
	case PROJECTION::ORTHOGRAPHIC:

		m_freezeMatrix = convertToProjectionMatrix(matrix);

		break;

	default:
		m_freezeMatrix = matrix;
		break;
	}


	const double OffsetX = sceneMetrics->toOGLX(sceneMetrics->coordinateAtCenterX());
	const double OffsetY = sceneMetrics->toOGLY(sceneMetrics->coordinateAtCenterY());

	m_offsetMatrix = Math::Matrix4x4().translate(OffsetX, OffsetY, 0);
	m_freezeMatrix = m_offsetMatrix * m_freezeMatrix;


	//These conversions seem to be necessary for whenever there is a different aspect ratio or
	//number of units. In this case the vector drawings coordinate system does not seem to have
	//the same origin as the peg coordinate system. It is quite likely that the hereafter calculated
	//cornversion matrices can alreay readily be obtained elsewhere and just haven't been found

	const double defaultXRatio = 4;
	const double defaultYRatio = 3;
	const double currentRatio = sceneMetrics->toOGLX(1) / sceneMetrics->toOGLY(1);
	const double aspectOffset = defaultXRatio - (currentRatio * defaultYRatio);

	const double defaultXUnits = 12;
	const double defaultYUnits = 12;
	const double unitOffset = defaultXUnits - sceneMetrics->designFieldChartX() - (defaultYUnits - sceneMetrics->designFieldChartY());
	const double unitOffsetScaleFactor = defaultXUnits  * sceneMetrics->designFieldChartY() / sceneMetrics->designFieldChartX() / defaultYUnits;

	const double zoomFactor = defaultYUnits / sceneMetrics->designFieldChartY();

	m_zoomMatrix = Math::Matrix4x4().scale(zoomFactor, zoomFactor, 1);
	m_unitOffsetMatrix = Math::Matrix4x4().translate(sceneMetrics->toOGLX(unitOffset), 0, 0);
	m_unitOffsetScaleMatrix = Math::Matrix4x4().scale(1, unitOffsetScaleFactor,1);
	m_ratioMatrix = Math::Matrix4x4().translate(sceneMetrics->toOGLY(aspectOffset * (defaultXRatio)), 0, 0);

	m_sceneSettingsMatrix = m_ratioMatrix * m_zoomMatrix * m_unitOffsetMatrix;
}


FrameRange FreezeManager::getFrameRange() const
{
	return m_frameRange;
}


void FreezeManager::setFrameRange(const FrameRange frameRange)
{
	m_frameRange = frameRange;
}


double FreezeManager::getSelFrame() const
{
	return m_selFrame;
}


void FreezeManager::setSelFrame(const double frameNo)
{
	m_selFrame = frameNo;
}


void FreezeManager::updateFrameRange(const FrameRange& newRange)
{
	if (newRange.start < m_frameRange.start)
		m_frameRange.start = newRange.start;

	if (newRange.end > m_frameRange.end)
		m_frameRange.end = newRange.end;

	return;
}


void FreezeManager::addCommand(std::shared_ptr<CO_OrCommand> command)
{
	commands.push_back(std::move(command));
}


void FreezeManager::executeCommands()
{
	if (commands.empty())
		return;


	for (size_t i = 0; i < commands.size(); i++)
	{
		commands[i]->execute();
	}

	return;
}
