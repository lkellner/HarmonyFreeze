#include "transformationModule.h"

#include <BaseCore/maths/MT_Matrix4x4.h>
#include <SceneCore/attribute/AT_AttrCmds.h>
#include <SceneCore/attribute/AT_EnumAttrTemplate.h>
#include <SceneCore/attribute/AT_PositionAttrCmds.h>
#include <SceneCore/attribute/AT_Rotation3dAttrCmds.h>
#include <SceneCore/attribute/AT_Scale3dAttrCmds.h>
#include <SceneCore/attribute/AT_ScaleAttrCmds.h>
#include <SceneCore/selectable/SLB_CmdManipulator.h>
#include <SceneCore/selectable/SLB_ManipContext.h>
#include <Util/command/CO_OrCommand.h>
#include <Util/pluginmanager/PLUG_Services.h>

#include <algorithm>
#include <limits>
#include <type_traits>

template <typename T = AT_Rotation3dAttr>
bool isQuarternionKeyframe(const T& t, double frame, Math::Angle3d& angle)
{
	bool isCtrlPnt  = false;
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

TransformationModule::TransformationModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType, bool isFreeze)
	: ModuleBase(freezeManager, modulePtr, moduleType)
	, m_positionAttr(nullptr)
	, m_scaleAttr(nullptr)
	, m_rotationAttr(nullptr)
	, m_skewAttr(nullptr)
	, m_pivotAttr(nullptr)
	, m_is3D(false)
	, m_isFreeze(isFreeze)
{
	for (const AT_AttrDesc& desc : getAttributeList())
	{
		const QString& keyword = desc._pAttr->keyword();

		const auto loadAttr = [&desc](auto& attr)
		{
			using Attr = std::remove_cvref_t<decltype(attr)>;

			if constexpr (std::is_same_v<Attr, bool>) {
				auto* boolAttr = dynamic_cast<AT_BoolAttr*>(desc._pAttr);
				attr = boolAttr && boolAttr->localValue();
			} else {
				attr = dynamic_cast<Attr>(desc._pAttr);
			}
		};

		if (keyword == QLatin1String("POSITION") || keyword == QLatin1String("OFFSET"))
			loadAttr(m_positionAttr);
		else if (keyword == QLatin1String("SCALE"))
			loadAttr(m_scaleAttr);
		else if (keyword == QLatin1String("ROTATION"))
			loadAttr(m_rotationAttr);
		else if (keyword == QLatin1String("SKEW"))
			loadAttr(m_skewAttr);
		else if (keyword == QLatin1String("PIVOT"))
			loadAttr(m_pivotAttr);
		else if (keyword == QLatin1String("ENABLE_3D"))
			loadAttr(m_is3D);
	}

	if (!m_positionAttr)
		throw std::runtime_error("missing attribute: 'POSITION/OFFSET' for " + modulePtr->qualifiedName().toStdString());
	if (!m_scaleAttr)
		throw std::runtime_error("missing attribute: 'SCALE' for " + modulePtr->qualifiedName().toStdString());
	if (!m_rotationAttr)
		throw std::runtime_error("missing attribute: 'ROTATION' for " + modulePtr->qualifiedName().toStdString());
	if (!m_skewAttr)
		throw std::runtime_error("missing attribute: 'SKEW' for " + modulePtr->qualifiedName().toStdString());
	if (!m_pivotAttr)
		throw std::runtime_error("missing attribute: 'PIVOT' for " + modulePtr->qualifiedName().toStdString());

	initPivots();
}


void TransformationModule::initPivots()
{
	initDrawingPivotSources();

	if (m_isFreeze)
		saveWorldCombinedPivots();
}

