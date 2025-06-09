#pragma once

#include <string>
#include <vector>
#include <memory>
#include "tui_terminal.h" // For Color and Terminal control

enum class SuggestionType {
    COMMAND,
    FUNCTION,
    VARIABLE
};

struct SuggestionItem {
    std::string text;
    SuggestionType type;
    std::string displayText; // Pre-formatted text for display
};

enum class SuggestionAction {
    IGNORED,          // Key was not handled by suggestion box
    NAVIGATION,       // Up/Down arrow was used for navigation
    APPLY_SUGGESTION, // Enter was pressed on a suggestion
    CLOSE_BOX         // Escape was pressed
};

class SuggestionBox {
private:
    bool visible;
    std::vector<SuggestionItem> suggestions;
    size_t selectedIndex;
    int maxDisplayItems;
    std::string currentPrefix; // The input prefix that generated these suggestions
    int termWidth; // To help with formatting

    std::string getFormattedSuggestionText(const std::string& text, SuggestionType type, int boxWidth);
    char getSuggestionTypeChar(SuggestionType type);

public:
    SuggestionBox(int terminalWidth, int maxItems = 5);

    void updateSuggestions(const std::string& prefix,
                           const std::vector<std::string>& variableNames,
                           const std::vector<std::string>& functionNames,
                           const std::vector<std::string>& commandNames);

    void draw(int inputRow, int inputColPromptOffset, int currentWordStartCol); // inputColPromptOffset is the column of '>'
                                                                                // currentWordStartCol is the column where the current word begins

    SuggestionAction handleKey(int key);

    bool isVisible() const;
    void hide();
    void show();

    SuggestionItem getSelectedSuggestion() const;
    const std::string& getCurrentInputPrefix() const;
};
