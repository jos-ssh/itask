#include <inc/string.h>
#include <inc/error.h>
#include <inc/convert.h>

#define LONG_MAX ((long)((1ul << 63) - 1))
#define LONG_MIN ((long)(1ul << 63))

// Bash version of Base64
static const char Digits[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";

static int
chk_add_ulong(unsigned long* dst, unsigned long lhs, unsigned long rhs) {
    if (lhs + rhs >= lhs) {
        *dst = lhs + rhs;
        return 0;
    }
    return -1;
}

static int
chk_mul_ulong(unsigned long* dst, unsigned long lhs, unsigned long rhs) {
    unsigned long res = lhs * rhs;
    if (lhs == 0 || res / lhs == rhs) {
        *dst = res;
        return 0;
    }
    return -1;
}

int
str_to_ulong(const char* str, uint8_t base, unsigned long* out) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (!*str) { return -E_INVAL; }

    unsigned long result = 0;

    for (; *str; ++str) {
        int chk = 0;
        uint8_t digit = strchr(Digits, *str) - Digits;

        if (digit >= base) { return -E_INVAL; }
        chk = chk_mul_ulong(&result, result, base);
        if (chk < 0) { return -E_INVAL; }
        chk = chk_add_ulong(&result, result, digit);
        if (chk < 0) { return -E_INVAL; }
    }

    *out = result;
    return 0;
}

int
strn_to_ulong(const char* str, size_t n, uint8_t base, unsigned long* out) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (!*str) { return -E_INVAL; }
    if (n == 0) { return -E_INVAL; }

    unsigned long result = 0;

    for (size_t i = 0; i < n && *str; ++i, ++str) {
        int chk = 0;
        uint8_t digit = strchr(Digits, *str) - Digits;

        if (digit >= base) { return -E_INVAL; }

        chk = chk_mul_ulong(&result, result, base);
        if (chk < 0) { return -E_INVAL; }
        chk = chk_add_ulong(&result, result, digit);
        if (chk < 0) { return -E_INVAL; }
    }

    *out = result;
    return 0;
}

int
ulong_to_str(unsigned long val, uint8_t base, char buffer[], size_t n) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (n == 0) { return -E_INVAL; }

    size_t length = 0;
    for (; length < n && val > 0; ++length, val /= base) {
        buffer[length] = Digits[val % base];
    }
    if (length == 0) {
        buffer[0] = '0';
        ++length;
    }

    if (val > 0) {
        return -E_NO_MEM;
    }
    for (size_t left = 0, right = length - 1; left < right; ++left, --right) {
        char tmp = buffer[left];
        buffer[left] = buffer[right];
        buffer[right] = tmp;
    }

    return length;
}

int
str_to_long(const char* str, uint8_t base, long* out) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (!*str) { return -E_INVAL; }

    long sign = 1;
    if (*str == '-') {
        sign = -1;
        ++str;
    } else if (*str == '+') {
        ++str;
    }

    unsigned long abs = 0;
    int res = str_to_ulong(str, base, &abs);
    if (res < 0) { return res; }

    if (sign > 0 && abs > (unsigned long)LONG_MAX) { return -E_INVAL; }
    if (sign < 0 && abs > (unsigned long)LONG_MIN) { return -E_INVAL; }

    *out = sign * (long)abs;
    return 0;
}

int
strn_to_long(const char* str, size_t n, uint8_t base, long* out) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (!*str) { return -E_INVAL; }
    if (n == 0) { return -E_INVAL; }

    long sign = 1;
    if (*str == '-') {
        sign = -1;
        ++str;
        --n;
    } else if (*str == '+') {
        ++str;
        --n;
    }

    unsigned long abs = 0;
    int res = strn_to_ulong(str, n, base, &abs);
    if (res < 0) { return res; }

    if (sign > 0 && abs > (unsigned long)LONG_MAX) { return -E_INVAL; }
    if (sign < 0 && abs > (unsigned long)LONG_MIN) { return -E_INVAL; }

    *out = sign * (long)abs;
    return 0;
}

int
long_to_str(long val, uint8_t base, char buffer[], size_t n) {
    if (base < MIN_BASE || base > MAX_BASE) { return -E_INVAL; }
    if (n == 0) { return -E_INVAL; }

    unsigned long abs = 0;
    if (val < 0) {
        *buffer = '-';
        ++buffer;
        --n;

        if (val == LONG_MIN) {
            abs = (unsigned long)val;
        } else {
            abs = -val;
        }

    } else {
        abs = val;
    }

    int res = ulong_to_str(abs, base, buffer, n);
    if (res < 0) { return res; }

    return res + (val < 0);
}
