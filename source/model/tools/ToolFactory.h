#ifndef TOOLFACTORY_H
#define TOOLFACTORY_H

#include "model/Definitions.h"

class ToolFactory
{
public:
	virtual ~ToolFactory() = default;

	virtual SimulationAccess* buildSimulationAccess() const = 0;
};

#endif // TOOLFACTORY_H
