// LinguaSpy - Conversation Editor tab

#include "SConversationEditor.h"

#include "ConversationAsset.h"
#include "ConversationSubsystem.h"
#include "CoverComponent.h"
#include "LanguagePackAsset.h"
#include "MissionAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "UObject/StructOnScope.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "LinguaSpyConversationEditor"

namespace
{
	const TCHAR* NodeTypeLabel(EConversationNodeType Type)
	{
		switch (Type)
		{
		case EConversationNodeType::NpcLine:         return TEXT("LINE");
		case EConversationNodeType::PlayerChallenge: return TEXT("REPLY");
		case EConversationNodeType::PlayerChoice:    return TEXT("CHOICE");
		case EConversationNodeType::End:             return TEXT("END");
		}
		return TEXT("?");
	}

	FLinearColor NodeTypeColor(EConversationNodeType Type)
	{
		switch (Type)
		{
		case EConversationNodeType::NpcLine:         return FLinearColor(0.55f, 0.75f, 0.95f);
		case EConversationNodeType::PlayerChallenge: return FLinearColor(0.95f, 0.78f, 0.35f);
		case EConversationNodeType::PlayerChoice:    return FLinearColor(0.85f, 0.55f, 0.95f);
		case EConversationNodeType::End:             return FLinearColor(0.6f, 0.9f, 0.6f);
		}
		return FLinearColor::White;
	}

	/** Splits a line into tokens: '|' if present (or required), otherwise whitespace. */
	TArray<FString> SplitTokens(const FString& Line, bool bUsesSpacing)
	{
		TArray<FString> Tokens;
		if (Line.Contains(TEXT("|")) || !bUsesSpacing)
		{
			Line.ParseIntoArray(Tokens, TEXT("|"), true);
		}
		else
		{
			Line.ParseIntoArrayWS(Tokens);
		}
		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
		}
		Tokens.RemoveAll([](const FString& Token) { return Token.IsEmpty(); });
		return Tokens;
	}

	TArray<FString> SplitLines(const FString& Text)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, true);
		for (FString& Line : Lines)
		{
			Line.TrimStartAndEndInline();
		}
		Lines.RemoveAll([](const FString& Line) { return Line.IsEmpty(); });
		return Lines;
	}
}

// =====================================================================================================================
// Construction
// =====================================================================================================================

