#pragma once
#include "tui_terminal.h"
#include <string>
#include <vector>
#include <utility>

struct HelpTopic {
    std::string title;
    std::string content;
};

struct HelpPage {
    std::string pageTitle;
    std::vector<HelpTopic> topics;
};

class EnhancedHelpViewer {
public:
    enum class ViewerResult {
        CONTINUE,
        EXIT
    };

private:
    std::vector<HelpPage> helpPages;
    size_t currentPageIndex;
    size_t currentTopicIndex;
    size_t scrollOffset;

    int terminalRows;
    int terminalCols;

    // Layout dimensions
    int listStartRow, listStartCol, listHeight, listWidth;
    int detailStartRow, detailStartCol, detailHeight, detailWidth;

    std::string statusMessage;

    void initializeHelpContent();

    // Drawing methods
    void drawLayout();
    void drawTopicList();
    void drawDetailView();
    void clearScreen();

    // Helper methods
    void updateLayout();
    void updateScrolling();
    void updateStatus(const std::string& msg);

public:
    EnhancedHelpViewer(int termRows, int termCols);
    ~EnhancedHelpViewer();

    ViewerResult handleInput(int key);
    void draw();
    std::string getStatusMessage() const;
    void updateDimensions(int termRows, int termCols);
};
