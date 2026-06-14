#include "curveModule.h"

#include <SceneCore/module/MO_PortTransform.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>

#include <Util/pluginmanager/PLUG_Services.h>

#include <QString>

#include <limits>
#include <tuple>
#include <utility>

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
	, m_longitudinalRadiusAttr(nullptr)
	, m_longitudinalRadiusBeginAttr(nullptr)
	, m_transversalRadiusAttr(nullptr)
	, m_transversalRadiusRightAttr(nullptr)
	, m_influenceFadeRadiusAttr(nullptr)
{
	loadAttributes();

	m_bezierHandles.emplace_back(getFreezeManager(), m_length0Attr, m_orientation0Attr,
		QLatin1String("length0"), QLatin1String("orientation0"), BezierHandle::Kind::Animatable);

	m_bezierHandles.emplace_back(getFreezeManager(), m_length1Attr, m_orientation1Attr,
		QLatin1String("length1"), QLatin1String("orientation1"), BezierHandle::Kind::Animatable);

	m_bezierHandles.emplace_back(getFreezeManager(), m_restLength0Attr, m_restingOrientation0Attr,
		QLatin1String("restlength0"), QLatin1String("restingOrientation0"), BezierHandle::Kind::Rest);

	m_bezierHandles.emplace_back(getFreezeManager(), m_restLength1Attr, m_restingOrientation1Attr,
		QLatin1String("restlength1"), QLatin1String("restingOrientation1"), BezierHandle::Kind::Rest);
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

	while (parent && (parent->keyword() == QLatin1String("CurveModule")
				|| parent->keyword() == QLatin1String("OffsetModule")))
	{
		AT_AttrList attributes;
		parent->getFullAttrList(attributes);
		//TODO: replace with findAttr

		for (const AT_AttrDesc& attribute : attributes)
		{
			if (attribute._pAttr->keyword() == QLatin1String("orientation1"))
			{
				//Curve module
				if (dynamic_cast<AT_DoubleAttr*>(attribute._pAttr)->getNextKey(0, &key))
					updateFrameRange(range, key);

				if (dynamic_cast<AT_DoubleAttr*>(attribute._pAttr)->getPrevKey(std::numeric_limits<int>::max(), &key))
					updateFrameRange(range, key);
			}

			if (attribute._pAttr->keyword() == QLatin1String("orientation"))
			{
				//Offset module
				if (dynamic_cast<AT_DoubleAttr*>(attribute._pAttr)->getNextKey(0, &key))
					updateFrameRange(range, key);

				if (dynamic_cast<AT_DoubleAttr*>(attribute._pAttr)->getPrevKey(std::numeric_limits<int>::max(), &key))
					updateFrameRange(range, key);
			}
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
			AttrBinding { "orientation1", &CurveModule::m_orientation1Attr },
			AttrBinding { "transversalRadius", &CurveModule::m_transversalRadiusAttr },
			AttrBinding { "transversalRadiusRight", &CurveModule::m_transversalRadiusRightAttr },
			AttrBinding { "longitudinalRadiusBegin", &CurveModule::m_longitudinalRadiusBeginAttr },
			AttrBinding { "longitudinalRadius", &CurveModule::m_longitudinalRadiusAttr },
			AttrBinding { "influenceFade", &CurveModule::m_influenceFadeRadiusAttr }
	);

	for (const AT_AttrDesc & desc : std::as_const(attributes))
	{
		const auto dispatch = [this, &desc](auto&& ...bindings)
		{
			const QString& keyword = desc._pAttr->keyword();

			const auto testAttr = [this, &keyword, &desc](const auto& binding)
			{
				//if (keyword != QLatin1StringView(binding.kw)) FIXME: QT 6.4 +
				if (keyword != QLatin1String(binding.kw))
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
	if (!m_transversalRadiusAttr)
		throw std::runtime_error("missing attribute: 'transversalRadius' for " + modulePtr->qualifiedName().toStdString());
	if (!m_transversalRadiusRightAttr)
		throw std::runtime_error("missing attribute: 'transversalRadiusRight' for " + modulePtr->qualifiedName().toStdString());
	if (!m_longitudinalRadiusBeginAttr)
		throw std::runtime_error("missing attribute: 'longitudinalRadiusBegin' for " + modulePtr->qualifiedName().toStdString());
	if (!m_longitudinalRadiusAttr)
		throw std::runtime_error("missing attribute: 'longitudinalRadius' for " + modulePtr->qualifiedName().toStdString());
	if (!m_influenceFadeRadiusAttr)
		throw std::runtime_error("missing attribute: 'influenceFade' for " + modulePtr->qualifiedName().toStdString());
}

void CurveModule::readjustSecondary()
{
	FreezeManager *fm = getFreezeManagerPtr();

	Math::Matrix4x4 changeMatrix = fm->getFreezeMatrix();

	//Defining complexity before adjusting changeMatrix as
	//altering the parent's rotation could impact transformation
	setComplexTransform(defineMatrixComplexity(changeMatrix, false));

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
	readjustRegionOfInfluence(*curMacro, changeMatrix);

	fm->addCommand(std::move(curMacro));
}

void CurveModule::readjustRegionOfInfluence(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix)
{
	Math::Point3d scale = getScale(matrix);
	double scaleFactor = (abs(scale.x()) + abs(scale.y())) / 2;

	double transversalRadius = m_transversalRadiusAttr->localValue() * scaleFactor;
	double transversalRadiusRight = m_transversalRadiusRightAttr->localValue() * scaleFactor;
	double longitudinalRadiusBegin = m_longitudinalRadiusBeginAttr->localValue() * scaleFactor;
	double longitudinalRadius = m_longitudinalRadiusAttr->localValue() * scaleFactor;
	double influenceFadeRadius = m_influenceFadeRadiusAttr->localValue() * scaleFactor;


	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_transversalRadiusAttr, transversalRadius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_transversalRadiusRightAttr, transversalRadiusRight));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_longitudinalRadiusBeginAttr, longitudinalRadiusBegin));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_longitudinalRadiusAttr, longitudinalRadius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_influenceFadeRadiusAttr, influenceFadeRadius));
	}
	else
	{
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("transversalradius"), transversalRadius},
			StaticAttrData{ QLatin1String("transversalradiusright"), transversalRadiusRight },
			StaticAttrData{ QLatin1String("longitudinalradius"), longitudinalRadius },
			StaticAttrData{ QLatin1String("longitudinalradiusbegin"), longitudinalRadiusBegin },
			StaticAttrData{ QLatin1String("influencefade"), influenceFadeRadius });
	}
}


