#pragma once


#include "CoreMinimal.h"
#include "AssetActionUtility.h"


#include "YasiuAssetActions.generated.h"


UCLASS(Blueprintable, BlueprintType)
class YASIUTOOLS_API UYasiuThumbnailAction : public UAssetActionUtility {
    GENERATED_BODY()


public:
    UFUNCTION(BlueprintCallable, Category = "Thumbnail")
    void PipeStart();
    
    UFUNCTION(BlueprintNativeEvent, Category = "Thumbnail")
    void ReadCameraOffset(const AActor* Actor);

protected:
    // UFUNCTION(Category = "Thumbnail")
    void ProcessActor( const AActor* Actor, const FTransform& CameraOffset );
};