void SConversationEditor::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Asset-level settings (everything except the Nodes array, which the flow list handles)
	FDetailsViewArgs AssetArgs;
	AssetArgs.bHideSelectionTip = true;
	AssetArgs.bAllowSearch = false;
	AssetArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	AssetDetails = PropertyEditor.CreateDetailView(AssetArgs);
	AssetDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent)
	{
		return PropertyAndParent.Property.GetFName() != GET_MEMBER_NAME_CHECKED(UConversationAsset, Nodes);
	}));

	// Node details (a view onto the selected FConversationNode inside the asset)
	FDetailsViewArgs NodeArgs;
	NodeArgs.bHideSelectionTip = true;
	NodeArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	NodeArgs.NotifyHook = this;
	FStructureDetailsViewArgs StructArgs;
	NodeDetails = PropertyEditor.CreateStructureDetailView(NodeArgs, StructArgs, TSharedPtr<FStructOnScope>());

	ObjectChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SConversationEditor::OnObjectPropertyChanged);

	auto SectionTitle = [](const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"));
	};

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)

		// ---- Left: conversations
		+ SSplitter::Slot().Value(0.2f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SectionTitle(LOCTEXT("Conversations", "Conversations"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(SSearchBox)
				.OnTextChanged_Lambda([this](const FText& Text)
				{
					ConversationFilter = Text.ToString();
					RefreshConversationList();
				})
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
			[
				SAssignNew(ConversationList, SListView<TSharedPtr<FAssetData>>)
				.ListItemsSource(&ConversationItems)
				.OnGenerateRow(this, &SConversationEditor::GenerateConversationRow)
				.OnSelectionChanged(this, &SConversationEditor::OnConversationSelected)
				.SelectionMode(ESelectionMode::Single)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(2.f)
				[
					SNew(SButton).Text(LOCTEXT("New", "New")).ToolTipText(LOCTEXT("NewTip", "Create a new conversation next to the selected one"))
					.OnClicked(this, &SConversationEditor::OnNewConversation)
				]
				+ SHorizontalBox::Slot().Padding(2.f)
				[
					SNew(SButton).Text(LOCTEXT("Refresh", "Refresh"))
					.OnClicked_Lambda([this]() { RefreshConversationList(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().Padding(2.f)
				[
					SNew(SButton).Text(LOCTEXT("ValidateAll", "Validate all")).ToolTipText(LOCTEXT("ValidateAllTip", "Validate every conversation and mission"))
					.OnClicked(this, &SConversationEditor::OnValidateAll)
				]
			]
		]

		// ---- Middle: settings, flow, validation
		+ SSplitter::Slot().Value(0.38f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot().Value(0.28f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					AssetDetails.ToSharedRef()
				]
			]
			+ SSplitter::Slot().Value(0.52f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SectionTitle(LOCTEXT("Flow", "Flow (indented by distance from the start node)"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SNew(SWrapBox)
					.UseAllottedSize(true)
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("AddLine", "+ Line")).OnClicked(this, &SConversationEditor::OnAddNode, 0) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("AddReply", "+ Reply")).OnClicked(this, &SConversationEditor::OnAddNode, 1) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("AddChoice", "+ Choice")).OnClicked(this, &SConversationEditor::OnAddNode, 2) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("AddEnd", "+ End")).OnClicked(this, &SConversationEditor::OnAddNode, 3) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("Up", "Up")).OnClicked(this, &SConversationEditor::OnMoveNode, -1) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("Down", "Down")).OnClicked(this, &SConversationEditor::OnMoveNode, 1) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("SetStart", "Set start")).OnClicked(this, &SConversationEditor::OnSetStartNode) ]
					+ SWrapBox::Slot().Padding(2.f)[ SNew(SButton).Text(LOCTEXT("Delete", "Delete")).OnClicked(this, &SConversationEditor::OnDeleteNode) ]
					+ SWrapBox::Slot().Padding(2.f)
					[
						SNew(SButton).Text(LOCTEXT("Play", "Play from node"))
						.ToolTipText(LOCTEXT("PlayTip", "Start Play-In-Editor in this conversation's city and jump straight to the selected node"))
						.OnClicked(this, &SConversationEditor::OnPlayFromNode)
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
				[
					SAssignNew(NodeList, SListView<TSharedPtr<FConversationNodeRow>>)
					.ListItemsSource(&NodeRows)
					.OnGenerateRow(this, &SConversationEditor::GenerateNodeRow)
					.OnSelectionChanged(this, &SConversationEditor::OnNodeSelected)
					.SelectionMode(ESelectionMode::Single)
				]
			]
			+ SSplitter::Slot().Value(0.2f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
				[
					SectionTitle(LOCTEXT("Validation", "Validation"))
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
				[
					SAssignNew(ProblemList, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&Problems)
					.OnGenerateRow_Lambda([](TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
					{
						return SNew(STableRow<TSharedPtr<FString>>, Owner)
						[
							SNew(STextBlock).Text(FText::FromString(*Item)).AutoWrapText(true)
						];
					})
				]
			]
		]

		// ---- Right: node details + quick tiles
		+ SSplitter::Slot().Value(0.42f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(4.f)
			[
				SectionTitle(LOCTEXT("Node", "Selected node"))
			]
			+ SScrollBox::Slot()
			[
				NodeDetails->GetWidget().ToSharedRef()
			]
			+ SScrollBox::Slot().Padding(4.f, 12.f, 4.f, 4.f)
			[
				SNew(SSeparator)
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SectionTitle(LOCTEXT("QuickTiles", "Quick tiles"))
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(STextBlock).AutoWrapText(true)
				.Text(LOCTEXT("QuickTilesHelp", "Type the accepted answers (one per line). Tokens are split on spaces, or on '|' "
					"(required for languages without spaces, e.g. コーヒー|を|ください). Keep elided words as one tile (s'il, d'acqua). "
					"Tiles are generated from the answers plus the distractors. Readings: one 'tile=reading' per line."))
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Target", "Challenge"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SAssignNew(QuickTargetCombo, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&QuickTargets)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget> { return SNew(STextBlock).Text(FText::FromString(*Item)); })
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
					{
						SelectedQuickTarget = Item;
						LoadQuickTilesFromChallenge();
					})
					[
						SNew(STextBlock).Text_Lambda([this]() { return SelectedQuickTarget.IsValid() ? FText::FromString(*SelectedQuickTarget) : LOCTEXT("NoChallenge", "(no challenge on this node)"); })
					]
				]
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Intent", "Intent")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SAssignNew(IntentBox, SEditableTextBox).HintText(LOCTEXT("IntentHint", "What the player should say, e.g. 'Politely order a coffee.'")) ]
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[ SNew(STextBlock).Text(LOCTEXT("Skill", "Skill")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SAssignNew(SkillBox, SEditableTextBox).HintText(LOCTEXT("SkillHint", "Greetings, Ordering, Directions...")) ]
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SBox).MinDesiredHeight(70.f)
				[
					SAssignNew(AnswersBox, SMultiLineEditableTextBox).HintText(LOCTEXT("AnswersHint", "Accepted answers, one per line"))
				]
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SAssignNew(DistractorsBox, SEditableTextBox).HintText(LOCTEXT("DistractorsHint", "Distractor tiles (space or | separated)"))
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SBox).MinDesiredHeight(50.f)
				[
					SAssignNew(ReadingsBox, SMultiLineEditableTextBox).HintText(LOCTEXT("ReadingsHint", "Optional readings, one 'tile=reading' per line"))
				]
			]
			+ SScrollBox::Slot().Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("Apply", "Apply to challenge")).OnClicked(this, &SConversationEditor::OnApplyQuickTiles)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
				[
					SNew(SButton).Text(LOCTEXT("Reload", "Reload from challenge"))
					.OnClicked_Lambda([this]() { LoadQuickTilesFromChallenge(); return FReply::Handled(); })
				]
			]
		]
	];

	RefreshConversationList();
}

