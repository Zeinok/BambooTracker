#include "paste_mix_copied_data_to_pattern_qt_command.hpp"
#include "command_id.hpp"

PasteMixCopiedDataToPatternQtCommand::PasteMixCopiedDataToPatternQtCommand(PatternEditorPanel* panel, QUndoCommand* parent)
	: QUndoCommand(parent),
	  panel_(panel)
{
}

void PasteMixCopiedDataToPatternQtCommand::redo()
{
	panel_->redrawByPatternChanged(true);
}

void PasteMixCopiedDataToPatternQtCommand::undo()
{
	panel_->redrawByPatternChanged(true);
}

int PasteMixCopiedDataToPatternQtCommand::id() const
{
	return CommandId::PasteMixCopiedDataToPattern;
}
