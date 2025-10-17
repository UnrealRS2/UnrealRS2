// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FUnrealRS2Module : public FDefaultGameModuleImpl
{
	public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static UTexture2D* GClientTexture;
	static int32 CurrentDrawColor;	
};

DECLARE_LOG_CATEGORY_EXTERN(LogRS2, Log, All);

