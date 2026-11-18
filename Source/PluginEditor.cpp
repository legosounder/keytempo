#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyTempoAudioProcessorEditor::KeyTempoAudioProcessorEditor (KeyTempoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Title Setup
    titleLabel.setText ("KeyTempo", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFECEFF1));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    // Initial tempo capture on launch from DAW playhead (without enabling continuous sync)
    editorInitialBpmApplied = audioProcessor.initialBpmCaptured;
    if (!audioProcessor.initialBpmCaptured)
    {
        if (auto* playHead = audioProcessor.getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpm = position->getBpm())
                {
                    audioProcessor.baseTempo = *bpm;
                    audioProcessor.lockedTempo = *bpm;
                    audioProcessor.initialBpmCaptured = true;
                    editorInitialBpmApplied = true;
                }
            }
        }
    }

    // Tempo Input Setup
    tempoLabel.setText ("Base Tempo:", juce::dontSendNotification);
    tempoLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    tempoLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFB0BEC5));
    addAndMakeVisible (tempoLabel);

    tempoInput.setText (juce::String (audioProcessor.baseTempo), juce::dontSendNotification);
    tempoInput.setInputRestrictions (6, "0123456789.");
    tempoInput.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF37474F));
    tempoInput.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    tempoInput.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xFF7B2CBF));
    tempoInput.addListener (this);
    tempoInput.setEnabled (!audioProcessor.useHostTempo);
    addAndMakeVisible (tempoInput);

    // Sync Toggle Setup
    syncToggle.setButtonText ("Sync to DAW");
    syncToggle.setToggleState (audioProcessor.useHostTempo, juce::dontSendNotification);
    syncToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xFFECEFF1));
    syncToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xFF7B2CBF));
    syncToggle.onClick = [this] {
        audioProcessor.useHostTempo = syncToggle.getToggleState();
        tempoInput.setEnabled (!audioProcessor.useHostTempo);
        if (audioProcessor.useHostTempo)
        {
            audioProcessor.initialBpmCaptured = true;
            editorInitialBpmApplied = true;
            updateCalculations();
        }
    };
    addAndMakeVisible (syncToggle);

    // Lock Button Setup
    lockButton.setLocked (audioProcessor.isTempoLocked);
    lockButton.onClick = [this] {
        audioProcessor.isTempoLocked = !audioProcessor.isTempoLocked;
        lockButton.setLocked (audioProcessor.isTempoLocked);
        if (audioProcessor.isTempoLocked)
        {
            audioProcessor.lockedTempo = audioProcessor.baseTempo;
        }
        else
        {
            updateCalculations();
        }
    };
    addAndMakeVisible (lockButton);

    // Key Selector Setup
    keyLabel.setText ("Base Key:", juce::dontSendNotification);
    keyLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    keyLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFB0BEC5));
    addAndMakeVisible (keyLabel);

    const std::vector<juce::String> notes = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < (int)notes.size(); ++i)
        keySelector.addItem (notes[i], i + 1);
    keySelector.setSelectedId (audioProcessor.baseKeyIndex + 1);
    keySelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF37474F));
    keySelector.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    keySelector.setColour (juce::ComboBox::focusedOutlineColourId, juce::Colour (0xFF7B2CBF));
    keySelector.addListener (this);
    addAndMakeVisible (keySelector);

    // Scale Selector Setup
    scaleLabel.setText ("Scale Mode:", juce::dontSendNotification);
    scaleLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    scaleLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFB0BEC5));
    addAndMakeVisible (scaleLabel);

    scaleSelector.addItem ("Major", 1);
    scaleSelector.addItem ("Minor", 2);
    scaleSelector.setSelectedId (audioProcessor.baseScaleIndex + 1);
    scaleSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF37474F));
    scaleSelector.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    scaleSelector.setColour (juce::ComboBox::focusedOutlineColourId, juce::Colour (0xFF7B2CBF));
    scaleSelector.addListener (this);
    addAndMakeVisible (scaleSelector);

    // Table Setup
    resultsTable.setModel (this);
    resultsTable.getHeader().addColumn ("Semitone", 1, 100, 50, 150);
    resultsTable.getHeader().addColumn ("New Key", 2, 150, 80, 200);
    resultsTable.getHeader().addColumn ("New Tempo", 3, 170, 100, 250);
    resultsTable.getHeader().addColumn ("Speed Ratio (Resample)", 4, 220, 120, 300);
    resultsTable.setColour (juce::TableListBox::backgroundColourId, juce::Colour (0xFF1A1926));
    resultsTable.setColour (juce::TableListBox::outlineColourId, juce::Colour (0xFF37474F));
    addAndMakeVisible (resultsTable);

    // Initial Calculation & Window Resize
    updateCalculations();
    startTimerHz (25); // snappily update from host tempo
    setSize (700, 500);
}

