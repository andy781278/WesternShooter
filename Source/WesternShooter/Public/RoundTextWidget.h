#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTextWidget.generated.h"

class UTextBlock;

UCLASS()
class WESTERNSHOOTER_API URoundTextWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// The Widget Blueprint must contain a Text widget named RoundText.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RoundText;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// Last text set, so the Text widget is only updated when the message changes.
	FString CachedText;
};