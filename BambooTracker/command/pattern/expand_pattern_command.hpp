#pragma once

#include "abstract_command.hpp"
#include <memory>
#include <vector>
#include <string>
#include "module.hpp"

class ExpandPatternCommand : public AbstractCommand
{
public:
	ExpandPatternCommand(std::weak_ptr<Module> mod, int songNum,
						 int beginTrack, int beginColumn, int beginOrder, int beginStep,
						 int endTrack, int endColumn, int endStep);
	void redo() override;
	void undo() override;
	CommandId getID() const override;

private:
	std::weak_ptr<Module> mod_;
	int song_, bTrack_, bCol_, order_, bStep_;
	std::vector<std::vector<std::string>> prevCells_;
};
