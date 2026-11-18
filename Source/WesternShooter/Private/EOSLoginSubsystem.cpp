#include "EOSLoginSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

IOnlineIdentityPtr UEOSLoginSubsystem::GetIdentity() const
{
	// Online::GetSubsystem with a world gives each PIE window its own subsystem instance
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetGameInstance()->GetWorld());
	return Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
}

void UEOSLoginSubsystem::Login()
{
	if (bLoginInProgress || IsLoggedIn())
	{
		return;
	}

	IOnlineIdentityPtr Identity = GetIdentity();
	if (!Identity.IsValid())
	{
		FinishLogin(false, TEXT("No identity interface"));
		return;
	}

	bLoginInProgress = true;
	LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSLoginSubsystem::HandleLoginComplete));

	// AutoLogin order: AUTH_TYPE/AUTH_LOGIN/AUTH_PASSWORD from the command line if present,
	// otherwise the saved Epic token, otherwise a browser sign-in (account portal)
	if (!Identity->AutoLogin(0))
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
		FinishLogin(false, TEXT("AutoLogin could not start"));
	}
}

bool UEOSLoginSubsystem::IsLoggedIn() const
{
	IOnlineIdentityPtr Identity = GetIdentity();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UEOSLoginSubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (IOnlineIdentityPtr Identity = GetIdentity())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("EOS login succeeded. User id: %s"), *UserId.ToString());
	}
	FinishLogin(bWasSuccessful, Error);
}

void UEOSLoginSubsystem::FinishLogin(bool bWasSuccessful, const FString& Error)
{
	bLoginInProgress = false;
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("EOS login failed: %s"), *Error);
	}
	OnLoginFinished.Broadcast(bWasSuccessful, Error);
}