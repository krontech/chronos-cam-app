#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
const char* git_version_str = STR(__GIT_VERSION);

