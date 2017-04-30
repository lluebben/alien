#ifndef SIMULATIONUNITCONTEXT_H
#define SIMULATIONUNITCONTEXT_H

#include <QList>
#include <QSize>

#include "model/definitions.h"

class SimulationUnitContext
	: public QObject
{
	Q_OBJECT
public:
	SimulationUnitContext(QObject* parent) : QObject(parent) {}
	virtual ~SimulationUnitContext() {}
	
	virtual void init(SymbolTable * symbolTable, Topology* topology) = 0;

    virtual Topology* getTopology () const = 0;
	virtual EnergyParticleMap* getEnergyParticleMap() const = 0;
	virtual CellMap* getCellMap() const = 0;
	virtual SymbolTable* getSymbolTable() const = 0;
	virtual SimulationParameters* getSimulationParameters() const = 0;

	virtual QList<CellCluster*>& getClustersRef() = 0;
    virtual QList<EnergyParticle*>& getEnergyParticlesRef () = 0;
};

#endif // SIMULATIONUNITCONTEXT_H
