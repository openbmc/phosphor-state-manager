AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

check_PROGRAMS = test_utility

test_utility_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)

test_utility_CXXFLAGS = $(PTHREAD_CFLAGS) $(GPIOPLUS_FLAGS)

test_utility_LDFLAGS = -lgtest_main -lgtest \
	$(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) $(GPIOPLUS_LIBS)

test_utility_SOURCES = test_utility.cpp

test_utility_LDADD = ../chassiskill-utility.o