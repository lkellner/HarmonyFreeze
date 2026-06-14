#include "elementModule.h"
#include "moduleBase.h"

#include <SceneCore//attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_AttrCmds.h>
#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttrCmds.h>
#include <SceneCore/module/MO_FrameKey.h>

#include <stdexcept>

ModuleBase::ModuleBase(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType)
	: m_freezeManager(std::move(freezeManager))
	, m_modulePtr(modulePtr)
	, m_moduleType(moduleType)
{
	if (!modulePtr)
		throw std::invalid_argument("modulePtr");
}

ModuleType ModuleBase::getModuleType() const
{
	return m_moduleType;
}

MO_Module* ModuleBase::getModulePtr() const
{
	return m_modulePtr;
}

FrameRange ModuleBase::getFrameRange() const
{
	return {};
}

AT_AttrList ModuleBase::getAttributeList() const
{
	return ::getAttributeList(m_modulePtr);
}

std::shared_ptr<FreezeManager> ModuleBase::getFreezeManager() const
{
	return m_freezeManager;
}

FreezeManager* ModuleBase::getFreezeManagerPtr() const
{
	return m_freezeManager.get();
}

Math::Matrix4x4 ModuleBase::getIncomingMatrix(unsigned int port, double frameNo, bool useDeformation) const
{
	if (port < 0)
	{
		printf("invalid port for incoming matrix\n");
		return {};
	}

	const MO_Node::InPorts inPorts = m_modulePtr->getInPorts();

	if (inPorts.size() + 1 < port)
	{
		printf("invalid port for incoming matrix\n");
		return {};
	}

	const MO_BaseContext context{MO_FrameKey(frameNo)};
	const MO_PortTransform * portTransform = m_modulePtr->computeTransformationParent(context, port);

	if (!portTransform)
	{
		return {};
	}

	CC_Transformation ccTransform = portTransform->transformation();

	ccTransform.finalizeDeformation();
	return ccTransform.matrix(useDeformation);
}
