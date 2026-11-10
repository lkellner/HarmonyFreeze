#include "curveModule.h"

using namespace Qt::StringLiterals;

BezierHandle::BezierHandle(std::shared_ptr<FreezeManager> freezeManager,
	AT_DoubleAttr* lengthAttr,
	AT_DoubleAttr* orientationAttr,
	QString lengthAttrJS,
	QString orientationAttrJS,
	Kind kind,
	bool isIndependent)
	: m_freezeManager(std::move(freezeManager))
	, m_lengthAttr(lengthAttr)
	, m_orientationAttr(orientationAttr)
	, m_lengthAttrJS(std::move(lengthAttrJS))
	, m_orientationAttrJS(std::move(orientationAttrJS))
	, m_kind(kind)
	, m_isIndependent(isIndependent)
{
}

void BezierHandle::updateKeyframeData(KeyframeData& kfData, const BezierHandleInfo handleInfo) const
{
	kfData.orientation = kfData.orientation || kfData.isForceKeyframe || (kfData.isAdjustKeyframe && m_prevAngle != handleInfo.angle);
	kfData.length = kfData.length || kfData.isForceKeyframe || (kfData.isAdjustKeyframe && m_prevRadius != handleInfo.radius);
}

void BezierHandle::recalculateStaticHandlePoint(const MO_Module* modulePtr,
	const Math::Matrix4x4& changeMatrix,
	CO_OrCommand& curMacro)
{
	if (!modulePtr)
		return;

	BezierHandleInfo handleInfo = getRecalculatedHandleInfo(modulePtr, changeMatrix,
		m_lengthAttr->localValue(), m_orientationAttr->localValue());

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

void BezierHandle::recalculateAnimatedHandlePoint(const MO_Module* modulePtr,
	const Math::Matrix4x4 changeMatrix,
	CO_OrCommand& curMacro,
	KeyframeData& kfData,
	const double frameNo)
{
	if (!modulePtr)
		return;

	const BezierHandleInfo handleInfo = getRecalculatedHandleInfo(modulePtr, changeMatrix,
		m_lengthAttr->value(frameNo), m_orientationAttr->value(frameNo));

	updateKeyframeData(kfData, handleInfo);
	//Sometimes it might be necessary to add an extra keyframe to the radius/length as it also depends on the orientation.
	//An extra keyframe is being added here, if it is the last frame in the animation or if the value differs from.
	//the following one.

	m_prevRadius = handleInfo.radius; //Saving the current value for the next keyframe
	m_prevAngle = handleInfo.angle;

	if (m_freezeManager->isExperimentalMode())
	{
		//Need to use the version that allows for forcing keyframes
		if (kfData.length)
			curMacro.add(Attr::Double::createSetValueCmd(m_lengthAttr, frameNo, handleInfo.radius, kfData.isConstSegLength, true));

		if (kfData.orientation)
			curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, handleInfo.angle, kfData.isConstSegOrientation, true));
	}
	else
	{
		if (kfData.length || kfData.orientation)
		{
			m_freezeManager->applyAttributes(modulePtr->qualifiedName(),
				AttrData{ m_lengthAttrJS, handleInfo.radius, frameNo, kfData.length },
				AttrData{ m_orientationAttrJS, handleInfo.angle, frameNo, kfData.orientation });
		}
	}
}

BezierHandleInfo BezierHandle::getRecalculatedHandleInfo(const MO_Module* modulePtr,
	const Math::Matrix4x4& changeMatrix,
	const double length,
	const double angle) const
{
	const Math::Matrix4x4 transMatrix = Math::Matrix4x4().translate(length, 0, 0);

	Math::Matrix4x4 rotMatrix = Math::Matrix4x4().rotateDegrees(angle);

	rotMatrix *= transMatrix;

	if (m_isIndependent)
		rotMatrix = recalculateIndependentHandleMatrix(modulePtr->sceneMetrics(), rotMatrix, changeMatrix);
	else
		rotMatrix = changeMatrix.rotation() * rotMatrix;

	const Math::Point3d origin = rotMatrix.origin();

	//Rougly estimated angle used as a reference know whether to add n*360 degrees
	//to final angle. This is to make sure that animations don't break
	const double referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + angle;

	const double orientation = matchFullRotations(referenceAngle, getAngle2d(rotMatrix.getTransform2d()));

	return { origin.toVector().length(), orientation };
}

