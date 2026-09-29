// LinguaSpy - language / city pack

#include "LanguagePackAsset.h"

const FPrimaryAssetType ULanguagePackAsset::PrimaryAssetType(TEXT("LanguagePack"));

FPrimaryAssetId ULanguagePackAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
