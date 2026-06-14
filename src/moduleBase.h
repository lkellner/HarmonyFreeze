/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef MODULEBASE_H
#define MODULEBASE_H

#include "utils.h"
#include "freezeManager.h"

#include <algorithm>

class MO_Module;

enum class ModuleType
{
	ELEMENT,
	PEG,
	DRAWING_TRANSFORMATION,
	STATIC_TRANSFORMATION,
	OGL_CONTROLLER,
	CURVE_OFFSET,
	CURVE,
};

class ModuleBase
{
public:
	ModuleBase(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType);
	virtual ~ModuleBase() = default;

	virtual void readjustSecondary() = 0;
	virtual void readjustTertiary() {};

	ModuleType getModuleType() const;
	MO_Module* getModulePtr() const;

	virtual FrameRange getFrameRange() const;

	AT_AttrList getAttributeList() const;

	template <typename T>
	T* findAttribute(const QString& keyword)
	{
		return ::findAttribute<T>(keyword, getModulePtr());
	}


protected:
	std::shared_ptr<FreezeManager> getFreezeManager() const;
	FreezeManager* getFreezeManagerPtr() const;

	virtual Math::Matrix4x4 getIncomingMatrix(unsigned int port, double frameNo, bool useDeformation) const;

private:
	std::shared_ptr<FreezeManager> m_freezeManager;

	MO_Module* m_modulePtr;
	ModuleType m_moduleType;
};

#endif
