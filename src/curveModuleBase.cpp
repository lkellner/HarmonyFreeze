#include "curveModuleBase.h"

using namespace Qt::StringLiterals;

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
				StaticAttrData{ u"restingoffset.x"_s, restingOffset.x() },
				StaticAttrData{ u"restingoffset.y"_s, restingOffset.y() },
				StaticAttrData{ u"offset.x"_s, staticOffset.x() },
				StaticAttrData{ u"offset.y"_s, staticOffset.y() });
	}

	//Animated values

	const FrameRange range = getFrameRange();

	bool isCtrlPnt2D = false;
	bool isCtrlPntX = false;
	bool isCtrlPntY = false;	
	bool isConstSeg2D = false;
	bool isConstSegX = false;
	bool isConstSegY = false;

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = curFrame;
		Math::Point2d offset;

		m_offsetAttr->getValue(frameNo, offset, &isCtrlPnt2D, &isConstSeg2D);
		m_offsetAttr->separateX()->value(frameNo, &isCtrlPntX, &isConstSegX);
		m_offsetAttr->separateY()->value(frameNo, &isCtrlPntY, &isConstSegY);

		//Set keyframe either way. Forcing is necessary as curves' values also can 
		//also depend on their parents' rotation 
		const bool isForceKeyframe = curFrame == range.start && isComplexTransform();

		//Set keyframe in case it different from the previous value
		bool isAdjustKeyframe = (!isIndependent() && isParentKeyframe(frameNo)) || fm->isSetInbetweenKfMode();
		isAdjustKeyframe = isAdjustKeyframe && isComplexTransform();

		if (!isCtrlPnt2D && !isCtrlPntX && !isCtrlPntY && !isAdjustKeyframe && !isForceKeyframe)
			continue;

		if (!isIndependent())
			offset *= adjustDependentMatrix(changeMatrix, true, false, frameNo).getTransform2d();
		else
			recalculateIndependentPoint(getModulePtr(), changeMatrix, offset);


		isCtrlPnt2D = isCtrlPnt2D || isForceKeyframe || (isAdjustKeyframe && offset != m_prevOffset);
		isCtrlPntX = isCtrlPntX || isForceKeyframe || ((isAdjustKeyframe || isCtrlPntY) && offset.x() != m_prevOffset.x());
		isCtrlPntY = isCtrlPntY || isForceKeyframe || ((isAdjustKeyframe || isCtrlPntX) && offset.y() != m_prevOffset.y());


		//Setting storing offset for the next keyframe;

		m_prevOffset = offset;

		if (fm->isExperimentalMode())
		{
			//C++
			//Attributes are being split to be able to set the proper isConstSeg value
			if (m_offsetAttr->useSeparate())
			{
				if(isCtrlPntX)
					curMacro.add(Attr::Double::createSetValueCmd(m_offsetAttr->separateX(), frameNo, offset.x(), isConstSegX, true));
				if (isCtrlPntY)
					curMacro.add(Attr::Double::createSetValueCmd(m_offsetAttr->separateY(), frameNo, offset.y(), isConstSegY, true));
			}
			else
			{
				if (isCtrlPnt2D)
					curMacro.add(Attr::Position2d::createSetValueCurrentFrameCmd(getModulePtr()->sceneCommon(), m_offsetAttr, frameNo, 
						offset.x(), offset.y(), { isConstSeg2D, true, false, false}));
			}
		}
		else
		{
			//JS
			if (m_offsetAttr->useSeparate())
			{
				fm->applyAttributes(getModulePtr()->qualifiedName(),
					AttrData{ u"offset.x"_s, offset.x(), frameNo, isCtrlPntX },
					AttrData{ u"offset.y"_s, offset.y(), frameNo, isCtrlPntY });
			}
			else
			{
				fm->applyAttributes(getModulePtr()->qualifiedName(),
					Point2dAttrData{ u"offset"_s, Math::Point2d(offset.x(),offset.y()) , frameNo, isCtrlPnt2D });
			}
		}
	}
}

