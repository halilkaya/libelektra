/**
 * @file
 *
 * @brief Tests for the Backend builder class
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 *
 */


#include <backend.hpp>
#include <backends.hpp>
#include <backendbuilder.hpp>
#include <plugindatabase.hpp>

#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include <kdb.hpp>
#include <gtest/gtest.h>

class MockPluginDatabase : public kdb::tools::PluginDatabase
{
public:
	mutable std::unordered_map <kdb::tools::PluginSpec, std::unordered_map<std::string,std::string>> data;

	std::string lookupInfo(kdb::tools::PluginSpec const & spec, std::string const & which) const
	{
		return data[spec][which];
	}

	kdb::tools::PluginSpec lookupProvides (std::string const & which) const
	{
		for (auto const & plugin : data)
		{
			if (plugin.first.name == which)
			{
				return plugin.first;
			}

			if (lookupInfo (plugin.first, "provides") == which)
			{
				return plugin.first;
			}
		}

		throw kdb::tools::NoPlugin("No plugin " + which + " could be found");
	}
};

TEST(BackendBuilder, withDatabase)
{
	using namespace kdb;
	using namespace kdb::tools;
	std::shared_ptr<MockPluginDatabase> mpd = std::make_shared<MockPluginDatabase>();
	mpd->data[PluginSpec("a")]["ordering"] = "d";
	mpd->data[PluginSpec("b")]["ordering"] = "d";
	mpd->data[PluginSpec("c")]["ordering"];
	BackendBuilder bb(mpd);
	bb.addPlugin(PluginSpec("a"));
	bb.addPlugin(PluginSpec("b"));
	bb.addPlugin(PluginSpec("c"));
}


TEST(BackendBuilder, parsePluginArguments)
{
	using namespace kdb;
	using namespace kdb::tools;
	EXPECT_EQ (KeySet(5, *Key("user/a", KEY_VALUE, "5", KEY_END), KS_END),
		   BackendBuilder::parsePluginArguments("a=5"));
	EXPECT_EQ (KeySet(5, *Key("user", KEY_END), KS_END),
		   BackendBuilder::parsePluginArguments("="));
	EXPECT_EQ (KeySet (5,
			*Key("user/a", KEY_VALUE, "5", KEY_END),
			*Key("user/ax", KEY_VALUE, "a", KEY_END),
			*Key("user/ax/bx", KEY_VALUE, "8", KEY_END),
			KS_END),
		  BackendBuilder::parsePluginArguments ("a=5,ax=a,ax/bx=8"));
	EXPECT_EQ (KeySet (5,
			*Key("user", KEY_VALUE, "5", KEY_END),
			*Key("user/ax", KEY_END, KEY_END),
			*Key("user/ax/bx", KEY_VALUE, "8", KEY_END),
			KS_END),
		  BackendBuilder::parsePluginArguments ("=5,ax=,ax/bx=8"));
}

bool cmpPsv(kdb::tools::PluginSpecVector psv1, kdb::tools::PluginSpecVector psv2)
{
	EXPECT_EQ(psv1.size(), psv2.size());
	if (psv1.size() != psv2.size()) return false;
	for (size_t i=0; i<psv1.size(); ++i)
	{
		EXPECT_EQ (psv1[i], psv2[i]);
		if (!(psv1[i] == psv2[i])) return false;
	}
	return true;
}

TEST(BackendBuilder, parseArguments)
{
	using namespace kdb;
	using namespace kdb::tools;
	PluginSpecVector psv1;
	psv1.push_back (PluginSpec ("a", KeySet(5, *Key("user/a", KEY_VALUE, "5", KEY_END), KS_END)));
	psv1.push_back (PluginSpec ("b"));
	psv1.push_back (PluginSpec ("c"));
	PluginSpecVector psv2 = BackendBuilder::parseArguments ("a a=5 b c");
	EXPECT_TRUE(cmpPsv (psv1, psv2));
	psv2 = BackendBuilder::parseArguments ("  a  a=5  b c   ");
	EXPECT_TRUE(cmpPsv (psv1, psv2));
	psv2 = BackendBuilder::parseArguments ("  a 	 a=5	  b c ,  ");
	EXPECT_TRUE(cmpPsv (psv1, psv2));
	EXPECT_THROW(BackendBuilder::parseArguments ("a=5 a b c"), ParseException);
}

TEST(BackendBuilder, basicAddRem)
{
	using namespace kdb;
	using namespace kdb::tools;
	try {
		Backend b;
		b.addPlugin("resolver");
		b.addPlugin("dump");
	}
	catch (std::exception const & e)
	{
		std::cout << "Plugin missing, abort test case: " << e.what() << std::endl;
		return;
	}
	BackendBuilder bb;
	bb.addPlugin(PluginSpec("resolver"));
	EXPECT_FALSE(bb.validated());

	bb.addPlugin(PluginSpec("dump"));
	EXPECT_TRUE(bb.validated());

	bb.remPlugin(PluginSpec("dump"));
	EXPECT_FALSE(bb.validated());

	bb.addPlugin(PluginSpec("dump"));
	EXPECT_TRUE(bb.validated());
}

