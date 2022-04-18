// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <UIAutomationCore.h>
#include <LibraryResources.h>
#include "InteractivityAutomationPeer.h"
#include "InteractivityAutomationPeer.g.cpp"

#include "XamlUiaTextRange.h"
#include "../types/UiaTracing.h"

using namespace Microsoft::Console::Types;
using namespace winrt::Windows::UI::Xaml::Automation::Peers;
using namespace winrt::Windows::Graphics::Display;

namespace UIA
{
    using ::ITextRangeProvider;
    using ::SupportedTextSelection;
}

namespace XamlAutomation
{
    using winrt::Windows::UI::Xaml::Automation::SupportedTextSelection;
    using winrt::Windows::UI::Xaml::Automation::Provider::IRawElementProviderSimple;
    using winrt::Windows::UI::Xaml::Automation::Provider::ITextRangeProvider;
}

namespace winrt::Microsoft::Terminal::Control::implementation
{
    InteractivityAutomationPeer::InteractivityAutomationPeer(Control::implementation::ControlInteractivity* owner) :
        _interactivity{ owner }
    {
        THROW_IF_FAILED(::Microsoft::WRL::MakeAndInitialize<::Microsoft::Terminal::TermControlUiaProvider>(&_uiaProvider, _interactivity->GetUiaData(), this));
    };

    void InteractivityAutomationPeer::SetControlBounds(const Windows::Foundation::Rect bounds)
    {
        _controlBounds = til::rect{ til::math::rounding, bounds };
    }
    void InteractivityAutomationPeer::SetControlPadding(const Core::Padding padding)
    {
        _controlPadding = til::rect{ til::math::rounding, padding };
    }
    void InteractivityAutomationPeer::ParentProvider(Windows::UI::Xaml::Automation::Provider::IRawElementProviderSimple parentProvider)
    {
        _parentProvider = parentProvider;
    }

    // Method Description:
    // - Signals the ui automation client that the terminal's selection has
    //   changed and should be updated
    // - We will raise a new event, for out embedding control to be able to
    //   raise the event. AutomationPeer by itself doesn't hook up to the
    //   eventing mechanism, we need the FrameworkAutomationPeer to do that.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void InteractivityAutomationPeer::SignalSelectionChanged()
    {
        _SelectionChangedHandlers(*this, nullptr);
    }

    // Method Description:
    // - Signals the ui automation client that the terminal's output has changed
    //   and should be updated
    // - We will raise a new event, for out embedding control to be able to
    //   raise the event. AutomationPeer by itself doesn't hook up to the
    //   eventing mechanism, we need the FrameworkAutomationPeer to do that.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void InteractivityAutomationPeer::SignalTextChanged()
    {
        _TextChangedHandlers(*this, nullptr);
    }

    // Method Description:
    // - Signals the ui automation client that the cursor's state has changed
    //   and should be updated
    // - We will raise a new event, for out embedding control to be able to
    //   raise the event. AutomationPeer by itself doesn't hook up to the
    //   eventing mechanism, we need the FrameworkAutomationPeer to do that.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void InteractivityAutomationPeer::SignalCursorChanged()
    {
        _CursorChangedHandlers(*this, nullptr);
    }

    void InteractivityAutomationPeer::NotifyNewOutput(std::wstring_view newOutput)
    {
        _NewOutputHandlers(*this, hstring{ newOutput });
    }

#pragma region ITextProvider
    com_array<XamlAutomation::ITextRangeProvider> InteractivityAutomationPeer::GetSelection()
    {
        SAFEARRAY* pReturnVal;
        THROW_IF_FAILED(_uiaProvider->GetSelection(&pReturnVal));
        return WrapArrayOfTextRangeProviders(pReturnVal);
    }

    com_array<XamlAutomation::ITextRangeProvider> InteractivityAutomationPeer::GetVisibleRanges()
    {
        SAFEARRAY* pReturnVal;
        THROW_IF_FAILED(_uiaProvider->GetVisibleRanges(&pReturnVal));
        return WrapArrayOfTextRangeProviders(pReturnVal);
    }

    XamlAutomation::ITextRangeProvider InteractivityAutomationPeer::RangeFromChild(XamlAutomation::IRawElementProviderSimple childElement)
    {
        UIA::ITextRangeProvider* returnVal;
        // ScreenInfoUiaProvider doesn't actually use parameter, so just pass in nullptr
        THROW_IF_FAILED(_uiaProvider->RangeFromChild(/* IRawElementProviderSimple */ nullptr,
                                                     &returnVal));

        // const auto parentProvider = this->ProviderFromPeer(*this);
        const auto parentProvider = _parentProvider;
        const auto xutr = winrt::make_self<XamlUiaTextRange>(returnVal, parentProvider);
        return xutr.as<XamlAutomation::ITextRangeProvider>();
    }

