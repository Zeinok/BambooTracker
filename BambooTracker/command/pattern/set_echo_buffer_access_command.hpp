#pragma once

#include "abstract_command.hpp"
#include <string>
#include <memory>
#include "module.hpp"

class SetEchoBufferAccessCommand : public AbstractCommand
{
public:
	SetEchoBufferAccessCommand(std::weak_ptr<Module> mod, int songNum, int trackNum, int orderNum, int stepNum, int bufNum);
	void redo() override;
	void undo() override;
	CommandId getID() const override;

private:
	std::weak_ptr<Module> mod_;
	int song_, track_, order_, step_, buf_;
	int prevNote_;
};
