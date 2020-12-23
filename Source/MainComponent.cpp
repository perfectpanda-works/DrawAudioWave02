#include "MainComponent.h"
#include <Windows.h>

//==============================================================================
MainComponent::MainComponent()
    : state(Stopped),
      thumbnailCache(5),
      thumbnail(512, formatManager, thumbnailCache)
{
    addAndMakeVisible(&openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);
    thumbnail.addChangeListener(this);

    setAudioChannels(0, 2);

    setSize (800, 600);

    startTimer(40);//add start timer
}

void  MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource) transportSourceChanged();
    if (source == &thumbnail)       thumbnailChanged();
}

void MainComponent::transportSourceChanged()
{
    changeState(transportSource.isPlaying() ? Playing : Stopped);
}

void MainComponent::thumbnailChanged()
{
    repaint();
}

void MainComponent::changeState(MainComponent::TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
        case Stopped:
            stopButton.setEnabled(false);
            playButton.setEnabled(true);
            transportSource.setPosition(0.0);
            break;

        case Starting:
            playButton.setEnabled(false);
            transportSource.start();
            break;

        case Playing:
            stopButton.setEnabled(true);
            break;

        case Stopping:
            transportSource.stop();
            break;
        }
    }
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

void MainComponent::openButtonClicked()
{
    juce::FileChooser chooser("Select a Wave file to play...",
        {},
        "*.wav");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        auto* reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            playButton.setEnabled(true);
            thumbnail.setSource(new juce::FileInputSource(file));
            readerSource.reset(newSource.release());
        }
    }
}

void MainComponent::playButtonClicked()
{
    changeState(Starting);
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopping);
}

void MainComponent::paint (juce::Graphics& g)
{
    juce::Rectangle<int> thumbnailBounds(10, 100, getWidth() - 20, getHeight() - 120);

    if (thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g, thumbnailBounds);
    else
        paintIfFileLoaded(g, thumbnailBounds);
}

void MainComponent::resized()
{
    openButton.setBounds(10, 10, getWidth() - 20, 20);
    playButton.setBounds(10, 40, getWidth() - 20, 20);
    stopButton.setBounds(10, 70, getWidth() - 20, 20);
}

void MainComponent::paintIfNoFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(thumbnailBounds);
    g.setColour(juce::Colours::white);
    g.drawFittedText("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
}

void MainComponent::paintIfFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::white);
    g.fillRect(thumbnailBounds);

    g.setColour(juce::Colours::red);

    auto audioLength = (float)thumbnail.getTotalLength();//add

    thumbnail.drawChannels(g,
        thumbnailBounds,
        0.0,
        thumbnail.getTotalLength(),
        1.0f);

    //add draw line
    g.setColour(juce::Colours::green);

    auto audioPosition = (float)transportSource.getCurrentPosition();
    auto drawPosition = (audioPosition / audioLength) * (float)thumbnailBounds.getWidth()
        + (float)thumbnailBounds.getX();
    g.drawLine(drawPosition, (float)thumbnailBounds.getY(), drawPosition,
        (float)thumbnailBounds.getBottom(), 2.0f);
}

//add timer callback
void MainComponent::timerCallback()
{
    repaint();
}
