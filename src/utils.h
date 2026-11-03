/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef UTILS_H
#define UTILS_H

#include <BaseCore/maths/MT_Matrix4x4.h>

#include <CelCore/Cel/CEL_Cel.h>
#include <CelCore/CelVector/CELVEC_Tbd.h>

#include <GraphicCore/GraphicLib/GR_CompositeDrawingInfo.h>
#include <GraphicCore/GraphicLib/GR_CompositeVectorDrawingObj.h>
#include <GraphicCore/GraphicLib/GR_VectorDrawingObj.h>

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_DrawingAttr.h>
#include <SceneCore/attribute/AT_ElementAttr.h>
#include <SceneCore/attribute/AT_ElementLayerAttr.h>
#include <SceneCore/attribute/AT_EnumAttrTemplate.h>
#include <SceneCore/attribute/AT_Enums.h>
#include <SceneCore/attribute/AT_TimingAttr.h>
#include <SceneCore/celcache/CA_CelKey.h>
#include <SceneCore/celcache/CA_CelPtr.h>
#include <SceneCore/celcache/CA_CelRef.h>
#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_Port.h>
#include <SceneCore/module/MO_SoftContext.h>
#include <SceneCore/module/MO_VectorCompositeInfo.h>
#include <SceneCore/scenecommon/SC_SceneMetrics.h>

#include <SDKExtension/SDK_CelCache.h>
#include <SDKExtension/SDK_Column.h>
#include <SDKExtension/SDK_Drawing.h>
#include <SDKExtension/SDK_Selection.h>


#include <vector>

enum class MatrixComplexity : uint8_t
{
	Simple,
	ScaleTranslationOnly,
	UniformScaleTranslationOnly,
	Complex
};

struct StaticAttrData
{
	QString name;
	double value;
};

struct AttrData
{
	QString name;
	double value;
	double frameNo;
	bool isKeyframe;
};

struct Point2dAttrData
{
	QString name;
	Math::Point2d value;
	double frameNo;
	bool isKeyframe;
};

struct QuaternionAttrData
{
	QString name;
	Math::Angle3d value;
	double frameNo;
	bool isKeyframe;
};

struct TextAttrData
{
	QString name;
	double value;
	double frameNo;
	bool isKeyframe;
};

struct MatrixParameters
{
	double tx = 0;
	double ty = 0;
	double tz = 0;
	double sx = 1;
	double sy = 1;
	double sz = 1;
	double sxy = 1;
	double ax = 0;
	double ay = 0;
	double az = 0;
	double skewx = 0;
	double skewy = 0;
	double skewz = 0;
};

struct FrameRange
{
	int start = INT_MAX;
	int end = -1;
};

struct CelInfo
{
	CA_CelPtr celPtr;
	long long drawingId = -1;
	int elementId = -1;
	QString layerAttr;
	Math::Point3d pivot;
};

struct Exposure
{
	int frameNo;
	CA_CelPtr cel;
};


//General

void getIncomingFrameRange(const MO_Node* node, FrameRange& frameRange);
void updateFrameRange(FrameRange& range, const int frame);
void clampValues(MatrixParameters& params);
void clampValues(Math::Point3d &point);
void clampValue(double & value, const double reference);

//Conversions

double convertRotation(const Math::Matrix4x4& unitOffsetMatrix, const double rotation);
double fieldsToOgl(const SC_SceneMetrics* sceneMetrics, const double rotation);
Math::Matrix4x4 fieldsToOgl(const SC_SceneMetrics* sceneMetrics, const Math::Matrix4x4 matrix);
Math::Matrix4x4 oglToFields(const SC_SceneMetrics* sceneMetrics, const Math::Matrix4x4 matrix);
void convertFromOGL(MatrixParameters& params, const SC_SceneMetrics* sceneMetrics);

Math::Matrix4x4 oglToVectorModificationMatrix(const SC_SceneMetrics* sceneMetrics,
	const Math::Matrix4x4 modMatrix, const CELVEC_Tbd* tbd);
Math::Matrix4x4 oglToFieldsModificationMatrix(const SC_SceneMetrics* sceneMetrics,
	const Math::Matrix4x4 modMatrix);
Math::Matrix4x4 getElementFlipMatrix(const MO_Module* modulePtr);

