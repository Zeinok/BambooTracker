#pragma once

#include "abstruct_command.hpp"
#include <string>
#include <memory>
#include "module.hpp"

class EraseStepCommand : public AbstructCommand
{
public:
	EraseStepCommand(std::weak_ptr<Module> mod, int songNum, int trackNum, int orderNum, int stepNum);
	void redo() override;
	void undo() override;
	int getID() const override;

private:
	std::weak_ptr<Module> mod_;
	int song_, track_, order_, step_;
	int prevNote_, prevInst_, prevVol_, prevEffVal_;
	std::string prevEffID_;
};