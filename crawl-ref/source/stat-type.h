#pragma once

enum stat_type
{
    STAT_STR,
    STAT_INT,
    STAT_DEX,
    NUM_STATS,
    STAT_ALL, // must remain after NUM_STATS
    STAT_RANDOM,
};

enum jivya_stat_target_type
{
    JSTAT_LOW = 0,
    JSTAT_AVG,
    JSTAT_MAX,
    JSTAT_UNSET, // default before being set once.
};