SConversationEditor::~SConversationEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectChangedHandle);
	if (PostPIEHandle.IsValid())
	{
		FEditorDelegates::PostPIEStarted.Remove(PostPIEHandle);
	}
	if (bInTransaction && GEditor)
	{
		GEditor->EndTransaction();
	}
}

// =====================================================================================================================
// Conversation list
// =====================================================================================================================

void SConversationEditor::RefreshConversationList()
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UConversationAsset::StaticClass()->GetClassPathName(), Assets, true);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });

	ConversationItems.Reset();
	for (const FAssetData& Asset : Assets)
	{
		if (ConversationFilter.IsEmpty() || Asset.PackageName.ToString().Contains(ConversationFilter))
		{
			ConversationItems.Add(MakeShared<FAssetData>(Asset));
		}
	}
	if (ConversationList.IsValid())
	{
		ConversationList->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SConversationEditor::GenerateConversationRow(TSharedPtr<FAssetData> Item, const TSharedRef<STableViewBase>& Owner)
{
	// Show the language folder to tell cities apart
	FString Folder = FPaths::GetCleanFilename(FPaths::GetPath(Item->PackageName.ToString()));
	return SNew(STableRow<TSharedPtr<FAssetData>>, Owner)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(STextBlock).Text(FText::FromString(Folder)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(STextBlock).Text(FText::FromName(Item->AssetName))
		]
	];
}

void SConversationEditor::OnConversationSelected(TSharedPtr<FAssetData> Item, ESelectInfo::Type SelectInfo)
{
	Conversation = Item.IsValid() ? Cast<UConversationAsset>(Item->GetAsset()) : nullptr;
	SelectedNode = INDEX_NONE;
	AssetDetails->SetObject(Conversation.Get());
	RefreshAll();
}

FReply SConversationEditor::OnNewConversation()
{
	FString Folder = TEXT("/Game/LinguaSpy/Data");
	ULanguagePackAsset* Language = nullptr;
	if (UConversationAsset* Current = GetConversation())
	{
		Folder = FPaths::GetPath(Current->GetPathName());
		Language = Current->Language;
	}

	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString PackageName, AssetName;
	AssetTools.Get().CreateUniqueAssetName(Folder / TEXT("DA_Conv_New"), TEXT(""), PackageName, AssetName);

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UConversationAsset::StaticClass();
	UConversationAsset* NewConversation = Cast<UConversationAsset>(
		AssetTools.Get().CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UConversationAsset::StaticClass(), Factory));
	if (!NewConversation)
	{
		return FReply::Handled();
	}

	NewConversation->Title = LOCTEXT("NewTitle", "New conversation");
	NewConversation->Language = Language;
	NewConversation->StartNode = TEXT("start");
	FConversationNode Start;
	Start.NodeId = TEXT("start");
	Start.Type = EConversationNodeType::End;
	NewConversation->Nodes.Add(Start);
	NewConversation->MarkPackageDirty();

	RefreshConversationList();
	for (const TSharedPtr<FAssetData>& Item : ConversationItems)
	{
		if (Item->AssetName == NewConversation->GetFName())
		{
			ConversationList->SetSelection(Item);
			ConversationList->RequestScrollIntoView(Item);
		}
	}
	return FReply::Handled();
}

