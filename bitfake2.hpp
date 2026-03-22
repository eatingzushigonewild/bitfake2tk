#ifndef BITFAKE2_HPP
#define BITFAKE2_HPP

// bitfake2.hpp
// A AIO bitfake2 "api" header file for projects.

#include <cstdio>
#include <filesystem>
namespace fs = std::filesystem;

namespace BitFake
{
    // defining some basic types and structures for the bitfake2 toolkit
    // goes before getting into the juicy stuff.
    class Codec
    {
        public:
        enum class AudioCodecType
        {
            MP3,
            FLAC,
            WAV,
            AAC,
            OGG,
            OPUS,
            ALAC,
            AIFF,
            WMA,
            PCM
        };
    };


    class Type
    {
        public:
        struct BasicMD
        {
            const char* Title;
            const char* Artist;
            const char* Album;
            const char* Year;
        };

        struct ExtendedMD
        {
            const char* Title, Artist, Album, Year, Genre;
            int TrackNo, DiscNo, TotalTracks, TotalDiscs;
            int Duration; // in seconds
            int Bitrate, Channels;
            BitFake::Codec::AudioCodecType AudioCodec;
        };

        struct ReplayGainMD
        {
            float TrackGain, AlbumGain;
            int TrackPeak, AlbumPeak;
        };
    };

    class GetMD
    {
        public:
        static Type::BasicMD GetBasicMD(const fs::path& filepath)
        {
            // placeholder implementation
            Type::BasicMD md;
            md.Title = "Unknown Title";
            md.Artist = "Unknown Artist";
            md.Album = "Unknown Album";
            md.Year = "Unknown Year";
            if (!fs::exists(filepath))
            {
                fprintf(stderr, "File does not exist: %s\n", filepath.string().c_str());
                return md;
            }

            // getting metadata
            // place holder code, its 02:17 am 
            return md;
        }
    };
}

#endif // BITFAKE2_HPP