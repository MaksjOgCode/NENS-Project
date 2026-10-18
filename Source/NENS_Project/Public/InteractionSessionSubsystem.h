#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "InteractionSessionSubsystem.generated.h"
class UUserWidget;
class APlayerController;

/** Shared modal ownership/input for inspections and widget minigames. */
UCLASS()
class NENS_PROJECT_API UInteractionSessionSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    bool Open(UUserWidget* Widget, AActor* Owner, FSimpleDelegate Closed);
    void Close(UUserWidget* ExpectedWidget);
    UFUNCTION(BlueprintPure, Category="Interaction|Session") bool IsOpen() const { return ActiveWidget != nullptr || bClosing; }
    bool IsActive(const UUserWidget* Widget) const { return Widget && ActiveWidget == Widget; }
    UUserWidget* GetActiveWidget() const { return ActiveWidget; }
    virtual void Deinitialize() override;
private:
    UPROPERTY(Transient) TObjectPtr<UUserWidget> ActiveWidget;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> ActiveOwner;
    UPROPERTY(Transient) TWeakObjectPtr<APlayerController> SessionController;
    FSimpleDelegate OnClosed;
    bool bPreviousCursor = false;
    bool bClosing = false;
    UFUNCTION() void OwnerEnded(AActor* Actor, EEndPlayReason::Type Reason);
};
