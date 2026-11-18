#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "EOSLoginSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginFinished, bool, bWasSuccessful, const FString&, Error);

UCLASS()
class WESTERNSHOOTER_API UEOSLoginSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EOS")
	void Login();

	UFUNCTION(BlueprintPure, Category = "EOS")
	bool IsLoggedIn() const;

	UPROPERTY(BlueprintAssignable, Category = "EOS")
	FOnEOSLoginFinished OnLoginFinished;

private:
	bool bLoginInProgress = false;
	FDelegateHandle LoginCompleteHandle;

	IOnlineIdentityPtr GetIdentity() const;
	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void FinishLogin(bool bWasSuccessful, const FString& Error);
};