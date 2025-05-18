#ifndef GIT_COMMAND_HANDLER_H
#define GIT_COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <algorithm>

class GitCommandHandler
{
private:
    std::vector<std::string> localBranches;
    std::string executeGitCommand(const std::string &command);

public:
    GitCommandHandler();
    std::vector<std::string> getLocalBranches();
    void updateLocalBranches();
    std::string getCurrentBranch();
    std::string getRepositoryStatus();
    std::string executeCommand(const std::string &command);
    bool isLocalBranch(const std::string &branchName) const;

    // Branch menu commands
    std::string addFiles(const std::string &files = "."); // Default to all files
    std::string commitChanges(const std::string &message);
    std::string pushChanges(const std::string &remote = "origin", const std::string &branch = "");
    std::string pullChanges(const std::string &remote = "origin", const std::string &branch = "");
};

#endif // GIT_COMMAND_HANDLER_H