CurveModule::CurveModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType)
	: CurveModuleBase(freezeManager, modulePtr, moduleType)
	, m_restLength0Attr(nullptr)
	, m_restLength1Attr(nullptr)
	, m_restingOrientation0Attr(nullptr)
	, m_restingOrientation1Attr(nullptr)
	, m_length0Attr(nullptr)
	, m_length1Attr(nullptr)
	, m_orientation0Attr(nullptr)
	, m_orientation1Attr(nullptr)
{
	loadAttributes();

	m_bezierHandles.emplace_back(getFreezeManager(), m_length0Attr, m_orientation0Attr,
		u"length0"_s, u"orientation0"_s, BezierHandle::Kind::Animatable, isIndependent());

	m_bezierHandles.emplace_back(getFreezeManager(), m_length1Attr, m_orientation1Attr,
		u"length1"_s, u"orientation1"_s, BezierHandle::Kind::Animatable, isIndependent());

	m_bezierHandles.emplace_back(getFreezeManager(), m_restLength0Attr, m_restingOrientation0Attr,
		u"restlength0"_s, u"restingOrientation0"_s, BezierHandle::Kind::Rest, isIndependent());

	m_bezierHandles.emplace_back(getFreezeManager(), m_restLength1Attr, m_restingOrientation1Attr,
		u"restlength1"_s, u"restingOrientation1"_s, BezierHandle::Kind::Rest, isIndependent());
}

