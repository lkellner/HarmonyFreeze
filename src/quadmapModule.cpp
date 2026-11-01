#include "quadmapModule.h"

using namespace Qt::StringLiterals;

QuadmapModule::QuadmapModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: PointModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
	//Quadmaps are a bit misleading at a first glance as, when they are connected underneath a peg
	//they don't change their visual position/shape to match the pegs animation.
	//Therefore readjusting their points in readjustSecondary will give the impression that they have
	//moved. This transformation is however what's necessary to keep the drawings/pegs underneath in place.

	AT_AttrList attrList = getAttributeList();

	for (auto& attr : attrList)
	{
		if (attr._pAttr->typeName() == "POSITION_2D"_L1)
		{
			AT_Position2dAttr* pointAttr = dynamic_cast<AT_Position2dAttr*>(attr._pAttr);

			if (!pointAttr)
				continue;

			if (pointAttr->keyword() == "PIVOT"_L1)
				addTransformPoint(std::make_unique<TransformPoint>(pointAttr->xmlKeyword(), pointAttr, false));
			else
				addTransformPoint(std::make_unique<TransformPoint>(pointAttr->xmlKeyword(), pointAttr, true));
		}
	}
}
