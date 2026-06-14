#include "curveModuleBase.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>

#include <limits>
#include <stdexcept>

static void recalculateIndependentPoint(const MO_Module* modulePtr,
		const Math::Matrix4x4& matrix,
		Math::Point2d& point)
{
	point = modulePtr->sceneMetrics()->toOGL(point);
	point = point * matrix.getTransform2d();
	point = modulePtr->sceneMetrics()->fromOGL(point);
}

static const QString& attrKeyword(const AT_AttrDesc& desc)
{
	return desc._pAttr->keyword();
}

BezierHandle::BezierHandle(std::shared_ptr<FreezeManager> freezeManager,
		AT_DoubleAttr* lengthAttr,
		AT_DoubleAttr* orientationAttr,
		QString lengthAttrJS,
		QString orientationAttrJS,
		Kind kind)
	: m_freezeManager(std::move(freezeManager))
	, m_lengthAttr(lengthAttr)
	, m_orientationAttr(orientationAttr)
	, m_lengthAttrJS(std::move(lengthAttrJS))
	, m_orientationAttrJS(std::move(orientationAttrJS))
	, m_kind(kind)
{
}

void BezierHandle::recalculateStaticHandlePoint(MO_Module* modulePtr,
		const Math::Matrix4x4& changeMatrix,
		CO_OrCommand& curMacro,
		bool isIndependent)
{
	if (!modulePtr)
		return;

	BezierHandleInfo handleInfo = getRecalculatedHandleInfo(modulePtr->sceneMetrics(), changeMatrix,
			m_lengthAttr->localValue(), m_orientationAttr->localValue(), isIndependent);

	m_prevRadius = handleInfo.radius; //Saving the current value for the first animated keyframe
	m_prevAngle = handleInfo.angle;

	if (m_freezeManager->isExperimentalMode())
	{
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_lengthAttr, handleInfo.radius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_orientationAttr, handleInfo.angle));
	}
	else
	{
		m_freezeManager->applyAttributes(modulePtr->qualifiedName(),
				StaticAttrData{ m_lengthAttrJS, handleInfo.radius },
				StaticAttrData{ m_orientationAttrJS, handleInfo.angle });
	}
}

// FIXME: get rid of these bools
void BezierHandle::recalculateAnimatedHandlePoint(MO_Module* modulePtr,
		Math::Matrix4x4 changeMatrix,
		CO_OrCommand& curMacro,
		const BezKeyframeData& kfData,
		double frameNo,
		bool isIndependent)
{
	if (!modulePtr)
		return;

	const BezierHandleInfo handleInfo = getRecalculatedHandleInfo(modulePtr->sceneMetrics(), changeMatrix,
			m_lengthAttr->value(frameNo), m_orientationAttr->value(frameNo), isIndependent);


	//Sometimes it might be necessary to add an extra keyframe to the radius/length as it also depends on the orientation.
	//An extra keyframe is being added here, if it is the last frame in the animation or if the value differs from.
	//the following one.
	const bool isSetRadius = kfData.length == KeyframeState::Keyframe ||
		(m_prevRadius != handleInfo.radius && kfData.length == KeyframeState::PossibleKeyframe);
	const bool isSetAngle = kfData.orientation == KeyframeState::Keyframe ||
		(m_prevAngle != handleInfo.angle && kfData.orientation == KeyframeState::PossibleKeyframe);

	m_prevRadius = handleInfo.radius; //Saving the current value for the next keyframe
	m_prevAngle = handleInfo.angle;

	if (m_freezeManager->isExperimentalMode())
	{
		if (isSetRadius)
			curMacro.add(Attr::Double::createSetValueCmd(m_lengthAttr, frameNo, handleInfo.radius));

		if (isSetAngle)
			curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, handleInfo.angle));
	}
	else
	{
		m_freezeManager->applyAttributes(modulePtr->qualifiedName(),
			AttrData{ m_lengthAttrJS, handleInfo.radius, frameNo, isSetRadius },
			AttrData{ m_orientationAttrJS, handleInfo.angle, frameNo, isSetAngle });
	}
}

BezierHandleInfo getRecalculatedHandleInfo(SC_SceneMetrics* sceneMetrics,
		const Math::Matrix4x4& changeMatrix,
		double length,
		double angle,
		bool isIndependent) // FIXME: remove bool if possible
{
	const Math::Matrix4x4 transMatrix = Math::Matrix4x4().translate(length, 0, 0);

	Math::Matrix4x4 rotMatrix = Math::Matrix4x4().rotateDegrees(angle);

	rotMatrix *= transMatrix;

	if(isIndependent)
	{
		rotMatrix = fieldsToOgl(sceneMetrics, rotMatrix);
		rotMatrix = changeMatrix.rotation() * rotMatrix;
		rotMatrix = oglToFields(sceneMetrics, rotMatrix);
	}
	else
		rotMatrix = changeMatrix.rotation() * rotMatrix;

	const Math::Point3d origin = rotMatrix.origin();

	//Rougly estimated angle used as a reference know whether to add n*360 degrees
	//to final angle. This is to make sure that animatio don't break
	const double referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + angle;

	const double orientation = matchFullRotations(referenceAngle, getAngle2d(rotMatrix.getTransform2d()));

	return { origin.toVector().length(), orientation};
}


