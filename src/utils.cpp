#include "utils.h"
#include <GraphicCore/GraphicLib/GR_DrawingAccess.h>
#include <GraphicCore/GraphicLib/GR_ColorDict.h>
#include <SceneCore/attribute/AT_BoolAttr.h>

//Caution:  Math::Matrix4x4 uses the inverse matrix multiplication order of Math::Matrix3x2


double matchFullRotations(const double referenceAngle, const double angle)
{
	double difference = std::fmod(referenceAngle - angle, 360);

	if (difference > 180)
		difference = -(360 - difference);

	if (difference < -180)
		difference = 360 + difference;

	return referenceAngle - difference;
}

//There already is a Math::Matrix4x4::isIdentity() function, however it doesn't seem to account for
//any floating point errors
bool isIdentity(Math::Matrix4x4 matrix)
{
	constexpr double threshold = 0.0001;

	return	std::abs(matrix(Math::Row::Row_0, Math::Column::Column_0) - 1) < threshold &&
		std::abs(matrix(Math::Row::Row_0, Math::Column::Column_1)) < threshold &&
		std::abs(matrix(Math::Row::Row_0, Math::Column::Column_2)) < threshold &&
		std::abs(matrix(Math::Row::Row_0, Math::Column::Column_3)) < threshold &&
		std::abs(matrix(Math::Row::Row_1, Math::Column::Column_0)) < threshold &&
		std::abs(matrix(Math::Row::Row_1, Math::Column::Column_1) - 1) < threshold &&
		std::abs(matrix(Math::Row::Row_1, Math::Column::Column_2)) < threshold &&
		std::abs(matrix(Math::Row::Row_1, Math::Column::Column_3)) < threshold &&
		std::abs(matrix(Math::Row::Row_2, Math::Column::Column_0)) < threshold &&
		std::abs(matrix(Math::Row::Row_2, Math::Column::Column_1)) < threshold &&
		std::abs(matrix(Math::Row::Row_2, Math::Column::Column_2) - 1) < threshold &&
		std::abs(matrix(Math::Row::Row_2, Math::Column::Column_3)) < threshold &&
		std::abs(matrix(Math::Row::Row_3, Math::Column::Column_0)) < threshold &&
		std::abs(matrix(Math::Row::Row_3, Math::Column::Column_1)) < threshold &&
		std::abs(matrix(Math::Row::Row_3, Math::Column::Column_2)) < threshold &&
		std::abs(matrix(Math::Row::Row_3, Math::Column::Column_3) - 1) < threshold;
}

MatrixComplexity defineMatrixComplexity(const Math::Matrix4x4& matrix, bool is3D)
{
	//matrix only contains position values
	if (isIdentity(matrix.rotation()))
		return MatrixComplexity::Simple;

	//matrix contains rot/skew values
	if (!isIdentity(matrix.rotationNormalized()))
		return MatrixComplexity::Complex;

	Math::Point3d scale = getScale(matrix);

	constexpr double threshold = 0.0001;

	if (std::abs(scale.x() - scale.y()) < threshold && std::abs(scale.y() - scale.z()) < threshold)
		return MatrixComplexity::Simple;

	if (!is3D && std::abs(scale.x() - scale.y()) < threshold)
		return MatrixComplexity::Simple;

	return MatrixComplexity::ScaleTranslationOnly;
}

//Created new function as Math::Matrix4x4::RotationNormalized() includes skew values
Math::Matrix4x4 get2dRotationMatrix(Math::Matrix2x3 matrix)
{
	return Math::Matrix4x4().rotateDegrees(getAngle2d(matrix));
}

Math::Matrix4x4 convertToProjectionMatrix(Math::Matrix4x4 matrix)
{
	Math::Matrix4x4 projectionMatrix = Math::Matrix4x4();

	projectionMatrix = Math::Matrix4x4().scale(1, 1, 0);
	projectionMatrix = projectionMatrix * matrix;
	projectionMatrix = Math::Matrix4x4(projectionMatrix.getTransform2d());

	return projectionMatrix;
}

Math::Point3d getScale(const Math::Matrix4x4 & matrix)
{
	//Math::Matrix4x4::rotation() includes rotation, scale and shear,
	//Math::Matrix4x4::rotationNormalized() includes rotation and shear
	Math::Matrix4x4 scaleMatrix = matrix.rotationNormalized().getInverse() * matrix.rotation();

	const double xScale = scaleMatrix(Math::Row::Row_0, Math::Column::Column_0);
	const double yScale = scaleMatrix(Math::Row::Row_1, Math::Column::Column_1);
	const double zScale = scaleMatrix(Math::Row::Row_2, Math::Column::Column_2);

	return Math::Point3d(xScale, yScale, zScale);
}