FrameRange CurveModule::getFrameRange() const
{
	FrameRange range = CurveModuleBase::getFrameRange();

	int key;

	if (m_length0Attr)
	{
		if (m_length0Attr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_length0Attr->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}

	if (m_length1Attr)
	{
		if (m_length1Attr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_length1Attr->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}

	if (m_orientation0Attr)
	{
		if (m_orientation0Attr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_orientation0Attr->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}

	if (m_orientation1Attr)
	{
		if (m_orientation1Attr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_orientation1Attr->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}

	if (isIndependent())
		return range;

	//Need to include the whole chain's range for dependent curve modules as sometimes it's necessary
	//to force keyframes
	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && (parent->keyword() == "CurveModule"_L1
				|| parent->keyword() == "OffsetModule"_L1))
	{
		AT_DoubleAttr* orientationAttr  = nullptr;

		if (parent->keyword() == "OffsetModule"_L1)
			orientationAttr = ::findAttribute<AT_DoubleAttr>("orientation"_L1, parent);
		else
			orientationAttr = ::findAttribute<AT_DoubleAttr>("orientation1"_L1, parent);

		if (orientationAttr)
		{
			if (orientationAttr->getNextKey(0, &key))
				updateFrameRange(range, key);				

			if (orientationAttr->getPrevKey(std::numeric_limits<int>::max(), &key))
				updateFrameRange(range, key);
		}

		parent = parent->getParentNode();
	}

	return range;
}

template <typename T>
struct AttrBinding
{
	const char *kw;
	T m;
};

void CurveModule::loadAttributes()
{
	const AT_AttrList attributes = getAttributeList();

	static constexpr auto bindings = std::make_tuple(
			AttrBinding { "restLength0", &CurveModule::m_restLength0Attr },
			AttrBinding { "restLength1", &CurveModule::m_restLength1Attr },
			AttrBinding { "Length0", &CurveModule::m_length0Attr },
			AttrBinding { "Length1", &CurveModule::m_length1Attr },
			AttrBinding { "restingOrientation0", &CurveModule::m_restingOrientation0Attr },
			AttrBinding { "restingOrientation1", &CurveModule::m_restingOrientation1Attr },
			AttrBinding { "orientation0", &CurveModule::m_orientation0Attr },
			AttrBinding { "orientation1", &CurveModule::m_orientation1Attr }
	);

	for (const AT_AttrDesc & desc : std::as_const(attributes))
	{
		const auto dispatch = [this, &desc](auto&& ...bindings)
		{
			const QString& keyword = desc._pAttr->keyword();

			const auto testAttr = [this, &keyword, &desc](const auto& binding)
			{
				if (keyword != QLatin1StringView(binding.kw))
					return false;

				auto &ptr = this->*binding.m;
				ptr = dynamic_cast<std::remove_reference_t<decltype(ptr)>>(desc._pAttr);
				return true;
			};

			(testAttr(bindings) || ...);
		};

		std::apply(dispatch, bindings);
	}

	MO_Module* modulePtr = getModulePtr();

	m_regionOfInfluence = std::make_unique<RegionOfInfluence>(getFreezeManager(), modulePtr);

	if (!m_restLength0Attr)
		throw std::runtime_error("missing attribute: 'restLength0' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restLength1Attr)
		throw std::runtime_error("missing attribute: 'restLength1' for " + modulePtr->qualifiedName().toStdString());
	if (!m_length0Attr)
		throw std::runtime_error("missing attribute: 'Length0' for " + modulePtr->qualifiedName().toStdString());
	if (!m_length1Attr)
		throw std::runtime_error("missing attribute: 'Length1' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restingOrientation0Attr)
		throw std::runtime_error("missing attribute: 'restingOrientation0' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restingOrientation1Attr)
		throw std::runtime_error("missing attribute: 'restingOrientation1' for " + modulePtr->qualifiedName().toStdString());
	if (!m_orientation0Attr)
		throw std::runtime_error("missing attribute: 'orientation0' for " + modulePtr->qualifiedName().toStdString());
	if (!m_orientation1Attr)
		throw std::runtime_error("missing attribute: 'orientation1' for " + modulePtr->qualifiedName().toStdString());
	if (!m_regionOfInfluence)
		throw std::runtime_error("missing attribute set: 'm_regionOfInfluence' for " + modulePtr->qualifiedName().toStdString());	
}

void CurveModule::readjustSecondary()
{
	FreezeManager *fm = getFreezeManagerPtr();

	Math::Matrix4x4 changeMatrix = fm->getFreezeMatrix();

	setMatrixComplexity(fm->getMatrixComplexity(false));

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	if (!isIndependent())
	{
		//getting only skew and scale parts of the matrix since translation and rotation are only applied to the offset
		changeMatrix = get2dRotationMatrix(changeMatrix.getTransform2d()).getInverse() * changeMatrix.rotation();

		//Freeze matrix doesn't contain any scale/shear part, nothing left to do
		if (isIdentity(changeMatrix))
			return;
	}

	readjustBezierHandles(*curMacro, changeMatrix);
	readjustOffsets(*curMacro, changeMatrix);
	m_regionOfInfluence->readjustSecondary(*curMacro, changeMatrix);

	fm->addCommand(std::move(curMacro));
}

void CurveModule::readjustBezierHandles(CO_OrCommand& curMacro, const Math::Matrix4x4 matrix)
{
	for (auto& bezierHandle : m_bezierHandles)
	{
		if (!isIndependent())
			bezierHandle.recalculateStaticHandlePoint(getModulePtr(), adjustDependentMatrix(matrix, false, bezierHandle.isRest()), curMacro);
		else
			bezierHandle.recalculateStaticHandlePoint(getModulePtr(), matrix, curMacro);
	}

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = static_cast<double>(curFrame);

		for (auto& bezierHandle : m_bezierHandles)
		{
			if(!bezierHandle.isAnimatable())
				continue;

			BezierHandle::KeyframeData kfData = generateKeyframeData(bezierHandle, frameNo, curFrame == range.start);

			if (!kfData.isProcessFrame)
				continue;

			if (!isIndependent())
				bezierHandle.recalculateAnimatedHandlePoint(getModulePtr(), adjustDependentMatrix(matrix, true, frameNo),
					curMacro, kfData, frameNo);
			else
				bezierHandle.recalculateAnimatedHandlePoint(getModulePtr(), matrix, curMacro, kfData, frameNo);
		}
	}
}

BezierHandle::KeyframeData CurveModule::generateKeyframeData(const BezierHandle& bezierHandle,
	const double frameNo,
	const bool isFirst) const
{
	BezierHandle::KeyframeData kfData;

	bezierHandle.getLengthAttr()->value(frameNo, &kfData.length, &kfData.isConstSegLength);
	bezierHandle.getOrientationAttr()->value(frameNo, &kfData.orientation, &kfData.isConstSegOrientation);

	kfData.isAdjustKeyframe = kfData.orientation || kfData.length || (!isIndependent() && isParentKeyframe(frameNo))
		|| getFreezeManagerPtr()->isSetInbetweenKfMode();

	kfData.isAdjustKeyframe = kfData.isAdjustKeyframe && isComplexTransform();
	kfData.isForceKeyframe = isFirst && isComplexTransform();

	kfData.isProcessFrame = kfData.isAdjustKeyframe || kfData.isForceKeyframe || kfData.orientation || kfData.length;

	return kfData;
}