CurveModuleBase::CurveModuleBase(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
	, m_offsetAttr(nullptr)
	, m_restingOffsetAttr(nullptr)
	, m_isIndependent(true)
{
	initAttributes();
}

FrameRange CurveModuleBase::getFrameRange() const
{
	int key;

	FrameRange range;

	if (m_offsetAttr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateX()->getPrevKey(std::numeric_limits<int>::max(), &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateY()->getPrevKey(std::numeric_limits<int>::max(), &key))
		updateFrameRange(range, key);

	return range;
}

void CurveModuleBase::readjustOffsets(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix)
{
	const MO_Module *mod = getModulePtr();

	Math::Point2d restingOffset;
	Math::Point2d staticOffset;

	//---resting and local (static) offset

	m_restingOffsetAttr->getLocalValue(restingOffset);
	m_offsetAttr->getLocalValue(staticOffset);

	m_prevOffset = staticOffset;

	if (isIndependent())
	{
		recalculateIndependentPoint(mod, changeMatrix, restingOffset);
		recalculateIndependentPoint(mod, changeMatrix, staticOffset);
	}
	else
	{
		restingOffset *= adjustDependentMatrix(changeMatrix, false, true).getTransform2d();
		staticOffset *= adjustDependentMatrix(changeMatrix, false, false).getTransform2d();
	}

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(m_restingOffsetAttr, restingOffset.x(), restingOffset.y()));
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(m_offsetAttr, staticOffset.x(), staticOffset.y()));
	}
	else
	{
		//JS
		fm->applyAttributes(getModulePtr()->qualifiedName(),
				StaticAttrData{ QLatin1String("restingoffset.x"), restingOffset.x() },
				StaticAttrData{ QLatin1String("restingoffset.y"), restingOffset.y() },
				StaticAttrData{ QLatin1String("offset.x"), staticOffset.x() },
				StaticAttrData{ QLatin1String("offset.y"), staticOffset.y() });
	}

	//Animated values

	const FrameRange range = getFrameRange();

	bool isCtrlPnt = false;

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = curFrame;
		Math::Point2d offset;

		m_offsetAttr->getValue(frameNo, offset, &isCtrlPnt);
		//TODO: could maybe split this one up

		//Set keyframe either way
		bool isForceKeyframe = curFrame == range.start && isComplexTransform();

		//Set keyframe in case it different from the previous value
		bool isAdjustKeyframe = (!isIndependent() && isParentKeyframe(frameNo)) || fm->isSetInbetweenKfMode();
		isAdjustKeyframe = isAdjustKeyframe && isComplexTransform();

		if (!isIndependent())
			offset *= adjustDependentMatrix(changeMatrix, true, frameNo).getTransform2d();
		else
			recalculateIndependentPoint(getModulePtr(), changeMatrix, offset);


		if (!isCtrlPnt && !(m_prevOffset != offset && isAdjustKeyframe) && !isForceKeyframe)
		{
			//This is neither a previous keyframe, nor does it need to be set either way
			//-> no keyframe needs to be set here
			m_prevOffset = offset;
			continue;
		}

		//Setting storing offset for the next keyframe;

		m_prevOffset = offset;

		if (fm->isExperimentalMode())
		{
			//C++
			curMacro.add(Attr::Position2d::createSetValueCmd(m_offsetAttr, frameNo, offset.x(), offset.y()));
		}
		else
		{
			//JS
			fm->applyAttributes(getModulePtr()->qualifiedName(),
					AttrData{ QLatin1String("offset.x"), offset.x(), frameNo, true },
					AttrData{ QLatin1String("offset.y"), offset.y(), frameNo, true });
		}
	}
}

Math::Matrix4x4 CurveModuleBase::adjustDependentMatrix(const Math::Matrix4x4 matrix,
		const bool isAnimated,
		const double frameNo,
		const bool isRest) const
{
	const Math::Matrix4x4 parentMatrix = isAnimated
		?  Math::Matrix4x4().rotateDegrees(getChainRotation(frameNo))
		: Math::Matrix4x4().rotateDegrees(getStaticChainRotation(isRest));

	const Math::Matrix4x4 newParentMatrix = get2dRotationMatrix((matrix * parentMatrix).getTransform2d());

	FreezeManager* fm = getFreezeManagerPtr();

	return fm->getUnitOffsetScaleMatrix() * newParentMatrix.getInverse() *  matrix  * parentMatrix * fm->getUnitOffsetScaleMatrix().getInverse();
}