FReply SConversationEditor::OnValidateAll()
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Problems.Reset();
	int32 Checked = 0;

	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UConversationAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		if (const UConversationAsset* Loaded = Cast<UConversationAsset>(Asset.GetAsset()))
		{
			++Checked;
			for (const FText& Problem : Loaded->Validate())
			{
				Problems.Add(MakeShared<FString>(FString::Printf(TEXT("%s: %s"), *Asset.AssetName.ToString(), *Problem.ToString())));
			}
		}
	}

	Assets.Reset();
	Registry.GetAssetsByClass(UMissionAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		if (const UMissionAsset* Loaded = Cast<UMissionAsset>(Asset.GetAsset()))
		{
			++Checked;
			for (const FText& Problem : Loaded->Validate())
			{
				Problems.Add(MakeShared<FString>(FString::Printf(TEXT("%s: %s"), *Asset.AssetName.ToString(), *Problem.ToString())));
			}
		}
	}

	Problems.Insert(MakeShared<FString>(FString::Printf(TEXT("Validated %d assets: %d problem(s)"), Checked, Problems.Num())), 0);
	ProblemList->RequestListRefresh();
	return FReply::Handled();
}

// =====================================================================================================================
// Node list
// =====================================================================================================================

void SConversationEditor::RefreshNodes()
{
	NodeRows.Reset();
	const UConversationAsset* Current = GetConversation();
	if (Current)
	{
		// Breadth-first distance from the start node
		TMap<FName, int32> Depths;
		TArray<FName> Queue;
		if (Current->FindNode(Current->StartNode))
		{
			Depths.Add(Current->StartNode, 0);
			Queue.Add(Current->StartNode);
		}
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FConversationNode* Node = Current->FindNode(Queue[Head]);
			const int32 Depth = Depths[Queue[Head]];
			TArray<FName> Links = { Node->NextNode, Node->FailNode };
			for (const FConversationChoice& Choice : Node->Choices)
			{
				Links.Add(Choice.NextNode);
				Links.Add(Choice.FailNode);
			}
			for (FName Link : Links)
			{
				if (!Link.IsNone() && !Depths.Contains(Link) && Current->FindNode(Link))
				{
					Depths.Add(Link, Depth + 1);
					Queue.Add(Link);
				}
			}
		}

		for (int32 i = 0; i < Current->Nodes.Num(); ++i)
		{
			TSharedPtr<FConversationNodeRow> Row = MakeShared<FConversationNodeRow>();
			Row->Index = i;
			const int32* Depth = Depths.Find(Current->Nodes[i].NodeId);
			Row->bReachable = Depth != nullptr;
			Row->Depth = Depth ? FMath::Min(*Depth, 8) : 0;
			NodeRows.Add(Row);
		}
	}

	if (NodeList.IsValid())
	{
		NodeList->RequestListRefresh();
		if (NodeRows.IsValidIndex(SelectedNode))
		{
			NodeList->SetSelection(NodeRows[SelectedNode], ESelectInfo::Direct);
		}
	}
}

