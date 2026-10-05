// LinguaSpy - local learning log for the pilot study (JSON lines on disk; no network code at all)

#include "LearningTelemetrySubsystem.h"
#include "LinguaSpySaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Personal.h"

void ULearningTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();

	SessionStart = FDateTime::UtcNow();
	SessionId = SessionStart.ToString(TEXT("%Y%m%d-%H%M%S"));
	bPilotMode = FParse::Param(FCommandLine::Get(), TEXT("PilotMode"))
		|| FPaths::FileExists(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("pilot.ini")));

	if (bPilotMode && SaveSubsystem && !SaveSubsystem->GetSettings().bTelemetryEnabled)
	{
		SaveSubsystem->SetTelemetryEnabled(true);
	}
	UE_LOG(LogPersonal, Log, TEXT("Learning telemetry: %s%s"), IsEnabled() ? TEXT("on") : TEXT("off"), bPilotMode ? TEXT(" (pilot mode)") : TEXT(""));
}

void ULearningTelemetrySubsystem::Deinitialize()
{
	if (bSessionStartLogged)
	{
		Log(FTelemetryEvent(TEXT("session_end")).Int(TEXT("duration_s"), (FDateTime::UtcNow() - SessionStart).GetTotalSeconds()));
	}
	Super::Deinitialize();
}

bool ULearningTelemetrySubsystem::IsEnabled() const
{
	return SaveSubsystem && SaveSubsystem->GetSave() && SaveSubsystem->GetSettings().bTelemetryEnabled;
}

FString ULearningTelemetrySubsystem::GetLogDirectory() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Telemetry")));
}

FString ULearningTelemetrySubsystem::GetSessionFile() const
{
	FString Participant = SaveSubsystem ? SaveSubsystem->GetParticipantCode() : FString();
	if (Participant.IsEmpty())
	{
		Participant = TEXT("anon");
	}
	return FPaths::Combine(GetLogDirectory(), FString::Printf(TEXT("%s_%s.jsonl"), *Participant, *SessionId));
}

void ULearningTelemetrySubsystem::OpenLogFolder() const
{
	IFileManager::Get().MakeDirectory(*GetLogDirectory(), true);
	FPlatformProcess::ExploreFolder(*GetLogDirectory());
}

FString ULearningTelemetrySubsystem::FormatLine(const FTelemetryEvent& Event, const FString& InSessionId, const FString& Participant, const FDateTime& UtcNow, int64 SessionMs)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("t"), UtcNow.ToIso8601());
	Root->SetNumberField(TEXT("t_ms"), static_cast<double>(SessionMs));
	Root->SetStringField(TEXT("session"), InSessionId);
	Root->SetStringField(TEXT("participant"), Participant.IsEmpty() ? TEXT("anon") : Participant);
	Root->SetStringField(TEXT("event"), Event.Name);
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Event.Fields->Values)
	{
		Root->SetField(Field.Key, Field.Value);
	}

	FString Line;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Line);
	FJsonSerializer::Serialize(Root, Writer);
	return Line;
}

void ULearningTelemetrySubsystem::Log(const FTelemetryEvent& Event)
{
	if (!IsEnabled())
	{
		return;
	}
	const FDateTime Now = FDateTime::UtcNow();
	const FString Participant = SaveSubsystem->GetParticipantCode();
	if (!bSessionStartLogged && Event.Name != TEXT("session_start"))
	{
		bSessionStartLogged = true;
		WriteLine(FormatLine(FTelemetryEvent(TEXT("session_start")).Bool(TEXT("pilot"), bPilotMode).Str(TEXT("build"), TEXT(__DATE__)),
			SessionId, Participant, Now, 0));
	}
	WriteLine(FormatLine(Event, SessionId, Participant, Now, static_cast<int64>((Now - SessionStart).GetTotalMilliseconds())));
}

void ULearningTelemetrySubsystem::LogSimple(const FString& Event, const FString& Key, const FString& Value)
{
	Log(FTelemetryEvent(Event).Str(Key, Value));
}

void ULearningTelemetrySubsystem::WriteLine(const FString& Line)
{
	const FString Path = GetSessionFile();
	if (!FFileHelper::SaveStringToFile(Line + TEXT("\n"), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append))
	{
		UE_LOG(LogPersonal, Warning, TEXT("Telemetry: could not write %s"), *Path);
	}
}

void ULearningTelemetrySubsystem::LogEvent(const UObject* WorldContext, const FTelemetryEvent& Event)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (ULearningTelemetrySubsystem* Telemetry = GameInstance ? GameInstance->GetSubsystem<ULearningTelemetrySubsystem>() : nullptr)
	{
		Telemetry->Log(Event);
	}
}
