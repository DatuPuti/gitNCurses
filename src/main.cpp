#include <ncurses.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <array>
#include <map>
#include <algorithm>
#include <sstream>
#include "GitCommandHandler.h"

class GitNCurses
{
private:
    WINDOW *mainWin;
    WINDOW *menuWin;
    WINDOW *outputWin;
    WINDOW *inputWin;
    WINDOW *statusWin; // New status bar window
    int maxY, maxX;
    std::vector<std::string> commandHistory;
    int historyIndex;
    bool isMenuActive;                    // Track if menu is active
    WINDOW *activeWindow;                 // Track the currently active window
    int scrollPosition;                   // Track current scroll position
    std::vector<std::string> outputLines; // Store output lines for scrolling
    GitCommandHandler gitHandler;         // Add GitCommandHandler instance

    struct MenuItem
    {
        std::string name;
        std::vector<std::string> submenu;
        bool isDynamic;                          // Flag to indicate if submenu should be dynamically populated
        std::vector<std::string> dynamicSubmenu; // For nested dynamic submenus
    };

    std::vector<MenuItem> mainMenu;
    int selectedMenu;
    bool showSubmenu;
    int selectedSubmenu;
    bool showDynamicSubmenu;    // Flag for showing nested submenu
    int selectedDynamicSubmenu; // Selection in nested submenu

    struct DialogResult
    {
        bool confirmed;
        std::string input;
    };

    DialogResult showCommitDialog()
    {
        // Calculate dialog dimensions
        int dialogWidth = 60;
        int dialogHeight = 7;
        int startY = (maxY - dialogHeight) / 2;
        int startX = (maxX - dialogWidth) / 2;

        // Create dialog window
        WINDOW *dialogWin = newwin(dialogHeight, dialogWidth, startY, startX);
        box(dialogWin, 0, 0);
        keypad(dialogWin, TRUE);

        // Draw title
        mvwprintw(dialogWin, 0, 2, " Commit Message ");

        // Draw prompt
        mvwprintw(dialogWin, 2, 2, "Please provide a commit message:");

        // Create input field with box
        WINDOW *inputWin = derwin(dialogWin, 3, 54, 3, 2);
        box(inputWin, 0, 0);
        wrefresh(dialogWin);
        wrefresh(inputWin);

        // Draw buttons
        mvwprintw(dialogWin, 5, 15, "[ OK ]");
        mvwprintw(dialogWin, 5, 35, "[ Cancel ]");
        wrefresh(dialogWin);

        // Input handling
        std::string input;
        int cursorPos = 0;
        bool confirmed = false;
        bool done = false;
        int selectedItem = 0; // 0 for input field, 1 for OK button, 2 for Cancel button

        while (!done)
        {
            // Clear and redraw input field
            wclear(inputWin);
            box(inputWin, 0, 0);

            // Draw input text with subtle highlighting when selected
            if (selectedItem == 0)
            {
                wattron(inputWin, A_DIM);
                box(inputWin, 0, 0);
                wattroff(inputWin, A_DIM);
            }
            mvwprintw(inputWin, 1, 1, "%s", input.c_str());
            wmove(inputWin, 1, cursorPos + 1);
            wrefresh(inputWin);

            // Draw buttons with appropriate highlighting
            for (int i = 0; i < 2; i++)
            {
                if (selectedItem == i + 1)
                {
                    wattron(dialogWin, A_REVERSE);
                    mvwprintw(dialogWin, 5, 15 + (i * 20), "[ %s ]", i == 0 ? "OK" : "Cancel");
                    wattroff(dialogWin, A_REVERSE);
                }
                else
                {
                    mvwprintw(dialogWin, 5, 15 + (i * 20), "[ %s ]", i == 0 ? "OK" : "Cancel");
                }
            }
            wrefresh(dialogWin);

            int ch = wgetch(dialogWin);
            switch (ch)
            {
            case '\t': // Tab key
                selectedItem = (selectedItem + 1) % 3;
                break;
            case KEY_BTAB: // Shift+Tab
                selectedItem = (selectedItem - 1 + 3) % 3;
                break;
            case '\n':
                if (selectedItem == 1 && !input.empty())
                { // OK button
                    confirmed = true;
                    done = true;
                }
                else if (selectedItem == 2)
                { // Cancel button
                    done = true;
                }
                break;
            case 27: // ESC
                done = true;
                break;
            case KEY_BACKSPACE:
            case 127:
                if (selectedItem == 0 && cursorPos > 0)
                {
                    input.erase(cursorPos - 1, 1);
                    cursorPos--;
                }
                break;
            default:
                if (selectedItem == 0 && isprint(ch) && input.length() < 50)
                {
                    input.insert(cursorPos, 1, static_cast<char>(ch));
                    cursorPos++;
                }
                break;
            }
        }

        // Clean up
        delwin(inputWin);
        delwin(dialogWin);
        refresh();

        return {confirmed, input};
    }