TSharedRef<ITableRow> SConversationEditor::GenerateNodeRow(TSharedPtr<FConversationNodeRow> Row, const TSharedRef<STableViewBase>& Owner)
{
	const UConversationAsset* Current = GetConversation();
	if (!Current || !Current->Nodes.IsValidIndex(Row->Index))
	{
		return SNew(STableRow<TSharedPtr<FConversationNodeRow>>, Owner);
	}
	const FConversationNode& Node = Current->Nodes[Row->Index];

	// Outgoing links as a compact summary
	TArray<FString> Links;
	if (!Node.NextNode.IsNone()) { Links.Add(FString::Printf(TEXT("-> %s"), *Node.NextNode.ToString())); }
	if (!Node.FailNode.IsNone()) { Links.Add(FString::Printf(TEXT("fail -> %s"), *Node.FailNode.ToString())); }
	for (int32 i = 0; i < Node.Choices.Num(); ++i)
	{
		Links.Add(FString::Printf(TEXT("[%d] -> %s"), i + 1, Node.Choices[i].NextNode.IsNone() ? TEXT("(end)") : *Node.Choices[i].NextNode.ToString()));
	}
	if (Node.Type == EConversationNodeType::End)
	{
		Links.Add(FString::Printf(TEXT("outcome: %s"), Node.Outcome.IsNone() ? TEXT("Completed") : *Node.Outcome.ToString()));
	}

	FString Title = Node.NodeId.ToString();
	if (Node.NodeId == Current->StartNode) { Title += TEXT("  (start)"); }
	if (!Row->bReachable) { Title += TEXT("  (unreachable)"); }

	return SNew(STableRow<TSharedPtr<FConversationNodeRow>>, Owner)
	.Padding(FMargin(4.f + Row->Depth * 16.f, 3.f, 4.f, 3.f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(SBox).WidthOverride(56.f)
			[
				SNew(STextBlock).Text(FText::FromString(NodeTypeLabel(Node.Type))).ColorAndOpacity(NodeTypeColor(Node.Type))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Title)).Font(FAppStyle::GetFontStyle("BoldFont"))
				.ColorAndOpacity(Row->bReachable ? FSlateColor::UseForeground() : FSlateColor(FLinearColor(1.f, 0.4f, 0.4f)))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Node.NativeLine).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(FString::Join(Links, TEXT("   ")))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
	];
}

void SConversationEditor::OnNodeSelected(TSharedPtr<FConversationNodeRow> Row, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo == ESelectInfo::Direct)
	{
		return; // programmatic re-selection after a refresh
	}
	SelectedNode = Row.IsValid() ? Row->Index : INDEX_NONE;
	RebuildNodeDetails();
	RefreshQuickTargets();
}

FReply SConversationEditor::OnAddNode(int32 TypeIndex)
{
	UConversationAsset* Current = GetConversation();
	if (!Current)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Conversation Node"));
	Current->Modify();

	FConversationNode Node;
	Node.Type = static_cast<EConversationNodeType>(TypeIndex);
	int32 Suffix = Current->Nodes.Num() + 1;
	do
	{
		Node.NodeId = FName(*FString::Printf(TEXT("node_%d"), Suffix++));
	}
	while (Current->FindNode(Node.NodeId));

	if (Node.Type == EConversationNodeType::PlayerChoice)
	{
		Node.Choices.AddDefaulted(2);
	}

	const int32 InsertAt = Current->Nodes.IsValidIndex(SelectedNode) ? SelectedNode + 1 : Current->Nodes.Num();

	// Link the previous node to the new one if it has no next yet
	if (Current->Nodes.IsValidIndex(SelectedNode) && Current->Nodes[SelectedNode].NextNode.IsNone()
		&& Current->Nodes[SelectedNode].Type != EConversationNodeType::End && Current->Nodes[SelectedNode].Type != EConversationNodeType::PlayerChoice)
	{
		Current->Nodes[SelectedNode].NextNode = Node.NodeId;
	}

	Current->Nodes.Insert(Node, InsertAt);
	if (Current->StartNode.IsNone())
	{
		Current->StartNode = Node.NodeId;
	}
	SelectedNode = InsertAt;
	MarkEdited();
	RefreshAll();
	return FReply::Handled();
}

FReply SConversationEditor::OnDeleteNode()
{
	UConversationAsset* Current = GetConversation();
	if (!Current || !Current->Nodes.IsValidIndex(SelectedNode))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("DeleteNode", "Delete Conversation Node"));
	Current->Modify();
	Current->Nodes.RemoveAt(SelectedNode);
	SelectedNode = FMath::Min(SelectedNode, Current->Nodes.Num() - 1);
	MarkEdited();
	RefreshAll();
	return FReply::Handled();
}

