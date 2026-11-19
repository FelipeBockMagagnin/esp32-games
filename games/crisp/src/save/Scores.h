#ifndef SCORES_H
#define SCORES_H

// Every game's best score, kept as one record in NVS: an id (Cart::id) and a score per game
// that has one, so a game added or taken out of library.json never shifts another's
namespace Scores
{
    void load(const char *nameSpace);
    void store(const char *nameSpace);
}

#endif // SCORES_H
