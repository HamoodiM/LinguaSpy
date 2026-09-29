// LinguaSpy - Conversation Editor tab

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "AssetRegistry/AssetData.h"

class UConversationAsset;
class IDetailsView;
class IStructureDetailsView;
class FStructOnScope;
class SMultiLineEditableTextBox;
class SEditableTextBox;
template <typename OptionType> class SComboBox;

/** One row in the node flow list. */
struct FConversationNodeRow
{
	int32 Index = INDEX_NONE;
	int32 Depth = 0;
	bool bReachable = true;
};

/**
 *  Authoring tool for UConversationAsset:
 *  - left: all conversations (filterable), New / Validate all
 *  - middle: asset settings, node flow list (indented by distance from the start node), add/delete/reorder,
 *    validation messages
 *  - right: node details (standard property editor, undoable) + quick tile generator + Play from node
 */
class SConversationEditor : public SCompoundWidget, public FNotifyHook, public FSelfRegisteringEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SConversationEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SConversationEditor() override;

	// FNotifyHook (node struct edits)
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override { RefreshAll(); }
	virtual void PostRedo(bool bSuccess) override { RefreshAll(); }

private:

	// ---- Conversation list
	void RefreshConversationList();
	TSharedRef<ITableRow> GenerateConversationRow(TSharedPtr<FAssetData> Item, const TSharedRef<STableViewBase>& Owner);
	void OnConversationSelected(TSharedPtr<FAssetData> Item, ESelectInfo::Type SelectInfo);
	FReply OnNewConversation();
	FReply OnValidateAll();

	// ---- Node list
	void RefreshNodes();
	TSharedRef<ITableRow> GenerateNodeRow(TSharedPtr<FConversationNodeRow> Row, const TSharedRef<STableViewBase>& Owner);
	void OnNodeSelected(TSharedPtr<FConversationNodeRow> Row, ESelectInfo::Type SelectInfo);
	FReply OnAddNode(int32 TypeIndex);
	FReply OnDeleteNode();
	FReply OnMoveNode(int32 Direction);
	FReply OnSetStartNode();
	FReply OnPlayFromNode();

	// ---- Node details + quick tiles
	void RebuildNodeDetails();
	void RefreshQuickTargets();
	void LoadQuickTilesFromChallenge();
	FReply OnApplyQuickTiles();
	struct FWordTileChallenge* GetQuickTargetChallenge() const;

	// ---- Validation
	void RefreshValidation();

	void RefreshAll();
	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	void MarkEdited();

	UConversationAsset* GetConversation() const { return Conversation.Get(); }

	TWeakObjectPtr<UConversationAsset> Conversation;
	int32 SelectedNode = INDEX_NONE;

	FString ConversationFilter;
	TArray<TSharedPtr<FAssetData>> AllConversations;
	TArray<TSharedPtr<FAssetData>> ConversationItems;
	TSharedPtr<SListView<TSharedPtr<FAssetData>>> ConversationList;

	TArray<TSharedPtr<FConversationNodeRow>> NodeRows;
	TSharedPtr<SListView<TSharedPtr<FConversationNodeRow>>> NodeList;

	TSharedPtr<IDetailsView> AssetDetails;
	TSharedPtr<IStructureDetailsView> NodeDetails;
	TSharedPtr<FStructOnScope> NodeStruct;

	TArray<TSharedPtr<FString>> QuickTargets;
	TSharedPtr<FString> SelectedQuickTarget;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> QuickTargetCombo;
	TSharedPtr<SMultiLineEditableTextBox> AnswersBox;
	TSharedPtr<SEditableTextBox> DistractorsBox;
	TSharedPtr<SMultiLineEditableTextBox> ReadingsBox;
	TSharedPtr<SEditableTextBox> IntentBox;
	TSharedPtr<SEditableTextBox> SkillBox;

	TArray<TSharedPtr<FString>> Problems;
	TSharedPtr<SListView<TSharedPtr<FString>>> ProblemList;

	bool bInTransaction = false;
	FDelegateHandle ObjectChangedHandle;
	FDelegateHandle PostPIEHandle;
};
