#include "GitCommandHandler.h"
#include <cstdio>

GitCommandHandler::GitCommandHandler()
{
    updateLocalBranches();
}

std::string GitCommandHandler::executeGitCommand(const std::string &command)
{
    std::string fullCommand = "git " + command;
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(fullCommand.c_str(), "r"), pclose);

    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    return result;
}

std::vector<std::string> GitCommandHandler::getLocalBranches()
{
    std::vector<std::string> branches;
    try
    {
        std::string output = executeGitCommand("branch --format='%(refname:short)'");
        std::istringstream iss(output);
        std::string branch;
        while (std::getline(iss, branch))
        {
            if (!branch.empty())
            {
                branches.push_back(branch);
            }
        }
    }
    catch (...)
    {
        // If there's an error, return empty vector
    }
    return branches;
}

void GitCommandHandler::updateLocalBranches()
{
    localBranches = getLocalBranches();
}

std::string GitCommandHandler::getCurrentBranch()
{
    try
    {
        std::string output = executeGitCommand("branch --show-current");
        if (output.empty())
            return "detached HEAD";
        return output.substr(0, output.length() - 1); // Remove trailing newline
    }
    catch (...)
    {
        return "unknown";
    }
}

std::string GitCommandHandler::getRepositoryStatus()
{
    try
    {
        std::string output = executeGitCommand("status --porcelain");
        if (output.empty())
            return "Clean";
        return "Modified";
    }
    catch (...)
    {
        return "Error";
    }
}

std::string GitCommandHandler::executeCommand(const std::string &command)
{
    std::string cmd = command;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    if (cmd == "help")
    {
        return "Available commands:\n"
               "  status    - Show working tree status\n"
               "  branch    - List branches\n"
               "  log       - Show commit logs\n"
               "  add       - Add file contents to index\n"
               "  commit    - Record changes to repository\n"
               "  push      - Update remote refs\n"
               "  pull      - Fetch and integrate changes\n"
               "  help      - Show this help message\n"
               "  exit      - Exit the application\n";
    }

    try
    {
        std::string fullCommand;
        std::string output;

        // Handle Branch menu commands
        if (cmd == "add")
        {
            fullCommand = "git add .";
            output = addFiles();
        }
        else if (cmd.substr(0, 7) == "commit ")
        {
            std::string message = cmd.substr(7);
            fullCommand = "git commit -m \"" + message + "\"";
            output = commitChanges(message);
        }
        else if (cmd == "push")
        {
            fullCommand = "git push";
            output = pushChanges();
        }
        else if (cmd == "pull")
        {
            fullCommand = "git fetch";
            output = pullChanges();
        }
        // Check if the command is a branch name (for switching)
        else if (std::find(localBranches.begin(), localBranches.end(), command) != localBranches.end())
        {
            fullCommand = "git checkout " + command;
            output = executeGitCommand(cmd);

            // Update branch list if we switched branches
            updateLocalBranches();
        }
        else
        {
            fullCommand = "git " + cmd;
            output = executeGitCommand(cmd);
        }

        // Combine command and output
        std::string combinedOutput = "$ " + fullCommand + "\n\n" + output;
        return combinedOutput;
    }
    catch (const std::exception &e)
    {
        return std::string("Error: ") + e.what() + "\n";
    }
}

bool GitCommandHandler::isLocalBranch(const std::string &branchName) const
{
    return std::find(localBranches.begin(), localBranches.end(), branchName) != localBranches.end();
}

std::string GitCommandHandler::addFiles(const std::string &files)
{
    try
    {
        std::string command = "add " + files;
        std::string output = executeGitCommand(command);
        if (output.empty())
        {
            return "Files added successfully.";
        }
        return output;
    }
    catch (const std::exception &e)
    {
        return std::string("Error adding files: ") + e.what();
    }
}

std::string GitCommandHandler::commitChanges(const std::string &message)
{
    try
    {
        if (message.empty())
        {
            return "Error: Commit message cannot be empty.";
        }

        // Escape quotes in the message
        std::string escapedMessage = message;
        size_t pos = 0;
        while ((pos = escapedMessage.find("\"", pos)) != std::string::npos)
        {
            escapedMessage.replace(pos, 1, "\\\"");
            pos += 2;
        }

        std::string command = "commit -m \"" + escapedMessage + "\"";
        std::string output = executeGitCommand(command);
        if (output.empty())
        {
            return "No changes to commit.";
        }
        return output;
    }
    catch (const std::exception &e)
    {
        return std::string("Error committing changes: ") + e.what();
    }
}

std::string GitCommandHandler::pushChanges(const std::string &remote, const std::string &branch)
{
    try
    {
        std::string command = "push " + remote;
        if (!branch.empty())
        {
            command += " " + branch;
        }
        std::string output = executeGitCommand(command);
        if (output.empty())
        {
            return "No changes to push.";
        }
        return output;
    }
    catch (const std::exception &e)
    {
        return std::string("Error pushing changes: ") + e.what();
    }
}

std::string GitCommandHandler::pullChanges(const std::string &remote, const std::string &branch)
{
    try
    {
        std::string command = "pull " + remote;
        if (!branch.empty())
        {
            command += " " + branch;
        }
        std::string output = executeGitCommand(command);
        if (output.empty())
        {
            return "No changes to pull.";
        }
        return output;
    }
    catch (const std::exception &e)
    {
        return std::string("Error pulling changes: ") + e.what();
    }
}