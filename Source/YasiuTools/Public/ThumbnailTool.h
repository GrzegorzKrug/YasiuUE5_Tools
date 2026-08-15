#pragma once


#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"


#include "ThumbnailTool.generated.h"


// class ABasePassiveBuilding;
class UTextureRenderTarget2D;


UCLASS(Blueprintable, BlueprintType)
class YASIUTOOLS_API UCityThumbnailGenerator : public UEditorUtilityWidget {
    GENERATED_BODY()


public:

    /**
     * Thumbnail resolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thumbnail")
    int32 Resolution = 512;

    /**
     * Generate / overwrite the UTexture2D asset.
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Thumbnail")
    virtual void GenerateThumbnail( AActor* Actor, const FVector& Location, const FRotator& Rotation );


private:
    UPROPERTY()
    TObjectPtr<AActor> SelectedActor;
    
    /**
     * Render the actor into a temporary render target.
     */
    UTextureRenderTarget2D* RenderActor( AActor* Actor, const FVector& Location, const FRotator& Rotation );

    /**
     * Create or overwrite the UTexture2D asset.
     */
    UTexture2D* CreateTextureAsset( AActor* Actor, const TArray<FColor>& Pixels, int32 Width, int32 Height );

    /**
     * Package path for the generated texture.
     */
    FString GetAssetPackageName( const AActor* Actor ) const;
};
