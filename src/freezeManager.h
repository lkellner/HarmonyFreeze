/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEZEMANAGER_H
#define FREEZEMANAGER_H

#include "settings.h"
#include "utils.h"

#include <Util/command/CO_OrCommand.h>
#include <Util/pluginmanager/PLUG_Services.h>

#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QTemporaryFile>

struct LayerAttrInfo
{
	QString m_layerAttr;
	bool m_isUseDrawingPivot;
};


struct ElementRegistry
{
	int m_elementId;
	std::vector<LayerAttrInfo> m_layerAttrInfoVec;
};


enum class PROJECTION
{
	NONE,
	ORTHOGRAPHIC
};


class FreezeManager
{
public:
	FreezeManager();

	void setFreezePegPtr(MO_Module* freezePegPtr);
	MO_Module* getFreezePegPtr() { return m_freezePegPtr; };
	void setMatrices(const Math::Matrix4x4 matrix, SC_SceneMetrics* sceneMetrics);
	Math::Matrix4x4 getFreezeMatrix() { return m_freezeMatrix; };
	Math::Matrix4x4 getOffsetMatrix() { return m_offsetMatrix; };
	Math::Matrix4x4 getUnitOffsetScaleMatrix() { return m_unitOffsetScaleMatrix; };


	FrameRange getFrameRange() const;
	void setFrameRange(const FrameRange frameRange);
	void updateFrameRange(const FrameRange& newRange);

	double getSelFrame() const;
	void setSelFrame(const double frameNo);

	void addCommand(std::shared_ptr<CO_OrCommand> command); //threadsafe
	void executeCommands();


	void initializeFileJS();
	void finalizeFileJS();

	template <typename ...Args>
	void applyAttributes(const QString& moduleName, Args ...args)
	{
		QMutexLocker locker(&m_writeMutex);
		QTextStream out;

		if (isDebugMode())
			out.setDevice(&m_file);
		else
			out.setDevice(&m_tempFile);

		out << "\ttry\n";
		out << "\t{\n";

		((writeAttr(out, moduleName, args)), ...);

		out << "\t}\n";
		out << "\tcatch(e)\n";
		out << "\t{\n";
		out << "\t\t MessageLog.trace(\"Error setting attributes for " << moduleName << "\");\n";
		out << "\t}\n";

		out.flush();
	}

	bool isProcessedElement(int curId, const QString& layerAttr) const;
	void updateDrawingPivotStatus(int curId, const QString& layerAttr, bool hasUsedDrawingPivots);
	bool getDrawingPivotStatus(int curId, const QString& layerAttr);
	void addElement(int curId, const QString& layerAttr, bool hasUsedDrawingPivots);
	bool isExperimentalMode() const { return m_settings[settingIndex("ExperimentalMode")].value; }
	bool isMultithreadingMode() const { return m_settings[settingIndex("UseMultithreading")].value; }
	bool isDebugMode() const { return m_settings[settingIndex("DebugMode")].value; }
	bool isPassOnOgl() const { return m_settings[settingIndex("PassOnOglControllerTransformation")].value; }
	bool isMoveUnusedPivots() const { return m_settings[settingIndex("MoveUnusedPivots")].value; }
	bool isSetInbetweenKfMode() const { return m_settings[settingIndex("SetInbetweenKeyframesMode")].value; }

private:

	void writeAttr(QTextStream& out, const QString& moduleName, const StaticAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const TextAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const AttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const QuaternionAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const std::vector<CelInfo>& pivots);
	void writeAttr(QTextStream& out, const QString& moduleName, const Point2dAttrData& attr);

	QMutex m_writeMutex;

	Math::Matrix4x4 m_freezeMatrix;
	Math::Matrix4x4 m_offsetMatrix;
	Math::Matrix4x4 m_unitOffsetScaleMatrix;

	MO_Module* m_freezePegPtr;

	QFile m_file;
	QTemporaryFile m_tempFile;

	std::vector<std::shared_ptr<CO_OrCommand>> commands;
	std::vector<ElementRegistry> m_processedElements;

	FrameRange m_frameRange;

	double m_selFrame;

	// must be declared before m_projection, which is initialized from it
	SettingsDesc m_settings;
	PROJECTION m_projection;
};
#endif