    void initWindows()
    {
        // Initialize ncurses
        initscr();
        start_color();
        noecho();
        cbreak();
        keypad(stdscr, TRUE);
        curs_set(0); // Hide cursor

        // Get screen dimensions
        getmaxyx(stdscr, maxY, maxX);

        // Calculate window sizes
        int menuHeight = 3;
        int inputHeight = 3;
        int statusHeight = 1;                                              // Height for status bar
        int outputHeight = maxY - menuHeight - inputHeight - statusHeight; // Adjusted for status bar

        // Initialize colors
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_CYAN, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLUE);
        init_pair(5, COLOR_BLACK, COLOR_WHITE); // New color pair for status bar

        // Create menu window (top)
        menuWin = newwin(menuHeight, maxX, 0, 0);
        box(menuWin, 0, 0);

        // Create output window (middle)
        outputWin = newwin(outputHeight, maxX, menuHeight, 0);
        scrollok(outputWin, TRUE);
        box(outputWin, 0, 0);

        // Create input window (bottom)
        inputWin = newwin(inputHeight, maxX, menuHeight + outputHeight, 0);
        box(inputWin, 0, 0);

        // Create status bar window (very bottom)
        statusWin = newwin(statusHeight, maxX, menuHeight + outputHeight + inputHeight, 0);
        wbkgd(statusWin, COLOR_PAIR(5)); // Set background color for status bar

        // Enable keypad for special keys
        keypad(menuWin, TRUE);
        keypad(outputWin, TRUE);
        keypad(inputWin, TRUE);
        keypad(statusWin, TRUE);

        // Initialize menu structure
        mainMenu = {
            {"File", {"Exit"}},
            {"Branch", {"Status", "Branch", "Log", "Add", "Commit", "Push", "Pull"}},
            {"Git", {"Create Branch", "Rename Branch", "Delete Local Branch", "Checkout Remote", "List All Branches", "Merge Branch", "Switch Branch", "Stash Changes", "Stash Pop"}},
            {"Help", {"Help"}}};

        // Initialize local branches
        updateLocalBranches();

        selectedMenu = 0;
        showSubmenu = false;
        selectedSubmenu = 0;
        showDynamicSubmenu = false;
        selectedDynamicSubmenu = 0;

        // Initialize window states
        isMenuActive = true;
        activeWindow = menuWin;

        // Initialize scroll position
        scrollPosition = 0;

        // Clear the screen and refresh
        clear();
        refresh();

        // Draw and refresh all windows
        drawMenu();
        updateStatusBar(); // Initialize status bar
        wrefresh(menuWin);
        wrefresh(outputWin);
        wrefresh(inputWin);
        wrefresh(statusWin);

