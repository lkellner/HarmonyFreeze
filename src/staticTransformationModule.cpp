#include "staticTransformationModule.h"

#include <BaseCore/maths/MT_Matrix4x4.h>
#include <SceneCore/attribute/AT_AttrCmds.h>
#include <SceneCore/attribute/AT_PositionAttrCmds.h>
#include <SceneCore/attribute/AT_Rotation3dAttrCmds.h>
#include <SceneCore/attribute/AT_Scale3dAttrCmds.h>
#include <SceneCore/attribute/AT_ScaleAttrCmds.h>
#include <SceneCore/selectable/SLB_CmdManipulator.h>
#include <SceneCore/selectable/SLB_ManipContext.h>
#include <Util/command/CO_OrCommand.h>
#include <Util/pluginmanager/PLUG_Scripting.h>
#include <Util/pluginmanager/PLUG_Services.h>


StaticTransformationModule::StaticTransformationModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
	, m_positionAttr(findAttribute<AT_Position3dAttr>(QStringLiteral("translate")))
	, m_scaleAttr(findAttribute<AT_Scale3dAttr>(QStringLiteral("scale")))
	, m_rotationAttr(findAttribute<AT_Rotation3dAttr>(QStringLiteral("rotate")))
	, m_skewXAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("skewX")))
	, m_skewYAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("skewY")))
	, m_skewZAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("skewZ")))
	, m_invertedAttr(findAttribute<AT_BoolAttr>(QStringLiteral("inverted")))
{
	if (!m_positionAttr)
		throw std::runtime_error("missing attribute: 'translate' for " + modulePtr->qualifiedName().toStdString());
	if (!m_scaleAttr)
		throw std::runtime_error("missing attribute: 'scale' for " + modulePtr->qualifiedName().toStdString());
	if (!m_rotationAttr)
		throw std::runtime_error("missing attribute: 'rotate' for " + modulePtr->qualifiedName().toStdString());
	if (!m_skewXAttr)
		throw std::runtime_error("missing attribute: 'skewX' for " + modulePtr->qualifiedName().toStdString());
	if (!m_skewYAttr)
		throw std::runtime_error("missing attribute: 'skewY' for " + modulePtr->qualifiedName().toStdString());
	if (!m_skewZAttr)
		throw std::runtime_error("missing attribute: 'skewZ' for " + modulePtr->qualifiedName().toStdString());
	if (!m_invertedAttr)
		throw std::runtime_error("missing attribute: 'inverted' for " + modulePtr->qualifiedName().toStdString());
}

MatrixParameters StaticTransformationModule::convertMatrixToParams(const Math::Matrix4x4& matrix) const
{
	Math::Angle3d angle;
	m_rotationAttr->getLocalValue(angle);

	Math::Matrix4x4 newMatrix = matrix;

	if (m_invertedAttr->localValue())
	{
		angle.setXYZ(angle.x() * -1, angle.y() * -1, angle.z() * -1);
		//TODO: selecting "Invert Transformation" won't lead to an inverse matrix.
		//While the z, y and translation columns match, the z column differs.
		//This is listed as a limitation for now, but it might be possible to recreate the algorithm Harmony uses,
		// in order to always calculate the correct parameters
		newMatrix = matrix.getInverse();
	}


	MatrixParameters params = matrixParameters(newMatrix, Math::Point3d(0, 0, 0), angle);

	clampValues(params);

	convertFromOGL(params, getModulePtr()->sceneMetrics());

	params.sxy = (abs(params.sx) + abs(params.sy) + abs(params.sz)) / 3;

	return params;
}

Math::Matrix4x4 StaticTransformationModule::getLocalMatrix(const double frameNo) const
{
	const MO_FrameKey frameKey = MO_FrameKey(frameNo);
	const Math::Matrix4x4 parent = getIncomingMatrix(0, frameNo, false);
	const Math::Matrix4x4 child = getModulePtr()->getModelMatrix(frameKey);

	return isScaleZero(parent)
		? Math::Matrix4x4()
		: parent.getInverse() * child;
}


void StaticTransformationModule::readjustSecondary()
{
	FreezeManager* fm = getFreezeManagerPtr();

	Math::Matrix4x4 newStaticLocalMatrix = fm->getFreezeMatrix()
		* getLocalMatrix(fm->getSelFrame()) * fm->getFreezeMatrix().getInverse();

	if (isScaleZero(newStaticLocalMatrix))
		return;

	setMatrixAsAttributes(newStaticLocalMatrix);
	setMatrixAsAttributesJS(newStaticLocalMatrix);
}

void StaticTransformationModule::setMatrixAsAttributes(const Math::Matrix4x4& matrix)
{
	auto curMacro = std::make_shared<CO_OrCommand>();

	const MatrixParameters params = convertMatrixToParams(matrix);

	curMacro->add(Attr::Position3d::createSetLocalValueCmd(m_positionAttr, params.tx, params.ty, params.tz));
	curMacro->add(Attr::Scale3d::createSetLocalValueCmd(m_scaleAttr, params.sx, params.sy, params.sz));
	curMacro->add(Attr::Rotation3d::createSetLocalValueCmd(m_rotationAttr, Math::Angle3d(params.ax, params.ay, params.az)));
	curMacro->add(Attr::Double::createSetLocalValueCmd(m_skewXAttr, params.skewx));
	curMacro->add(Attr::Double::createSetLocalValueCmd(m_skewYAttr, params.skewy));
	curMacro->add(Attr::Double::createSetLocalValueCmd(m_skewZAttr, params.skewz));

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void StaticTransformationModule::setMatrixAsAttributesJS(const Math::Matrix4x4& matrix) const
{
	const MatrixParameters params = convertMatrixToParams(matrix);

	//scale.xy is the correct attribute name, even though scale.xyz would be more intuitive

	FreezeManager* fm = getFreezeManagerPtr();

	fm->applyAttributes(getModulePtr()->qualifiedName(),
		StaticAttrData{ QLatin1String("translate.x"), params.tx },
		StaticAttrData{ QLatin1String("translate.y"), params.ty },
		StaticAttrData{ QLatin1String("translate.z"), params.tz },
		StaticAttrData{ QLatin1String("rotate.anglex"), params.ax },
		StaticAttrData{ QLatin1String("rotate.angley"), params.ay },
		StaticAttrData{ QLatin1String("rotate.anglez"), params.az },
		StaticAttrData{ QLatin1String("scale.x"), params.sx },
		StaticAttrData{ QLatin1String("scale.y"), params.sy },
		StaticAttrData{ QLatin1String("scale.z"), params.sz },
		StaticAttrData{ QLatin1String("scale.xy"), params.sxy },
		StaticAttrData{ QLatin1String("skewx"), params.skewx },
		StaticAttrData{ QLatin1String("skewy"), params.skewy },
		StaticAttrData{ QLatin1String("skewz"), params.skewz });
}
