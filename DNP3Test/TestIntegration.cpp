/*
 * Licensed to Green Energy Corp (www.greenenergycorp.com) under one or more
 * contributor license agreements. See the NOTICE file distributed with this
 * work for additional information regarding copyright ownership.  Green Enery
 * Corp licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <boost/test/unit_test.hpp>
#include <APLTestTools/TestHelpers.h>

#include <APL/Log.h>
#include <APL/LogToStdio.h>

#include <boost/asio.hpp>

#include "IntegrationTest.h"

#define EXTRA_DEBUG				(0)

using namespace apl;
using namespace apl::dnp;
using namespace std;

/*
 * Each platform needs slightly different values.  Figure out what they are,
 * and use the macros in the test cases below.
 */
#if defined(WIN32)
	/* Windows platform */
	#define MACRO_PORT_START	(50000)
	#define MACRO_NUM_PAIRS		(10)
#elif defined(ARM)
	/* Linux on ARM platform */
	#define MACRO_PORT_START	(30000)
	#define MACRO_NUM_PAIRS		(10)
#else
	/* Generic Linux platform */
	#define MACRO_PORT_START	(30000)
	#define MACRO_NUM_PAIRS		(100)
#endif

BOOST_AUTO_TEST_SUITE(IntegrationSuite)



BOOST_AUTO_TEST_CASE(MasterToSlave)
{	

const boost::uint16_t START_PORT = MACRO_PORT_START;
const size_t NUM_PAIRS = MACRO_NUM_PAIRS;
const size_t NUM_POINTS = 500;
const size_t NUM_CHANGES = 10;

	EventLog log;
	if (EXTRA_DEBUG)
		log.AddLogSubscriber(LogToStdio::Inst());

	IntegrationTest t(log.GetLogger(LEV_WARNING, "test"), LEV_WARNING, START_PORT,
			NUM_PAIRS, NUM_POINTS);

	IDataObserver* pObs = t.GetFanout();

	/* Verify that the Master and Slave stacks were created */
	{
		std::vector<std::string> list;

		std::ostringstream oss;
		oss << "Port: " << START_PORT << " Client ";

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), NUM_PAIRS * 2);
		BOOST_REQUIRE_EQUAL(list[0], oss.str());
	}

	/* Verify that the Master and Slave ports were created */
	{
		std::vector<std::string> list;

		std::ostringstream oss;
		oss << "Port: " << START_PORT << " Client ";

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), NUM_POINTS * 2);
		BOOST_REQUIRE_EQUAL(list[0], oss.str());
	}

	/* Verify that GetVtoWriter behaves as expected */
	{
		std::ostringstream oss;
		oss << "Port: " << START_PORT;

		std::string client1 = oss.str() + " Client ";
		std::string client2 = oss.str() + " Server ";

		IVtoWriter* pWriter1a = NULL;
		IVtoWriter* pWriter1b = NULL;
		IVtoWriter* pWriter2 = NULL;

		BOOST_REQUIRE_NO_THROW(pWriter1a = t.GetVtoWriter(client1));
		BOOST_REQUIRE_NO_THROW(pWriter1b = t.GetVtoWriter(client1));
		BOOST_REQUIRE_NO_THROW(pWriter2 = t.GetVtoWriter(client2));

		BOOST_REQUIRE(pWriter1a == pWriter1b);
		BOOST_REQUIRE(pWriter1a != pWriter2);

		BOOST_REQUIRE_THROW(t.GetVtoWriter("trash"), ArgumentException);
	}

	StopWatch sw;
	for (size_t j = 0; j < NUM_CHANGES; ++j)
	{
		/*
		 * Resource Acquisition Is Initialization (RAII) Pattern.
		 * When the Transaction instance is created, it acquires the resource.
		 * When it is destroyed, it releases the resource.  The scoping using
		 * the {} block forces destruction of the Transaction at the right time.
		 */
		{
			Transaction tr(pObs);

			for (size_t i = 0; i < NUM_POINTS; ++i)
				pObs->Update(t.RandomBinary(), i);

			for (size_t i = 0; i < NUM_POINTS; ++i)
				pObs->Update(t.RandomAnalog(), i);

			for (size_t i = 0; i < NUM_POINTS; ++i)
				pObs->Update(t.RandomCounter(), i);
		}

		BOOST_REQUIRE(t.ProceedUntil(boost::bind(&IntegrationTest::SameData, &t)));

		if (EXTRA_DEBUG)
			cout << "***  Finished change set " <<  j << " ***" << endl;
	}

	if (EXTRA_DEBUG) {
		double elapsed_sec = sw.Elapsed() / 1000.0;
		size_t points = 3 * NUM_POINTS * NUM_CHANGES * NUM_PAIRS * 2;
		cout << "num points: " << points << endl;
		cout << "elapsed seconds: " << elapsed_sec << endl;
		cout << "points/sec: " << points/elapsed_sec << endl;
	}

	/* Remove a stack, but leave the port associated */
	{
		std::vector<std::string> list;

		std::ostringstream oss;
		oss << "Port: " << START_PORT << " Client ";

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), NUM_PAIRS * 2);
		BOOST_REQUIRE(list[0] == oss.str());

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), NUM_PAIRS * 2);
		BOOST_REQUIRE(list[0] == oss.str());

		BOOST_REQUIRE_NO_THROW(t.RemoveStack(oss.str()));

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 1);
		BOOST_REQUIRE(list[0] != oss.str());

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), NUM_PAIRS * 2);
		BOOST_REQUIRE(list[0] == oss.str());

		BOOST_REQUIRE_NO_THROW(t.RemovePort(oss.str()));

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 1);
		BOOST_REQUIRE(list[0] != oss.str());

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 1);
		BOOST_REQUIRE(list[0] != oss.str());
	}

	/* Remove a port and its associated stacks */
	{
		std::vector<std::string> list;

		std::ostringstream oss;
		oss << "Port: " << START_PORT << " Server ";

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 1);
		BOOST_REQUIRE(list[0] == oss.str());

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 1);
		BOOST_REQUIRE(list[0] == oss.str());

		BOOST_REQUIRE_NO_THROW(t.RemovePort(oss.str()));

		list = t.GetStackNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 2);
		BOOST_REQUIRE(list[0] != oss.str());

		list = t.GetPortNames();
		BOOST_REQUIRE_EQUAL(list.size(), (NUM_PAIRS * 2) - 2);
		BOOST_REQUIRE(list[0] != oss.str());
	}

	/* Try to remove a non-existant stack and port */
	{
		BOOST_REQUIRE_THROW(t.RemoveStack("trash"), ArgumentException);
		BOOST_REQUIRE_THROW(t.RemovePort("trash"), ArgumentException);
	}
}

BOOST_AUTO_TEST_SUITE_END()

/* vim: set ts=4 sw=4: */