        // Force a final refresh of the entire screen
        doupdate();
    }

    void drawMenu()
    {
        // Clear and redraw main menu
        wclear(menuWin);
        box(menuWin, 0, 0);
        int x = 2;
        for (size_t i = 0; i < mainMenu.size(); i++)
        {
            if (i == selectedMenu)
            {
                wattron(menuWin, COLOR_PAIR(4));
            }
            else
            {
                wattron(menuWin, COLOR_PAIR(3));
            }
            mvwprintw(menuWin, 1, x, "%s", mainMenu[i].name.c_str());
            wattroff(menuWin, COLOR_PAIR(3) | COLOR_PAIR(4));
            x += mainMenu[i].name.length() + 2;
        }

        // Redraw main windows first
        box(outputWin, 0, 0);
        box(inputWin, 0, 0);
        wrefresh(outputWin);
        wrefresh(inputWin);
        wrefresh(statusWin);
        wrefresh(menuWin);

        // Draw submenu if active
        if (showSubmenu)
        {
            // Calculate submenu position
            int submenuWidth = 20;
            int submenuHeight = mainMenu[selectedMenu].submenu.size() + 2;
            int submenuX = 2;
            for (size_t i = 0; i < selectedMenu; i++)
            {
                submenuX += mainMenu[i].name.length() + 2;
            }
            int submenuY = 3;

            // Create and draw main submenu
            WINDOW *submenuWin = newwin(submenuHeight, submenuWidth, submenuY, submenuX);
            wbkgd(submenuWin, COLOR_PAIR(3));
            box(submenuWin, 0, 0);

            // Draw submenu items
            for (size_t i = 0; i < mainMenu[selectedMenu].submenu.size(); i++)
            {
                if (i == selectedSubmenu)
                {
                    wattron(submenuWin, COLOR_PAIR(4));
                }
                else
                {
                    wattron(submenuWin, COLOR_PAIR(3));
                }
                mvwprintw(submenuWin, i + 1, 1, "%s", mainMenu[selectedMenu].submenu[i].c_str());
                wattroff(submenuWin, COLOR_PAIR(3) | COLOR_PAIR(4));
            }
            wrefresh(submenuWin);

            // Draw dynamic submenu if active
            if (showDynamicSubmenu && mainMenu[selectedMenu].submenu[selectedSubmenu] == "Switch Branch")
            {
                int dynamicSubmenuWidth = 30;
                int dynamicSubmenuHeight = mainMenu[selectedMenu].dynamicSubmenu.size() + 2;
                int dynamicSubmenuX = submenuX + submenuWidth;
                int dynamicSubmenuY = submenuY + selectedSubmenu + 1;

                WINDOW *dynamicSubmenuWin = newwin(dynamicSubmenuHeight, dynamicSubmenuWidth,
                                                   dynamicSubmenuY, dynamicSubmenuX);
                wbkgd(dynamicSubmenuWin, COLOR_PAIR(3));
                box(dynamicSubmenuWin, 0, 0);

                for (size_t i = 0; i < mainMenu[selectedMenu].dynamicSubmenu.size(); i++)
                {
                    if (i == selectedDynamicSubmenu)
                    {
                        wattron(dynamicSubmenuWin, COLOR_PAIR(4));
                    }
                    else
                    {
                        wattron(dynamicSubmenuWin, COLOR_PAIR(3));
                    }
                    mvwprintw(dynamicSubmenuWin, i + 1, 1, "%s", mainMenu[selectedMenu].dynamicSubmenu[i].c_str());
                    wattroff(dynamicSubmenuWin, COLOR_PAIR(3) | COLOR_PAIR(4));
                }
                wrefresh(dynamicSubmenuWin);
                delwin(dynamicSubmenuWin);
            }

            delwin(submenuWin);
        }

        // Force a final refresh to ensure everything is visible
        doupdate();
    }

    std::string getMenuDescription(const std::string &menu, const std::string &submenu)
    {
        if (menu == "File")
        {
            if (submenu == "Exit")
                return "Exit the application";
        }
        else if (menu == "Branch")
        {
            if (submenu == "Status")
                return "Show working tree status";
            else if (submenu == "Branch")
                return "List or create branches";
            else if (submenu == "Log")
                return "Show commit history";
            else if (submenu == "Add")
                return "Add files to staging area";
            else if (submenu == "Commit")
                return "Commit staged changes";
            else if (submenu == "Push")
                return "Push commits to remote";
            else if (submenu == "Pull")
                return "Pull changes from remote";
        }
        else if (menu == "Git")
        {
            if (submenu == "Create Branch")
                return "Create a new branch";
            else if (submenu == "Rename Branch")
                return "Rename current branch";
            else if (submenu == "Delete Local Branch")
                return "Delete a local branch";
            else if (submenu == "Checkout Remote")
                return "Checkout a remote branch";
            else if (submenu == "List All Branches")
                return "Show all local and remote branches";
            else if (submenu == "Merge Branch")
                return "Merge another branch into current";
            else if (submenu == "Switch Branch")
                return "Switch to another branch";
            else if (submenu == "Stash Changes")
                return "Save changes temporarily";
            else if (submenu == "Stash Pop")
                return "Apply stashed changes";
        }
        else if (menu == "Help")
        {
            if (submenu == "Help")
                return "Show help information";
        }
        return "";
    }

    void updateStatusBar()
    {
        wclear(statusWin);
        std::string branch = gitHandler.getCurrentBranch();
        std::string status = gitHandler.getRepositoryStatus();

        // Get menu description only for highlighted submenu items
        std::string description;
        if (showSubmenu)
        {
            description = getMenuDescription(mainMenu[selectedMenu].name,
                                             mainMenu[selectedMenu].submenu[selectedSubmenu]);
        }

        // Format status bar content
        std::string statusText = branch + " (" + status + ")";

        // Calculate positions
        int descX = 2;                                // Left-aligned description
        int statusX = maxX - statusText.length() - 2; // Right-aligned status

        // Print description and status
        mvwprintw(statusWin, 0, descX, "%s", description.c_str());
        mvwprintw(statusWin, 0, statusX, "%s", statusText.c_str());

        wrefresh(statusWin);
    }

    void updateLocalBranches()
    {
        // Update the dynamic submenu for Switch Branch
        for (auto &item : mainMenu)
        {
            if (item.name == "Git")
            {
                for (size_t i = 0; i < item.submenu.size(); i++)
                {
                    if (item.submenu[i] == "Switch Branch")
                    {
                        item.dynamicSubmenu = gitHandler.getLocalBranches();
                        break;
                    }
                }
            }
        }
    }

    void resizeWindows()
    {
        // Get new screen dimensions
        getmaxyx(stdscr, maxY, maxX);

        // Calculate window sizes
        int menuHeight = 3;
        int inputHeight = 3;
        int statusHeight = 1;                                              // Height for status bar
        int outputHeight = maxY - menuHeight - inputHeight - statusHeight; // Adjusted for status bar

        // Delete old windows
        delwin(menuWin);
        delwin(outputWin);
        delwin(inputWin);
        delwin(statusWin);

        // Create new windows with updated sizes
        menuWin = newwin(menuHeight, maxX, 0, 0);
        outputWin = newwin(outputHeight, maxX, menuHeight, 0);
        inputWin = newwin(inputHeight, maxX, menuHeight + outputHeight, 0);
        statusWin = newwin(statusHeight, maxX, menuHeight + outputHeight + inputHeight, 0);

        // Re-enable scrolling for output window
        scrollok(outputWin, TRUE);

        // Re-enable keypad for all windows
        keypad(menuWin, TRUE);
        keypad(outputWin, TRUE);
        keypad(inputWin, TRUE);
        keypad(statusWin, TRUE);

        // Set status bar background
        wbkgd(statusWin, COLOR_PAIR(5));

        // Redraw all windows
        clear();
        refresh();
        drawMenu();
        box(outputWin, 0, 0);
        box(inputWin, 0, 0);
        updateStatusBar();
        wrefresh(menuWin);
        wrefresh(outputWin);
        wrefresh(inputWin);
        wrefresh(statusWin);

        // Force a final refresh
        doupdate();
    }

    void handleMenuInput(int ch)
    {
        bool menuChanged = false;
        bool contentChanged = false;

        switch (ch)
        {
        case 'i':
        case 'I':
            if (isMenuActive)
            {
                isMenuActive = false;
                activeWindow = inputWin;
                std::string command = getInput();
                if (!command.empty())
                {
                    executeMenuCommand(command);
                }
                isMenuActive = true;
                activeWindow = menuWin;
                drawMenu();
            }
            break;
        case KEY_LEFT:
            if (!showSubmenu && isMenuActive)
            {
                selectedMenu = (selectedMenu - 1 + mainMenu.size()) % mainMenu.size();
                menuChanged = true;
            }
            break;
        case KEY_RIGHT:
            if (!showSubmenu && isMenuActive)
            {
                selectedMenu = (selectedMenu + 1) % mainMenu.size();
                menuChanged = true;
            }
            break;
        case KEY_UP:
            if (showDynamicSubmenu && isMenuActive)
            {
                selectedDynamicSubmenu = (selectedDynamicSubmenu - 1 + mainMenu[selectedMenu].dynamicSubmenu.size()) % mainMenu[selectedMenu].dynamicSubmenu.size();
                menuChanged = true;
            }
            else if (showSubmenu && isMenuActive)
            {
                selectedSubmenu = (selectedSubmenu - 1 + mainMenu[selectedMenu].submenu.size()) % mainMenu[selectedMenu].submenu.size();
                showDynamicSubmenu = false;
                menuChanged = true;
            }
            else if (!isMenuActive)
            {
                // Scroll up one line
                if (scrollPosition > 0)
                {
                    scrollPosition--;
                    contentChanged = true;
                }
            }
            break;
        case KEY_DOWN:
            if (showDynamicSubmenu && isMenuActive)
            {
                selectedDynamicSubmenu = (selectedDynamicSubmenu + 1) % mainMenu[selectedMenu].dynamicSubmenu.size();
                menuChanged = true;
            }
            else if (showSubmenu && isMenuActive)
            {
                selectedSubmenu = (selectedSubmenu + 1) % mainMenu[selectedMenu].submenu.size();
                showDynamicSubmenu = false;
                menuChanged = true;
            }
            else if (!isMenuActive)
            {
                // Scroll down one line
                int visibleLines = getmaxy(outputWin) - 2;
                if (scrollPosition < static_cast<int>(outputLines.size()) - visibleLines)
                {
                    scrollPosition++;
                    contentChanged = true;
                }
            }
            break;
        case '\n':
            if (!showSubmenu && isMenuActive)
            {
                showSubmenu = true;
                selectedSubmenu = 0;
                showDynamicSubmenu = false;
                menuChanged = true;
                if (mainMenu[selectedMenu].name == "Git")
                {
                    updateLocalBranches();
                }
            }
            else if (!showDynamicSubmenu && mainMenu[selectedMenu].submenu[selectedSubmenu] == "Switch Branch" && isMenuActive)
            {
                showDynamicSubmenu = true;
                selectedDynamicSubmenu = 0;
                menuChanged = true;
            }
            else if (showDynamicSubmenu && isMenuActive)
            {
                executeMenuCommand(mainMenu[selectedMenu].dynamicSubmenu[selectedDynamicSubmenu]);
                showDynamicSubmenu = false;
                showSubmenu = false;
                menuChanged = true;
            }
            else if (showSubmenu && isMenuActive)
            {
                executeMenuCommand(mainMenu[selectedMenu].submenu[selectedSubmenu]);
                showSubmenu = false;
                menuChanged = true;
            }
            break;
        case 27: // ESC
            if (showDynamicSubmenu && isMenuActive)
            {
                showDynamicSubmenu = false;
                selectedDynamicSubmenu = 0;
                menuChanged = true;
            }
            else if (showSubmenu && isMenuActive)
            {
                showSubmenu = false;
                selectedSubmenu = 0;
                menuChanged = true;
            }
            break;
        case KEY_PPAGE: // Page Up
            if (!isMenuActive)
            {
                int visibleLines = getmaxy(outputWin) - 2;
                scrollPosition = std::max(0, scrollPosition - visibleLines);
                contentChanged = true;
            }
            break;
        case KEY_NPAGE: // Page Down
            if (!isMenuActive)
            {
                int visibleLines = getmaxy(outputWin) - 2;
                scrollPosition = std::min(static_cast<int>(outputLines.size()) - visibleLines,
                                          scrollPosition + visibleLines);
                contentChanged = true;
            }
            break;
        }

        if (menuChanged)
        {
            drawMenu();
            updateStatusBar();
            doupdate();
        }
        else if (contentChanged)
        {
            // Redisplay the current content with new scroll position
            std::string currentContent;
            for (const auto &line : outputLines)
            {
                currentContent += line + "\n";
            }
            displayOutput(currentContent);
        }
    }

    void executeMenuCommand(const std::string &command)
    {
        std::string cmd = command;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c)
                       { return std::tolower(c); });

        if (cmd == "exit")
        {
            endwin();
            exit(0);
        }

        std::string output;
        if (cmd == "commit")
        {
            auto result = showCommitDialog();
            if (result.confirmed)
            {
                output = gitHandler.executeCommand("commit " + result.input);
            }
            else
            {
                // Return to menu without executing commit
                drawMenu();
                updateStatusBar();
                return;
            }
        }
        else
        {
            output = gitHandler.executeCommand(command);
        }

        displayOutput(output);
        updateStatusBar();

        // Keep menu active and visible
        isMenuActive = true;
        activeWindow = menuWin;
        drawMenu();
    }

    void displayOutput(const std::string &output)
    {
        wclear(outputWin);
        box(outputWin, 0, 0);

        // Get window dimensions
        int maxY, maxX;
        getmaxyx(outputWin, maxY, maxX);

        // Create a subwindow for the content area, leaving space for scrollbar
        WINDOW *contentWin = derwin(outputWin, maxY - 2, maxX - 3, 1, 1);
        scrollok(contentWin, TRUE);

        // Split output into lines and store them
        outputLines.clear();
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line))
        {
            outputLines.push_back(line);
        }

        // Ensure scroll position is valid
        int visibleLines = maxY - 2;
        scrollPosition = std::min(scrollPosition, static_cast<int>(outputLines.size()) - visibleLines);
        scrollPosition = std::max(0, scrollPosition);

        // Display visible lines
        for (int i = 0; i < visibleLines && (i + scrollPosition) < outputLines.size(); i++)
        {
            mvwprintw(contentWin, i, 0, "%s", outputLines[i + scrollPosition].c_str());
        }

        // Draw scrollbar if needed
        if (outputLines.size() > visibleLines)
        {
            // Draw scrollbar track
            for (int i = 1; i < maxY - 1; i++)
            {
                mvwaddch(outputWin, i, maxX - 2, ACS_VLINE);
            }

            // Calculate scrollbar thumb position and length
            float scrollbarRatio = static_cast<float>(visibleLines) / outputLines.size();
            int scrollbarLength = std::max(1, static_cast<int>(visibleLines * scrollbarRatio));
            int scrollbarPos = static_cast<int>((scrollPosition * (visibleLines - scrollbarLength)) /
                                                (outputLines.size() - visibleLines));

            // Draw scrollbar thumb
            for (int i = 0; i < scrollbarLength; i++)
            {
                mvwaddch(outputWin, 1 + scrollbarPos + i, maxX - 2, ACS_BLOCK);
            }
        }

        // Refresh both windows
        wrefresh(contentWin);
        wrefresh(outputWin);

        // Clean up the subwindow
        delwin(contentWin);
    }

    std::string getInput()
    {
        wclear(inputWin);
        box(inputWin, 0, 0);
        mvwprintw(inputWin, 1, 1, "git> ");
        wrefresh(inputWin);

        char input[256];
        echo();
        wgetstr(inputWin, input);
        noecho();

        std::string command(input);
        if (!command.empty())
        {
            commandHistory.push_back(command);
            historyIndex = commandHistory.size();
        }

        return command;
    }

public:
    GitNCurses() : historyIndex(0)
    {
        initWindows();
    }

    ~GitNCurses()
    {
        delwin(menuWin);
        delwin(outputWin);
        delwin(inputWin);
        delwin(statusWin);
        endwin();
    }

    void run()
    {
        // Clear output window and show empty state
        wclear(outputWin);
        box(outputWin, 0, 0);
        wrefresh(outputWin);

        while (true)
        {
            int ch = wgetch(activeWindow);
            if (ch == KEY_MOUSE)
            {
                // Handle mouse events if needed
                continue;
            }

            if (ch == KEY_RESIZE)
            {
                resizeWindows();
                continue;
            }

            handleMenuInput(ch);

            // Also allow direct command input
            if (ch == '\n' && !showSubmenu && !isMenuActive)
            {
                std::string command = getInput();
                if (!command.empty())
                {
                    executeMenuCommand(command);
                }
            }
        }
    }
};

int main()
{
    GitNCurses app;
    app.run();
    return 0;
}