/*
  ==============================================================================

    MidiTranslator.h
    Created: 5 Jan 2024 5:17:19pm
    Author:  Abhishek Shivakumar

  ==============================================================================
*/

#pragma once

typedef std::vector<int> Chord;

class MidiNoteTracker : public juce::MidiInputCallback
{
public:
    MidiNoteTracker() {}

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();
            pressedNotes.insert(note); // Add the pressed note
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            pressedNotes.erase(note); // Remove the released note
        }
    }

    const std::set<int>& getPressedNotes() const { return pressedNotes; }
    bool isNotePressed(int note) const { return pressedNotes.find(note) != pressedNotes.end(); }

private:
    std::set<int> pressedNotes; // Set of pressed notes
};

class MidiTranslator
{
public:
    std::map<Chord, Chord> translationMap;
    Chord translatedChord;
    
    MidiNoteTracker noteTracker;  // MidiNoteTracker instance
    
    int noteIndex = 0;

    int startingMidiNote;
    int rootNote = 60;
     //cmajor 0, 2, 4, 5, 7, 9, 10, 12
    MidiTranslator()
    {
        // Example translation map initialization
        translationMap =
        {
            {{60, 64, 67}, {65, 69, 72}} // Example: C Major to F Major
        };
    }
    

    
    std::vector<int> indexRoot;// {3, 5, 4, 6, 2, 7};
    
    //These are the avilable notes
    std::vector<int> available1 {0, 3, 5, 4, 6, 2, 7};
    std::vector<int> available2 {0, 3, 5, 6, 2, 4, 7};
    std::vector<int> available3 {0, 3, 5, 2, 4, 6, 7};
    std::vector<int> available4 {0, 3, 5, 2, 6, 7, 4};
    
    std::vector<int> mode {0, 2, 4, 5, 7, 10, 12};
    std::vector<int> OGMode {0, 2, 4, 5, 7, 10, 12};

    
    //Yay, the mode is rotating
    int getModeOffset (int modeIndex, int rotationalOffset = 0)
    {
        for (int i = 0; i < mode.size(); i++)
        {
            juce::Logger::writeToLog (juce::String (mode[i]) + juce::String (" "));
        }
        
        return mode[(modeIndex + rotationalOffset) % mode.size()];
    }
    
    int getOGModeOffset (int modeIndex, int rotationalOffset = 0)
    {
        return OGMode[(modeIndex + rotationalOffset) % OGMode.size()];
    }
    
    //TODO: change to getChordIntervalOffset
    int getChordNoteIntervalOffset (int modeIndex, int rotationalOffset = 0) //starting from 0
    {
        return mode[abs(modeIndex + rotationalOffset) % 7];
    }
    
    int getNote(int modeIndex) //starting from 1
    {
        return rootNote + getChordNoteIntervalOffset (modeIndex);
    }
    
    int getXFromMidi (int note)
    {
        return ((note - 44) % 8);
    }
    
    int getYFromMidi (int note)
    {
        return ((note-44) / 8);
    }
    
    double calculateDistance (int x1, int y1, int x2, int y2)
    {
        int dx = x2 - x1;
        int dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }
    