    XamlAutomation::ITextRangeProvider InteractivityAutomationPeer::RangeFromPoint(Windows::Foundation::Point screenLocation)
    {
        UIA::ITextRangeProvider* returnVal;
        THROW_IF_FAILED(_uiaProvider->RangeFromPoint({ screenLocation.X, screenLocation.Y }, &returnVal));

        // const auto parentProvider = this->ProviderFromPeer(*this);
        const auto parentProvider = _parentProvider;
        const auto xutr = winrt::make_self<XamlUiaTextRange>(returnVal, parentProvider);
        return xutr.as<XamlAutomation::ITextRangeProvider>();
    }

    XamlAutomation::ITextRangeProvider InteractivityAutomationPeer::DocumentRange()
    {
        UIA::ITextRangeProvider* returnVal;
        THROW_IF_FAILED(_uiaProvider->get_DocumentRange(&returnVal));

        // const auto parentProvider = this->ProviderFromPeer(*this);
        const auto parentProvider = _parentProvider;
        const auto xutr = winrt::make_self<XamlUiaTextRange>(returnVal, parentProvider);
        return xutr.as<XamlAutomation::ITextRangeProvider>();
    }

    XamlAutomation::SupportedTextSelection InteractivityAutomationPeer::SupportedTextSelection()
    {
        UIA::SupportedTextSelection returnVal;
        THROW_IF_FAILED(_uiaProvider->get_SupportedTextSelection(&returnVal));
        return static_cast<XamlAutomation::SupportedTextSelection>(returnVal);
    }

#pragma endregion

#pragma region IControlAccessibilityInfo
    COORD InteractivityAutomationPeer::GetFontSize() const noexcept
    {
        return til::size{ til::math::rounding, _interactivity->Core().FontSize() }.to_win32_coord();
    }

    RECT InteractivityAutomationPeer::GetBounds() const noexcept
    {
        return _controlBounds.to_win32_rect();
    }

    HRESULT InteractivityAutomationPeer::GetHostUiaProvider(IRawElementProviderSimple** provider)
    {
        RETURN_HR_IF(E_INVALIDARG, provider == nullptr);
        *provider = nullptr;

        return S_OK;
    }

    RECT InteractivityAutomationPeer::GetPadding() const noexcept
    {
        return _controlPadding.to_win32_rect();
    }

    double InteractivityAutomationPeer::GetScaleFactor() const noexcept
    {
        return _interactivity->Core().DisplayScale();
        // return DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();
    }

    void InteractivityAutomationPeer::ChangeViewport(const SMALL_RECT NewWindow)
    {
        _interactivity->UpdateScrollbar(NewWindow.Top);
    }
#pragma endregion

    XamlAutomation::ITextRangeProvider InteractivityAutomationPeer::_CreateXamlUiaTextRange(UIA::ITextRangeProvider* returnVal) const
    {
        // LOAD-BEARING: use _parentProvider->ProviderFromPeer(_parentProvider) instead of this->ProviderFromPeer(*this).
        // Since we split the automation peer into TermControlAutomationPeer and InteractivityAutomationPeer,
        // using "this" returns null. This can cause issues with some UIA Client scenarios like any navigation in Narrator.
        // TODO! what TF happened in this merge
        // const auto parent{ _parentProvider.get() };
        // if (!parent)
        // {
        //     return nullptr;
        // }
        // const auto xutr = winrt::make_self<XamlUiaTextRange>(returnVal, parent.ProviderFromPeer(parent));
        const auto xutr = winrt::make_self<XamlUiaTextRange>(returnVal, _parentProvider);
        return xutr.as<XamlAutomation::ITextRangeProvider>();
    };

    // Method Description:
    // - extracts the UiaTextRanges from the SAFEARRAY and converts them to Xaml ITextRangeProviders
    // Arguments:
    // - SAFEARRAY of UIA::UiaTextRange (ITextRangeProviders)
    // Return Value:
    // - com_array of Xaml Wrapped UiaTextRange (ITextRangeProviders)
    com_array<XamlAutomation::ITextRangeProvider> InteractivityAutomationPeer::WrapArrayOfTextRangeProviders(SAFEARRAY* textRanges)
    {
        // transfer ownership of UiaTextRanges to this new vector
        auto providers = SafeArrayToOwningVector<::Microsoft::Terminal::TermControlUiaTextRange>(textRanges);
        int count = gsl::narrow<int>(providers.size());

        std::vector<XamlAutomation::ITextRangeProvider> vec;
        vec.reserve(count);
        // auto parentProvider = this->ProviderFromPeer(*this);
        // const auto parentProvider = _parentProvider;
        for (int i = 0; i < count; i++)
        {
            if (auto xutr = _CreateXamlUiaTextRange(providers[i].detach()))
            {
                vec.emplace_back(std::move(xutr));
            }
        }

        com_array<XamlAutomation::ITextRangeProvider> result{ vec };

        return result;
    }
}