double getAffineDeterminant(Math::Matrix4x4 matrix)
{
	//Simplified version assuming the last row is 0,0,0,1
	//There is a Math::Matrix4x4.determinant() function, however it doesn't exist in H24 and below
	double add01 = matrix(Math::Row::Row_0, Math::Column::Column_0) *
				matrix(Math::Row::Row_1, Math::Column::Column_1) *
				matrix(Math::Row::Row_2, Math::Column::Column_2);

	double add02 = matrix(Math::Row::Row_0, Math::Column::Column_1) *
				matrix(Math::Row::Row_1, Math::Column::Column_2) *
				matrix(Math::Row::Row_2, Math::Column::Column_0);

	double add03 = matrix(Math::Row::Row_0, Math::Column::Column_2) *
				matrix(Math::Row::Row_1, Math::Column::Column_0) *
				matrix(Math::Row::Row_2, Math::Column::Column_1);

	double sub01 = matrix(Math::Row::Row_0, Math::Column::Column_2) *
				matrix(Math::Row::Row_1, Math::Column::Column_1) *
				matrix(Math::Row::Row_2, Math::Column::Column_0);

	double sub02 = matrix(Math::Row::Row_0, Math::Column::Column_1) *
				matrix(Math::Row::Row_1, Math::Column::Column_0) *
				matrix(Math::Row::Row_2, Math::Column::Column_2);

	double sub03 = matrix(Math::Row::Row_0, Math::Column::Column_0) *
				matrix(Math::Row::Row_1, Math::Column::Column_2) *
				matrix(Math::Row::Row_2, Math::Column::Column_1);

	return add01 + add02 + add03 - sub01 - sub02 - sub03;
}

//There is Math::Matrix4x4::isTransform2d() function, however it
//seems to classify some matrices with scale z != 1 as 2d matrices
bool isTransform2d(const Math::Matrix4x4& matrix)
{
	return matrix(Math::Row::Row_0, Math::Column::Column_2) == 0 &&
		matrix(Math::Row::Row_1, Math::Column::Column_2) == 0 &&
		matrix(Math::Row::Row_2, Math::Column::Column_0) == 0 &&
		matrix(Math::Row::Row_2, Math::Column::Column_1) == 0 &&
		matrix(Math::Row::Row_2, Math::Column::Column_2) == 1;
}

double toRadians(double val)
{
	return val * M_PI / 180;
}

double fromRadians(double val)
{
	return val * 180 / M_PI;
}

void updateFrameRange(FrameRange& range, int frame)
{
	if (frame < 1)
		return;

	if (frame < range.start)
		range.start = frame;

	if (frame > range.end)
		range.end = frame;

	return;
}

Math::Matrix4x4 getElementFlipMatrix(MO_Module * modulePtr)
{
	if (!modulePtr|| modulePtr->keyword() != QLatin1String("READ"))
		return {};

	AT_BoolAttr* flipHAttr = findAttribute<AT_BoolAttr>(QLatin1String("FLIP_HOR"), modulePtr);
	AT_BoolAttr* flipVAttr = findAttribute<AT_BoolAttr>(QLatin1String("FLIP_VERT"), modulePtr);

	if (!flipHAttr || !flipVAttr)
		return {};

	Math::Matrix4x4 flipMatrix = Math::Matrix4x4();

	if (flipHAttr->localValue())
		flipMatrix.scale(-1, 1, 1);

	if (flipVAttr->localValue())
		flipMatrix.scale(1, -1, 1);

	return flipMatrix;
}

bool isScaleZero(const Math::Matrix4x4& matrix)
{
	MatrixParameters p;
	matrix.extractParameters(0, 0, 0, p.tx, p.ty, p.tz, p.sx, p.sy, p.sz, p.ax, p.ay, p.az);

	if (getAffineDeterminant(matrix) == 0)
	{
		//scale zero detected in parameters
		matrix.print("Determinant is zero\n");
		return true;
	}
	else
	{
		try
		{
			//Making an additional check as there have been matrices that couldn't be inverted nonetheless (floating point error?)
			matrix.getInverse();
			return false;
		}
		catch (...)
		{
			matrix.print("Couldn't invert matrix\n");
			return true;
		}
	}

	return true;
}

