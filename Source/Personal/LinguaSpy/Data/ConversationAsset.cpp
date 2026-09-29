// LinguaSpy - authored conversation

#include "ConversationAsset.h"

const FConversationNode* UConversationAsset::FindNode(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}
	return Nodes.FindByPredicate([NodeId](const FConversationNode& Node) { return Node.NodeId == NodeId; });
}
