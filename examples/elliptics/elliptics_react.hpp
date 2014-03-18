#ifndef ELLIPTICS_REACT_HPP
#define ELLIPTICS_REACT_HPP

#include "react/react.hpp"
#include "elliptics_react.h"

react::action_guard make_action_guard(int action_code);

typedef react::concurrent_call_tree_t<react::call_tree_t> concurrent_call_tree;

class elliptics_react_manager_t {
public:
	elliptics_react_manager_t();

	void add_tree(concurrent_call_tree &call_tree);
	const react::unordered_call_tree_t &get_total_call_tree() const;
	const react::call_tree_t &get_last_call_tree() const;
private:
	std::mutex mutex;
	react::unordered_call_tree_t total_call_tree;
	react::call_tree_t last_call_tree;
};

#endif // ELLIPTICS_REACT_HPP
