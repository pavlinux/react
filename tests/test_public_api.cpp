#include "tests.hpp"

#include <boost/test/output_test_stream.hpp>

#include "react/react.hpp"
#include "react/actions_set.hpp"

BOOST_AUTO_TEST_SUITE( public_api_suite )

struct cerr_redirect {
	cerr_redirect(std::streambuf *new_buffer):
		old(std::cerr.rdbuf(new_buffer)) {}

	~cerr_redirect() {
		std::cerr.rdbuf(old);
	}

private:
	std::streambuf *old;
};

BOOST_AUTO_TEST_CASE( react_define_new_action_test )
{
	int action_code = react_define_new_action("ACTION");
	BOOST_CHECK_EQUAL( react_define_new_action("ACTION"), action_code );
	BOOST_CHECK_NE( react_define_new_action("ANOTHER_ACTION"), action_code );
}

BOOST_AUTO_TEST_CASE( react_is_active_test )
{
	BOOST_CHECK( !react_is_active() );
}

BOOST_AUTO_TEST_CASE( react_activate_test )
{
	react_activate();
	BOOST_CHECK( react_is_active() );
	int err = react_deactivate();
	BOOST_CHECK_EQUAL( err, 0 );
	BOOST_CHECK( !react_is_active() );
}

BOOST_AUTO_TEST_CASE( react_double_activate_test )
{
	boost::test_tools::output_test_stream error_output;
	cerr_redirect guard(error_output.rdbuf());

	react_activate();
	BOOST_CHECK(react_is_active());

	react_activate();
	BOOST_CHECK(react_is_active());

	BOOST_CHECK( error_output.is_empty() );

	react_deactivate();
	BOOST_CHECK(react_is_active());

	react_deactivate();
	BOOST_CHECK(!react_is_active());

	BOOST_CHECK( error_output.is_empty() );

}

BOOST_AUTO_TEST_CASE( react_not_active_deactivate_test )
{
	boost::test_tools::output_test_stream error_output;
	cerr_redirect guard(error_output.rdbuf());

	int err = react_deactivate();
	BOOST_CHECK_NE( err, 0 );
	BOOST_CHECK( !react_is_active() );
}

BOOST_AUTO_TEST_CASE( react_start_and_stop_action_test )
{
	react_activate();

	int action_code = react_define_new_action("ACTION");
	int err = react_start_action(action_code);
	BOOST_CHECK_EQUAL( err, 0 );
	err = react_stop_action(action_code);
	BOOST_CHECK_EQUAL( err, 0 );

	react_deactivate();
}

BOOST_AUTO_TEST_CASE( react_start_and_stop_invalid_action_test )
{
	react_activate();
	boost::test_tools::output_test_stream error_output;

	{
		cerr_redirect guard(error_output.rdbuf());
		int err = react_start_action(react::actions_set_t::NO_ACTION);
		BOOST_CHECK_NE( err, 0 );
		BOOST_CHECK( !error_output.is_empty() );
	}

	{
		cerr_redirect guard(error_output.rdbuf());
		int err = react_stop_action(react::actions_set_t::NO_ACTION);
		BOOST_CHECK_NE( err, 0 );
		BOOST_CHECK( !error_output.is_empty() );
	}

	{
		cerr_redirect guard(error_output.rdbuf());
		int action_code = react_define_new_action("ACTION");
		int another_action_code = react_define_new_action("ANOTHER_ACTION");

		react_start_action(action_code);
		BOOST_CHECK( error_output.is_empty() );

		react_stop_action(another_action_code);
		BOOST_CHECK( !error_output.is_empty() );

		react_stop_action(action_code);
	}

	react_deactivate();
}

BOOST_AUTO_TEST_CASE( forgotten_stop_action_test )
{
	react_activate();
	boost::test_tools::output_test_stream error_output;
	cerr_redirect guard(error_output.rdbuf());

	int action_code = react_define_new_action("ACTION");

	react_start_action(action_code);
	// Forgot stop_action before deactivating

	react_deactivate();
	BOOST_CHECK( !error_output.is_empty() );
}

BOOST_AUTO_TEST_CASE( react_not_active_start_and_stop_test )
{
	boost::test_tools::output_test_stream error_output;
	int action_code = react_define_new_action("ACTION");

	{
		cerr_redirect guard(error_output.rdbuf());
		int err = react_start_action(action_code);
		BOOST_CHECK_EQUAL( err, 0 );
		BOOST_CHECK( error_output.is_empty() );
	}

	{
		cerr_redirect guard(error_output.rdbuf());
		int err = react_stop_action(action_code);
		BOOST_CHECK_EQUAL( err, 0 );
		BOOST_CHECK( error_output.is_empty() );
	}
}

BOOST_AUTO_TEST_CASE( get_actions_set_test )
{
	int action_code = react_define_new_action("ACTION");
	BOOST_CHECK_EQUAL( react::get_actions_set().get_action_name(action_code), "ACTION" );
}

BOOST_AUTO_TEST_CASE( action_guard_test )
{
	react_activate();
	int action_code = react_define_new_action("ACTION");
	{
		react::action_guard guard(action_code);
	}
	react_deactivate();
}

BOOST_AUTO_TEST_CASE( react_not_active_action_guard_test )
{
	int action_code = react_define_new_action("ACTION");
	react::action_guard guard(action_code);
}

BOOST_AUTO_TEST_CASE( action_guard_stop_test )
{
	react_activate();
	int action_code = react_define_new_action("ACTION");
	react::action_guard guard(action_code);
	guard.stop();
	react_deactivate();
}

BOOST_AUTO_TEST_SUITE_END()
