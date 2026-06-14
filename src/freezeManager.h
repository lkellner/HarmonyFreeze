/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEZEMANAGER_H
#define FREEZEMANAGER_H

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

	void readSettings();

	void setMatrices(const Math::Matrix4x4 matrix, SC_SceneMetrics* sceneMetrics);
	Math::Matrix4x4 getFreezeMatrix() { return m_freezeMatrix; };
	Math::Matrix4x4 getOffsetMatrix() { return m_offsetMatrix; };
	Math::Matrix4x4 getSceneSettingsMatrix() { return m_sceneSettingsMatrix; };
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

		if (m_isDebugMode)
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
	bool isExperimentalMode() { return m_isExperimentalMode; }
	bool isMultithreadingMode() { return m_isMultithreadingMode; }
	bool isDebugMode() { return m_isDebugMode; }
	bool isPassOnOgl() { return m_isPassOnOgl; }
	bool isMoveUnusedPivots() { return m_isMoveUnusedPivots; }
	bool isSetInbetweenKfMode() { return m_isSetInbetweenKfMode; }

private:

	void writeAttr(QTextStream& out, const QString& moduleName, const StaticAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const TextAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const AttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const QuaternionAttrData& attr);
	void writeAttr(QTextStream& out, const QString& moduleName, const std::vector<CelInfo>& pivots);

	QMutex m_writeMutex;

	Math::Matrix4x4 m_freezeMatrix;
	Math::Matrix4x4 m_offsetMatrix;
	Math::Matrix4x4 m_zoomMatrix;
	Math::Matrix4x4 m_unitOffsetMatrix;
	Math::Matrix4x4 m_unitOffsetScaleMatrix;
	Math::Matrix4x4 m_ratioMatrix;
	Math::Matrix4x4 m_sceneSettingsMatrix;

	QFile m_file;
	QTemporaryFile m_tempFile;

	std::vector<std::shared_ptr<CO_OrCommand>> commands;
	std::vector<ElementRegistry> m_processedElements;

	FrameRange m_frameRange;

	double m_selFrame;

	PROJECTION m_projection;

	bool m_isExperimentalMode;
	bool m_isPassOnOgl;
	bool m_isMultithreadingMode;
	bool m_isMoveUnusedPivots;
	bool m_isDebugMode;
	bool m_isSetInbetweenKfMode;
};
#endif
