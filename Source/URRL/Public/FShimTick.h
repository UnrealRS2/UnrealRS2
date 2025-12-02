#pragma once
//Win32 BULLSHIT
#define NOMINMAX

#include "Tickable.h"

class FShimTick : public FTickableGameObject
{
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FCameraTick, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return true; }
};
