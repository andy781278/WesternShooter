#include "LobbySubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// Lobby attribute that holds the join code. The plugin turns it into a public lobby attribute.
static const FName JoinCodeKey(TEXT("JOINCODE"));

void ULobbySubsystem::Deinitialize()
{
	LeaveLobby();
	Super::Deinitialize();
}

IOnlineSessionPtr ULobbySubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetGameInstance()->GetWorld());
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

FString ULobbySubsystem::MakeJoinCode()
{
	// No 0/O or 1/I, so codes are easy to read and type
	static const TCHAR* Alphabet = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
	const int32 AlphabetLength = FCString::Strlen(Alphabet);

	FString Code;
	for (int32 i = 0; i < JoinCodeLength; ++i)
	{
		Code.AppendChar(Alphabet[FMath::RandRange(0, AlphabetLength - 1)]);
	}
	return Code;
}

void ULobbySubsystem::HostLobby(const TSoftObjectPtr<UWorld>& GameMap)
{
	if (bBusy)
	{
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		FailHost(TEXT("No session interface"));
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		FailHost(TEXT("Already in a lobby"));
		return;
	}

	PendingMapPath = GameMap.GetLongPackageName();
	if (PendingMapPath.IsEmpty())
	{
		FailHost(TEXT("No game map set"));
		return;
	}

	CurrentJoinCode = MakeJoinCode();

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = MaxPlayers;   // > 0 makes the lobby searchable
	Settings.bShouldAdvertise = true;
	Settings.bIsLANMatch = false;
	Settings.bUseLobbiesIfAvailable = true;        // EOS lobby, not EOS session
	Settings.bUsesPresence = false;
	Settings.bAllowJoinViaPresence = false;
	Settings.bAllowJoinInProgress = true;
	Settings.Set(SETTING_HOST_MIGRATION, false, EOnlineDataAdvertisementType::DontAdvertise); // lobby closes when host leaves
	Settings.Set(JoinCodeKey, CurrentJoinCode, EOnlineDataAdvertisementType::ViaOnlineService);

	bBusy = true;
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ULobbySubsystem::HandleCreateComplete));
	Sessions->CreateSession(0, NAME_GameSession, Settings);
}

void ULobbySubsystem::HandleCreateComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	bBusy = false;

	if (!bWasSuccessful)
	{
		CurrentJoinCode.Reset();
		FailHost(TEXT("Could not create lobby"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Lobby created. Join code: %s"), *CurrentJoinCode);
	OnHostFinished.Broadcast(true, CurrentJoinCode);

	// Host becomes the listen server; NetDriverEOS accepts P2P connections
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*PendingMapPath), true, TEXT("listen"));
}

void ULobbySubsystem::JoinLobby(const FString& JoinCode)
{
	if (bBusy)
	{
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		FailJoin(TEXT("No session interface"));
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		FailJoin(TEXT("Already in a lobby"));
		return;
	}

	const FString Code = JoinCode.TrimStartAndEnd().ToUpper();
	if (Code.Len() != JoinCodeLength)
	{
		FailJoin(TEXT("Code must be 4 characters"));
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 5;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);  // search lobbies, not sessions
	SessionSearch->QuerySettings.Set(JoinCodeKey, Code, EOnlineComparisonOp::Equals);     // filter by code

	CurrentJoinCode = Code;
	bBusy = true;
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ULobbySubsystem::HandleFindComplete));

	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		bBusy = false;
		FailJoin(TEXT("Search could not start"));
	}
}

void ULobbySubsystem::HandleFindComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		bBusy = false;
		FailJoin(TEXT("No session interface"));
		return;
	}
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);

	if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		bBusy = false;
		FailJoin(TEXT("No lobby with that code"));
		return;
	}

	FOnlineSessionSearchResult Result = SessionSearch->SearchResults[0];
	Result.Session.SessionSettings.bUsesPresence = false; // must match the host's setting

	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ULobbySubsystem::HandleJoinComplete));
	Sessions->JoinSession(0, NAME_GameSession, Result);
}

void ULobbySubsystem::HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	bBusy = false;

	if (!Sessions.IsValid() || Result != EOnJoinSessionCompleteResult::Success)
	{
		FailJoin(TEXT("Could not join lobby"));
		return;
	}

	// Host's EOS P2P address
	FString ConnectString;
	if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		LeaveLobby();
		FailJoin(TEXT("Could not get host address"));
		return;
	}

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		LeaveLobby();
		FailJoin(TEXT("No local player controller"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Joined lobby. Travelling to %s"), *ConnectString);
	OnJoinFinished.Broadcast(true, FString());
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void ULobbySubsystem::LeaveLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
	}
	CurrentJoinCode.Reset();
}

void ULobbySubsystem::FailHost(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("Host failed: %s"), *Error);
	OnHostFinished.Broadcast(false, Error);
}

void ULobbySubsystem::FailJoin(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("Join failed: %s"), *Error);
	CurrentJoinCode.Reset();
	OnJoinFinished.Broadcast(false, Error);
}