double CurveModuleBase::getStaticChainRotation(bool isRest) const
{
	const auto curveAttrKeyword = isRest
		? QStringLiteral("restingOrientation1")
		: QStringLiteral("orientation1");

	const auto offsetAttrKeyword = isRest
		? QStringLiteral("restingOrientation")
		: QStringLiteral("orientation");

	return getRotationImpl(curveAttrKeyword, offsetAttrKeyword,
			[](const AT_DoubleAttr& a) { return a.localValue(); });
}

double CurveModuleBase::getChainRotation(const double frameNo) const
{
	return getRotationImpl(QStringLiteral("orientation1"), QStringLiteral("orientation"),
			[frameNo](const AT_DoubleAttr& a) { return a.value(frameNo); });
}

void CurveModuleBase::initAttributes()
{
	for (const AT_AttrDesc& a : getAttributeList())
	{
		const QString& kw = attrKeyword(a);

		if (kw == QLatin1String("offset"))
			m_offsetAttr = dynamic_cast<AT_Position2dAttr*>(a._pAttr);
		else if (kw == QLatin1String("restingOffset"))
			m_restingOffsetAttr = dynamic_cast<AT_Position2dAttr*>(a._pAttr);
		else if (kw == QLatin1String("restingOffset"))
			m_restingOffsetAttr = dynamic_cast<AT_Position2dAttr*>(a._pAttr);
		else if (kw == QLatin1String("localReferential") && getModuleType() != ModuleType::CURVE_OFFSET)
		{
			const auto *attr = dynamic_cast<const AT_BoolAttr*>(a._pAttr);
			//Even though it doesn't seem to do anything in offset modules, this attribute still exists
			m_isIndependent = attr && !attr->localValue();
		}

		if (m_offsetAttr && m_restingOffsetAttr)
			return;
	}

	MO_Module* modulePtr = getModulePtr();

	if (!m_offsetAttr)
		throw std::runtime_error("missing attribute: 'offset' for " + modulePtr->qualifiedName().toStdString());

	if (!m_restingOffsetAttr)
		throw std::runtime_error("missing attribute: 'restingOffset' for " + modulePtr->qualifiedName().toStdString());
}

template <typename ValueFunc>
double CurveModuleBase::getRotationImpl(const QString& curveAttrKeyword,
		const QString& offsetAttrKeyword,
		ValueFunc&& valFunc) const
{
	FreezeManager* fm = getFreezeManagerPtr();

	const MO_Module *mod = getModulePtr();

	const MO_Node* parent = mod->getParentNode();

	double rotation = 0.0;

	while (parent && (parent->keyword() == QLatin1String("CurveModule")
				|| parent->keyword() == QLatin1String("OffsetModule")))
	{
		const AT_AttrList attributes = ::getAttributeList(parent);

		for (const AT_AttrDesc& attribute : std::as_const(attributes))
		{
			if (attribute._pAttr->keyword() == curveAttrKeyword)
			{
				const auto *a = dynamic_cast<const AT_DoubleAttr*>(attribute._pAttr);

				//Curve module
				if (a)
					//rotation += valFunc(*a);
					rotation += applyUnitOffset(fm->getUnitOffsetScaleMatrix(), valFunc(*a));
			}
			else if (attribute._pAttr->keyword() == offsetAttrKeyword)
			{
				const auto *a = dynamic_cast<const AT_DoubleAttr*>(attribute._pAttr);

				//Offset module
				if (a)
					rotation += fieldsToOgl(mod->sceneMetrics(), valFunc(*a));
			}
		}

		parent = parent->getParentNode();
	}

	return rotation;
}

bool CurveModuleBase::isParentKeyframe(double frameNo)
{
	bool isKeyframe = false;

	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && (parent->keyword() == QLatin1String("CurveModule")
		|| parent->keyword() == QLatin1String("OffsetModule")))
	{
		AT_DoubleAttr* attr;

		if(parent->keyword() == QLatin1String("CurveModule"))
			attr = ::findAttribute<AT_DoubleAttr>(QLatin1String("orientation1"), parent);

		if (parent->keyword() == QLatin1String("OffsetModule"))
			attr = ::findAttribute<AT_DoubleAttr>(QLatin1String("orientation"), parent);


		if(attr)
			attr->value(frameNo, &isKeyframe);

		if (isKeyframe)
				return true;

		parent = parent->getParentNode();
	}

	return false;
}
