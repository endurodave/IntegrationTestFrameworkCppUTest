#include "IntegrationTest.h"
#include "Logger.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"

// Prevent conflict with GoogleTest ASSERT_TRUE macro definition
#ifdef ASSERT_TRUE
#undef ASSERT_TRUE
#endif

#include "CppUTest/CommandLineTestRunner.h"

//----------------------------------------------------------------------------
// GetInstance
//----------------------------------------------------------------------------
IntegrationTest& IntegrationTest::GetInstance()
{
    static IntegrationTest instance;
    return instance;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
IntegrationTest::IntegrationTest() :
    m_thread("IntegrationTestThread")
{
    m_thread.CreateThread();

    // Start integration tests 500mS after system startup. Alteratively, 
    // create your own worker thread and call Run() directly.
    m_timer.Expired = MakeDelegate(this, &IntegrationTest::Run, m_thread);
    m_timer.Start(std::chrono::milliseconds(500));
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
IntegrationTest::~IntegrationTest()
{
    m_timer.Expired = 0;
}

//----------------------------------------------------------------------------
// Run
//----------------------------------------------------------------------------
void IntegrationTest::Run()
{
    m_timer.Stop();

    int ac = 0;
    char** av = 0;

    // Disable memory leak detection for all integration tests. The CppUTest leak 
    // detection uses global state that is not thread safe. The IntegrationTestThread 
    // and production threads (e.g. LoggerThread) will run concurrently, which can 
    // cause false positive memory leak detection failures. See README.md for more. 
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();

    // Run all tests on the IntegrationTestThread and return the result
    int retVal = RUN_ALL_TESTS(ac, av);

    std::cout << "RUN_ALL_TESTS() return value: " << retVal << std::endl;

    m_complete = true;
}

