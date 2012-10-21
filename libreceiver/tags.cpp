#include "tags.h"

namespace receiver {

static const struct {
    int id;
    const char *name;
} tagmap [] = {
    { FID, "fid" },
    { TITLE, "title" },
    { ARTIST, "artist" },
    { SOURCE, "source" },
    { TRACKNR, "tracknr" },

    { GENRE, "genre" },
    { YEAR, "year" },
    { DURATION, "duration" },
    { CODEC, "codec" },
    { LENGTH, "length" },

    { BITRATE, "bitrate" },
    { SAMPLERATE, "samplerate" },
    { TYPE, "type" },
    { MOOD, "mood" },
    { ORIGINALARTIST, "originalartist" },

    { REMIXED, "remixed" },
    { CONDUCTOR, "conductor" },
    { COMPOSER, "composer" },
    { ENSEMBLE, "ensemble" },
    { LYRICIST, "lyricist" }
};

};
