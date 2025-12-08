#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace threadbare::dsp
{
struct UnravelState;
}

class UnravelProcessor;
class UnravelEditor final : public juce::AudioProcessorEditor
{
public:
    explicit UnravelEditor(UnravelProcessor&);
    ~UnravelEditor() override = default;

    void resized() override;

private:
    void handleUpdate();
    void loadInitialURL();

    UnravelProcessor& processorRef;
    juce::WebBrowserComponent webView;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    threadbare::dsp::UnravelState cachedVisualState{};
    bool hasCachedVisualState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnravelEditor)
};