FReply SConversationEditor::OnMoveNode(int32 Direction)
{
	UConversationAsset* Current = GetConversation();
	const int32 Target = SelectedNode + Direction;
	if (!Current || !Current->Nodes.IsValidIndex(SelectedNode) || !Current->Nodes.IsValidIndex(Target))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("MoveNode", "Move Conversation Node"));
	Current->Modify();
	Current->Nodes.Swap(SelectedNode, Target);
	SelectedNode = Target;
	MarkEdited();
	RefreshAll();
	return FReply::Handled();
}

FReply SConversationEditor::OnSetStartNode()
{
	UConversationAsset* Current = GetConversation();
	if (!Current || !Current->Nodes.IsValidIndex(SelectedNode))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("SetStart", "Set Start Node"));
	Current->Modify();
	Current->StartNode = Current->Nodes[SelectedNode].NodeId;
	MarkEdited();
	RefreshAll();
	return FReply::Handled();
}

FReply SConversationEditor::OnPlayFromNode()
{
	UConversationAsset* Current = GetConversation();
	if (!Current || !GEditor || GEditor->PlayWorld)
	{
		return FReply::Handled();
	}
	const FName StartNode = Current->Nodes.IsValidIndex(SelectedNode) ? Current->Nodes[SelectedNode].NodeId : Current->StartNode;

	// Open the conversation's city so the spy HUD/controller exist
	if (Current->Language && !Current->Language->CityMap.IsNull())
	{
		const FString MapPackage = Current->Language->CityMap.ToSoftObjectPath().GetLongPackageName();
		const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		if (!EditorWorld || EditorWorld->GetOutermost()->GetName() != MapPackage)
		{
			if (!FEditorFileUtils::SaveDirtyPackages(/*bPromptUserToSave*/ true, /*bSaveMapPackages*/ true, /*bSaveContentPackages*/ true))
			{
				return FReply::Handled();
			}
			FEditorFileUtils::LoadMap(FPackageName::LongPackageNameToFilename(MapPackage, FPackageName::GetMapPackageExtension()));
		}
	}

	TWeakObjectPtr<UConversationAsset> WeakConversation = Current;
	if (PostPIEHandle.IsValid())
	{
		FEditorDelegates::PostPIEStarted.Remove(PostPIEHandle);
	}
	PostPIEHandle = FEditorDelegates::PostPIEStarted.AddLambda([this, WeakConversation, StartNode](bool bIsSimulating)
	{
		FEditorDelegates::PostPIEStarted.Remove(PostPIEHandle);
		PostPIEHandle.Reset();

		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld.Get() : nullptr;
		UConversationSubsystem* Conversations = PlayWorld ? PlayWorld->GetSubsystem<UConversationSubsystem>() : nullptr;
		if (!Conversations || !WeakConversation.IsValid())
		{
			return;
		}
		UCoverComponent* Cover = nullptr;
		if (const APlayerController* PC = PlayWorld->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				Cover = Pawn->FindComponentByClass<UCoverComponent>();
			}
		}
		Conversations->StartConversationFromAsset(WeakConversation.Get(), StartNode, Cover);
	});

	FRequestPlaySessionParams Params;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	GEditor->RequestPlaySession(Params);
	return FReply::Handled();
}

// =====================================================================================================================
// Node details + quick tiles
// =====================================================================================================================

void SConversationEditor::RebuildNodeDetails()
{
	UConversationAsset* Current = GetConversation();
	if (Current && Current->Nodes.IsValidIndex(SelectedNode))
	{
		NodeStruct = MakeShared<FStructOnScope>(FConversationNode::StaticStruct(), reinterpret_cast<uint8*>(&Current->Nodes[SelectedNode]));
		NodeStruct->SetPackage(Current->GetOutermost());
	}
	else
	{
		NodeStruct.Reset();
	}
	NodeDetails->SetStructureData(NodeStruct);
}

