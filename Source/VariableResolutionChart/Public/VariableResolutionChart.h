// VariableResolutionChart.h
#pragma once
#include "Modules/ModuleManager.h"

class FVariableResolutionChartModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};