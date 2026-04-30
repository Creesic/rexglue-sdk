#include <cstdint>
#include <cstring>

extern "C" {

const uint8_t* __std_find_not_ch_1(const uint8_t* first, const uint8_t* last, uint8_t ch) {
    for (; first != last; ++first)
        if (*first != ch) return first;
    return last;
}

const uint8_t* __std_find_first_not_of_trivial_pos_1(const uint8_t* first, const uint8_t* last,
                                                      const uint8_t* needles, size_t needle_count) {
    for (; first != last; ++first)
        if (!memchr(needles, *first, needle_count)) return first;
    return last;
}

const uint8_t* __std_find_last_not_of_trivial_pos_1(const uint8_t* first, const uint8_t* last,
                                                     const uint8_t* needles, size_t needle_count) {
    const uint8_t* it = last;
    while (it != first) {
        --it;
        if (!memchr(needles, *it, needle_count)) return it;
    }
    return last;
}

uint8_t* __std_unique_4(uint8_t* first, uint8_t* last) {
    if (first == last) return last;
    uint8_t* dest = first;
    uint32_t prev;
    memcpy(&prev, first, 4);
    first += 4;
    while (first != last) {
        uint32_t cur;
        memcpy(&cur, first, 4);
        if (cur != prev) {
            dest += 4;
            if (dest != first) memcpy(dest, first, 4);
            prev = cur;
        }
        first += 4;
    }
    return dest + 4;
}

uint8_t* __std_unique_8(uint8_t* first, uint8_t* last) {
    if (first == last) return last;
    uint8_t* dest = first;
    uint64_t prev;
    memcpy(&prev, first, 8);
    first += 8;
    while (first != last) {
        uint64_t cur;
        memcpy(&cur, first, 8);
        if (cur != prev) {
            dest += 8;
            if (dest != first) memcpy(dest, first, 8);
            prev = cur;
        }
        first += 8;
    }
    return dest + 8;
}

}
