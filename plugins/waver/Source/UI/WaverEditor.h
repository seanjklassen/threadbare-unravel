#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class WaverProcessor;

class WaverEditor final : public juce::AudioProcessorEditor
{
public:
    explicit WaverEditor(WaverProcessor& proc);
    ~WaverEditor() override = default;

    void resized() override;

private:
    void handleUpdate();

    WaverProcessor& processorRef;
    juce::WebBrowserComponent webView;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaverEditor)
};