Math::Matrix4x4 oglToVector(const Math::Matrix4x4 matrix);
Math::Point3d vectorToOgl(const Math::Point3d point, const CA_CelPtr celPtr);

double matchFullRotations(const double referenceAngle, const double angle);

//Matrices

Math::Matrix4x4 get2dRotationMatrix(const Math::Matrix2x3 matrix);
Math::Matrix4x4 convertToProjectionMatrix(const Math::Matrix4x4 matrix);

Math::Point3d getScale(const Math::Matrix4x4& matrix);
double getAngle2d(const Math::Matrix2x3 matrix);

MatrixParameters matrixParameters(const Math::Matrix4x4& matrix, const Math::Point3d pivot,
	const Math::Angle3d rotation);
void printMatrixParameters(const MatrixParameters p);

MatrixComplexity defineMatrixComplexity(const Math::Matrix4x4& matrix, const bool is3D);
bool isIdentity(const Math::Matrix4x4 matrix);
bool isScaleZero(const Math::Matrix4x4& matrix);

//Node Tree

std::vector<MO_Node*> getAllChildren(const MO_Node* node);
std::vector<MO_Module*> getAllInputModules(const MO_Module* modulePtr);
bool isShapeAwareMatteOnly(const MO_Node* node);

//Drawing Utils

std::vector<CelInfo> getElementTimings(const MO_Module* modulePtr);
std::vector<Exposure> getExposures(const MO_Module* modulePtr, const FrameRange range);
Math::Point2d getDrawingPivot(const CA_CelPtr cel);
void setDrawingPivot(CA_CelPtr cel, const Math::Point3d pivot);

CELVEC_Tbd* getPermTbd(const CA_CelPtr celPtr);

//Attributes

bool hasUsedDrawingPivots(const MO_Module* modulePtr);
bool isDrawingPivotAppliedOnPeg(const MO_Module* modulePtr);

QString getLayerAttr(const MO_Module* modulePtr);

AT_AttrList getAttributeList(const MO_Node* node);
int getElementId(const MO_Module* modulePtr);
void printAttributes(const AT_AttrList attributes);

template <typename T>
T* findAttribute(const QLatin1StringView keyword, const MO_Node* node)
{
	if (!node)
		return nullptr;

	const AT_AttrList attributes = getAttributeList(node);

	const auto it = std::find_if(attributes.begin(), attributes.end(),
		[&keyword](const AT_AttrDesc& a) { return a._pAttr && a._pAttr->keyword() == keyword; });

	return it == attributes.end() ? nullptr : dynamic_cast<T*>(it->_pAttr);
}

template <typename T>
T* findSubAttribute(const AT_ComplexAttr* parent, const QLatin1StringView keyword)
{
	if (!parent)
		return nullptr;

	const auto it = std::find_if(parent->attrsBegin(), parent->attrsEnd(),
		[&keyword](const AT_Attr* a) { return  a && a->keyword() == keyword; });

	return it == parent->attrsEnd() ? nullptr : dynamic_cast<T*>(*it);
}

template <typename T>
T* findSubAttribute(const QLatin1StringView parentKeyword, const QLatin1StringView keyword, MO_Node* node)
{
	if(!node)
		return nullptr;

	AT_ComplexAttr * parent = findAttribute<AT_ComplexAttr>(parentKeyword, node);

	if (!parent)
		return nullptr;

	const auto it = std::find_if(parent->attrsBegin(), parent->attrsEnd(),
		[&keyword](const AT_Attr* a) { return  a && a->keyword() == keyword; });

	return it == parent->attrsEnd() ? nullptr : dynamic_cast<T*>(*it);
}

template <typename T = AT_Rotation3dAttr>
bool isQuarternionKeyframe(const T& t, double frame, Math::Angle3d& angle)
{
	bool isCtrlPnt = false;
	bool isConstSeg = false;

	if constexpr (requires { t.getValue(frame, angle, (double*)nullptr, &isCtrlPnt, &isConstSeg); })
	{
		t.getValue(frame, angle, static_cast<double*>(nullptr), &isCtrlPnt, &isConstSeg);
	}
	else
	{
		// older SDK
		t.getValue(frame, angle, &isCtrlPnt, &isConstSeg);
	}

	return isCtrlPnt;
}

constexpr bool supportsKeysRange()
{
#if SDK_MAJOR_VERSION > 25
return true;
#else
return false;
#endif

}
#endif