void CurveModule::readjustBezierHandles(CO_OrCommand& curMacro, Math::Matrix4x4 matrix)
{
	for (auto& bezierHandle : m_bezierHandles)
	{
		if (!isIndependent())
			bezierHandle.recalculateStaticHandlePoint(getModulePtr(), adjustDependentMatrix(matrix, false, bezierHandle.isRest()), curMacro, isIndependent());
		else
			bezierHandle.recalculateStaticHandlePoint(getModulePtr(), matrix, curMacro, isIndependent());
	}

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = static_cast<double>(curFrame);

		for (auto& bezierHandle : m_bezierHandles)
		{
			if(!bezierHandle.isAnimatable())
				continue;

			BezKeyframeData kfData = generateKeyframeData(bezierHandle, frameNo, curFrame == range.start);

			if (kfData.length == KeyframeState::NoKeyframe && kfData.orientation == KeyframeState::NoKeyframe)
				continue;

			if (!isIndependent())
				bezierHandle.recalculateAnimatedHandlePoint(getModulePtr(), adjustDependentMatrix(matrix, true, frameNo),
					curMacro, kfData, frameNo, isIndependent());
			else
				bezierHandle.recalculateAnimatedHandlePoint(getModulePtr(), matrix, curMacro, kfData, frameNo, isIndependent());
		}
	}
}

BezKeyframeData CurveModule::generateKeyframeData(BezierHandle& bezierHandle, double frameNo, bool isFirst)
{
	BezKeyframeData kfData;
	//Check if there is a keyframe on the attributes

	if(isFirst && isComplexTransform())
	{
		kfData.length = KeyframeState::Keyframe;
		kfData.orientation = KeyframeState::Keyframe;

		return kfData;
	}

	bool isCtrlPntRadius = false;
	bool isCtrlPntAngle = false;

	bezierHandle.getLengthAttr()->value(frameNo, &isCtrlPntRadius);
	bezierHandle.getOrientationAttr()->value(frameNo, &isCtrlPntAngle);

	bool isAdjustKeyframe = isCtrlPntAngle || isCtrlPntRadius || (!isIndependent() && isParentKeyframe(frameNo)) 
		|| getFreezeManagerPtr()->isSetInbetweenKfMode();

	isAdjustKeyframe = isAdjustKeyframe && isComplexTransform();

	if(isCtrlPntRadius)
		kfData.length = KeyframeState::Keyframe;
	else if (isAdjustKeyframe)
		kfData.length = KeyframeState::PossibleKeyframe;
	else
		kfData.length = KeyframeState::NoKeyframe;
	

	if(isCtrlPntAngle)
		kfData.orientation = KeyframeState::Keyframe;
	else if (isAdjustKeyframe)
		kfData.orientation = KeyframeState::PossibleKeyframe;
	else
		kfData.orientation = KeyframeState::NoKeyframe;

	 return kfData;
}
