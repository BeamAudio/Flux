#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "engine/session/undo_manager.hpp"
#include "engine/session/parameter.hpp"
#include "engine/core/flux_graph.hpp"
#include <vector>

namespace Beam {

/**
 * @class ParameterChangeCommand
 */
class ParameterChangeCommand : public FluxCommand {
// ... (existing code remains same)
public:
    ParameterChangeCommand(std::shared_ptr<Parameter> param, float oldValue, float newValue)
        : m_param(param), m_oldValue(oldValue), m_newValue(newValue) {}

    void execute() override { m_param->setValue(m_newValue); }
    void undo() override { m_param->setValue(m_oldValue); }
    std::string getName() const override { return "Change " + m_param->getName(); }

private:
    std::shared_ptr<Parameter> m_param;
    float m_oldValue;
    float m_newValue;
};

/**
 * @class RemoveNodeCommand
 */
class RemoveNodeCommand : public FluxCommand {
public:
    RemoveNodeCommand(FluxGraph* graph, size_t id)
        : m_graph(graph), m_id(id) {
        m_node = graph->getNode(id);
        // Backup connections
        for (const auto& c : graph->getConnections()) {
            if (c.srcNodeId == id || c.dstNodeId == id) {
                m_connections.push_back(c);
            }
        }
    }

    void execute() override {
        m_graph->removeNode(m_id);
    }

    void undo() override {
        if (m_node) {
            m_graph->addNodeWithId(m_node, m_id);
            for (const auto& c : m_connections) {
                m_graph->connect(c.srcNodeId, c.srcPortIdx, c.dstNodeId, c.dstPortIdx);
            }
        }
    }

    std::string getName() const override { return "Remove " + (m_node ? m_node->getName() : "Node"); }

private:
    FluxGraph* m_graph;
    std::shared_ptr<FluxNode> m_node;
    size_t m_id;
    std::vector<FluxConnection> m_connections;
};

/**
 * @class AddNodeCommand
 */
class AddNodeCommand : public FluxCommand {
public:
    AddNodeCommand(FluxGraph* graph, std::shared_ptr<FluxNode> node, size_t id)
        : m_graph(graph), m_node(node), m_id(id) {}

    void execute() override { m_graph->addNodeWithId(m_node, m_id); }
    void undo() override { m_graph->removeNode(m_id); }
    std::string getName() const override { return "Add " + m_node->getName(); }

private:
    FluxGraph* m_graph;
    std::shared_ptr<FluxNode> m_node;
    size_t m_id;
};

/**
 * @class DisconnectCommand
 */
class DisconnectCommand : public FluxCommand {
public:
    DisconnectCommand(FluxGraph* graph, size_t src, int srcP, size_t dst, int dstP)
        : m_graph(graph), m_src(src), m_srcP(srcP), m_dst(dst), m_dstP(dstP) {}

    void execute() override { m_graph->disconnect(m_src, m_srcP, m_dst, m_dstP); }
    void undo() override { m_graph->connect(m_src, m_srcP, m_dst, m_dstP); }
    std::string getName() const override { return "Disconnect Nodes"; }

private:
    FluxGraph* m_graph;
    size_t m_src, m_dst;
    int m_srcP, m_dstP;
};

/**
 * @class ConnectCommand
 */
class ConnectCommand : public FluxCommand {
public:
    ConnectCommand(FluxGraph* graph, size_t src, int srcP, size_t dst, int dstP)
        : m_graph(graph), m_src(src), m_srcP(srcP), m_dst(dst), m_dstP(dstP) {}

    void execute() override { m_graph->connect(m_src, m_srcP, m_dst, m_dstP); }
    void undo() override { m_graph->disconnect(m_src, m_srcP, m_dst, m_dstP); }
    std::string getName() const override { return "Connect Nodes"; }

private:
    FluxGraph* m_graph;
    size_t m_src, m_dst;
    int m_srcP, m_dstP;
};

} // namespace Beam

#endif // COMMANDS_HPP
