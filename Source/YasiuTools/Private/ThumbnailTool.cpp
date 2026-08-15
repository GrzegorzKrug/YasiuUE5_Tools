#include "ThumbnailTool.h"


#include "YasiuToolsLogs.h"


#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"

#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "RenderingThread.h"
#include "RHICommandList.h"

#include "Editor.h"


void UCityThumbnailGenerator::GenerateThumbnail( AActor* Actor, const FVector& Location, const FRotator& Rotation )
{
    if ( !Actor ) {
        UE_LOG(YasiuToolsPlugin, Warning, TEXT("ActorThumbnail: Select an ABasePassiveBuilding first."));

        return;
    }


    UE_LOG(YasiuToolsPlugin, Display, TEXT("ActorThumbnail: Generating thumbnail for %s"), *Actor->GetName());


    UTextureRenderTarget2D* RenderTarget = RenderActor(Actor, Location, Rotation);


    if ( !RenderTarget ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Rendering failed."));

        return;
    }


    /*
     * -------------------------------------------------------------
     * Read pixels AGAIN.
     *
     * RenderActor() only performs the render.
     * -------------------------------------------------------------
     */
    FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();


    if ( !Resource ) {
        return;
    }


    TArray<FColor> Pixels;

    FReadSurfaceDataFlags ReadFlags;

    ReadFlags.SetLinearToGamma(false);
    ReadFlags.SetOutputStencil(false);


    if ( !Resource->ReadPixels(Pixels, ReadFlags) ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Failed reading pixels."));

        return;
    }


    // /*
    //  * Flip vertically.
    //  */
    // for ( int32 Y = 0; Y < Resolution / 2; ++Y ) {
    //     const int32 OppositeY = Resolution - 1 - Y;
    //
    //     for ( int32 X = 0; X < Resolution; ++X ) {
    //         Pixels.Swap(Y * Resolution + X, OppositeY * Resolution + X);
    //     }
    // }


    /*
     * -------------------------------------------------------------
     * Create / overwrite UTexture2D.
     * -------------------------------------------------------------
     */

    UTexture2D* Texture = CreateTextureAsset(Actor, Pixels, Resolution, Resolution);


    if ( Texture ) {
        UE_LOG(YasiuToolsPlugin, Display, TEXT( "ActorThumbnail: Successfully generated %s" ), *Texture->GetPathName());
    }
}

UTextureRenderTarget2D* UCityThumbnailGenerator::RenderActor( AActor* Actor, const FVector& Location, const FRotator& Rotation )


{
    if ( !Actor ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: No ABasePassiveBuilding selected."));

        return nullptr;
    }

    if ( Resolution <= 0 ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Invalid resolution."));

        return nullptr;
    }

    UWorld* World = Actor->GetWorld();

    if ( !World ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Actor has no valid World."));

        return nullptr;
    }

    /*
     * -------------------------------------------------------------
     * Render target
     * -------------------------------------------------------------
     */

    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->RenderTargetFormat = RTF_RGBA8;

    RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    RenderTarget->bAutoGenerateMips = false;

    RenderTarget->InitCustomFormat(Resolution, Resolution, PF_B8G8R8A8, false);

    RenderTarget->UpdateResourceImmediate(true);


    /*
     * -------------------------------------------------------------
     * Scene capture
     * -------------------------------------------------------------
     */

    USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(GetTransientPackage());

    Capture->RegisterComponentWithWorld(World);

    Capture->TextureTarget = RenderTarget;

    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;

    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;


    /*
     * -------------------------------------------------------------
     * IMPORTANT:
     *
     * Render ONLY the selected actor.
     * -------------------------------------------------------------
     */

    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

    Capture->ShowOnlyActors.Empty();
    Capture->ShowOnlyActors.Add(Actor);


    /*
     * -------------------------------------------------------------
     * Camera
     * -------------------------------------------------------------
     */

    const FVector ActorLocation = Actor->GetActorLocation();

    Capture->SetWorldLocation(ActorLocation + Location);

    Capture->SetWorldRotation(Rotation);

    Capture->ProjectionType = ECameraProjectionMode::Perspective;

    Capture->FOVAngle = 40.0f;


    /*
     * -------------------------------------------------------------
     * Render
     * -------------------------------------------------------------
     */

    Capture->CaptureScene();

    FlushRenderingCommands();


    /*
     * -------------------------------------------------------------
     * Read pixels
     * -------------------------------------------------------------
     */

    FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();

    if ( !Resource ) {
        Capture->DestroyComponent();
        return nullptr;
    }

    TArray<FColor> Pixels;

    FReadSurfaceDataFlags ReadFlags;
    ReadFlags.SetLinearToGamma(false);
    ReadFlags.SetOutputStencil(false);

    if ( !Resource->ReadPixels(Pixels, ReadFlags) ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Failed to read render target."));

        Capture->DestroyComponent();

        return nullptr;
    }


    // /*
    //  * Render targets are vertically flipped compared
    //  * to normal texture data.
    //  */
    // for ( int32 Y = 0; Y < Resolution / 2; ++Y ) {
    //     const int32 OppositeY = Resolution - 1 - Y;
    //
    //     for ( int32 X = 0; X < Resolution; ++X ) {
    //         Pixels.Swap(Y * Resolution + X, OppositeY * Resolution + X);
    //     }
    // }


    Capture->DestroyComponent();

    return RenderTarget;
}


