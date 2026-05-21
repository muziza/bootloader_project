from platformio.public import UnityTestRunner
class CustomTestRunner(UnityTestRunner):
    EXTRA_LIB_DEPS = None  # Ignore "throwtheswitch/Unity" package