void convertFromOGL(MatrixParameters &params, SC_SceneMetrics* sceneMetrics)
{
	params.tx = sceneMetrics->fromOGLX(params.tx);
	params.ty = sceneMetrics->fromOGLY(params.ty);
	params.tz = sceneMetrics->fromOGLZ(params.tz);
}

void clampValues(Math::Point3d& point)
{
	if (point.x() > -0.00001 && point.x() < 0.00001)
		point.setX(0);

	if (point.y() > -0.00001 && point.y() < 0.00001)
		point.setY(0);

	if (point.z() > -0.00001 && point.z() < 0.00001)
		point.setZ(0);
}

void clampValue(double& value, const double reference)
{
	if (value > reference - 0.00001 && value < reference + 0.00001)
		value = reference;
}

void clampValues(MatrixParameters& params)
{
	clampValue(params.tx, 0);
	clampValue(params.ty, 0);
	clampValue(params.tz, 0);

	clampValue(params.ax, 0);
	clampValue(params.ay, 0);
	clampValue(params.az, 0);

	clampValue(params.sx, 1);
	clampValue(params.sy, 1);
	clampValue(params.sz, 1);
	clampValue(params.sxy, 1);

	clampValue(params.sx, -1);
	clampValue(params.sy, -1);
	clampValue(params.sz, -1);
	clampValue(params.sxy, -1);

	clampValue(params.skewx, 0);
	clampValue(params.skewy, 0);
	clampValue(params.skewz, 0);
}

Math::Point3d vectorToOgl(Math::Point3d point, CA_CelPtr celPtr)
{
	CELVEC_Tbd* tbd = getPermTbd(celPtr);
	if (!tbd)
	{
		//This seems to be the default value of domain scaling.
		//A nullptr as tbd refers to a drawing without any content but with a valid CA_CelPtr.
		//These can store drawing pivots which should be converted.

		const int coordConversion = 1875;
		return Math::Point3d(point.x() / (coordConversion), point.y() / coordConversion, point.z() / coordConversion);
	}

	return tbd->getDrawingDomainScalingMatrix() * point;
}

Math::Matrix4x4 getVectorModificationMatrix(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 modMatrix, CELVEC_Tbd* tbd)
{
	if (!tbd || !sceneMetrics)
		return modMatrix;

	//It is not entirely clear, how the domain scaling matrix is being influenced. The manual way of constructing matrix
	// to convert to vecor coordinates would be:
	//const int coordConversion = 1875;
	//Math::Matrix4x4 fromOglMatrix = Math::Matrix4x4().scale(coordConversion,coordConversion,coordConversion);
	//There are other matrices avaible via tbd, that also match these values. However these are influenced by tbd's applied
	//matrix that doesn't seem to have any influence on the vector drawings.

	Math::Matrix4x4 fromOglMatrix = tbd->getDrawingDomainScalingMatrix().getInverse();


	return fromOglMatrix * modMatrix * fromOglMatrix.getInverse();
}

double applyUnitOffset(const Math::Matrix4x4 & unitOffsetMatrix, const double rotation)
{
	Math::Matrix4x4 rotationMatrix = Math::Matrix4x4().rotateDegrees(rotation, {0,0,1});
	rotationMatrix = unitOffsetMatrix.getInverse() * rotationMatrix * unitOffsetMatrix;
	return getAngle2d(rotationMatrix.getTransform2d());
}


Math::Matrix4x4 getFieldsModificationMatrix(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 modMatrix)
{
	Math::Matrix4x4 fromOglMatrix = Math::Matrix4x4().scale(sceneMetrics->fromOGLX(1), sceneMetrics->fromOGLY(1), sceneMetrics->fromOGLZ(1));
	return fromOglMatrix * modMatrix * fromOglMatrix.getInverse();
}

double fieldsToOgl(SC_SceneMetrics* sceneMetrics, double rotation)
{
	Math::Matrix2x3 rotationMatrix = Math::Matrix2x3().Rotate(toRadians(rotation));
	Math::Matrix2x3 toOglMatrix = Math::Matrix2x3().Scale({sceneMetrics->toOGLX(1),sceneMetrics->toOGLY(1)});

	Math::Matrix2x3 fromOglMatrix = Math::Matrix2x3().Scale({sceneMetrics->fromOGLX(1),sceneMetrics->fromOGLY(1)});
	return getAngle2d(fromOglMatrix * rotationMatrix* toOglMatrix);
}

