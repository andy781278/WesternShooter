#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "LobbySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyHostFinished, bool, bWasSuccessful, const FString&, JoinCodeOrError);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyJoinFinished, bool, bWasSuccessful, const FString&, Error);

UCLASS()
class WESTERNSHOOTER_API ULobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// Creates a lobby with a random code, then opens GameMap as a listen server
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HostLobby(const TSoftObjectPtr<UWorld>& GameMap);

	// Finds the lobby with this code, joins it, then travels to the host
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void JoinLobby(const FString& JoinCode);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void LeaveLobby();

	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString GetJoinCode() const { return CurrentJoinCode; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsBusy() const { return bBusy; }

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyHostFinished OnHostFinished;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyJoinFinished OnJoinFinished;

private:
	static constexpr int32 MaxPlayers = 6;
	static constexpr int32 JoinCodeLength = 4;

	bool bBusy = false;
	FString CurrentJoinCode;
	FString PendingMapPath;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;

	IOnlineSessionPtr GetSessionInterface() const;
	static FString MakeJoinCode();

	void HandleCreateComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindComplete(bool bWasSuccessful);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void FailHost(const FString& Error);
	void FailJoin(const FString& Error);
};