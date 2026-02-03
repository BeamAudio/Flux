#ifndef UNDO_MANAGER_HPP
#define UNDO_MANAGER_HPP

#include <vector>
#include <memory>
#include <string>
#include <stack>

namespace Beam {

/**
 * @class FluxCommand
 * @brief Base class for all undoable actions in the DAW.
 */
class FluxCommand {
public:
    virtual ~FluxCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getName() const = 0;
};

/**
 * @class UndoManager
 * @brief Manages the undo and redo stacks.
 */
class UndoManager {
public:
    static UndoManager& get() {
        static UndoManager instance;
        return instance;
    }

    void perform(std::unique_ptr<FluxCommand> command) {
        command->execute();
        m_undoStack.push(std::move(command));
        // Clear redo stack on new action
        while (!m_redoStack.empty()) m_redoStack.pop();
    }

    void undo() {
        if (m_undoStack.empty()) return;
        auto command = std::move(m_undoStack.top());
        m_undoStack.pop();
        command->undo();
        m_redoStack.push(std::move(command));
    }

    void redo() {
        if (m_redoStack.empty()) return;
        auto command = std::move(m_redoStack.top());
        m_redoStack.pop();
        command->execute();
        m_undoStack.push(std::move(command));
    }

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    std::string getUndoName() const {
        return m_undoStack.empty() ? "" : m_undoStack.top()->getName();
    }

    std::string getRedoName() const {
        return m_redoStack.empty() ? "" : m_redoStack.top()->getName();
    }

    void clear() {
        while (!m_undoStack.empty()) m_undoStack.pop();
        while (!m_redoStack.empty()) m_redoStack.pop();
    }

private:
    std::stack<std::unique_ptr<FluxCommand>> m_undoStack;
    std::stack<std::unique_ptr<FluxCommand>> m_redoStack;
};

} // namespace Beam

#endif // UNDO_MANAGER_HPP