UTexture2D* UCityThumbnailGenerator::CreateTextureAsset( AActor* Actor, const TArray<FColor>& Pixels, int32 Width, int32 Height )
{
    // if ( !Actor || Pixels.Num() != Width * Height ) {
    //     return nullptr;
    // }

    const FString PackageName = GetAssetPackageName(Actor);

    if ( PackageName.IsEmpty() ) {
        return nullptr;
    }

    const FString AssetName = FPackageName::GetShortName(PackageName);


    /*
     * -------------------------------------------------------------
     * Check for an existing asset.
     * -------------------------------------------------------------
     */

    UPackage* Package = CreatePackage(*PackageName);

    if ( !Package ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Failed to create package %s"), *PackageName);
        return nullptr;
    }


    /*
     * If the texture already exists, remove the old object.
     */

    UTexture2D* ExistingTexture = FindObject<UTexture2D>(Package, *AssetName);

    if ( ExistingTexture ) {
        ExistingTexture->ClearFlags(RF_Standalone);

        ExistingTexture->MarkAsGarbage();

        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }


    /*
     * -------------------------------------------------------------
     * Create texture.
     * -------------------------------------------------------------
     */

    UTexture2D* Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);


    if ( !Texture ) {
        return nullptr;
    }


    /*
     * -------------------------------------------------------------
     * Texture settings.
     * -------------------------------------------------------------
     */

    Texture->MipGenSettings = TMGS_NoMipmaps;

    Texture->CompressionSettings = TC_EditorIcon;

    Texture->SRGB = true;

    Texture->NeverStream = true;


    /*
     * -------------------------------------------------------------
     * Texture source data.
     *
     * This is the important part:
     * the pixels are copied directly into the UTexture2D.
     * -------------------------------------------------------------
     */

    Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8);


    uint8* MipData = Texture->Source.LockMip(0);


    const int64 DataSize = static_cast<int64>(Width) * static_cast<int64>(Height) * sizeof(FColor);


    FMemory::Memcpy(MipData, Pixels.GetData(), DataSize);


    Texture->Source.UnlockMip(0);


    /*
     * Create GPU resource.
     */

    Texture->UpdateResource();


    /*
     * -------------------------------------------------------------
     * Register with Asset Registry.
     * -------------------------------------------------------------
     */

    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    AssetRegistry.AssetCreated(Texture);


    /*
     * Mark package modified.
     */
    Package->MarkPackageDirty();


    /*
     * -------------------------------------------------------------
     * Save .uasset
     * -------------------------------------------------------------
     */
    const FString PackageFilename = FPackageName::LongPackageNameToFilename(
        PackageName,
        FPackageName::GetAssetPackageExtension()
    );


    FSavePackageArgs SaveArgs;

    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

    SaveArgs.Error = GError;


    const bool bSaved = UPackage::SavePackage(Package, Texture, *PackageFilename, SaveArgs);


    if ( !bSaved ) {
        UE_LOG(YasiuToolsPlugin, Error, TEXT("ActorThumbnail: Failed to save %s"), *PackageFilename);

        return nullptr;
    }


    UE_LOG(YasiuToolsPlugin, Display, TEXT("ActorThumbnail: Created %s"), *PackageName);


    return Texture;
}

FString UCityThumbnailGenerator::GetAssetPackageName( const AActor* Actor ) const
{
    if ( !Actor ) {
        return FString();
    }

    const FString AssetName = Actor->GetName() + TEXT("Thumbnail");

    return FString::Printf(TEXT("/Game/ActorThumbnails/%s"), *AssetName);
}