Math::Matrix4x4 fieldsToOgl(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 matrix)
{
	Math::Matrix4x4 toOglMatrix = Math::Matrix4x4().scale(sceneMetrics->toOGLX(1), sceneMetrics->toOGLY(1), sceneMetrics->toOGLZ(1));
	return toOglMatrix * matrix;
}


Math::Matrix4x4 oglToFields(SC_SceneMetrics* sceneMetrics, Math::Matrix4x4 matrix)
{
	Math::Matrix4x4 fromOglMatrix = Math::Matrix4x4().scale(sceneMetrics->fromOGLX(1), sceneMetrics->fromOGLY(1), sceneMetrics->fromOGLZ(1));
	return fromOglMatrix * matrix;
}

Math::Angle3d getAngles(const Math::Matrix4x4 matrix, const double angleZ)
{
	Math::Angle3d angle;

	Math::Vector3d nX = Math::Vector3d(matrix(Math::Row::Row_0, Math::Column::Column_0), matrix(Math::Row::Row_1, Math::Column::Column_0), matrix(Math::Row::Row_2, Math::Column::Column_0));
	Math::Vector3d nY = Math::Vector3d(matrix(Math::Row::Row_0, Math::Column::Column_1), matrix(Math::Row::Row_1, Math::Column::Column_1), matrix(Math::Row::Row_2, Math::Column::Column_1));
	Math::Vector3d nZ = Math::Vector3d(matrix(Math::Row::Row_0, Math::Column::Column_2), matrix(Math::Row::Row_1, Math::Column::Column_2), matrix(Math::Row::Row_2, Math::Column::Column_2));

	nX /= nX.length();

	nY = nY - nX.getDotProduct(nY) * nX;
	nY = nY / nY.length();

	nZ = nZ - nX.getDotProduct(nZ) * nX - nY.getDotProduct(nZ) * nY;
	nZ = nZ / nZ.length();

	angle.setY(atan2(-nX.z(), sqrt(nX.x() * nX.x() + nX.y() * nX.y())));

	if (cos(angle.y()) > 0.00001 || cos(angle.y()) < -0.00001)
	{
		angle.setX(atan2(nY.z() / cos(angle.y()), nZ.z() / cos(angle.y())));
		angle.setZ(atan2(nX.y() / cos(angle.y()), nX.x() / cos(angle.y())));
	}
	else
	{
		// Gimbal lock
		angle.setZ(toRadians(angleZ));
		angle.setX(atan2(-nZ.y(), nY.y()) + angle.z());
	}

	angle.setX(fromRadians(angle.x()));
	angle.setY(fromRadians(angle.y()));
	angle.setZ(fromRadians(angle.z()));

	return angle;
}

double getAngle2d(const Math::Matrix2x3 matrix)
{
	Math::Vector2d nX = Math::Vector2d(matrix(Math::Row::Row_0, Math::Column::Column_0), matrix(Math::Row::Row_1, Math::Column::Column_0));
	nX.Normalize();

	return fromRadians(atan2(nX.y(), nX.x()));
}

double getAngleBetween(double angle01, double angle02)
{
	double difference01 = std::fmod(angle01 - angle02, 360);

	if (difference01 < 0)
		difference01 = 360 - difference01;

	double difference02 = std::fmod(angle02 - angle01, 360);

	if (difference02 < 0)
		difference02 = 360 - difference02;

	return difference01 < difference02 ? difference01 : difference02;
}

double getAngleOffset(Math::Angle3d angle01, Math::Angle3d angle02)
{
	double offset = getAngleBetween(angle01.x(), angle02.x());
	offset += getAngleBetween(angle01.y(), angle02.y());
	offset += getAngleBetween(angle01.z(), angle02.z());

	return offset;
}

