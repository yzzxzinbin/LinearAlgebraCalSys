#include "tui_suggestion_box.h"
#include "tui_terminal.h" // For Terminal, Color, keys
#include <algorithm>    // For std::sort, std::transform
#include <iostream>     // For std::cout (used by Terminal)

SuggestionBox::SuggestionBox(int terminalWidth, int maxItems)
    : visible(false), selectedIndex(0), maxDisplayItems(maxItems), termWidth(terminalWidth) {}

char SuggestionBox::getSuggestionTypeChar(SuggestionType type) {
    switch (type) {
        case SuggestionType::COMMAND:  return 'c';
        case SuggestionType::FUNCTION: return 'f';
        case SuggestionType::VARIABLE: return 'v';
        default: return ' ';
    }
}

std::string SuggestionBox::getFormattedSuggestionText(const std::string& text, SuggestionType type, int boxWidth) {
    char typeChar = getSuggestionTypeChar(type);
    std::string formatted = text;
    std::string marker = " [" + std::string(1, typeChar) + "]";

    // Ensure boxWidth is reasonable
    if (boxWidth <= static_cast<int>(marker.length())) {
        return text.substr(0, std::max(0, boxWidth -1)) + (boxWidth > 0 ? "~" : "");
    }

    int availableSpace = boxWidth - marker.length();
    if (static_cast<int>(text.length()) > availableSpace) {
        formatted = text.substr(0, availableSpace - 1) + "~"; // Truncate with tilde
    }
    
    formatted.resize(availableSpace, ' '); // Pad with spaces
    return formatted + marker;
}


void SuggestionBox::updateSuggestions(const std::string& prefix,
                                   const std::vector<std::string>& variableNames,
                                   const std::vector<std::string>& functionNames,
                                   const std::vector<std::string>& commandNames) {
    suggestions.clear();
    currentPrefix = prefix;

    if (prefix.empty()) {
        hide();
        return;
    }

    std::string lowerPrefix = prefix;
    std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

    auto addMatching = [&](const std::vector<std::string>& source, SuggestionType type) {
        for (const auto& itemText : source) {
            std::string lowerItemText = itemText;
            std::transform(lowerItemText.begin(), lowerItemText.end(), lowerItemText.begin(), ::tolower);
            if (lowerItemText.rfind(lowerPrefix, 0) == 0) { // Check if starts with prefix
                suggestions.push_back({itemText, type, ""}); // displayText will be set later or during draw
            }
        }
    };

    addMatching(commandNames, SuggestionType::COMMAND);
    addMatching(functionNames, SuggestionType::FUNCTION);
    addMatching(variableNames, SuggestionType::VARIABLE);

    // Sort suggestions: commands, then functions, then variables, then alphabetically
    std::sort(suggestions.begin(), suggestions.end(), [](const SuggestionItem& a, const SuggestionItem& b) {
        if (a.type != b.type) {
            return static_cast<int>(a.type) < static_cast<int>(b.type);
        }
        return a.text < b.text;
    });

    if (suggestions.empty()) {
        hide();
    } else if (suggestions.size() == 1 && suggestions[0].text == prefix) {
        // 如果只有一个候选词，且与当前输入完全一致，不显示候选框
        hide();
    } else {
        if (suggestions.size() > static_cast<size_t>(maxDisplayItems)) {
            suggestions.resize(maxDisplayItems);
        }
        selectedIndex = 0;
        show();
    }
}

void SuggestionBox::draw(int inputRow, int inputColPromptOffset, int currentWordStartColInInput) {
    if (!visible || suggestions.empty()) {
        return;
    }

    // Calculate box width based on the longest suggestion or a max width
    int maxSuggestionTextWidth = 0;
    for (const auto& item : suggestions) {
        if (item.text.length() > static_cast<size_t>(maxSuggestionTextWidth)) {
            maxSuggestionTextWidth = item.text.length();
        }
    }
    int boxContentWidth = maxSuggestionTextWidth + 4; // " [X]" marker
    int boxWidth = std::min(termWidth - (inputColPromptOffset + currentWordStartColInInput) -1 , boxContentWidth);
    boxWidth = std::max(10, boxWidth); // Minimum width


    int boxStartY = inputRow - static_cast<int>(suggestions.size());
    // Ensure box doesn't go off screen top
    if (boxStartY < 0) boxStartY = 0; 
    
    // The box should align with the start of the current word being typed
    int boxStartX = inputColPromptOffset + currentWordStartColInInput;


    for (size_t i = 0; i < suggestions.size(); ++i) {
        if (boxStartY + static_cast<int>(i) >= inputRow) break; // Don't overlap with input line itself

        Terminal::setCursor(boxStartY + i, boxStartX);
        
        std::string displayText = getFormattedSuggestionText(suggestions[i].text, suggestions[i].type, boxWidth);

        if (i == selectedIndex) {
            Terminal::setBackground(Color::WHITE);
            Terminal::setForeground(Color::BLACK);
        } else {
            Terminal::setBackground(Color::BLACK); // Or your default background
            Terminal::setForeground(Color::CYAN);   // Or your default suggestion text color
        }
        std::cout << displayText;
        // Clear rest of the line for this suggestion item if box is narrower than previous
        for(int k=displayText.length(); k < boxWidth; ++k) std::cout << " ";

        Terminal::resetColor();
    }
}

SuggestionAction SuggestionBox::handleKey(int key) {
    if (!visible || suggestions.empty()) {
        return SuggestionAction::IGNORED;
    }

    switch (key) {
        case KEY_UP:
            if (selectedIndex > 0) {
                selectedIndex--;
            } else {
                selectedIndex = suggestions.size() - 1; // Wrap around
            }
            return SuggestionAction::NAVIGATION;
        case KEY_DOWN:
            if (selectedIndex < suggestions.size() - 1) {
                selectedIndex++;
            } else {
                selectedIndex = 0; // Wrap around
            }
            return SuggestionAction::NAVIGATION;
        case KEY_TAB:  // 改用Tab键代替Enter键来选择候选词
            return SuggestionAction::APPLY_SUGGESTION;
        case KEY_ESCAPE:
            hide();
            return SuggestionAction::CLOSE_BOX;
        default:
            return SuggestionAction::IGNORED;
    }
}

bool SuggestionBox::isVisible() const {
    return visible;
}

void SuggestionBox::hide() {
    visible = false;
}

void SuggestionBox::show() {
    visible = true;
}

SuggestionItem SuggestionBox::getSelectedSuggestion() const {
    if (!suggestions.empty() && selectedIndex < suggestions.size()) {
        return suggestions[selectedIndex];
    }
    return {"", SuggestionType::COMMAND, ""}; // Should not happen if visible
}

const std::string& SuggestionBox::getCurrentInputPrefix() const {
    return currentPrefix;
}
