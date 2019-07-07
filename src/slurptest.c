#include <stdlib.h>
#include <string.h>

char *slurp(const char *path);

int main(void)
{
    /* Let's ignore the newline and potential trailing garbage. */
    const char *expected = "test file, please ignore";
    char *actual = slurp("assets/test");
    return strncmp(actual, expected, strlen(expected));
}