KeyTempoAudioProcessorEditor::~KeyTempoAudioProcessorEditor()
{
}

void KeyTempoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Premium dark-mode gradient background
    g.fillAll (juce::Colour (0xFF1E1E24));
    
    // Draw subtle border around control region
    g.setColour (juce::Colour (0xFF37474F));
    g.drawRoundedRectangle (15.0f, 50.0f, getWidth() - 30.0f, 75.0f, 6.0f, 1.5f);
}

void KeyTempoAudioProcessorEditor::resized()
{
    titleLabel.setBounds (10, 10, getWidth() - 20, 30);

    // Layout controls to fit the new Sync to DAW toggle button and Lock button
    int controlY = 62;

    tempoLabel.setBounds (27, controlY, 75, 24);
    tempoInput.setBounds (107, controlY, 65, 24);
    lockButton.setBounds (182, controlY, 24, 24);
    syncToggle.setBounds (216, controlY, 105, 24);

    keyLabel.setBounds (341, controlY, 65, 24);
    keySelector.setBounds (411, controlY, 65, 24);

    scaleLabel.setBounds (496, controlY, 80, 24);
    scaleSelector.setBounds (581, controlY, 92, 24);

    // Table placement
    resultsTable.setBounds (15, 140, getWidth() - 30, getHeight() - 155);
}

int KeyTempoAudioProcessorEditor::getNumRows()
{
    return (int)calculatedRows.size();
}

void KeyTempoAudioProcessorEditor::paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused (rowIsSelected);

    if (rowNumber == 12) // 0 semitones (original)
    {
        g.fillAll (juce::Colour (0xFF4A154B)); // Distinctive deep purple/magenta
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll (juce::Colour (0xFF242331));
    }
    else
    {
        g.fillAll (juce::Colour (0xFF1A1926));
    }
}

void KeyTempoAudioProcessorEditor::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused (rowIsSelected);

    if (rowNumber < 0 || rowNumber >= (int)calculatedRows.size())
        return;

    const auto& data = calculatedRows[rowNumber];
    bool isOriginal = (data.semitoneShift == 0);

    if (isOriginal)
    {
        g.setColour (juce::Colour (0xFFE0A96D)); // Premium light-gold text
        g.setFont (juce::Font (14.0f, juce::Font::bold));
    }
    else
    {
        g.setColour (juce::Colour (0xFFECEFF1));
        g.setFont (juce::Font (14.0f));
    }

    juce::String text;

    switch (columnId)
    {
        case 1:
            text = (data.semitoneShift > 0 ? "+" : "") + juce::String (data.semitoneShift);
            break;
        case 2:
            text = data.keyString;
            break;
        case 3:
            text = juce::String (data.tempo, 2) + " BPM";
            break;
        case 4:
            text = "x" + juce::String (data.ratio, 5);
            if (data.semitoneShift != 0)
            {
                double pct = (data.ratio - 1.0) * 100.0;
                text += " (";
                if (pct > 0.0)
                    text += "+";
                text += juce::String (pct, 2) + "%)";
            }
            break;
        default:
            break;
    }

    g.drawText (text, 10, 0, width - 20, height, juce::Justification::centredLeft, true);
}

void KeyTempoAudioProcessorEditor::textEditorTextChanged (juce::TextEditor& editor)
{
    if (&editor == &tempoInput)
    {
        audioProcessor.initialBpmCaptured = true;
        editorInitialBpmApplied = true;
        double newTempo = tempoInput.getText().getDoubleValue();
        if (newTempo > 0.0)
        {
            audioProcessor.baseTempo = newTempo;
            updateCalculations();
        }
    }
}

void KeyTempoAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &keySelector)
    {
        audioProcessor.baseKeyIndex = keySelector.getSelectedId() - 1;
        updateCalculations();
    }
    else if (comboBoxThatHasChanged == &scaleSelector)
    {
        audioProcessor.baseScaleIndex = scaleSelector.getSelectedId() - 1;
        updateCalculations();
    }
}

