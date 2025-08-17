// Integration tests for the Logger subsystem
// 
// @see https://github.com/endurodave/IntegrationTestFramework
// David Lafreniere, Oct 2024.
//
// All tests run within the IntegrationTest thread context. Logger subsystem runs 
// within the Logger thread context. The Delegate library is used to invoke 
// functions across thread boundaries. The Google Test library is used to execute 
// tests and collect results.

#include "Logger.h"
#include "DelegateMQ.h"
#include "SignalThread.h"
#include "IT_Util.h"		// Include this last

#define CPPUTEST_MEM_LEAK_DETECTION_DISABLED
#include "CppUTest/TestHarness.h"

using namespace std;
using namespace std::chrono;
using namespace dmq;

// Local integration test variables
static SignalThread signalThread;
static vector<string> callbackStatus;
static milliseconds flushDuration;
static mutex mtx;

// Logger callback handler function invoked from Logger thread context
void FlushTimeCb(milliseconds duration)
{
    // Protect flushTime against multiple thread access by IntegrationTest 
    // thread and Logger thread
    lock_guard<mutex> lock(mtx);

    // Save the flush time
    flushDuration = duration;
}

// Logger callback handler function invoked from Logger thread context
void LoggerStatusCb(const string& status)
{
    // Protect callbackStatus against multiple thread access by IntegrationTest 
    // thread and Logger thread
    lock_guard<mutex> lock(mtx);

    // Save logger callback status
    callbackStatus.push_back(status);

    // Signal the waiting thread to continue
    signalThread.SetSignal();
}

// Test group setup/teardown
TEST_GROUP(Logger_IT)
{
    void setup() override
    {
        // Optional setup before each test
        Logger::GetInstance().SetCallback(&LoggerStatusCb);
    }

    void teardown() override
    {
        // Cleanup after each test
        Logger::GetInstance().SetCallback(nullptr);

        callbackStatus.clear(); // Clear the callback status for next test
        callbackStatus.shrink_to_fit();  // Prevent memory leak test failing
    }
};

TEST(Logger_IT, Write)
{
    // Write a Logger string value using public API
    Logger::GetInstance().Write("LoggerTest, Write");

    // Wait for LoggerStatusCb callback up to 500ms
    bool success = signalThread.WaitForSignal(500);
    CHECK_TRUE(success);

    // Wait for 2nd LoggerStatusCb callback up to 2 seconds
    bool success2 = signalThread.WaitForSignal(2000);
    CHECK_TRUE(success2);

    {
        std::lock_guard<std::mutex> lock(mtx);

        CHECK_EQUAL(2, (int)callbackStatus.size());

        if (callbackStatus.size() >= 2)
        {
            STRCMP_EQUAL("Write success!", callbackStatus[0].c_str());
            STRCMP_EQUAL("Flush success!", callbackStatus[1].c_str());
        }
    }
}

TEST(Logger_IT, Flush)
{
    // Create an asynchronous blocking delegate targeted at the LogData::Flush function
    auto flushAsyncBlockingDelegate = MakeDelegate(
        &Logger::GetInstance().m_logData,	// LogData object within Logger class
        &LogData::Flush,					// LogData function to invoke
        Logger::GetInstance(),				// Thread to invoke Flush (Logger is-a Thread)
        milliseconds(100));					// Wait up to 100mS for Flush function to be called

    // Invoke LogData::Flush on the Logger thread and obtain the return value
    std::optional<bool> retVal = flushAsyncBlockingDelegate.AsyncInvoke();

    // Check test results
    CHECK_TRUE(retVal.has_value());       // Did async LogData::Flush function call succeed?
    if (retVal.has_value())
    {
        CHECK_TRUE(retVal.value());       // Did LogData::Flush return true?
    }
}

