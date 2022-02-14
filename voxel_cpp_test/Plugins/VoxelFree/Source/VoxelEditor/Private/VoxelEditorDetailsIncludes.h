// Copyright 2021 Phyronnaz

#pragma once

#include "VoxelMinimal.h"

#include "UnrealEd.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "IPropertyUtilities.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "UserInterface/PropertyEditor/PropertyEditorConstants.h"

namespace FVoxelEditorUtilities
{
	FSimpleDelegate MakeRefreshDelegate(const IPropertyTypeCustomizationUtils& CustomizationUtils);

	template<typename T>
	bool GetPropertyValue(const TSharedPtr<IPropertyHandle>& Handle, T*& OutValue)
	{
		OutValue = nullptr;
		
		if (!ensure(Handle))
		{
			return false;
		}
		
		void* Address = nullptr;
		if (!ensure(Handle->GetValueData(Address) == FPropertyAccess::Success) || !ensure(Address))
		{
			return false;
		}

		OutValue = static_cast<T*>(Address);

		return true;
	}
}

#define GET_CHILD_PROPERTY(PropertyHandle, Class, Property) PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STATIC(Class, Property)).ToSharedRef()