void KeyTempoAudioProcessorEditor::updateCalculations()
{
    calculatedRows.clear();

    const std::vector<juce::String> notes = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    juce::String scaleType = (audioProcessor.baseScaleIndex == 0) ? " Major" : " Minor";

    for (int shift = -12; shift <= 12; ++shift)
    {
        int semitoneShift = shift;

        // Shift root note key with wrapping
        int newKeyIndex = (audioProcessor.baseKeyIndex + shift) % 12;
        if (newKeyIndex < 0)
            newKeyIndex += 12;

        juce::String keyString = notes[newKeyIndex] + scaleType;

        // Math: tempo * 2^(shift / 12)
        double ratio = std::pow (2.0, static_cast<double> (shift) / 12.0);
        double tempo = audioProcessor.baseTempo * ratio;

        calculatedRows.push_back ({ semitoneShift, keyString, tempo, ratio });
    }

    resultsTable.updateContent();
    resultsTable.repaint();
}

void KeyTempoAudioProcessorEditor::timerCallback()
{
    // Update the notification timer
    if (notificationTimer > 0)
    {
        notificationTimer--;
        if (notificationTimer == 0)
        {
            titleLabel.setText ("KeyTempo", juce::dontSendNotification);
            titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFECEFF1));
        }
    }

    // Keep lockButton synchronized with the processor state
    if (lockButton.getLocked() != audioProcessor.isTempoLocked)
        lockButton.setLocked (audioProcessor.isTempoLocked);

    // Keep syncToggle synchronized with the processor state
    if (syncToggle.getToggleState() != audioProcessor.useHostTempo)
    {
        syncToggle.setToggleState (audioProcessor.useHostTempo, juce::dontSendNotification);
        tempoInput.setEnabled (!audioProcessor.useHostTempo);
    }

    // On launch, capture host tempo once even if Sync to DAW is unchecked
    if (!editorInitialBpmApplied)
    {
        if (audioProcessor.initialBpmCaptured)
        {
            tempoInput.setText (juce::String (audioProcessor.baseTempo), juce::dontSendNotification);
            updateCalculations();
            editorInitialBpmApplied = true;
        }
        else if (auto* playHead = audioProcessor.getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpm = position->getBpm())
                {
                    audioProcessor.baseTempo = *bpm;
                    audioProcessor.lockedTempo = *bpm;
                    audioProcessor.initialBpmCaptured = true;
                    tempoInput.setText (juce::String (audioProcessor.baseTempo), juce::dontSendNotification);
                    updateCalculations();
                    editorInitialBpmApplied = true;
                }
            }
        }
    }

    // Handle playhead updates
    if (audioProcessor.useHostTempo)
    {
        if (! audioProcessor.isTempoLocked)
        {
            double hostTempo = audioProcessor.baseTempo;
            juce::String currentText = tempoInput.getText();
            double currentVal = currentText.getDoubleValue();

            if (std::abs (currentVal - hostTempo) > 0.001 || currentText.isEmpty())
            {
                tempoInput.setText (juce::String (hostTempo, 2), juce::dontSendNotification);
                updateCalculations();
            }
        }
        else
        {
            // If locked, make sure we display the locked tempo in the input field
            double lockedTempoVal = audioProcessor.lockedTempo;
            juce::String currentText = tempoInput.getText();
            double currentVal = currentText.getDoubleValue();
            if (std::abs (currentVal - lockedTempoVal) > 0.001 || currentText.isEmpty())
            {
                tempoInput.setText (juce::String (lockedTempoVal, 2), juce::dontSendNotification);
                updateCalculations();
            }
        }
    }
}

void KeyTempoAudioProcessorEditor::cellDoubleClicked (int rowNumber, int columnId, const juce::MouseEvent& event)
{
    juce::ignoreUnused (columnId, event);
    if (rowNumber >= 0 && rowNumber < (int)calculatedRows.size())
    {
        double clickedTempo = calculatedRows[rowNumber].tempo;
        juce::String tempoStr = juce::String (clickedTempo, 2);
        juce::SystemClipboard::copyTextToClipboard (tempoStr);

        titleLabel.setText ("Copied " + tempoStr + " BPM to clipboard!", juce::dontSendNotification);
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFE0A96D)); // Gold color
        notificationTimer = 38; // ~1.5 seconds at 25Hz
    }
}
