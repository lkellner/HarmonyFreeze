#include "pointModuleBase.h"

#include <SceneCore/attribute/AT_AttrCmds.h>

using namespace Qt::StringLiterals;

PointModuleBase::PointModuleBase(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
}

void PointModuleBase::readjustSecondary()
{
	setMatrixComplexity();

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	for (const auto& point : m_transformPoints)
		processPoint(point.get(), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}

void PointModuleBase::processPoint(const TransformPoint* point, CO_OrCommand& curMacro)
{
	Math::Matrix4x4 changeMatrix = oglToFieldsModificationMatrix(getModulePtr()->sceneMetrics(),
		getFreezeManagerPtr()->getFreezeMatrix());

	Math::Point2d position;
	point->posAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	setStaticAttributes(point, pos3d,  curMacro);

	if(!point->isAnimated)
		return;

	const FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		KeyframeData kfData = generateKeyframeData(point, curFrame, curFrame == range.start);

		if (!kfData.isProcessFrame)
			continue;

		point->posAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(point, pos3d, kfData, curMacro, curFrame);

		//Within the current algorithm, there is no need to initialize m_prevBoneData with static values first, 
		//as, for modules containing keyframes, the first Keyframe would be set anyway for complexMatrices
		m_prevPos = pos3d;
	}
}

PointModuleBase::KeyframeData PointModuleBase::generateKeyframeData(const TransformPoint* point,
	const double frameNo, const bool isFirst) const
{
	KeyframeData kfData;
	kfData.isSeparate = point->posAttr->useSeparate();

	Math::Point2d tempPos;

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account
	if (kfData.isSeparate)
	{
		point->posAttr->separateX()->value(frameNo, &kfData.posX, &kfData.isConstSegX);
		point->posAttr->separateY()->value(frameNo, &kfData.posY, &kfData.isConstSegY);
	}
	else
	{
		getPos2DValues(*(point->posAttr), frameNo, tempPos, kfData.pos2D, kfData.isConstSeg2D);
	}

	kfData.isForceKeyframe = isFirst && isComplexTransform();
	kfData.isAdjustKeyframe = getFreezeManagerPtr()->isSetInbetweenKfMode() && isComplexTransform();

	kfData.isAdjustKeyframe = kfData.isAdjustKeyframe && isComplexTransform();

	kfData.isProcessFrame = kfData.isForceKeyframe || kfData.isAdjustKeyframe || kfData.pos2D || kfData.posX || kfData.posY;
	/*Keyframes are being set if
	*- there already is a keyframe
	*-it's the first frame and the freeze is complex
	*-the freeze is complex and the transformation type is not simple and the value doesn't match the previous one
	*-the attribute is set to separate, and the partner attribute has a keyframe and the value doesn't match the previous one
	*/

	return kfData;
}

void PointModuleBase::updateKeyframeData(KeyframeData& kfData, const Math::Point3d position) const
{
	kfData.pos2D = kfData.pos2D || kfData.isForceKeyframe || (kfData.isAdjustKeyframe && position != m_prevPos);
	kfData.posX = kfData.posX || kfData.isForceKeyframe || ((kfData.isAdjustKeyframe || kfData.posY) && position.x() != m_prevPos.x());
	kfData.posY = kfData.posY || kfData.isForceKeyframe || ((kfData.isAdjustKeyframe || kfData.posX) && position.y() != m_prevPos.y());
}

void PointModuleBase::setStaticAttributes(const TransformPoint* point, Math::Point3d position,
	CO_OrCommand& curMacro) const
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->posAttr, position.x(), position.y()));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ point->name + u".x"_s, position.x() },
			StaticAttrData{ point->name + u".y"_s, position.y() });
	}
}

void PointModuleBase::setAttributes(const TransformPoint* point, Math::Point3d position,
	KeyframeData& kfData, CO_OrCommand& curMacro, const double frameNo) const
{
	updateKeyframeData(kfData, position);

	if (!kfData.posX && !kfData.posY && !kfData.pos2D)
		return;

	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();


	if (fm->isExperimentalMode())
	{
		//C++
		if (kfData.isSeparate)
		{
			if (kfData.posX)
				curMacro.add(Attr::Double::createSetValueCmd(point->posAttr->separateX(), frameNo, position.x(), kfData.isConstSegX, true));
			if (kfData.posY)
				curMacro.add(Attr::Double::createSetValueCmd(point->posAttr->separateY(), frameNo, position.y(), kfData.isConstSegY, true));
		}
		else
		{
			if (kfData.pos2D)
				createSetValueCurrentFrameCmdPos2D(getModulePtr(), curMacro, point->posAttr, frameNo, Math::Point2d(position.x(), position.y()),
					kfData.isConstSeg2D, true, false, false);
		}
	}
	else
	{
		//JS

		if (kfData.isSeparate)
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ point->name + u".x"_s, position.x(), frameNo, kfData.posX },
				AttrData{ point->name + u".y"_s, position.y(), frameNo, kfData.posY });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{ point->name, Math::Point2d(position.x(),position.y()) , frameNo, kfData.pos2D });
		}
	}
}

FrameRange PointModuleBase::getFrameRange() const
{
	FrameRange range;

	int key;

	//Main attribute only detects point2d keyframes
	for (const auto& point : m_transformPoints)
	{
		//Resting position, etc can't have any keyframes
		if(!point->isAnimated)
			continue;

		if (point->posAttr)
		{
			if (point->posAttr->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);

			if (point->posAttr->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateX()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateX()->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateY()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateY()->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);
		}
	}

	if (!supportsKeysRange())
	{
		//This is a workaround for H24 and H25 when Path2D keyframes aren't properly detected
		//As the freezeRange might not equal the module's own range, it is still possible that some
		//keyframes go undetected
		const FrameRange freezeRange = getFreezeManagerPtr()->getFrameRange();
		updateFrameRange(range, freezeRange.start);
		updateFrameRange(range, freezeRange.end);
	}

	return range;
}

void PointModuleBase::addTransformPoint(std::unique_ptr<TransformPoint> point)
{
	m_transformPoints.push_back(std::move(point));
}