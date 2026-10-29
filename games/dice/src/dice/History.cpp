#include "History.h"
#include <string.h>

static void copy(char *to, const char *from, size_t len)
{
    strncpy(to, from != nullptr ? from : "", len - 1);
    to[len - 1] = '\0';
}

void History::add(Tool tool, Mark mark, int16_t result, const char *what, const char *detail, unsigned long at)
{
    HistoryEntry &entry = entries[head];
    entry.tool = tool;
    entry.mark = mark;
    entry.result = result;
    entry.at = at;
    copy(entry.what, what, sizeof(entry.what));
    copy(entry.detail, detail, sizeof(entry.detail));
    head = (head + 1) % CAPACITY;
    if (size < CAPACITY)
    {
        size++;
    }
    version++;
}

const HistoryEntry &History::get(uint8_t index) const
{
    if (index >= size)
    {
        index = 0;
    }
    return entries[(head + CAPACITY - 1 - index) % CAPACITY];
}
