// small test file for the bitfake2 library, to test the basic metadata extraction functionality.
// you do NOT need to use this at all. this is just testing my shit

#include "bitfake2.hpp"
#include <iostream>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audiofile>" << std::endl;
        return 1;
    }

    for (int i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " <operation> <audiofile>" << std::endl;
            return 0;
        }

        if (std::strcmp(argv[i], "-bmd") == 0 || std::strcmp(argv[i], "--basicmd") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto md = BitFake::GetMD::GetBasicMD(filepath, BitFake::Codec::AudioCodecType::MP3);
            std::cout << "Title: " << md.Title << std::endl;
            std::cout << "Artist: " << md.Artist << std::endl;
            std::cout << "Album: " << md.Album << std::endl;
            std::cout << "Year: " << md.Year << std::endl;
        }

        if (std::strcmp(argv[i], "-emd") == 0 || std::strcmp(argv[i], "--extendedmd") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto md = BitFake::GetMD::GetExtendedMD(filepath, BitFake::Codec::AudioCodecType::MP3);
            std::cout << "Title: " << md.Title << std::endl;
            std::cout << "Artist: " << md.Artist << std::endl;
            std::cout << "Album: " << md.Album << std::endl;
            std::cout << "Year: " << md.Year << std::endl;
            std::cout << "Genre: " << md.Genre << std::endl;
            std::cout << "Track No: " << md.TrackNo << std::endl;
            std::cout << "Disc No: " << md.DiscNo << std::endl;
            std::cout << "Total Tracks: " << md.TotalTracks << std::endl;
            std::cout << "Total Discs: " << md.TotalDiscs << std::endl;
            std::cout << "Duration: " << md.Duration << " seconds" << std::endl;
            std::cout << "Bitrate: " << md.Bitrate << " kbps" << std::endl;
            std::cout << "Channels: " << md.Channels << std::endl;
        }

        if (std::strcmp(argv[i], "-cm") == 0 || std::strcmp(argv[i], "--checkmagic") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto codec = BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::MP3);
            std::cout << "Detected codec: ";
            if (codec) {
                std::cout << "MP3" << std::endl;
            } else {
                std::cout << "Not MP3" << std::endl;
            }

            // again, basic test, should work with all codecs
        }
    }

    return 0;
}