void SConversationEditor::RefreshQuickTargets()
{
	QuickTargets.Reset();
	SelectedQuickTarget.Reset();
	const UConversationAsset* Current = GetConversation();
	if (Current && Current->Nodes.IsValidIndex(SelectedNode))
	{
		const FConversationNode& Node = Current->Nodes[SelectedNode];
		if (Node.Type == EConversationNodeType::PlayerChallenge)
		{
			QuickTargets.Add(MakeShared<FString>(TEXT("Reply challenge")));
		}
		else if (Node.Type == EConversationNodeType::PlayerChoice)
		{
			for (int32 i = 0; i < Node.Choices.Num(); ++i)
			{
				QuickTargets.Add(MakeShared<FString>(FString::Printf(TEXT("Choice %d: %s"), i + 1, *Node.Choices[i].Label.ToString())));
			}
		}
	}
	if (QuickTargets.Num() > 0)
	{
		SelectedQuickTarget = QuickTargets[0];
	}
	if (QuickTargetCombo.IsValid())
	{
		QuickTargetCombo->RefreshOptions();
		QuickTargetCombo->SetSelectedItem(SelectedQuickTarget);
	}
	LoadQuickTilesFromChallenge();
}

FWordTileChallenge* SConversationEditor::GetQuickTargetChallenge() const
{
	UConversationAsset* Current = GetConversation();
	if (!Current || !Current->Nodes.IsValidIndex(SelectedNode) || !SelectedQuickTarget.IsValid())
	{
		return nullptr;
	}
	FConversationNode& Node = Current->Nodes[SelectedNode];
	if (Node.Type == EConversationNodeType::PlayerChallenge)
	{
		return &Node.Challenge;
	}
	const int32 ChoiceIndex = QuickTargets.IndexOfByKey(SelectedQuickTarget);
	return (Node.Type == EConversationNodeType::PlayerChoice && Node.Choices.IsValidIndex(ChoiceIndex)) ? &Node.Choices[ChoiceIndex].Challenge : nullptr;
}

void SConversationEditor::LoadQuickTilesFromChallenge()
{
	const FWordTileChallenge* Challenge = GetQuickTargetChallenge();
	const UConversationAsset* Current = GetConversation();
	const bool bSpacing = !Current || !Current->Language || Current->Language->bUsesWordSpacing;
	const TCHAR* Separator = bSpacing ? TEXT(" ") : TEXT("|");

	TArray<FString> AnswerLines;
	TArray<FString> Distractors;
	TArray<FString> Readings;
	if (Challenge)
	{
		TArray<FString> Used;
		for (const FAcceptedAnswer& Answer : Challenge->AcceptedAnswers)
		{
			AnswerLines.Add(FString::Join(Answer.Tokens, Separator));
			for (const FString& Token : Answer.Tokens)
			{
				Used.AddUnique(Token);
			}
		}
		for (const FWordTile& Tile : Challenge->Tiles)
		{
			if (!Used.Contains(Tile.Text))
			{
				Distractors.Add(Tile.Text);
			}
			if (!Tile.Reading.IsEmpty())
			{
				Readings.AddUnique(FString::Printf(TEXT("%s=%s"), *Tile.Text, *Tile.Reading));
			}
		}
	}

	if (AnswersBox.IsValid()) { AnswersBox->SetText(FText::FromString(FString::Join(AnswerLines, TEXT("\n")))); }
	if (DistractorsBox.IsValid()) { DistractorsBox->SetText(FText::FromString(FString::Join(Distractors, Separator))); }
	if (ReadingsBox.IsValid()) { ReadingsBox->SetText(FText::FromString(FString::Join(Readings, TEXT("\n")))); }
	if (IntentBox.IsValid()) { IntentBox->SetText(Challenge ? Challenge->IntentPrompt : FText::GetEmpty()); }
	if (SkillBox.IsValid()) { SkillBox->SetText(Challenge ? FText::FromName(Challenge->SkillTag) : FText::GetEmpty()); }
}