MatrixParameters getAnglesAndFlip3d(const Math::Matrix4x4& matrix, Math::Angle3d rotation)
{
	MatrixParameters p;

	Math::Angle3d angle;


	if (getAffineDeterminant(matrix) < 0)
	{
		Math::Matrix4x4 matrixXFlip = matrix;
		matrixXFlip.scale(-1, 1, 1);
		Math::Angle3d angleXFlip = getAngles(matrixXFlip, rotation.z());
		double angleOffsetX = getAngleOffset(angleXFlip, rotation);

		Math::Matrix4x4 matrixYFlip = matrix;
		matrixYFlip.scale(1, -1, 1);
		Math::Angle3d angleYFlip = getAngles(matrixYFlip, rotation.z());
		double angleOffsetY = getAngleOffset(angleYFlip, rotation);

		Math::Matrix4x4 matrixZFlip = matrix;
		matrixZFlip.scale(1, 1, -1);
		Math::Angle3d angleZFlip = getAngles(matrixZFlip, rotation.z());
		double angleOffsetZ = getAngleOffset(angleZFlip, rotation);


		if (angleOffsetX <= angleOffsetY && angleOffsetX <= angleOffsetZ)
		{
			angle = angleXFlip;
			p.sx = -1;
		}
		else if (angleOffsetY <= angleOffsetZ)
		{
			angle = angleYFlip;
			p.sy = -1;
		}
		else
		{
			angle = angleZFlip;
			p.sz = -1;
		}
	}
	else
	{
		angle = getAngles(matrix, rotation.z());
	}


	p.ax = matchFullRotations(rotation.x(), angle.x());
	p.ay = matchFullRotations(rotation.y(), angle.y());
	p.az = matchFullRotations(rotation.z(), angle.z());

	return p;
}

Math::Vector2d getFlip2d(const Math::Matrix2x3& matrix, double rotationZ)
{
	Math::Vector2d p = Math::Vector2d(1,1);

	if (matrix.Determinant() < 0)
	{
		Math::Matrix2x3 matrixXFlip = matrix;
		matrixXFlip.Scale(Math::Vector2d(-1, 1));

		double angleXFlip = getAngle2d(matrixXFlip);
		double angleOffsetX = getAngleBetween(angleXFlip, rotationZ);

		Math::Matrix2x3 matrixYFlip = matrix;
		matrixYFlip.Scale(Math::Vector2d(1, -1));
		double angleYFlip = getAngle2d(matrixYFlip);
		double angleOffsetY = getAngleBetween(angleYFlip, rotationZ);


		if (angleOffsetX <= angleOffsetY)
			p.setX(-1);
		else
			p.setY(-1);
	}
	else
	{
		double angleNoFlip = getAngle2d(matrix);
		double angleOffsetNoFlip = getAngleBetween(angleNoFlip, rotationZ);


		Math::Matrix2x3 matrixDoubleFlip = matrix;
		matrixDoubleFlip.Scale(Math::Vector2d(-1, -1));
		double angleDoubleFlip = getAngle2d(matrixDoubleFlip);
		double angleOffsetDoubleFlip = getAngleBetween(angleDoubleFlip, rotationZ);

		if (angleOffsetNoFlip > angleOffsetDoubleFlip)
		{
			p.setX(-1);
			p.setY(-1);
		}
	}


	return p;
}

MatrixParameters matrixParameters(const Math::Matrix4x4& matrix, const Math::Point3d pivot, Math::Angle3d rotation)
{

	if (isTransform2d(matrix))
		return matrixParameters2d(matrix, pivot, rotation.z());
	else
		return matrixParameters3d(matrix, pivot, rotation);
}

MatrixParameters matrixParameters2d(const Math::Matrix4x4& matrix, const Math::Point3d pivot, double rotationZ)
{
	Math::Vector2d flipValues;

	//Determining which flip combination leads to the rotation values closest to the original value
	flipValues = getFlip2d(matrix.getTransform2d(), rotationZ);

	MatrixParameters p;

	if (flipValues.x() == 1 && flipValues.y() == 1)
		matrix.extractParameters(true, true, pivot.x(), pivot.y(), p.tx, p.ty, p.tz, p.sx, p.sy, p.az, p.skewx);
	else if (flipValues.x() == -1 && flipValues.y() == 1)
		matrix.extractParameters(false, true, pivot.x(), pivot.y(), p.tx, p.ty, p.tz, p.sx, p.sy, p.az, p.skewx);
	else if (flipValues.x() == 1 && flipValues.y() == -1)
		matrix.extractParameters(true, false, pivot.x(), pivot.y(), p.tx, p.ty, p.tz, p.sx, p.sy, p.az, p.skewx);
	else
		matrix.extractParameters(false, false, pivot.x(), pivot.y(), p.tx, p.ty, p.tz, p.sx, p.sy, p.az, p.skewx);

	p.az = matchFullRotations(rotationZ, p.az);

	return p;
}

