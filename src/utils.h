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

#include <SceneCore/attribute/AT_DrawingAttr.h>
#include <SceneCore/attribute/AT_ElementAttr.h>
#include <SceneCore/attribute/AT_ElementLayerAttr.h>
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

#include <SceneCore/attribute/AT_EnumAttrTemplate.h>
#include <SceneCore/attribute/AT_Enums.h>
#include <SceneCore/attribute/AT_DoubleAttr.h>


#include <vector>

enum class MatrixComplexity : uint8_t
{
	Simple,
	ScaleTranslationOnly,
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

void updateFrameRange(FrameRange& range, int frame);
void clampValues(MatrixParameters& params);
void clampValues(Math::Point3d &point);
void clampValue(double & value, const double reference);

//Conversions

double applyUnitOffset(const Math::Matrix4x4& unitOffsetMatrix, const double rotation);
double fieldsToOgl(SC_SceneMetrics* sceneMetrics, double rotation);
Math::Matrix4x4 fieldsToOgl(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 matrix);
Math::Matrix4x4 oglToFields(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 matrix);
void convertFromOGL(MatrixParameters& params, SC_SceneMetrics* sceneMetrics);

Math::Matrix4x4 getVectorModificationMatrix(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 modMatrix, CELVEC_Tbd* tbd);
Math::Matrix4x4 getFieldsModificationMatrix(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 modMatrix);
Math::Matrix4x4 getElementFlipMatrix(MO_Module* modulePtr);

Math::Matrix4x4 oglToVector(Math::Matrix4x4 matrix);
Math::Point3d vectorToOgl(Math::Point3d point, CA_CelPtr celPtr);

double toRadians(double val);
double fromRadians(double val);

double matchFullRotations(const double referenceAngle, const double angle);


//Matrices

Math::Point3d getScale(const Math::Matrix4x4 & matrix);
Math::Matrix4x4 get2dRotationMatrix(Math::Matrix2x3 matrix);

double getAngleBetween(double angle01, double angle02);
double getAngle2d(const Math::Matrix2x3 matrix);
double getAngleOffset(Math::Angle3d angle01, Math::Angle3d angle02);
Math::Angle3d getAngles(Math::Matrix4x4 matrix, double angleZ);
Math::Vector2d getFlip2d(const Math::Matrix2x3& matrix, double rotationZ);
MatrixParameters getAnglesAndFlip3d(const Math::Matrix4x4& matrix, Math::Angle3d rotation);

MatrixParameters matrixParameters(const Math::Matrix4x4& matrix, const Math::Point3d pivot, Math::Angle3d rotation);
MatrixParameters matrixParameters3d(const Math::Matrix4x4& matrix, const Math::Point3d pivot, Math::Angle3d rotation);
MatrixParameters matrixParameters2d(const Math::Matrix4x4& matrix, const Math::Point3d pivot, double rotationZ);
void printMatrixParameters(MatrixParameters p);

Math::Matrix4x4 convertToProjectionMatrix(Math::Matrix4x4 matrix);

double getAffineDeterminant(Math::Matrix4x4 matrix);
MatrixComplexity defineMatrixComplexity(const Math::Matrix4x4& matrix, bool is3D);
bool isIdentity(Math::Matrix4x4 matrix);
bool isScaleZero(const Math::Matrix4x4& matrix);
bool isTransform2d(const Math::Matrix4x4& matrix);


//Node Tree

std::vector<MO_Node*> getAllChildren(MO_Node* node);
std::vector<MO_Module*> getAllInputModules(MO_Module* modulePtr);

//Drawing Utils

std::vector<CelInfo> getElementTimings(MO_Module* modulePtr);
std::vector<Exposure> getExposures(MO_Module* modulePtr, FrameRange range);
Math::Point2d getDrawingPivot(const CA_CelPtr cel);
void setDrawingPivot(CA_CelPtr cel, const Math::Point3d pivot);

CELVEC_Tbd* getPermTbd(CA_CelPtr celPtr);


//Attributes

bool hasUsedDrawingPivots(MO_Module* modulePtr);
bool isDrawingPivotAppliedOnPeg(MO_Module* modulePtr);

QString getLayerAttr(MO_Module* modulePtr);

AT_AttrList getAttributeList(const MO_Node* node);

template <typename T>
T* findAttribute(const QString& keyword, MO_Node* node)
{
	const AT_AttrList attributes = getAttributeList(node);

	const auto it = std::find_if(attributes.begin(), attributes.end(),
		[&keyword](const AT_AttrDesc& a) { return a._pAttr->keyword() == keyword; });

	return it == attributes.end() ? nullptr : dynamic_cast<T*>(it->_pAttr);
}


int getElementId(MO_Module* modulePtr);
void printAttributes(AT_AttrList attributes);

#endif
