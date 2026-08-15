#include "YasiuAssetActions.h"


#include "AssetSelection.h"
#include "YasiuToolsLogs.h"


#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Factories/Texture2DFactoryNew.h"
#include "IAssetTools.h"

#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

#include "ContentBrowserMenuContexts.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"


void UYasiuThumbnailAction::PipeStart()
{
    TArray<FAssetData> Selection;
    AssetSelectionUtils::GetSelectedAssets(Selection);

    UE_LOGFMT(YasiuToolsPlugin, Log, "");
    UE_LOGFMT(YasiuToolsPlugin, Log, "Image generation start");
    for ( const FAssetData item : Selection ) {
        /**/
        auto* InAsset = item.GetAsset();

        auto* BP = Cast<UBlueprint>(InAsset);
        if ( !BP ) {
            UE_LOGFMT(YasiuToolsPlugin, Log, "Skipping invalid asset. ITS NOT a BP!");
            continue;
        }
        const auto* act = BP->GeneratedClass->GetDefaultObject<AActor>();

        if ( act ) {
            ReadCameraOffset(act);
        }
        else {
            if ( BP ) {
                UE_LOGFMT(YasiuToolsPlugin, Log, "Skipping selected asset: {0}", BP->GetFName());
            }
            else {
                UE_LOGFMT(YasiuToolsPlugin, Log, "Skipping invalid asset.");
            }
        }
    }
}

void UYasiuThumbnailAction::ReadCameraOffset_Implementation( const AActor* Actor )
{
    const FTransform None;
    ProcessActor(Actor, None);
}

void UYasiuThumbnailAction::ProcessActor( const AActor* Actor, const FTransform& CameraOffset )
{
    if ( !IsValid(Actor) ) {
        UE_LOGFMT(YasiuToolsPlugin, Warning, "Skipping actor, not valid pointer");
        return;
    }

    // const FString ActorName = Actor->GetPathName();
    const FString DefaultName = Actor->GetClass()->GetName();
    const FString TextureFile = DefaultName + TEXT("_Thumbnail");
    const FString FolderPath = FPaths::GetPath(Actor->GetClass()->GetPathName());

    // FString PackagePath;
    UE_LOGFMT(YasiuToolsPlugin, Log, "Class Path: {0}", DefaultName);
    UE_LOGFMT(YasiuToolsPlugin, Log, "TextName: {0}, Texture path: {1}", TextureFile, FolderPath);
    // const FString TextureAssetPath = FolderPath / TextureFile;
    FSoftObjectPath AssetSoftPath(FolderPath / TextureFile + "." + TextureFile);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    FAssetData ExistingAsset = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetSoftPath));

    auto* some = ExistingAsset.GetAsset();
    UTexture2D* Texture = Cast<UTexture2D>(some);

    if ( !ExistingAsset.IsValid() ) {
        UE_LOGFMT(YasiuToolsPlugin, Log, "File does not exists!");
        UTexture2DFactoryNew* Factory = NewObject<UTexture2DFactoryNew>();
        if ( !Factory ) {
            UE_LOGFMT(YasiuToolsPlugin, Log, "Invalid factory!");
            return;
        }

        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        IAssetTools& AssetTools = AssetToolsModule.Get();
        UE_LOGFMT(YasiuToolsPlugin, Log, "Creating asset for: {0}", AssetSoftPath);
        UObject* NewAsset = AssetTools.CreateAsset(
            TextureFile,
            FolderPath,
            UTexture2D::StaticClass(),
            Factory,
            FName(TEXT("YasiuThumbnailAction"))
        );

        Texture = Cast<UTexture2D>(NewAsset);
    }
    else {
        // auto TempPath = FolderPath + "/" + TextureFile + "." + TextureFile;
        UE_LOGFMT(YasiuToolsPlugin, Log, "Loading texture: {0}", AssetSoftPath);
        Texture = Cast<UTexture2D>(FSoftObjectPath(AssetSoftPath).TryLoad());
    }

    if ( !Texture ) {
        UE_LOGFMT(YasiuToolsPlugin, Warning, "Invalid texture pointer for {0}", DefaultName);
        return;
    }

    return;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, GetTransientPackage());

    if ( !World )
        return;

    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
    WorldContext.SetCurrentWorld(World);

    World->InitializeNewWorld(
        UWorld::InitializationValues().RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false).
                                       AllowAudioPlayback(false)
    );

    World->BeginPlay();

    // Duplicate actor into preview world

    // Better: duplicate the actor directly
    auto* PreviewActor = DuplicateObject<AActor>(Actor, World);

    if ( !PreviewActor )
        return;

    PreviewActor->SetFlags(RF_Transient);
    PreviewActor->SetActorTransform(FTransform::Identity);
    PreviewActor->RegisterAllComponents();

    // ---------------------------------------------------------
    // Render target
    // ---------------------------------------------------------

    constexpr int32 Size = 512;

    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->RenderTargetFormat = RTF_RGBA8;
    RenderTarget->InitAutoFormat(Size, Size);
    RenderTarget->UpdateResourceImmediate(true);

    // ---------------------------------------------------------
    // Camera
    // ---------------------------------------------------------

    ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), CameraOffset);

    if ( !CaptureActor )
        return;

    USceneCaptureComponent2D* Capture = CaptureActor->GetCaptureComponent2D();

    Capture->TextureTarget = RenderTarget;
    Capture->CaptureSource = SCS_FinalColorLDR;

    // ---------------------------------------------------------
    // Light
    // ---------------------------------------------------------

    ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass());

    if ( Light ) {
        Light->SetActorRotation(FRotator(-45.f, -45.f, 0.f));

        Light->GetLightComponent()->SetIntensity(5.f);
    }

    Capture->CaptureScene();
}