TEST(BackendBuilder, basicSort)
{
	using namespace kdb;
	using namespace kdb::tools;
	try {
		Backend b;
		b.addPlugin("resolver");
		b.addPlugin("glob");
		b.addPlugin("keytometa");
		b.addPlugin("augeas");
	}
	catch (std::exception const & e)
	{
		std::cout << "Plugin missing, abort test case: " << e.what() << std::endl;
		return;
	}
	BackendBuilder bb;
	bb.addPlugin(PluginSpec("resolver"));
	EXPECT_FALSE(bb.validated());

	bb.addPlugin(PluginSpec("keytometa"));
	EXPECT_FALSE(bb.validated());

	bb.addPlugin(PluginSpec("glob"));
	EXPECT_FALSE(bb.validated());

	bb.addPlugin(PluginSpec("augeas"));
	EXPECT_TRUE(bb.validated()) << "Reordering not successful?";
}


TEST(BackendBuilder, allSort)
{
	using namespace kdb;
	using namespace kdb::tools;
	try {
		Backend b;
		b.addPlugin("resolver");
		b.addPlugin("glob");
		b.addPlugin("keytometa");
		b.addPlugin("augeas");
	}
	catch (std::exception const & e)
	{
		std::cout << "Plugin missing, abort test case: " << e.what() << std::endl;
		return;
	}

	std::vector <std::string> permutation = {"augeas", "glob", "keytometa", "resolver"};

	do {
		// for (auto const & p : permutation) std::cout << p << " ";
		// std::cout << std::endl;
		BackendBuilder bb;
		bb.addPlugin(PluginSpec(permutation[0]));
		bb.addPlugin(PluginSpec(permutation[1]));
		bb.addPlugin(PluginSpec(permutation[2]));
		bb.addPlugin(PluginSpec(permutation[3]));
		EXPECT_TRUE(bb.validated()) << "Reordering not successful?";
	} while (std::next_permutation(permutation.begin(), permutation.end()));
}


TEST(BackendBuilder, resolveNeeds)
{
	using namespace kdb;
	using namespace kdb::tools;
	try {
		Backend b;
		b.addPlugin("resolver");
		b.addPlugin("line");
		b.addPlugin("null");
	}
	catch (std::exception const & e)
	{
		std::cout << "Plugin missing, abort test case: " << e.what() << std::endl;
		return;
	}
	BackendBuilder bb;
	bb.addPlugin(PluginSpec("resolver"));
	EXPECT_FALSE(bb.validated()) << "resolver+null should be missing";
	bb.addPlugin(PluginSpec("line"));
	EXPECT_FALSE(bb.validated()) << "null should be missing";
	bb.resolveNeeds();
	EXPECT_TRUE(bb.validated()) << "Did not add null automatically";
}


TEST(BackendBuilder, resolveDoubleNeeds)
{
	using namespace kdb;
	using namespace kdb::tools;
	std::shared_ptr<MockPluginDatabase> mpd = std::make_shared<MockPluginDatabase>();
	mpd->data[PluginSpec("a")]["needs"] = "c v";
	mpd->data[PluginSpec("c")]["provides"] = "v";
	BackendBuilder bb(mpd);
	bb.addPlugin(PluginSpec("resolver"));
	bb.addPlugin(PluginSpec("a"));
	EXPECT_EQ(bb.size(), 2);
	bb.resolveNeeds();
	ASSERT_EQ(bb.size(), 3);
	EXPECT_EQ(bb[0], PluginSpec("resolver"));
	EXPECT_EQ(bb[1], PluginSpec("a"));
	EXPECT_EQ(bb[2], PluginSpec("c"));
}

TEST(BackendBuilder, resolveDoubleNeedsVirtual)
{
	using namespace kdb;
	using namespace kdb::tools;
	std::shared_ptr<MockPluginDatabase> mpd = std::make_shared<MockPluginDatabase>();
	mpd->data[PluginSpec("a")]["needs"] = "v c";
	mpd->data[PluginSpec("c")]["provides"] = "v";
	BackendBuilder bb(mpd);
	bb.addPlugin(PluginSpec("resolver"));
	bb.addPlugin(PluginSpec("a"));
	EXPECT_EQ(bb.size(), 2);
	bb.resolveNeeds();
	ASSERT_EQ(bb.size(), 3);
	EXPECT_EQ(bb[0], PluginSpec("resolver"));
	EXPECT_EQ(bb[1], PluginSpec("a"));
	EXPECT_EQ(bb[2], PluginSpec("c"));
}
