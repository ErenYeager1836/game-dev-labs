#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gamedev::gameloop {

enum class NodeStatus {
    Success,
    Failure,
    Running
};

class Blackboard {
public:
    void set_bool(const std::string& key, bool val) { bools_[key] = val; }
    bool get_bool(const std::string& key, bool default_val = false) const {
        auto it = bools_.find(key);
        return it != bools_.end() ? it->second : default_val;
    }

    void set_int(const std::string& key, int val) { ints_[key] = val; }
    int get_int(const std::string& key, int default_val = 0) const {
        auto it = ints_.find(key);
        return it != ints_.end() ? it->second : default_val;
    }

    void set_float(const std::string& key, float val) { floats_[key] = val; }
    float get_float(const std::string& key, float default_val = 0.0f) const {
        auto it = floats_.find(key);
        return it != floats_.end() ? it->second : default_val;
    }

    void set_string(const std::string& key, const std::string& val) { strings_[key] = val; }
    std::string get_string(const std::string& key, const std::string& default_val = "") const {
        auto it = strings_.find(key);
        return it != strings_.end() ? it->second : default_val;
    }

    bool has(const std::string& key) const {
        return bools_.count(key) || ints_.count(key) || floats_.count(key) || strings_.count(key);
    }

    void clear() {
        bools_.clear();
        ints_.clear();
        floats_.clear();
        strings_.clear();
    }

private:
    std::unordered_map<std::string, bool> bools_;
    std::unordered_map<std::string, int> ints_;
    std::unordered_map<std::string, float> floats_;
    std::unordered_map<std::string, std::string> strings_;
};

class BTNode {
public:
    virtual ~BTNode() = default;
    virtual NodeStatus tick(Blackboard& bb) = 0;
    virtual void reset() {}
};

/**
 * Sequence Composite: Executes child nodes in order.
 * If any child fails, Sequence fails immediately.
 * If a child is running, Sequence returns Running.
 * If all children succeed, Sequence succeeds (Logical AND).
 */
class SequenceNode : public BTNode {
public:
    explicit SequenceNode(std::vector<std::shared_ptr<BTNode>> children = {})
        : children_(std::move(children)) {}

    void add_child(std::shared_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }

    NodeStatus tick(Blackboard& bb) override {
        for (; current_child_ < children_.size(); ++current_child_) {
            NodeStatus status = children_[current_child_]->tick(bb);
            if (status == NodeStatus::Running) {
                return NodeStatus::Running;
            }
            if (status == NodeStatus::Failure) {
                reset();
                return NodeStatus::Failure;
            }
        }
        reset();
        return NodeStatus::Success;
    }

    void reset() override {
        current_child_ = 0;
        for (auto& child : children_) {
            child->reset();
        }
    }

private:
    std::vector<std::shared_ptr<BTNode>> children_;
    size_t current_child_ = 0;
};

/**
 * Selector (Fallback) Composite: Executes child nodes in order.
 * If any child succeeds, Selector succeeds immediately.
 * If a child is running, Selector returns Running.
 * If all children fail, Selector fails (Logical OR).
 */
class SelectorNode : public BTNode {
public:
    explicit SelectorNode(std::vector<std::shared_ptr<BTNode>> children = {})
        : children_(std::move(children)) {}

    void add_child(std::shared_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }

    NodeStatus tick(Blackboard& bb) override {
        for (; current_child_ < children_.size(); ++current_child_) {
            NodeStatus status = children_[current_child_]->tick(bb);
            if (status == NodeStatus::Running) {
                return NodeStatus::Running;
            }
            if (status == NodeStatus::Success) {
                reset();
                return NodeStatus::Success;
            }
        }
        reset();
        return NodeStatus::Failure;
    }

    void reset() override {
        current_child_ = 0;
        for (auto& child : children_) {
            child->reset();
        }
    }

private:
    std::vector<std::shared_ptr<BTNode>> children_;
    size_t current_child_ = 0;
};

/**
 * Action Leaf Node executing custom logic.
 */
class ActionNode : public BTNode {
public:
    using ActionFunc = std::function<NodeStatus(Blackboard&)>;

    explicit ActionNode(ActionFunc func) : func_(std::move(func)) {}

    NodeStatus tick(Blackboard& bb) override {
        if (func_) return func_(bb);
        return NodeStatus::Success;
    }

private:
    ActionFunc func_;
};

/**
 * Condition Leaf Node returning Success if condition evaluates to true, else Failure.
 */
class ConditionNode : public BTNode {
public:
    using ConditionFunc = std::function<bool(Blackboard&)>;

    explicit ConditionNode(ConditionFunc func) : func_(std::move(func)) {}

    NodeStatus tick(Blackboard& bb) override {
        if (func_ && func_(bb)) {
            return NodeStatus::Success;
        }
        return NodeStatus::Failure;
    }

private:
    ConditionFunc func_;
};

/**
 * Inverter Decorator: Inverts Success <-> Failure.
 */
class InverterNode : public BTNode {
public:
    explicit InverterNode(std::shared_ptr<BTNode> child) : child_(std::move(child)) {}

    NodeStatus tick(Blackboard& bb) override {
        if (!child_) return NodeStatus::Failure;
        NodeStatus s = child_->tick(bb);
        if (s == NodeStatus::Success) return NodeStatus::Failure;
        if (s == NodeStatus::Failure) return NodeStatus::Success;
        return NodeStatus::Running;
    }

    void reset() override {
        if (child_) child_->reset();
    }

private:
    std::shared_ptr<BTNode> child_;
};

} // namespace gamedev::gameloop
