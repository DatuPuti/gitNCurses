#ifndef DIALOG_H
#define DIALOG_H

#include <ncurses.h>
#include <string>

class Dialog
{
public:
    struct DialogResult
    {
        bool confirmed;
        std::string input;
    };

    Dialog(int width = 60, int height = 7);
    ~Dialog();

    // Show a dialog with a title, prompt, and optional default value
    DialogResult show(const std::string &title,
                      const std::string &prompt,
                      const std::string &defaultValue = "");

private:
    int dialogWidth;
    int dialogHeight;
    WINDOW *dialogWin;
    WINDOW *inputWin;

    void drawDialog(const std::string &title,
                    const std::string &prompt,
                    const std::string &input,
                    int cursorPos,
                    int selectedItem);
};

#endif // DIALOG_H