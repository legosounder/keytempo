#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LockButton : public juce::Button
{
public:
    LockButton() : juce::Button ("Lock") {}
    
    void setLocked (bool shouldBeLocked)
    {
        if (isLocked != shouldBeLocked)
        {
            isLocked = shouldBeLocked;
            repaint();
        }
    }
    
    bool getLocked() const { return isLocked; }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto width = getWidth();
        auto height = getHeight();
        
        g.setColour (shouldDrawButtonAsDown ? juce::Colour (0xFF1A1926) : 
                     (shouldDrawButtonAsHighlighted ? juce::Colour (0xFF455A64) : juce::Colour (0xFF37474F)));
        g.fillRoundedRectangle (0, 0, width, height, 4.0f);
        
        g.setColour (isLocked ? juce::Colour (0xFFE0A96D) : juce::Colour (0xFF90A4AE));
        g.drawRoundedRectangle (0.5f, 0.5f, width - 1.0f, height - 1.0f, 4.0f, 1.0f);
        
        float cx = width * 0.5f;
        float cy = height * 0.5f;
        
        float bodyW = 12.0f;
        float bodyH = 8.0f;
        float bodyX = cx - bodyW * 0.5f;
        float bodyY = cy - 1.0f;
        
        g.setColour (isLocked ? juce::Colour (0xFFE0A96D) : juce::Colour (0xFFECEFF1));
        g.fillRoundedRectangle (bodyX, bodyY, bodyW, bodyH, 1.5f);
        
        g.setColour (isLocked ? juce::Colour (0xFFE0A96D) : juce::Colour (0xFFB0BEC5));
        
        juce::Path shacklePath;
        if (isLocked)
        {
            shacklePath.startNewSubPath (cx - 4.0f, bodyY + 2.0f);
            shacklePath.lineTo (cx - 4.0f, bodyY - 1.0f);
            shacklePath.addCentredArc (cx, bodyY - 1.0f, 4.0f, 4.0f, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
            shacklePath.lineTo (cx + 4.0f, bodyY + 2.0f);
        }
        else
        {
            shacklePath.startNewSubPath (cx - 4.0f, bodyY - 2.0f);
            shacklePath.lineTo (cx - 4.0f, bodyY - 3.0f);
            shacklePath.addCentredArc (cx, bodyY - 3.0f, 4.0f, 4.0f, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
            shacklePath.lineTo (cx + 4.0f, bodyY - 1.0f);
        }
        
        g.strokePath (shacklePath, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        
        g.setColour (shouldDrawButtonAsDown ? juce::Colour (0xFFECEFF1) : juce::Colour (0xFF1E1E24));
        g.fillEllipse (cx - 1.0f, bodyY + 3.0f, 2.0f, 2.0f);
    }

private:
    bool isLocked = false;
};

class KeyTempoAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      public juce::TableListBoxModel,
                                      public juce::TextEditor::Listener,
                                      public juce::ComboBox::Listener,
                                      public juce::Timer
{
public:
    KeyTempoAudioProcessorEditor (KeyTempoAudioProcessor&);
    ~KeyTempoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellDoubleClicked (int rowNumber, int columnId, const juce::MouseEvent& event) override;

    // Listeners
    void textEditorTextChanged (juce::TextEditor& editor) override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

    // Timer Callback
    void timerCallback() override;

private:
    void updateCalculations();

    KeyTempoAudioProcessor& audioProcessor;

    // GUI Components
    juce::Label titleLabel;
    
    juce::Label tempoLabel;
    juce::TextEditor tempoInput;
    LockButton lockButton;
    juce::ToggleButton syncToggle;

    int notificationTimer = 0;
    bool editorInitialBpmApplied = false;

    juce::Label keyLabel;
    juce::ComboBox keySelector;

    juce::Label scaleLabel;
    juce::ComboBox scaleSelector;

    juce::TableListBox resultsTable;

    // Calculation Data
    struct RowData {
        int semitoneShift;
        juce::String keyString;
        double tempo;
        double ratio;
    };
    std::vector<RowData> calculatedRows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyTempoAudioProcessorEditor)
};