Math::Matrix4x4 CurveModuleBase::adjustDependentMatrix(const Math::Matrix4x4 matrix,
		const bool isAnimated,
		const bool isRest,
		const double frameNo) const
{
	const Math::Matrix4x4 parentMatrix = isAnimated
		?  Math::Matrix4x4().rotateDegrees(getChainRotation(frameNo))
		: Math::Matrix4x4().rotateDegrees(getStaticChainRotation(isRest));

	const Math::Matrix4x4 newParentMatrix = get2dRotationMatrix((matrix * parentMatrix).getTransform2d());

	const FreezeManager* fm = getFreezeManagerPtr();

	return fm->getUnitOffsetScaleMatrix() * newParentMatrix.getInverse() *  matrix  * parentMatrix * fm->getUnitOffsetScaleMatrix().getInverse();
}

double CurveModuleBase::getStaticChainRotation(bool isRest) const
{
	const auto curveAttrKeyword = isRest
		? "restingOrientation1"_L1
		: "orientation1"_L1;

	const auto offsetAttrKeyword = isRest
		? "restingOrientation"_L1
		: "orientation"_L1;

	return getRotationImpl(curveAttrKeyword, offsetAttrKeyword,
			[](const AT_DoubleAttr& a) { return a.localValue(); });
}

double CurveModuleBase::getChainRotation(const double frameNo) const
{
	return getRotationImpl("orientation1"_L1, "orientation"_L1,
			[frameNo](const AT_DoubleAttr& a) { return a.value(frameNo); });
}

void CurveModuleBase::initAttributes()
{
	for (const AT_AttrDesc& a : getAttributeList())
	{
		const QString& kw = attrKeyword(a);

		if (kw == "offset"_L1)
			m_offsetAttr = dynamic_cast<AT_Position2dAttr*>(a._pAttr);
		else if (kw == "restingOffset"_L1)
			m_restingOffsetAttr = dynamic_cast<AT_Position2dAttr*>(a._pAttr);
		else if (kw == "localReferential"_L1 && getModuleType() != ModuleType::CURVE_OFFSET)
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
double CurveModuleBase::getRotationImpl(const QLatin1StringView curveAttrKeyword,
		const QLatin1StringView offsetAttrKeyword,
		ValueFunc&& valFunc) const
{
	FreezeManager* fm = getFreezeManagerPtr();

	const MO_Module *mod = getModulePtr();

	const MO_Node* parent = mod->getParentNode();

	double rotation = 0.0;

	while (parent && (parent->keyword() == "CurveModule"_L1
				|| parent->keyword() == "OffsetModule"_L1))
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
					rotation += convertRotation(fm->getUnitOffsetScaleMatrix(), valFunc(*a));
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

bool CurveModuleBase::isParentKeyframe(const double frameNo) const
{
	bool isKeyframe = false;

	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && (parent->keyword() == "CurveModule"_L1
		|| parent->keyword() == "OffsetModule"_L1))
	{
		AT_DoubleAttr* attr;

		if(parent->keyword() == "CurveModule"_L1)
			attr = ::findAttribute<AT_DoubleAttr>("orientation1"_L1, parent);

		if (parent->keyword() == "OffsetModule"_L1)
			attr = ::findAttribute<AT_DoubleAttr>("orientation"_L1, parent);


		if(attr)
			attr->value(frameNo, &isKeyframe);

		if (isKeyframe)
				return true;

		parent = parent->getParentNode();
	}

	return false;
}

Math::Matrix4x4 recalculateIndependentHandleMatrix(const SC_SceneMetrics* sceneMetrics,
	Math::Matrix4x4 handleMatrix, const Math::Matrix4x4& changeMatrix)
{
	handleMatrix = fieldsToOgl(sceneMetrics, handleMatrix);
	handleMatrix = changeMatrix.rotation() * handleMatrix;
	return oglToFields(sceneMetrics, handleMatrix);
}