    int firstNote = 0;
    void process (juce::MidiBuffer& midiMessages)
    {
        static std::set<int> activeNotes; // Tracks active notes
        static std::map<int, int> noteMap; // Maps original notes to translated notes
        juce::MidiBuffer::Iterator midiBufferIterator (midiMessages);
        juce::MidiMessage message;
        int sampleNumber;

        while (midiBufferIterator.getNextEvent (message, sampleNumber))
        {
            noteTracker.handleIncomingMidiMessage (nullptr, message); // Track notes
            
     //       DBG ("Pressing physical note: " + juce::String (message.getNoteNumber()));
        }

        const auto& pressedNotes = noteTracker.getPressedNotes();
        juce::MidiBuffer translatedMessages;

        // Process new notes (note on)
        for (const auto& note : pressedNotes)
        {
      //      DBG (noteIndex);
            
            if (note >= 36 && note <= 43)
            {
                rootNote = 36 + getOGModeOffset (note - 36) + 36;
                midiMessages.clear();
      //          DBG ("Root note changed to: " + juce::String (note));
      //          return;
            }
            

            if (activeNotes.find (note) == activeNotes.end()) // Note is new
            {
                int translatedNote;
                
                // Calculate the offset from the first pressed note
      //          DBG ("XY: " + juce::String (getXFromMidi (note)) + juce::String ("|") + juce::String (getYFromMidi (note)));
                int offset = calculateDistance (getXFromMidi (note), getYFromMidi(note), getXFromMidi (firstNote), getYFromMidi(firstNote));//note - firstNote;
                
                if (noteIndex == 0)
                {
                    available1 = {0, 3, 5, 4, 6, 2, 7};
                    available2 = {0, 3, 5, 6, 2, 4, 7};
                    available3 = {0, 3, 5, 2, 4, 6, 7};
                    available4 = {0, 3, 5, 2, 6, 7, 4};
                    
                    indexRoot = available1;
                    firstNote = note;
                    
                    offset = 0;
                }

                DBG ("Square OFFSET IS " + juce::String (offset));
                int index = indexRoot[offset]; //we get the index in the active mode
                DBG ("INDEX OF mode note IS " + juce::String (index));
                
                //    std::vector<int> mode {0, 2, 4, 5, 7, 10, 12};
                DBG ("rotated index: " + juce::String (std::max (index - 1, 0)));
                auto chordIntervalOffset = mode [std::max (index - 1, 0)];
                DBG ("Chord interval offset: " + juce::String (chordIntervalOffset));
                translatedNote = rootNote + chordIntervalOffset; // Apply the offset to MIDI note 60
                
                if (noteIndex == 0)
                {
                    translatedNote = rootNote;
                }
                else
                {
//                    available1.erase (std::remove (available1.begin(), available1.end(), index), available1.end());
//                    available2.erase (std::remove (available2.begin(), available2.end(), index), available2.end());
//                    available3.erase (std::remove (available3.begin(), available3.end(), index), available3.end());
//                    available4.erase (std::remove (available4.begin(), available4.end(), index), available4.end());
                }
                
                if (noteIndex == 1)
                {
                    indexRoot = available2;
                }
                else if (noteIndex == 2)
                {
                    indexRoot = available3;
                }
                else if (noteIndex == 3)
                {
                    indexRoot = available4;
                }
                
                
                for (int i = 0; i < 7; i++)
                {
                    juce::Logger::writeToLog (juce::String (indexRoot[i]) + juce::String (" "));
                }
                

                
                noteIndex++;
                
                
                DBG ("Playing note: " + juce::String (translatedNote));

                noteMap[note] = translatedNote; // Map original note to translated note

                auto messageOn = juce::MidiMessage::noteOn(1, translatedNote, static_cast<uint8>(100));
                translatedMessages.addEvent(messageOn, sampleNumber);
                activeNotes.insert (note);
            }
        }

        // Process released notes (note off)
        for (auto it = activeNotes.begin(); it != activeNotes.end(); )
        {
            if (pressedNotes.find(*it) == pressedNotes.end()) // Note is no longer pressed
            {
                int originalNote = *it;
                int translatedNote = noteMap[originalNote]; // Get the translated note
                auto messageOff = juce::MidiMessage::noteOff (1, translatedNote, static_cast<uint8>(100));
                translatedMessages.addEvent (messageOff, sampleNumber);
                
                noteIndex--;
                if (noteIndex < 0) noteIndex = 0;
                


                noteMap.erase(originalNote); // Remove the mapping
                it = activeNotes.erase(it); // Remove note from active notes
            }
            else
            {
                ++it;
            }
        }

        midiMessages.swapWith(translatedMessages);
    }



private:

};
