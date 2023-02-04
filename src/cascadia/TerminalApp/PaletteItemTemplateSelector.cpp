// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "TabPaletteItem.h"
#include "PaletteItemTemplateSelector.h"
#include "PaletteItemTemplateSelector.g.cpp"

namespace winrt::TerminalApp::implementation
{
    WUX::DataTemplate PaletteItemTemplateSelector::SelectTemplateCore(const WF::IInspectable& item, const WUX::DependencyObject& /*container*/)
    {
        return SelectTemplateCore(item);
    }

    // Method Description:
    // - This method is called once command palette decides how to render a filtered command.
    //   Currently we support two ways to render command, that depend on its palette item type:
    //   - For TabPalette item we render an icon, a title, and some tab-related indicators like progress bar (as defined by TabItemTemplate)
    //   - All other items are currently rendered with icon, title and optional key-chord (as defined by GeneralItemTemplate)
    // Arguments:
    // - item - an instance of filtered command to render
    // Return Value:
    // - data template to use for rendering
    WUX::DataTemplate PaletteItemTemplateSelector::SelectTemplateCore(const WF::IInspectable& item)
    {
        if (const auto filteredCommand{ item.try_as<MTApp::FilteredCommand>() })
        {
            if (filteredCommand.Item().try_as<MTApp::TabPaletteItem>())
            {
                return TabItemTemplate();
            }
            else if (const auto actionPaletteItem{ filteredCommand.Item().try_as<MTApp::ActionPaletteItem>() })
            {
                if (actionPaletteItem.Command().HasNestedCommands())
                {
                    return NestedItemTemplate();
                }
            }
        }

        return GeneralItemTemplate();
    }
}
