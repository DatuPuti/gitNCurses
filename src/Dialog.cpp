#include "Dialog.h"

Dialog::Dialog(int width, int height)
    : dialogWidth(width), dialogHeight(height), dialogWin(nullptr), inputWin(nullptr)
{
}

Dialog::~Dialog()
{
    if (inputWin)
        delwin(inputWin);
    if (dialogWin)
        delwin(dialogWin);
}

Dialog::DialogResult Dialog::show(const std::string &title,
                                  const std::string &prompt,
                                  const std::string &defaultValue)
{
    // Calculate dialog position
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int startY = (maxY - dialogHeight) / 2;
    int startX = (maxX - dialogWidth) / 2;

    // Create dialog window
    dialogWin = newwin(dialogHeight, dialogWidth, startY, startX);
    box(dialogWin, 0, 0);
    keypad(dialogWin, TRUE);

    // Create input field with box
    inputWin = derwin(dialogWin, 3, dialogWidth - 4, 3, 2);
    box(inputWin, 0, 0);
    wrefresh(dialogWin);
    wrefresh(inputWin);

    // Input handling
    std::string input = defaultValue;
    int cursorPos = input.length();
    bool confirmed = false;
    bool done = false;
    int selectedItem = 0; // 0 for input field, 1 for OK button, 2 for Cancel button

    while (!done)
    {
        drawDialog(title, prompt, input, cursorPos, selectedItem);

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
    inputWin = nullptr;
    dialogWin = nullptr;
    refresh();

    return {confirmed, input};
}

void Dialog::drawDialog(const std::string &title,
                        const std::string &prompt,
                        const std::string &input,
                        int cursorPos,
                        int selectedItem)
{
    // Clear and redraw dialog
    wclear(dialogWin);
    box(dialogWin, 0, 0);

    // Draw title
    mvwprintw(dialogWin, 0, 2, " %s ", title.c_str());

    // Draw prompt
    mvwprintw(dialogWin, 2, 2, "%s", prompt.c_str());

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
}