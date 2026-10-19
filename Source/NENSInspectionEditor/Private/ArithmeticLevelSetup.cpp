#include "InspectionEditorLibrary.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/Blueprint.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace InspectionSetup { FString Fail(const FString&); }

FString UInspectionEditorLibrary::ConnectArithmeticLevel(const FString& AssetPath)
{
    auto* World=LoadObject<UWorld>(nullptr,*AssetPath);
    if (!World || !World->PersistentLevel) return InspectionSetup::Fail(TEXT("Map missing"));
    const TCHAR* SpritePaths[]={
        TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_Electro_Station.SPR_Props_Electro_Station"),
        TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_DataComputer.SPR_Props_DataComputer")};
    const TCHAR* BPPaths[]={
        TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_ElectroStation.BP_Props_ElectroStation"),
        TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_DataComputer.BP_Props_DataComputer")};
    int32 Replaced=0;
    const TArray<TObjectPtr<AActor>> OriginalActors=World->PersistentLevel->Actors;
    for (int32 I=0;I<2;++I)
    {
        auto* Sprite=LoadObject<UPaperSprite>(nullptr,SpritePaths[I]);
        auto* BP=LoadObject<UBlueprint>(nullptr,BPPaths[I]);
        if (!Sprite || !BP || !BP->GeneratedClass) return InspectionSetup::Fail(TEXT("Interaction assets missing"));
        for (AActor* Original : OriginalActors)
        {
            auto* Old=Cast<APaperSpriteActor>(Original);
            if (!Old || Old->GetRenderComponent()->GetSprite()!=Sprite) continue;
            auto* Source=Old->GetRenderComponent(); Source->UpdateComponentToWorld();
            const FTransform VisibleTransform=Source->GetComponentTransform();
            FActorSpawnParameters Params; Params.OverrideLevel=World->PersistentLevel;
            Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* Actor=World->SpawnActor<AActor>(BP->GeneratedClass,VisibleTransform.GetLocation(),FRotator::ZeroRotator,Params);
            auto* Target=Actor ? Actor->FindComponentByClass<UPaperSpriteComponent>() : nullptr;
            if (!Target) return InspectionSetup::Fail(TEXT("Could not spawn interaction actor"));
            Target->SetWorldTransform(VisibleTransform);
            Target->SetSpriteColor(Source->GetSpriteColor());
            Target->SetTranslucentSortPriority(Source->TranslucencySortPriority);
            for (int32 M=0;M<Source->GetNumMaterials();++M) Target->SetMaterial(M,Source->GetMaterial(M));
            if (Target->GetSprite()!=Sprite || !Target->GetComponentTransform().Equals(VisibleTransform,.001f))
                return InspectionSetup::Fail(TEXT("Visual transform changed; map not saved"));
            Actor->SetActorLabel(Old->GetActorLabel()+TEXT("_Interactive")); Actor->Tags=Old->Tags;
            Actor->SetFolderPath(Old->GetFolderPath());
            if (!World->EditorDestroyActor(Old,false)) return InspectionSetup::Fail(TEXT("Could not replace decorative prop"));
            ++Replaced;
        }
    }
    if (Replaced)
    {
        UPackage* Package=World->GetOutermost(); Package->MarkPackageDirty();
        const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetMapPackageExtension());
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        if (!UPackage::SavePackage(Package,World,*Filename,Args)) return InspectionSetup::Fail(TEXT("Could not save map"));
    }
    return FString::Printf(TEXT("{\"success\":true,\"replaced\":%d}"),Replaced);
}
