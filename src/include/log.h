// none of these can be used as a function pointer, idiot!

#define log_info(fmt, ...) { \
    tty_puts(MIX(BLACK, GREEN), "[INFO]"); \
    tty_puts(MIX(WHITE, BLACK), ": "); \
    tty_putf(MIX(WHITE, BLACK), fmt __VA_OPT__(,) __VA_ARGS__); }

#define log_warn(fmt, ...) { \
    tty_puts(MIX(BLACK, YELLOW), "[WARN]"); \
    tty_putf(MIX(YELLOW, BLACK), ": "); \
    tty_putf(MIX(YELLOW, BLACK), fmt __VA_OPT__(,) __VA_ARGS__); }

#define log_err(fmt, ...) { \
    tty_puts(MIX(BLACK, RED), "[ERR!]"); \
    tty_putf(MIX(RED, BLACK), ": "); \
    tty_putf(MIX(RED, BLACK), fmt __VA_OPT__(,) __VA_ARGS__); \
    tty_disable_cursor(); \
    x86_hang(); }
