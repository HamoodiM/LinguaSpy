// LinguaSpy - local learning log for the pilot study (JSON lines on disk; no network code at all)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "LearningTelemetrySubsystem.generated.h"

class ULinguaSpySaveSubsystem;

/** Fluent builder for one telemetry event: FTelemetryEvent(TEXT("challenge")).Str(TEXT("skill"), ...).Int(...). */
struct PERSONAL_API FTelemetryEvent
{
	explicit FTelemetryEvent(const FString& InName) : Name(InName), Fields(MakeShared<FJsonObject>()) {}

	FTelemetryEvent& Str(const FString& Key, const FString& Value) { Fields->SetStringField(Key, Value); return *this; }
	FTelemetryEvent& Id(const FString& Key, FName Value) { Fields->SetStringField(Key, Value.ToString()); return *this; }
	FTelemetryEvent& Int(const FString& Key, int64 Value) { Fields->SetNumberField(Key, static_cast<double>(Value)); return *this; }
	FTelemetryEvent& Num(const FString& Key, double Value) { Fields->SetNumberField(Key, Value); return *this; }
	FTelemetryEvent& Bool(const FString& Key, bool Value) { Fields->SetBoolField(Key, Value); return *this; }

	FString Name;
	TSharedRef<FJsonObject> Fields;
};

/**
 *  Appends one JSON object per line to Saved/Telemetry/<participant or "anon">_<session>.jsonl.
 *  Off unless the player enables it in Settings or the game runs in Pilot mode (-PilotMode or Saved/pilot.ini),
 *  which also turns it on. Only anonymous gameplay data: no names, no free text typed by the player except
 *  answers to language challenges. Event list: docs/Pilot_Study_Protocol.md.
 */
UCLASS()
class PERSONAL_API ULearningTelemetrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Writes the event if logging is enabled. Safe to call from anywhere with a world context. */
	void Log(const FTelemetryEvent& Event);

	/** Python/Blueprint helper: an event with one string field. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Telemetry")
	void LogSimple(const FString& Event, const FString& Key, const FString& Value);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Telemetry")
	bool IsEnabled() const;

	/** Launched with -PilotMode or with Saved/pilot.ini present. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Telemetry")
	bool IsPilotMode() const { return bPilotMode; }

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Telemetry")
	FString GetLogDirectory() const;

	/** Path of this session's file (it may not exist yet if nothing was logged). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Telemetry")
	FString GetSessionFile() const;

	/** Opens the log folder in the file explorer (the "Export" button). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Telemetry")
	void OpenLogFolder() const;

	/** Shorthand used by gameplay code. */
	static void LogEvent(const UObject* WorldContext, const FTelemetryEvent& Event);

	/** Serializes an event to one JSON line (exposed for tests). */
	static FString FormatLine(const FTelemetryEvent& Event, const FString& SessionId, const FString& Participant, const FDateTime& UtcNow, int64 SessionMs);

private:

	void WriteLine(const FString& Line);

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;

	FString SessionId;
	FDateTime SessionStart;
	bool bPilotMode = false;
	bool bSessionStartLogged = false;
};
