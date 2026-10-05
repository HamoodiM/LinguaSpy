// LinguaSpy editor module: registers the Conversation Editor tab

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ToolMenus.h"
#include "SConversationEditor.h"

#define LOCTEXT_NAMESPACE "PersonalEditor"

namespace LinguaSpyEditor
{
	static const FName ConversationEditorTabId(TEXT("LinguaSpyConversationEditor"));
}

class FPersonalEditorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LinguaSpyEditor::ConversationEditorTabId,
			FOnSpawnTab::CreateRaw(this, &FPersonalEditorModule::SpawnConversationEditor))
			.SetDisplayName(LOCTEXT("TabTitle", "LinguaSpy Conversation Editor"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Author and validate LinguaSpy conversations"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPersonalEditorModule::RegisterMenus));

		// For automation / screenshots: "LinguaSpy.OpenConversationEditor" in the console
		OpenCommand = IConsoleManager::Get().RegisterConsoleCommand(TEXT("LinguaSpy.OpenConversationEditor"),
			TEXT("Opens the LinguaSpy Conversation Editor tab"),
			FConsoleCommandDelegate::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(LinguaSpyEditor::ConversationEditorTabId);
			}));
	}

	virtual void ShutdownModule() override
	{
		if (OpenCommand)
		{
			IConsoleManager::Get().UnregisterConsoleObject(OpenCommand);
			OpenCommand = nullptr;
		}
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LinguaSpyEditor::ConversationEditorTabId);
		}
	}

private:

	IConsoleObject* OpenCommand = nullptr;

	TSharedRef<SDockTab> SpawnConversationEditor(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SConversationEditor)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = Menu->FindOrAddSection("LinguaSpy", LOCTEXT("LinguaSpySection", "LinguaSpy"));
		Section.AddMenuEntry(
			"OpenConversationEditor",
			LOCTEXT("OpenConversationEditor", "Conversation Editor"),
			LOCTEXT("OpenConversationEditorTooltip", "Author and validate LinguaSpy conversations"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(LinguaSpyEditor::ConversationEditorTabId);
			})));
	}
};

IMPLEMENT_MODULE(FPersonalEditorModule, PersonalEditor)

#undef LOCTEXT_NAMESPACE