MatrixParameters matrixParameters3d(const Math::Matrix4x4& matrix, const Math::Point3d pivot, Math::Angle3d rotation)
{
	MatrixParameters p;

	p = getAnglesAndFlip3d(matrix, rotation);

	Math::Matrix4x4 decompMatrix = matrix;

	if (p.sx == -1)
		decompMatrix.scale(-1, 1, 1);

	if (p.sy == -1)
		decompMatrix.scale(1, -1, 1);

	if (p.sz == -1)
		decompMatrix.scale(1, 1, -1);


	//Using temp scale and rot values as it's an easier way to store the scale before without having it overriden
	//Only the position is being used the rest is being manually calculated
	double tempSx, tempSy, tempSz, tempAx, tempAy, tempAz;

	matrix.extractParameters(pivot.x(), pivot.y(), pivot.z(), p.tx, p.ty, p.tz, tempSx, tempSy, tempSz, tempAx, tempAy, tempAz);


	//Reconstructing original rotation matrix


	Math::Angle3d angle = Math::Angle3d(p.ax, p.ay, p.az);

	Math::Matrix4x4 rotSkewMatrix = angle.toRotationMatrix();

	//Getting skew/shear value

	const Math::Vector3d X = Math::Vector3d(decompMatrix(Math::Row::Row_0, Math::Column::Column_0),
		decompMatrix(Math::Row::Row_1, Math::Column::Column_0),
		decompMatrix(Math::Row::Row_2, Math::Column::Column_0));
	const Math::Vector3d Y = Math::Vector3d(decompMatrix(Math::Row::Row_0, Math::Column::Column_1),
		decompMatrix(Math::Row::Row_1, Math::Column::Column_1),
		decompMatrix(Math::Row::Row_2, Math::Column::Column_1));
	const Math::Vector3d Z = Math::Vector3d(decompMatrix(Math::Row::Row_0, Math::Column::Column_2),
		decompMatrix(Math::Row::Row_1, Math::Column::Column_2),
		decompMatrix(Math::Row::Row_2, Math::Column::Column_2));

	p.skewx = X.getDotProduct(Y) / (X.length() * Y.length());

	p.skewx = fromRadians(asin(p.skewx));


	//Adding skew/shear value to rotation matrix in order to find y-axis length before scale
	//Calculating matrices manually as
	const double skewXArr[16] = { 1,0,0,0,sin(p.skewx), cos(p.skewx), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	Math::Matrix4x4 skewMatrix = Math::Matrix4x4(skewXArr);

	p.skewz = Z.getDotProduct(Y) / (Z.length() * Y.length());
	p.skewz = fromRadians(asin(p.skewz));


	const double skewZArr[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, sin(p.skewz), cos(p.skewz), 0, 0, 0, 0, 1 };

	skewMatrix *= Math::Matrix4x4(skewZArr);

	rotSkewMatrix *= skewMatrix;

	const double yPosSkewLength = Math::Vector3d(rotSkewMatrix(Math::Row::Row_0, Math::Column::Column_1),
		rotSkewMatrix(Math::Row::Row_1, Math::Column::Column_1),
		rotSkewMatrix(Math::Row::Row_2, Math::Column::Column_1)).length();

	const double zPosSkewLength = Math::Vector3d(rotSkewMatrix(Math::Row::Row_0, Math::Column::Column_2),
		rotSkewMatrix(Math::Row::Row_1, Math::Column::Column_2),
		rotSkewMatrix(Math::Row::Row_2, Math::Column::Column_2)).length();


	p.sx *= X.length();
	p.sy *= Y.length() / yPosSkewLength;
	p.sz *= Z.length() / zPosSkewLength;

	return p;
}

void printMatrixParameters(MatrixParameters p)
{
	printf("matrix: \n translation %f %f %f \n scale: %f %f %f \n rotation %f %f %f \n skew %f %f %f\n\n",
	p.tx, p.ty, p.tz, p.sx, p.sy, p.sz, p.ax, p.ay, p.az, p.skewx, p.skewy, p.skewz);
}

void printAttributes(AT_AttrList attributes)
{
	for (const AT_AttrDesc & attribute : attributes)
	{
		const QString keyword = attribute._pAttr->keyword();

		printf("Attribute name: %s\n", keyword.toUtf8().data());
	}
}

static void getAllChildren_internal(MO_Node* node, std::vector<MO_Node*>& children)
{
	std::vector<MO_Port*> outPorts = node->getOutPorts();
	for (auto& port : outPorts)
	{
		if (!port->isMatrixPort())
			continue;

		std::vector<MO_Port*> dstPorts;
		port->realDstPorts(dstPorts);

		for (auto& dstPort : dstPorts)
		{
			if (dstPort->node() && dstPort->node()->toModule()
				&& find(children.begin(), children.end(), dstPort->node()->toModule()) == children.end())
			{
				children.push_back(dstPort->node()->toModule());

				getAllChildren_internal(dstPort->node(), children);
			}
		}
	}
}

std::vector<MO_Node*> getAllChildren(MO_Node* node)
{
	std::vector<MO_Node*> children;
	getAllChildren_internal(node, children);
	return children;
}

void getAllInputModules_internal(MO_Module* modulePtr, std::vector<MO_Module*>& inputModules)
{
	for (unsigned int i = 0; i < modulePtr->nInPorts(); i++)
	{
		MO_Module* inputModule = modulePtr->inputModule(i);

		if (!inputModule)
			continue;

		if (!inputModule->hasImageOutput())
			continue;

		if (inputModule->keyword() == QLatin1String("READ"))
			inputModules.push_back(inputModule);
		else
			getAllInputModules_internal(inputModule, inputModules);
	}
}

AT_AttrList getAttributeList(const MO_Node* node)
{
	AT_AttrList attributes;
	node->getFullAttrList(attributes);

	return attributes;
}

QString getLayerAttr(MO_Module* modulePtr)
{
	if (!modulePtr)
		return QLatin1String("");

	const AT_ElementAttr* elementAttr = findAttribute<AT_ElementAttr>(QLatin1String("ELEMENT"), modulePtr);

	if (!elementAttr)
		return QLatin1String("");

	const AT_ElementLayerAttr* layerAttr = elementAttr->layerAttr();

	if (!layerAttr)
		return QLatin1String("");


	if (layerAttr->localValue() == QLatin1String(""))
		return QLatin1String("null");
	else
		return layerAttr->localValue();
}

int getElementId(MO_Module* modulePtr)
{
	if (!modulePtr)
		return -1;


	AT_DrawingAttr* drawingAttr = findAttribute<AT_DrawingAttr>(QLatin1String("DRAWING"), modulePtr);

	if (!drawingAttr)
		return -1;

	QString colName = drawingAttr->linkedColumnName(QLatin1String("ELEMENT"), false);

	if (colName.isEmpty())
		return -1;


	if (!isElementColumn(SDK_Column::Descriptor(colName)))
		return -1;

	return getElementIdOfColumn(SDK_Column::Descriptor(colName));
}

std::vector<MO_Module*> getAllInputModules(MO_Module* modulePtr)
{
	std::vector<MO_Module*> inputModules;
	getAllInputModules_internal(modulePtr, inputModules);
	return inputModules;
}

std::vector<CelInfo> getElementTimings(MO_Module* modulePtr)
{
	std::vector<CelInfo> celInfoVec;

	AT_ElementAttr* elementAttr = findAttribute<AT_ElementAttr>(QLatin1String("ELEMENT"), modulePtr);

	if (!elementAttr)
		return celInfoVec;

	AT_DrawingAttr* drawingAttr = findAttribute<AT_DrawingAttr>(QLatin1String("DRAWING"), modulePtr);
	if (!drawingAttr)
		return celInfoVec;

	QString colName = drawingAttr->linkedColumnName(QLatin1String("ELEMENT"), false);

	if (colName.isEmpty())
		return celInfoVec;

	QString currentEm = elementAttr->getElementName();

	if (!isElementColumn(SDK_Column::Descriptor(colName)))
		return celInfoVec;

	int elementId = getElementIdOfColumn(SDK_Column::Descriptor(colName));

	AT_ElementLayerAttr* layerAttr = elementAttr->layerAttr();

	if (!layerAttr)
		return celInfoVec;


	SDK_Column::DrawingCol_t timings;
	getAllElementDrawingsOfColumn(SDK_Column::Descriptor(colName), timings);

	for (auto it = timings.begin(); it != timings.end(); ++it)
	{
		CA_CelPtr celPtr = SDK_CelCache::getCelFromDrawingElement(elementId, *it, layerAttr->localValue());
		celInfoVec.push_back(CelInfo(celPtr, it->id(), elementId, layerAttr->localValue()));
	}

	return celInfoVec;
}

std::vector<Exposure> getExposures(MO_Module* modulePtr, FrameRange range)
{
	AT_ElementAttr* elementAttr = findAttribute<AT_ElementAttr>(QLatin1String("ELEMENT"), modulePtr);

	if (!elementAttr)
		return {};

	AT_DrawingAttr* drawingAttr = findAttribute<AT_DrawingAttr>(QLatin1String("DRAWING"), modulePtr);

	if (!drawingAttr)
		return {};

	QString colName = drawingAttr->linkedColumnName(QLatin1String("ELEMENT"), false);

	if (colName.isEmpty())
		return {};

	QString currentEm = elementAttr->getElementName();

	if (!isElementColumn(SDK_Column::Descriptor(colName)))
		return {};

	int elementId = getElementIdOfColumn(SDK_Column::Descriptor(colName));

	AT_ElementLayerAttr* layerAttr = elementAttr->layerAttr();

	if (!layerAttr)
		return {};

	const int capacity = range.end - range.start + 1;

	if (capacity <= 0)
		return {};

	std::vector<Exposure> ret;
	ret.reserve(capacity);

	for (int i = range.start; i <= range.end; i++)
	{
		EM_DrawingId drawingId = EM_DrawingId::null;

		if (!getDrawing(SDK_Column::Descriptor(colName), i, &elementId, &drawingId))
			continue;

		CA_CelPtr celPtr = SDK_CelCache::getCelFromDrawingElement(elementId, drawingId, layerAttr->localValue());

		if (!celPtr.isValid() || !celPtr.cel())
			continue;

		ret.emplace_back(i, celPtr);
	}

	return ret;
}

CELVEC_Tbd* getPermTbd(CA_CelPtr celPtr)
{
	CEL_Cel* cel = celPtr.cel();

	if (!cel)
		return nullptr;

	CEL_Rep rep = cel->findRepresentation(CEL_Representation::TBD);

	if (!rep.GetPtr())
		return nullptr;

	CELVEC_Tbd* tbd;

	tbd = dynamic_cast<CELVEC_Tbd*>(rep.GetPtr());

	return tbd;
}

Math::Point2d getDrawingPivot(const CA_CelPtr cel)
{
	CELVEC_Tbd* tbd = getPermTbd(cel);

	if (!tbd)
		return {};


	GR_CompositeVectorDrawingObj* compDrawing = tbd->getDrawingObject();

	if (!compDrawing)
		return {};

	GR_CompositeDrawingInfo* info = compDrawing->GetDrawingInfo();

	if (!info)
		return {};

	return info->GetPivotPoint();
}

void setDrawingPivot(CA_CelPtr cel, const Math::Point3d pivot)
{
	CELVEC_Tbd* tbd = getPermTbd(cel);

	if (!tbd)
		return;


	GR_CompositeVectorDrawingObj* compDrawing = tbd->getDrawingObject();

	if (!compDrawing)
		return;

	GR_CompositeDrawingInfo* info = compDrawing->GetDrawingInfo();

	if (!info)
		return;

	info->SetPivotPoint(Math::Point2d(pivot.x(), pivot.y()));
}

Math::Matrix4x4 oglToVector(Math::Matrix4x4 matrix)
{
	Math::Matrix4x4 scaleMatrix = Math::Matrix4x4().scale(1875, 1875, 1875);
	return scaleMatrix * matrix * scaleMatrix.getInverse();
}

bool isDrawingPivotAppliedOnPeg(MO_Module* modulePtr)
{
	if (!modulePtr)
		return false;

	const AT_Attr* attr = findAttribute<AT_Attr>(QLatin1String("USE_DRAWING_PIVOT"), modulePtr);

	if (attr)
		return attr->textValue(1) == QLatin1String("Apply Embedded Pivot on Parent Peg");

	return false;
}

bool hasUsedDrawingPivots(MO_Module* modulePtr)
{
	if (!modulePtr)
		return false;

	const AT_Attr* attr = findAttribute<AT_Attr>(QLatin1String("USE_DRAWING_PIVOT"), modulePtr);

	if (attr)
		return attr->textValue(1) != QLatin1String("Don't Use Embedded Pivot");

	return false;
}
