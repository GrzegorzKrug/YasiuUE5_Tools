/** 
 * Copyright (c) 2026 Grzegorz Krug.
 * All Rights Reserved.
 */


#include "YasiuTools.h"
#include "Logging/StructuredLog.h"


#define LOCTEXT_NAMESPACE "FYasiuTools"

void FYasiuToolsModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    UE_LOG(LogTemp, Warning, TEXT("YASIU TOOLS IS LAODED!!"));
}

void FYasiuToolsModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYasiuToolsModule, YasiuTools)
