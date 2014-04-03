/*
* 2013+ Copyright (c) Andrey Kashin <kashin.andrej@gmail.com>
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*/

#ifndef call_tree_hpp
#define call_tree_hpp

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "actions_set.hpp"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace react {

template<typename Container>
struct node_t {
	/*!
	 * \brief Pointer to node type
	 */
	typedef size_t pointer;

	/*!
	 * \brief Initializes node with \a action_code and zero consumed time
	 * \param action_code Action code of the node
	 */
	node_t(int action_code): action_code(action_code) {}

	/*!
	 * \brief action which this node represents
	 */
	int action_code;

	/*!
	 * \brief Child nodes, actions that happen inside this action
	 */
	Container links;
};

struct unordered_node_t: public node_t< std::unordered_map<int, size_t> > {
	typedef node_t< std::unordered_map<int, size_t> > Base;

	/*!
	 * \brief Initializes node with \a action_code and zero calls number
	 * \param action_code Action code of the node
	 */
	unordered_node_t(int action_code): Base(action_code), time(0), calls_number(0) {}

	/*!
	 * \brief total time consumed in this node
	 */
	int64_t time;

	/*!
	 * \brief number of calls in this node
	 */
	int64_t calls_number;
};

struct ordered_node_t: public node_t< std::vector<std::pair<int, size_t> > > {
	typedef node_t< std::vector< std::pair<int, size_t> > > Base;

	/*!
	 * \brief Initializes node with \a action_code
	 * \param action_code Action code of the node
	 */
	ordered_node_t(int action_code): Base(action_code), start_time(0), stop_time(0) {}

	/*!
	 * \brief time when node action was started
	 */
	int64_t start_time;

	/*!
	 * \brief time when node action was stopped
	 */
	int64_t stop_time;
};

class unordered_call_tree_t;

/*!
 * \brief Stores call tree.
 *
 * Each node of the tree represents information about single action:
 * - Action code
 * - Total time consumed during this action
 */
template<typename NodeType>
class call_tree_base_t {
public:
	typedef NodeType                   node_t;
	typedef typename NodeType::pointer p_node_t;

	/*!
	 * \brief Value for representing null pointer
	 */
	static const p_node_t NO_NODE = -1;

	/*!
	 * \brief initializes call tree with single root node and action set
	 * \param actions_set Set of available actions for monitoring in call tree
	 */
	call_tree_base_t(const actions_set_t &actions_set): actions_set(actions_set) {
		root = new_node(-1);
	}

	/*!
	 * \brief frees memory consumed by call tree
	 */
	virtual ~call_tree_base_t() {}

	/*!
	 * \brief Returns an action code for \a node
	 * \param node Target node
	 * \return Action code of the target node
	 */
	int get_node_action_code(p_node_t node) const {
		return nodes[node].action_code;
	}

	/*!
	 * \brief Returns actions set monitored by this tree
	 * \return Actions set monitored by this tree
	 */
	const actions_set_t& get_actions_set() const {
		return actions_set;
	}

	/*!
	 * \brief Root of the call tree
	 */
	p_node_t root;

	/*!
	 * \brief Converts call tree to json
	 * \param stat_value Json node for writing
	 * \param allocator Json allocator
	 * \return Modified json node
	 */
	rapidjson::Value& to_json(rapidjson::Value &stat_value,
							  rapidjson::Document::AllocatorType &allocator) const {
		return to_json(root, stat_value, allocator);
	}

	/*!
	 * \brief Merges this tree into \a another_tree
	 * \param another_tree Tree where current tree will be merged in
	 */
	void merge_into(unordered_call_tree_t& another_tree) const;

protected:
	virtual rapidjson::Value& to_json(p_node_t current_node, rapidjson::Value &stat_value,
							  rapidjson::Document::AllocatorType &allocator) const = 0;

	virtual void merge_into(p_node_t lhs_node, p_node_t rhs_node, unordered_call_tree_t& rhs_tree) const = 0;

	/*!
	 * \internal
	 *
	 * \brief Allocates space for new node
	 * \param action_code
	 * \return Pointer to newly created node
	 */
	p_node_t new_node(int action_code) {
		nodes.emplace_back(action_code);
		return nodes.size() - 1;
	}

	/*!
	 * \brief Tree nodes
	 */
	std::vector<node_t> nodes;

	/*!
	 * \brief Available actions for monitoring
	 */
	const actions_set_t &actions_set;
};

class unordered_call_tree_t : public call_tree_base_t<unordered_node_t> {
	typedef call_tree_base_t<unordered_node_t> Base;

public:
	unordered_call_tree_t(const actions_set_t &actions_set): Base(actions_set) {}

	/*!
	 * \brief frees memory consumed by call tree
	 */
	~unordered_call_tree_t() {}

	/*!
	 * \brief Sets total time consumed by action represented by \a node
	 * \param node Action's node
	 * \param time New time value
	 */
	void set_node_time(p_node_t node, int64_t time) {
		nodes[node].time = time;
	}

	/*!
	 * \brief Increments total time consumed by action represented by \a node
	 * \param node Action's node
	 * \param delta Value by which time will be incremented
	 */
	void inc_node_time(p_node_t node, int64_t delta) {
		nodes[node].time += delta;
	}

	/*!
	 * \brief Returns total time consumed by action represented by \a node
	 * \param node Action's node
	 * \return Time consumed by action
	 */
	int64_t get_node_time(p_node_t node) const {
		return nodes[node].time;
	}

	/*!
	 * \brief Sets total calls number of action represented by \a node
	 * \param node Action's node
	 * \param time New calls number
	 */
	void set_node_calls_number(p_node_t node, int64_t calls_number) {
		nodes[node].calls_number = calls_number;
	}

	/*!
	 * \brief Increments total calls number of action represented by \a node
	 * \param node Action's node
	 */
	void inc_node_calls_number(p_node_t node) {
		++nodes[node].calls_number;
	}

	/*!
	 * \brief Returns total calls number of action represented by \a node
	 * \param node Action's node
	 * \return Time calls number of action
	 */
	int64_t get_node_calls_number(p_node_t node) const {
		return nodes[node].calls_number;
	}

	/*!
	 * \brief Checks whether node has child with \a action_code
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return whether \a node has child with \a action_code
	 */
	bool node_has_link(p_node_t node, int action_code) const {
		return nodes[node].links.find(action_code) != nodes[node].links.end();
	}

	/*!
	 * \brief Gets node's child with \a action_code
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return Pointer to child with \a action_code
	 */
	p_node_t get_node_link(p_node_t node, int action_code) const {
		return nodes[node].links.at(action_code);
	}

	/*!
	 * \brief Adds new child to \a node with \a action_code
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return Pointer to newly created child of \a node with \a action_code
	 */
	p_node_t add_new_link(p_node_t node, int action_code) {
		p_node_t action_node = new_node(action_code);
		nodes[node].links.insert(std::make_pair(action_code, action_node));
		return action_node;
	}

	/*!
	 * \brief Adds new child to \a node with \a action_code if it's missing
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return Pointer to child of \a node with \a action_code
	 */
	p_node_t add_new_link_if_missing(p_node_t node, int action_code) {
		auto link = nodes[node].links.find(action_code);
		if (link == nodes[node].links.end()) {
			return add_new_link(node, action_code);
		}
		return link->second;
	}

	using Base::to_json;
	using Base::merge_into;

private:
	/*!
	 * \internal
	 *
	 * \brief Recursively converts subtree to json
	 * \param current_node Node which subtree will be converted
	 * \param stat_value Json node for writing
	 * \param allocator Json allocator
	 * \return Modified json node
	 */
	rapidjson::Value& to_json(p_node_t current_node, rapidjson::Value &stat_value,
							  rapidjson::Document::AllocatorType &allocator) const {
		if (current_node != root) {
			stat_value.AddMember("name", actions_set.get_action_name(get_node_action_code(current_node)).c_str(), allocator);
			stat_value.AddMember("time", (int64_t) get_node_time(current_node), allocator);
			stat_value.AddMember("calls", (int64_t) get_node_calls_number(current_node), allocator);
		}

		if (!nodes[current_node].links.empty()) {
			rapidjson::Value subtree_actions(rapidjson::kArrayType);

			for (auto it = nodes[current_node].links.begin(); it != nodes[current_node].links.end(); ++it) {
				p_node_t next_node = it->second;
				rapidjson::Value subtree_value(rapidjson::kObjectType);
				to_json(next_node, subtree_value, allocator);
				subtree_actions.PushBack(subtree_value, allocator);
			}

			stat_value.AddMember("actions", subtree_actions, allocator);
		}

		return stat_value;
	}

	/*!
	 * \internal
	 *
	 * \brief Recursively merges \a lhs_node into \a rhs_node
	 * \param lhs_node Node which will be merged
	 * \param rhs_node
	 * \param rhs_tree
	 */
	void merge_into(p_node_t lhs_node, p_node_t rhs_node, unordered_call_tree_t& rhs_tree) const {
		rhs_tree.set_node_time(rhs_node, rhs_tree.get_node_time(rhs_node) + get_node_time(lhs_node));
		rhs_tree.set_node_calls_number(rhs_node, rhs_tree.get_node_calls_number(rhs_node) + get_node_calls_number(lhs_node));

		for (auto it = nodes[lhs_node].links.begin(); it != nodes[lhs_node].links.end(); ++it) {
			int action_code = it->first;
			p_node_t lhs_next_node = it->second;
			if (!rhs_tree.node_has_link(rhs_node, action_code)) {
				rhs_tree.add_new_link(rhs_node, action_code);
			}
			p_node_t rhs_next_node = rhs_tree.get_node_link(rhs_node, action_code);
			merge_into(lhs_next_node, rhs_next_node, rhs_tree);
		}
	}
};

template<typename NodeType>
void call_tree_base_t<NodeType>::merge_into(unordered_call_tree_t& another_tree) const {
	merge_into(root, another_tree.root, another_tree);
}

class call_tree_t : public call_tree_base_t<ordered_node_t> {
	typedef call_tree_base_t<ordered_node_t> Base;

public:
	call_tree_t(const actions_set_t &actions_set): Base(actions_set) {}

	/*!
	 * \brief frees memory consumed by call tree
	 */
	~call_tree_t() {}

	/*!
	 * \brief Sets time when action represented by \a node was started
	 * \param node Action's node
	 * \param time Time when action was started
	 */
	void set_node_start_time(p_node_t node, int64_t time) {
		nodes[node].start_time = time;
	}

	/*!
	 * \brief Sets time when action represented by \a node was stopped
	 * \param node Action's node
	 * \param time Time when action was stopped
	 */
	void set_node_stop_time(p_node_t node, int64_t time) {
		nodes[node].stop_time = time;
	}

	/*!
	 * \brief Returns start time of action represented by \a node
	 * \param node Action's node
	 * \return Start time of action
	 */
	int64_t get_node_start_time(p_node_t node) const {
		return nodes[node].start_time;
	}

	/*!
	 * \brief Returns stop time of action represented by \a node
	 * \param node Action's node
	 * \return Stop time of action
	 */
	int64_t get_node_stop_time(p_node_t node) const {
		return nodes[node].stop_time;
	}

	/*!
	 * \brief Adds new child to \a node with \a action_code
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return Pointer to newly created child of \a node with \a action_code
	 */
	p_node_t add_new_link(p_node_t node, int action_code) {
		p_node_t action_node = new_node(action_code);
		nodes[node].links.push_back(std::make_pair(action_code, action_node));
		return action_node;
	}

	/*!
	 * \brief Adds new child to \a node with \a action_code if it's missing
	 * \param node Target node
	 * \param action_code Child's action code
	 * \return Pointer to child of \a node with \a action_code
	 */
	p_node_t add_new_link_if_missing(p_node_t node, int action_code) {
		return add_new_link(node, action_code);
	}

	using Base::to_json;
	using Base::merge_into;

private:
	/*!
	 * \internal
	 *
	 * \brief Recursively converts subtree to json
	 * \param current_node Node which subtree will be converted
	 * \param stat_value Json node for writing
	 * \param allocator Json allocator
	 * \return Modified json node
	 */
	rapidjson::Value& to_json(p_node_t current_node, rapidjson::Value &stat_value,
							  rapidjson::Document::AllocatorType &allocator) const {
		if (current_node != root) {
			stat_value.AddMember("name", actions_set.get_action_name(get_node_action_code(current_node)).c_str(), allocator);
			stat_value.AddMember("start_time", (int64_t) get_node_start_time(current_node), allocator);
			stat_value.AddMember("stop_time", (int64_t) get_node_stop_time(current_node), allocator);
		}

		if (!nodes[current_node].links.empty()) {
			rapidjson::Value subtree_actions(rapidjson::kArrayType);

			for (auto it = nodes[current_node].links.begin(); it != nodes[current_node].links.end(); ++it) {
				p_node_t next_node = it->second;
				rapidjson::Value subtree_value(rapidjson::kObjectType);
				to_json(next_node, subtree_value, allocator);
				subtree_actions.PushBack(subtree_value, allocator);
			}

			stat_value.AddMember("actions", subtree_actions, allocator);
		}

		return stat_value;
	}

	/*!
	 * \internal
	 *
	 * \brief Recursively merges \a lhs_node into \a rhs_node
	 * \param lhs_node Node which will be merged
	 * \param rhs_node
	 * \param rhs_tree
	 */
	void merge_into(p_node_t lhs_node, unordered_call_tree_t::p_node_t rhs_node, unordered_call_tree_t& rhs_tree) const {
		int64_t time_delta = get_node_stop_time(lhs_node) - get_node_start_time(lhs_node);
		rhs_tree.set_node_time(rhs_node, rhs_tree.get_node_time(rhs_node) + time_delta);
		rhs_tree.set_node_calls_number(rhs_node, rhs_tree.get_node_calls_number(rhs_node) + 1);

		for (auto it = nodes[lhs_node].links.begin(); it != nodes[lhs_node].links.end(); ++it) {
			int action_code = it->first;
			p_node_t lhs_next_node = it->second;
			if (!rhs_tree.node_has_link(rhs_node, action_code)) {
				rhs_tree.add_new_link(rhs_node, action_code);
			}
			p_node_t rhs_next_node = rhs_tree.get_node_link(rhs_node, action_code);
			merge_into(lhs_next_node, rhs_next_node, rhs_tree);
		}
	}
};

/*!
 * \brief Concurrent version of time stats tree to handle simultanious updates
 */
class concurrent_call_tree_t {
public:
	/*!
	 * \brief Initializes call_tree with \a actions_set
	 * \param actions_set Set of available action for monitoring
	 */
	concurrent_call_tree_t(actions_set_t &actions_set): call_tree(actions_set) {}

	/*!
	 * \brief Gets ownership of time stats tree
	 */
	void lock() const {
		tree_mutex.lock();
	}

	/*!
	 * \brief Releases ownership of time stats tree
	 */
	void unlock() const {
		tree_mutex.unlock();
	}

	/*!
	 * \brief Returns inner time stats tree
	 * \return Inner time stats tree
	 */
	call_tree_t& get_call_tree() {
		return call_tree;
	}

	/*!
	 * \brief Returns copy of inner time stats tree
	 * \return Copy of inner time stats tree
	 */
	call_tree_t copy_call_tree() const {
		lock();
		call_tree_t call_tree_copy = call_tree;
		unlock();
		return call_tree_copy;
	}

private:
	/*!
	 * \brief Lock to handle concurrency during updates
	 */
	mutable std::mutex tree_mutex;

	/*!
	 * \brief Inner call_tree
	 */
	call_tree_t call_tree;
};

} // namespace react

#endif // call_tree_hpp