TEST(Logger_IT, FlushTime)
{
    {
        // Protect access to flushDuration
        std::lock_guard<std::mutex> lock(mtx);
        flushDuration = std::chrono::milliseconds(-1);
    }

    // Register for a callback from Logger thread
    Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(&FlushTimeCb);

    // Clear the m_msgData list on Logger thread
    auto retVal1 = MakeDelegate(
        &Logger::GetInstance().m_logData.m_msgData,    // Object instance
        &std::list<std::string>::clear,                // Object function
        Logger::GetInstance(),                         // Thread to invoke object function
        std::chrono::milliseconds(50)).AsyncInvoke();

    // Check asynchronous function call succeeded
    CHECK_TRUE(retVal1.has_value());

    // Write lines of log data
    for (int i = 0; i < 100; i++)
    {
        auto retVal = MakeDelegate(
            &Logger::GetInstance().m_logData,
            &LogData::Write,
            Logger::GetInstance(),
            std::chrono::milliseconds(50)).AsyncInvoke("Flush Timer String");

        CHECK_TRUE(retVal.has_value());

        if (retVal.has_value())
        {
            CHECK_TRUE(retVal.value());
        }
    }

    // Call LogData::Flush on Logger thread
    auto retVal2 = MakeDelegate(
        &Logger::GetInstance().m_logData,
        &LogData::Flush,
        Logger::GetInstance(),
        std::chrono::milliseconds(100)).AsyncInvoke();

    CHECK_TRUE(retVal2.has_value());
    if (retVal2.has_value())
    {
        CHECK_TRUE(retVal2.value());
    }

    {
        // Protect access to flushDuration
        std::lock_guard<std::mutex> lock(mtx);

        // Check that flush executed in 10ms or less
        CHECK(flushDuration >= std::chrono::milliseconds(0));
        CHECK(flushDuration <= std::chrono::milliseconds(10));
    }

    // Unregister from callback
    Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(&FlushTimeCb);
}

TEST(Logger_IT, FlushTimeSimplified)
{
    {
        // Protect access to flushDuration
        std::lock_guard<std::mutex> lock(mtx);
        flushDuration = std::chrono::milliseconds(-1);
    }

    // Register for a callback from Logger thread
    Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(&FlushTimeCb);

    // Clear the m_msgData list on Logger thread
    auto retVal1 = AsyncInvoke(
        &Logger::GetInstance().m_logData.m_msgData,   // Object instance
        &std::list<std::string>::clear,              // Object function
        Logger::GetInstance(),                       // Thread to invoke object function
        std::chrono::milliseconds(50));              // Wait up to 50ms

    // Write lines of log data
    for (int i = 0; i < 100; i++)
    {
        auto retVal = AsyncInvoke(
            &Logger::GetInstance().m_logData,
            &LogData::Write,
            Logger::GetInstance(),
            std::chrono::milliseconds(50),
            "Flush Timer String");

        if (retVal.has_value())
        {
            CHECK_TRUE(retVal.value());
        }
    }

    // Call LogData::Flush on Logger thread
    auto retVal2 = AsyncInvoke(
        &Logger::GetInstance().m_logData,
        &LogData::Flush,
        Logger::GetInstance(),
        std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(mtx);

        // Check that flush executed in 10ms or less
        CHECK(flushDuration >= std::chrono::milliseconds(0));
        CHECK(flushDuration <= std::chrono::milliseconds(10));
    }

    // Unregister from callback
    Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(&FlushTimeCb);
}

// Exact same test as FlushTimeSimplified above, but use a private lambda callback 
// function to centralize the callback inside the test case. 
TEST(Logger_IT, FlushTimeSimplifiedWithLambda)
{
    // Logger callback handler lambda function invoked from Logger thread context
    auto FlushTimeLambdaCb = +[](std::chrono::milliseconds duration) -> void
        {
            std::lock_guard<std::mutex> lock(mtx);
            flushDuration = duration;
        };

    {
        std::lock_guard<std::mutex> lock(mtx);
        flushDuration = std::chrono::milliseconds(-1);
    }

    // Register for a callback from Logger thread
    Logger::GetInstance().m_logData.FlushTimeDelegate += MakeDelegate(FlushTimeLambdaCb);

    // Clear the m_msgData list on Logger thread
    auto retVal1 = AsyncInvoke(
        &Logger::GetInstance().m_logData.m_msgData,
        &std::list<std::string>::clear,
        Logger::GetInstance(),
        std::chrono::milliseconds(50));

    // Write lines of log data
    for (int i = 0; i < 10; i++)
    {
        auto retVal = AsyncInvoke(
            &Logger::GetInstance().m_logData,
            &LogData::Write,
            Logger::GetInstance(),
            std::chrono::milliseconds(50),
            "Flush Timer String");

        if (retVal.has_value())
        {
            CHECK_TRUE(retVal.value());
        }
    }

    // Call LogData::Flush on Logger thread
    auto retVal2 = AsyncInvoke(
        &Logger::GetInstance().m_logData,
        &LogData::Flush,
        Logger::GetInstance(),
        std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(mtx);

        CHECK(flushDuration >= std::chrono::milliseconds(0));
        CHECK(flushDuration <= std::chrono::milliseconds(10));
    }

    // Unregister from callback
    Logger::GetInstance().m_logData.FlushTimeDelegate -= MakeDelegate(FlushTimeLambdaCb);
}

// Dummy function to force linker to keep the code in this file
void Logger_IT_ForceLink() {}