FrameRange TransformationModule::getFrameRange() const
{
	FrameRange range;

	int key;


	if (m_positionAttr->useSeparate() && m_positionAttr->separateX()
		&& m_positionAttr->separateY() && m_positionAttr->separateZ())
	{
		if (m_positionAttr->separateX()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateX()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateY()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateY()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateZ()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateZ()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}
	else if(m_positionAttr->path3D())
	{
		if (m_positionAttr->path3D()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->path3D()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}


	//Scale attributes next

	if (m_scaleAttr->separate()->localValue() && m_scaleAttr->scaleXAttr()
		&& m_scaleAttr->scaleYAttr())
	{
		if (m_scaleAttr->scaleXAttr()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_scaleAttr->scaleXAttr()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);

		if (m_scaleAttr->scaleYAttr()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_scaleAttr->scaleYAttr()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);

		if (m_is3D && m_scaleAttr->scaleZAttr())
		{
			if (m_scaleAttr->scaleZAttr()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (m_scaleAttr->scaleZAttr()->getPrevKey(std::numeric_limits<int>::max(), &key))
				updateFrameRange(range, key);
		}
	}
	else if (m_scaleAttr->scaleXYAttr())
	{
		if (m_scaleAttr->scaleXYAttr()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_scaleAttr->scaleXYAttr()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}

	//Rotation

	if ((m_rotationAttr->useSeparate() || !m_is3D) && m_rotationAttr->separateZ())
	{
		if (m_is3D && m_rotationAttr->separateX() && m_rotationAttr->separateY())
		{
			if (m_rotationAttr->separateX()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (m_rotationAttr->separateX()->getPrevKey(std::numeric_limits<int>::max(), &key))
				updateFrameRange(range, key);

			if (m_rotationAttr->separateY()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (m_rotationAttr->separateY()->getPrevKey(std::numeric_limits<int>::max(), &key))
				updateFrameRange(range, key);
		}

		if (m_rotationAttr->separateZ()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_rotationAttr->separateZ()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}
	else if (m_rotationAttr->quaternionPath())
	{
		if (m_rotationAttr->quaternionPath()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_rotationAttr->quaternionPath()->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
	}


	//Skew

	if (m_skewAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_skewAttr->getPrevKey(std::numeric_limits<int>::max(), &key))
		updateFrameRange(range, key);


	return range;
}

Math::Matrix4x4 TransformationModule::getStaticLocalMatrixNoPivot()
{
	Math::Matrix4x4 matrix = Math::Matrix4x4();

	//POSITION
	Math::Point3d oglPos = Math::Point3d();

	//Need to get the local values directly from the double attributes instead of calling
	//m_positionAttr->getLocalValue(oglPos) as the values are incorrect whenever the 3dpath is selected

	oglPos.setX(m_positionAttr->separateX()->localValue());
	oglPos.setY(m_positionAttr->separateY()->localValue());
	oglPos.setZ(m_positionAttr->separateZ()->localValue());

	oglPos = getModulePtr()->sceneMetrics()->toOGL(oglPos);

	matrix.translate(oglPos.x(), oglPos.y(), oglPos.z());


	//ROTATION

	Math::Angle3d angle = Math::Angle3d();

	//Need to get the local values directly from the double attributes instead of calling
	//m_rotationAttr->getLocalValue(angle) as the values are incorrect whenever the quaternion is selected

	angle.setX(m_rotationAttr->separateX()->localValue());
	angle.setY(m_rotationAttr->separateY()->localValue());
	angle.setZ(m_rotationAttr->separateZ()->localValue());


	// Need to distinguish between the two here as "hidden" x and y rotations show up in 2d matrix
	if (m_is3D)
		matrix *= angle.toRotationMatrix();
	else
		matrix.rotateDegrees(angle.z(), Math::Vector3d(0, 0, 1));

	//SKEW
	matrix.skew(m_skewAttr->localValue());

	//Unlike rotation values, hidden scale z values don't show up
	//SCALE

	double scaleX = 1;
	double scaleY = 1;
	double scaleZ = 1;

	m_scaleAttr->getLocalValue(scaleX, scaleY, scaleZ);

	matrix.scale(scaleX, scaleY, scaleZ);

	matrix = getFreezeManagerPtr()->getOffsetMatrix().getInverse() * matrix;

	return matrix;
}


Math::Matrix4x4 TransformationModule::getLocalMatrixNoPivot(const double frameNo) const
{
	Math::Matrix4x4 matrix = Math::Matrix4x4();

	//POSITION

	Math::Point3d oglPos = Math::Point3d();

	m_positionAttr->getValue(frameNo, oglPos);
	oglPos = getModulePtr()->sceneMetrics()->toOGL(oglPos);

	matrix.translate(oglPos.x(), oglPos.y(), oglPos.z());


	//ROTATION

	Math::Angle3d angle = Math::Angle3d();

	m_rotationAttr->getValue(frameNo, angle);


	// Need to distinguish between the two here as "hidden" x and y rotations show up in 2d matrix
	if (m_is3D)
		matrix *= angle.toRotationMatrix();
	else
		matrix.rotateDegrees(angle.z(), Math::Vector3d(0, 0, 1));


	//SKEW
	matrix.skew(m_skewAttr->value(frameNo));


	//SCALE
	double scaleX = 1;
	double scaleY = 1;
	double scaleZ = 1;

	bool isKFX;
	bool isKFY;
	bool isKFZ;

	bool isConstX;
	bool isConstY;
	bool isConstZ;

	m_scaleAttr->getValue(frameNo, scaleX, scaleY, scaleZ, &isKFX, &isKFY, &isKFZ, &isConstX, &isConstY, &isConstZ);

	matrix.scale(scaleX, scaleY, scaleZ);
	matrix = getFreezeManagerPtr()->getOffsetMatrix().getInverse() * matrix;

	return matrix;
}


Math::Matrix4x4 TransformationModule::getLocalMatrix(const double frameNo) const
{
	const MO_FrameKey frameKey = MO_FrameKey(frameNo);

	//mainInPortId() should always be zero, the visual representation flips, when the port order is changed
	const Math::Matrix4x4 parent = getIncomingMatrix(getModulePtr()->mainInPortId(), frameNo, false);
	const Math::Matrix4x4 child = getModulePtr()->getModelMatrix(frameKey);

	return isScaleZero(parent)
		?  Math::Matrix4x4()
		: parent.getInverse() * child;
}


void TransformationModule::saveWorldCombinedPivots()
{
	Math::Point3d oglPivot;

	m_pivotAttr->getLocalValue(oglPivot);

	oglPivot = getModulePtr()->sceneMetrics()->toOGL(oglPivot);

	FrameRange range = getFrameRange();

	//Making sure the matrixFrame pivot gets saved as it will be needed for the rest matrix
	//even if there are no keyframes

	if (range.end == -1)
		range.start = range.end = getFreezeManagerPtr()->getSelFrame();

	for (int i = range.start; i <= range.end; i++)
	{
		m_combinedPivots[i] = oglPivot;
	}

	for (const auto& element : m_drawingPivotSources)
	{
		const Math::Matrix4x4 flipMatrix = getElementFlipMatrix(element);

		for (const auto& [frame, cel] : getExposures(element, range))
		{
			Math::Point3d pivot = Math::Point3d(getDrawingPivot(cel), 0);

			pivot = vectorToOgl(pivot, cel);

			pivot = flipMatrix * pivot;

			const auto it = m_combinedPivots.find(frame);

			if (it == m_combinedPivots.end())
				continue;

			auto& [_, p] = *it;

			p.setX(p.x() + pivot.x());
			p.setY(p.y() + pivot.y());
		}
	}
}


void TransformationModule::recalculateAllPivots()
{
	Math::Point3d oglPivot;

	m_pivotAttr->getLocalValue(oglPivot);

	oglPivot = getModulePtr()->sceneMetrics()->toOGL(oglPivot);

	FreezeManager* fm = getFreezeManagerPtr();

	Math::Matrix4x4 matrix = fm->getFreezeMatrix();

	if (m_isFreeze)
	{
		m_pivotOffset = getLocalMatrixNoPivot(fm->getSelFrame()).origin();

		for (auto& [_, pivot] : m_combinedPivots)
		{
			pivot = matrix * pivot;
			clampValues(pivot);
		}
	}
	else
	{
		m_pivotOffset = matrix * m_pivotOffset;
	}

	const int nDrawingSources = std::min<size_t>(m_drawingPivotSources.size(),
			std::numeric_limits<int>::max());

	Math::Matrix4x4 translationMatrix = Math::Matrix4x4();
	Math::Vector3d translateVec = matrix.origin().toVector() * (-nDrawingSources);

	translationMatrix = translationMatrix.translate(translateVec);
	Math::Matrix4x4 modifiedMatrix = translationMatrix * matrix;

	m_pivotAttrSurrogate = modifiedMatrix * oglPivot;

	if (!m_is3D)
		m_pivotAttrSurrogate.setZ(0);

	m_pivotAttrSurrogate = getModulePtr()->sceneMetrics()->fromOGL(m_pivotAttrSurrogate);
	clampValues(m_pivotAttrSurrogate);
}

void TransformationModule::setPivotJS() const
{
	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
									StaticAttrData{ QLatin1String("pivot.x"), m_pivotAttrSurrogate.x() },
									StaticAttrData{ QLatin1String("pivot.y"), m_pivotAttrSurrogate.y() },
									StaticAttrData{ QLatin1String("pivot.z"), m_pivotAttrSurrogate.z() });
}

void TransformationModule::setPivot(CO_OrCommand& curMacro)
{
	SC_SceneCommon* sceneCommon = getModulePtr()->sceneCommon();

	if (!sceneCommon)
		return;

	curMacro.add(Attr::Position3d::createSetLocalValueCmd(m_pivotAttr, m_pivotAttrSurrogate.x(), m_pivotAttrSurrogate.y(), m_pivotAttrSurrogate.z()));
}


void TransformationModule::readjustSecondary()
{
	//ReadjustSecondary doesn't use the "true" matrix of a transformation module
	//Instead it employs a matrix using a pseudo pivot located at 0,0,0 whose pos-transformation offset is saved in m_pivotOffset.
	//The advantage of this approach is that drawing pivots can be calculated independently and at a later stage.

	recalculateAllPivots();

	FreezeManager* fm = getFreezeManagerPtr();

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	const Math::Matrix4x4 freezeMatrix = fm->getFreezeMatrix();
	const Math::Matrix4x4 freezeMatrixInverse = freezeMatrix.getInverse();
	const Math::Matrix4x4 offsetMatrix = fm->getOffsetMatrix();

	m_freezeMatrixComplexity = defineMatrixComplexity(freezeMatrix, m_is3D);

	//Static matrix
	Math::Matrix4x4 newStaticLocalMatrix = freezeMatrix * getStaticLocalMatrixNoPivot() * freezeMatrixInverse;
	newStaticLocalMatrix = offsetMatrix * newStaticLocalMatrix;

	if (!isScaleZero(newStaticLocalMatrix)) //need to test otherwise parameters can't be extracted
		processStaticMatrix(newStaticLocalMatrix, *curMacro);

	//Keyframed matrices
	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		Math::Matrix4x4 newLocalMatrix = freezeMatrix * getLocalMatrixNoPivot(curFrame) * freezeMatrixInverse;
		newLocalMatrix = offsetMatrix * newLocalMatrix;

		if (!isScaleZero(newLocalMatrix))//need to test otherwise parameters can't be extracted //TODO: maybe can move this elsewhere?
			processMatrix(newLocalMatrix, *curMacro, curFrame, curFrame == range.start);
	}

	processPivotAttribute(*curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void TransformationModule::freeze()
{
	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	FreezeManager* fm = getFreezeManagerPtr();

	if (isScaleZero(getModulePtr()->getModelMatrix(MO_FrameKey(fm->getSelFrame()))))
		return;

	recalculateAllPivots();

	const Math::Matrix4x4 offsetMatrix = fm->getOffsetMatrix();
	const Math::Matrix4x4 offsetMatrixInverse = offsetMatrix.getInverse();

	Math::Matrix4x4 freezeMatrix = getLocalMatrix(fm->getSelFrame()).getInverse() * offsetMatrixInverse;

	m_freezeMatrixComplexity = defineMatrixComplexity(freezeMatrix, m_is3D);

	Math::Matrix4x4 newStaticLocalMatrix = getStaticLocalMatrixNoPivot()
		* getLocalMatrixNoPivot(fm->getSelFrame()).getInverse() * offsetMatrixInverse;

	newStaticLocalMatrix = offsetMatrix * newStaticLocalMatrix;

	//Static matrix
	if (!isScaleZero(newStaticLocalMatrix))
		processStaticMatrix(newStaticLocalMatrix, *curMacro);

	//Animated matrix
	//Matrices including pivot transformation will be used here to work,
	//in case there are drawing pivots set on the freeze matrix

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		m_pivotOffset = m_combinedPivots[curFrame];

		Math::Matrix4x4 newLocalMatrix = getLocalMatrix(curFrame) * freezeMatrix;
		newLocalMatrix = offsetMatrix * newLocalMatrix;

		if (!isScaleZero(newLocalMatrix))
			processMatrix(newLocalMatrix, *curMacro, curFrame, curFrame == range.start);
	}

	processPivotAttribute(*curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}

TransformationModule::KeyframeData TransformationModule::generateKeyframeData(const MatrixParameters& params, const double frameNo, const bool isFirst) const
{
	//Sometimes it's not only enough to check whether the current frame contains a keyframe,
	//As rotation, scale and skew values aren't always independent of each other.
	//In a similar way the separate position attributes interfere with each other in case of a rotation or a shear.
	//
	//The algorithm separates three categories of cases in which keyframes should be set:
	// *The attribute already contains a keyframe on this frame
	// *It is the first frame of the frame range  && the matrix transformation is complex (not just a simple translation or
	//    uniform scale. (This is done preemtatively, as there are cases where it only becomes clear later
	//    that a keyframe would have been needed)
	// *The value changes compared to the previous frame && the transformation is complex && (either another keyframe
	//    already exists for the module || the mode to set inbetween keyframes has been chosen
	//
	//This algorithm was chosen to find a equilibrium between creating as few differences as possible while
	// aiming to not make calculations considerable more complex and keeping the number of extra keyframes to a minimum.
	//The Curve/Offset Modules follow a similar logic.
	//There might be ways to narrow things down further

	KeyframeData kfData;

	kfData.isPosSeparate = m_positionAttr->useSeparate();
	kfData.isRotSeparate = m_rotationAttr->useSeparate();
	kfData.isScaleSeparate = m_scaleAttr->separate()->localValue();


	bool isPosXCtrlPnt = false;
	bool isPosYCtrlPnt = false;
	bool isPosZCtrlPnt = false;
	bool isPosCtrlPnt = false;
	bool isConstSeg = false;
	Math::Point3d tempPos;

	//POSITION
	if (m_positionAttr->useSeparate())
	{
		if (m_positionAttr->separateX())
			m_positionAttr->separateX()->value(frameNo, &isPosXCtrlPnt, &isConstSeg);

		if (m_positionAttr->separateY())
			m_positionAttr->separateY()->value(frameNo, &isPosYCtrlPnt, &isConstSeg);

		if (m_positionAttr->separateZ())
			m_positionAttr->separateZ()->value(frameNo, &isPosZCtrlPnt, &isConstSeg);
	}
	else
	{
		//TODO: might be possible to access sub attributes
		m_positionAttr->getValue(frameNo, tempPos, &isPosCtrlPnt, &isConstSeg);
	}

	//TODO: could return early here in case this is not the freeze peg
	const bool isForceKeyframePos = isFirst && m_freezeMatrixComplexity == MatrixComplexity::Complex;

	FreezeManager* fm = getFreezeManagerPtr();

	bool isAdjustKeyframePos = (isPosXCtrlPnt || isPosYCtrlPnt || isPosZCtrlPnt || fm->isSetInbetweenKfMode())
		&& m_freezeMatrixComplexity == MatrixComplexity::Complex;

	kfData.tx = isPosCtrlPnt || isPosXCtrlPnt || (m_prevFrameParams.tx != params.tx && isAdjustKeyframePos) || isForceKeyframePos;
	kfData.ty = isPosCtrlPnt || isPosYCtrlPnt || (m_prevFrameParams.ty != params.ty && isAdjustKeyframePos) || isForceKeyframePos;
	kfData.tz = isPosCtrlPnt || isPosZCtrlPnt || (m_prevFrameParams.tz != params.tz && isAdjustKeyframePos) || isForceKeyframePos;

	Math::Angle3d tempAngle = Math::Angle3d();

	bool isConstSegX = false;
	bool isConstSegY = false;
	bool isConstSegZ = false;

	bool isKeyframeX = false;
	bool isKeyframeY = false;
	bool isKeyframeZ = false;

	double tempScaleX;
	double tempScaleY;
	double tempScaleZ;


	//SCALE

	//Uniform scaling is automatically detected properly
	m_scaleAttr->getValue(frameNo, tempScaleX, tempScaleY, tempScaleZ, &isKeyframeX, &isKeyframeY, &isKeyframeZ,
		&isConstSegX, &isConstSegY, &isConstSegZ);

	//ROTATION

	bool isRotCtrlPnt = false;
	bool isRotXCtrlPnt = false;
	bool isRotYCtrlPnt = false;
	bool isRotZCtrlPnt = false;


	if (m_rotationAttr->useSeparate())
	{
		if (m_rotationAttr->separateX())
			m_rotationAttr->separateX()->value(frameNo, &isRotXCtrlPnt, &isConstSeg);

		if (m_rotationAttr->separateY())
			m_rotationAttr->separateY()->value(frameNo, &isRotYCtrlPnt, &isConstSeg);

		if (m_rotationAttr->separateZ())
			m_rotationAttr->separateZ()->value(frameNo, &isRotZCtrlPnt, &isConstSeg);
	}
	else
	{
		isRotCtrlPnt = isQuarternionKeyframe(*m_rotationAttr, frameNo, tempAngle);
	}

	//SKEW

	bool isSkewCtrlPnt = false;

	m_skewAttr->value(frameNo, &isSkewCtrlPnt, &isConstSeg);
	bool isForceKeyframe = isFirst && m_freezeMatrixComplexity != MatrixComplexity::Simple;

	bool isAdjustKeyframe = isKeyframeX || isKeyframeY || isKeyframeZ || isRotCtrlPnt
		|| isRotXCtrlPnt || isRotYCtrlPnt || isRotZCtrlPnt || isSkewCtrlPnt || fm->isSetInbetweenKfMode();

	isAdjustKeyframe = isAdjustKeyframe && m_freezeMatrixComplexity != MatrixComplexity::Simple;

	kfData.sx = isKeyframeX || (m_prevFrameParams.sx != params.sx && isAdjustKeyframe) || isForceKeyframe;
	kfData.sy = isKeyframeY || (m_prevFrameParams.sy != params.sy && isAdjustKeyframe) || isForceKeyframe;
	kfData.sz = isKeyframeZ || (m_prevFrameParams.sz != params.sz && isAdjustKeyframe) || isForceKeyframe;

	kfData.ax = isRotCtrlPnt || isRotXCtrlPnt || (m_prevFrameParams.ax != params.ax && isAdjustKeyframe) || isForceKeyframe;
	kfData.ay = isRotCtrlPnt || isRotYCtrlPnt || (m_prevFrameParams.ay != params.ay && isAdjustKeyframe) || isForceKeyframe;
	kfData.az = isRotCtrlPnt || isRotZCtrlPnt || (m_prevFrameParams.az != params.az && isAdjustKeyframe) || isForceKeyframe;

	kfData.skew = isSkewCtrlPnt || (m_prevFrameParams.skewx != params.skewx && isAdjustKeyframe) || isForceKeyframe;

	return kfData;
}


MatrixParameters TransformationModule::convertMatrixToParams(const Math::Matrix4x4 matrix, const Math::Angle3d angle) const
{
	Math::Point3d oglPivot = m_pivotOffset;

	MatrixParameters params = matrixParameters(matrix, oglPivot, angle);

	convertFromOGL(params, getModulePtr()->sceneMetrics());
	clampValues(params);

	if (m_is3D)
	{
		params.sxy = (abs(params.sx) + abs(params.sy) + abs(params.sz)) / 3;
	}
	else
	{
		params.ax = 0;
		params.ay = 0;
		params.sz = 1;
		params.sxy = (abs(params.sx) + abs(params.sy)) / 2;

		if (params.sx < 0 && params.sy < 0)
			params.sxy *= -1;
	}

	return params;
}


Math::Angle3d TransformationModule::getFreezeAngle(bool isStatic, const double frameNo) const
{
	Math::Angle3d angle;

	if (isStatic)
		m_rotationAttr->getLocalValue(angle);
	else
		m_rotationAttr->getValue(frameNo, angle);

	if (m_isFreeze)
	{
		Math::Angle3d freezeAngle;
		m_rotationAttr->getValue(getFreezeManagerPtr()->getSelFrame(), freezeAngle);
		angle -= freezeAngle;
	}

	return angle;
}


void TransformationModule::processMatrix(const Math::Matrix4x4& matrix, CO_OrCommand& curMacro, const double frameNo,const bool isFirst)
{
	MatrixParameters params = convertMatrixToParams(matrix, getFreezeAngle(false, frameNo));

	KeyframeData kfData = generateKeyframeData(params, frameNo, isFirst);
	//updating m_prevFrameParams to be used for the following frameNo
	//needs to happen after call to generateKeyframeData;
	m_prevFrameParams = params;

	if (getFreezeManagerPtr()->isExperimentalMode())
		setAttributes(params, kfData, curMacro, frameNo);
	else
		setAttributesJS(params, kfData, frameNo);
}

void TransformationModule::setAttributesJS(const MatrixParameters & params, const KeyframeData & kfData, const double frameNo) const
{
	//POSITION

	if (kfData.tx || kfData.ty || kfData.tz)
		setPositionJS(kfData, params, frameNo);


	//Need to set euler anglez if peg is in 2d mode, even if the official mode is quarternion

	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
		AttrData{ QLatin1String("scale.x"), params.sx, frameNo, kfData.sx && kfData.isScaleSeparate },
		AttrData{ QLatin1String("scale.y"), params.sy, frameNo, kfData.sy && kfData.isScaleSeparate },
		AttrData{ QLatin1String("scale.z"), params.sz, frameNo, kfData.sz && kfData.isScaleSeparate && m_is3D},
		AttrData{ QLatin1String("scale.xy"), params.sxy, frameNo, (kfData.sx || kfData.sy || kfData.sz) && !kfData.isScaleSeparate},
		AttrData{ QLatin1String("rotation.anglex"), params.ax, frameNo, kfData.ax && kfData.isRotSeparate && m_is3D },
		AttrData{ QLatin1String("rotation.angley"), params.ay, frameNo, kfData.ay && kfData.isRotSeparate && m_is3D },
		AttrData{ QLatin1String("rotation.anglez"), params.az, frameNo, kfData.az && (kfData.isRotSeparate || !m_is3D)},
		QuaternionAttrData{ QLatin1String("rotation.quaternionpath"), Math::Angle3d(params.ax, params.ay, params.az),
							frameNo, (kfData.ax || kfData.ay || kfData.az)  && !kfData.isRotSeparate && m_is3D},
		AttrData{ QLatin1String("skew"), params.skewx, frameNo, kfData.skew });
}

void TransformationModule::setAttributes(const MatrixParameters& params, const KeyframeData& kfData, CO_OrCommand& curMacro, const double frameNo)
{
	SC_SceneCommon* sceneCommon = getModulePtr()->sceneCommon();

	if (!sceneCommon)
		return;

	//POSITION
	//No need to split these up, automatically will only set a keyframe where it is required
	if (kfData.tx || kfData.ty || kfData.tz)
		curMacro.add(Attr::Position3d::createSetValueCmd(m_positionAttr, frameNo, params.tx, params.ty, params.tz));


	//SCALE
	//TODO: might split these up
	if (kfData.sx || kfData.sy || kfData.sz)
	{
		if (kfData.isScaleSeparate)
			curMacro.add(Attr::Scale3d::createSetValueCurrentFrameCmd(sceneCommon, m_scaleAttr, frameNo, params.sx, params.sy, params.sz, false));
		else
			curMacro.add(Attr::Scale3d::createSetValueCurrentFrameCmd(sceneCommon, m_scaleAttr, frameNo, params.sxy, params.sxy, params.sxy, false));
	}


	//ROTATION
	//TODO: might split these up
	if (!m_is3D)
	{
		//Specifically setting the z rotation for a 2d peg to avoid additional quarternion keyframe that happens
		//when quarternion is enabled in the background
		if (kfData.az)
			curMacro.add(Attr::Double::createSetValueCmd(m_rotationAttr->separateZ(), frameNo, params.az));
	}
	else
	{
		if (kfData.ax || kfData.ay || kfData.az)
			curMacro.add(Attr::Rotation3d::createSetValueCurrentFrameCmd(sceneCommon, m_rotationAttr, frameNo,
				Math::Angle3d(params.ax, params.ay, params.az), false));
	}

	//SKEW

	if (kfData.skew)
		curMacro.add(Attr::Double::createSetValueCmd(m_skewAttr, frameNo, params.skewx));
}


void TransformationModule::processStaticMatrix(const Math::Matrix4x4& matrix, CO_OrCommand& curMacro)
{
	MatrixParameters params = convertMatrixToParams(matrix, getFreezeAngle(true));

	//Need to store these as well, to know whether the first frame needs a keyframe
	m_prevFrameParams = params;

	if (getFreezeManagerPtr()->isExperimentalMode())
		setStaticAttributes(params, curMacro);
	else
		setStaticAttributesJS(params);
}


void TransformationModule::setStaticAttributesJS(const MatrixParameters& params) const
{
	setStaticPositionJS(params);

	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
		StaticAttrData{ QLatin1String("rotation.anglex"), params.ax },
		StaticAttrData{ QLatin1String("rotation.angley"), params.ay },
		StaticAttrData{ QLatin1String("rotation.anglez"), params.az },
		StaticAttrData{ QLatin1String("scale.x"), params.sx },
		StaticAttrData{ QLatin1String("scale.y"), params.sy },
		StaticAttrData{ QLatin1String("scale.z"), params.sz },
		StaticAttrData{ QLatin1String("scale.xy"), params.sxy },
		StaticAttrData{ QLatin1String("skew"), params.skewx });
}

void TransformationModule::setStaticAttributes(const MatrixParameters& params, CO_OrCommand& curMacro)
{
	Math::Angle3d tempAngle = Math::Angle3d();

	curMacro.add(Attr::Position3d::createSetLocalValueCmd(m_positionAttr, params.tx, params.ty, params.tz));

	curMacro.add(Attr::Scale3d::createSetLocalValueCmd(m_scaleAttr, params.sx, params.sy, params.sz));

	curMacro.add(Attr::Rotation3d::createSetLocalValueCmd(m_rotationAttr, Math::Angle3d(params.ax, params.ay, params.az)));

	curMacro.add(Attr::Double::createSetLocalValueCmd(m_skewAttr, params.skewx));
}


void TransformationModule::initDrawingPivotSources()
{
	switch (getModuleType())
	{
	case ModuleType::DRAWING_TRANSFORMATION:
		initElementDrawingPivotSources();
		break;
	case ModuleType::PEG:
		initPegDrawingPivotSources();
		break;
	default:
		break;
	}
}

void TransformationModule::initElementDrawingPivotSources()
{
	AT_Attr* attr = findAttribute<AT_Attr>(QLatin1String("USE_DRAWING_PIVOT"));
	if (attr && attr->textValue(1) == QLatin1String("Apply Embedded Pivot on Drawing Layer"))
		m_drawingPivotSources.push_back(getModulePtr());
}

void TransformationModule::initPegDrawingPivotSources()
{
	std::vector<MO_Port*> outPorts = getModulePtr()->getOutPorts();

	for (auto& port : outPorts)
	{
		if (!port->isMatrixPort())
			continue;

		std::vector<MO_Port*> dstPorts;
		port->realDstPorts(dstPorts);

		for (auto& dstPort : dstPorts)
		{
			if (!dstPort->node() || !dstPort->node()->toModule()
				|| dstPort->node()->keyword() != QLatin1String("READ"))
				continue;

			if (isDrawingPivotAppliedOnPeg(dstPort->node()->toModule()))
				m_drawingPivotSources.push_back(dstPort->node()->toModule());
		}
	}
}