FReply SConversationEditor::OnApplyQuickTiles()
{
	UConversationAsset* Current = GetConversation();
	if (!Current || !GetQuickTargetChallenge())
	{
		return FReply::Handled();
	}
	const bool bSpacing = !Current->Language || Current->Language->bUsesWordSpacing;

	const FScopedTransaction Transaction(LOCTEXT("ApplyQuickTiles", "Apply Quick Tiles"));
	Current->Modify();
	FWordTileChallenge* Challenge = GetQuickTargetChallenge();

	// Readings map
	TMap<FString, FString> ReadingMap;
	for (const FString& Line : SplitLines(ReadingsBox->GetText().ToString()))
	{
		FString Tile, Reading;
		if (Line.Split(TEXT("="), &Tile, &Reading))
		{
			ReadingMap.Add(Tile.TrimStartAndEnd(), Reading.TrimStartAndEnd());
		}
	}

	// Answers, and the tile multiset needed to build any of them
	Challenge->AcceptedAnswers.Reset();
	TMap<FString, int32> NeededCounts;
	TArray<FString> TileOrder;
	for (const FString& Line : SplitLines(AnswersBox->GetText().ToString()))
	{
		FAcceptedAnswer Answer;
		Answer.Tokens = SplitTokens(Line, bSpacing);
		if (Answer.Tokens.IsEmpty())
		{
			continue;
		}
		TMap<FString, int32> Counts;
		for (const FString& Token : Answer.Tokens)
		{
			++Counts.FindOrAdd(Token);
			TileOrder.AddUnique(Token);
		}
		for (const TPair<FString, int32>& Pair : Counts)
		{
			int32& Needed = NeededCounts.FindOrAdd(Pair.Key);
			Needed = FMath::Max(Needed, Pair.Value);
		}
		Challenge->AcceptedAnswers.Add(Answer);
	}

	Challenge->Tiles.Reset();
	auto AddTile = [&Challenge, &ReadingMap](const FString& Text)
	{
		FWordTile Tile;
		Tile.Text = Text;
		if (const FString* Reading = ReadingMap.Find(Text))
		{
			Tile.Reading = *Reading;
		}
		Challenge->Tiles.Add(Tile);
	};
	for (const FString& Token : TileOrder)
	{
		for (int32 i = 0; i < NeededCounts[Token]; ++i)
		{
			AddTile(Token);
		}
	}
	for (const FString& Distractor : SplitTokens(DistractorsBox->GetText().ToString(), bSpacing))
	{
		AddTile(Distractor);
	}

	Challenge->IntentPrompt = IntentBox->GetText();
	if (!SkillBox->GetText().IsEmpty())
	{
		Challenge->SkillTag = FName(*SkillBox->GetText().ToString());
	}

	MarkEdited();
	RebuildNodeDetails();
	RefreshNodes();
	RefreshValidation();
	return FReply::Handled();
}

// =====================================================================================================================
// Change tracking
// =====================================================================================================================

void SConversationEditor::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	UConversationAsset* Current = GetConversation();
	if (Current && GEditor && !bInTransaction)
	{
		GEditor->BeginTransaction(LOCTEXT("EditNode", "Edit Conversation Node"));
		Current->Modify();
		bInTransaction = true;
	}
}

void SConversationEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}
	if (bInTransaction && GEditor)
	{
		GEditor->EndTransaction();
		bInTransaction = false;
	}
	MarkEdited();

	// Structural edits inside the node (e.g. adding a choice) keep the node address; just refresh summaries
	RefreshNodes();
	RefreshValidation();
	RefreshQuickTargets();
}

void SConversationEditor::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// Edits made elsewhere (asset editor, asset settings panel) may reallocate the Nodes array
	if (Object && Object == GetConversation() && !bInTransaction)
	{
		RefreshAll();
	}
}

void SConversationEditor::MarkEdited()
{
	if (UConversationAsset* Current = GetConversation())
	{
		Current->MarkPackageDirty();
	}
}

void SConversationEditor::RefreshValidation()
{
	Problems.Reset();
	if (const UConversationAsset* Current = GetConversation())
	{
		for (const FText& Problem : Current->Validate())
		{
			Problems.Add(MakeShared<FString>(Problem.ToString()));
		}
		if (Problems.IsEmpty())
		{
			Problems.Add(MakeShared<FString>(TEXT("No problems found.")));
		}
	}
	if (ProblemList.IsValid())
	{
		ProblemList->RequestListRefresh();
	}
}

void SConversationEditor::RefreshAll()
{
	const UConversationAsset* Current = GetConversation();
	if (!Current || !Current->Nodes.IsValidIndex(SelectedNode))
	{
		SelectedNode = Current && Current->Nodes.Num() > 0 ? FMath::Clamp(SelectedNode, 0, Current->Nodes.Num() - 1) : INDEX_NONE;
	}
	AssetDetails->ForceRefresh();
	RefreshNodes();
	RebuildNodeDetails();
	RefreshQuickTargets();
	RefreshValidation();
}

#undef LOCTEXT_NAMESPACE
