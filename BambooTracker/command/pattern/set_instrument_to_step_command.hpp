#pragma once

#include "abstract_command.hpp"
#include <memory>
#include "module.hpp"

class SetInstrumentToStepCommand : public AbstractCommand
{
public:
	SetInstrumentToStepCommand(std::weak_ptr<Module> mod, int songNum, int trackNum, int orderNum, int stepNum, int instNum, bool secondEntry);
	void redo() override;
	void undo() override;
	CommandId getID() const override;
	bool mergeWith(const AbstractCommand* other) override;

	int getSong() const;
	int getTrack() const;
	int getOrder() const;
	int getStep() const;
	bool isSecondEntry() const;
	int getInst() const;

private:
	std::weak_ptr<Module> mod_;
	int song_, track_, order_, step_, inst_;
	int prevInst_;
	bool isSecond_;
};
