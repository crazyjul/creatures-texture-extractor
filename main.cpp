#include <iostream>
#include <fstream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

const uint32_t C16_565_format = 1;
const uint32_t C16_16bit = 2;

struct Bitmap
{
    uint32_t offset;
    uint16_t width;
    uint16_t height;
    std::vector< uint32_t > offsets;
};

void convert_s16_to_rgba8( const uint16_t *input_image,
    uint32_t *output_image,
    size_t pixel_count,
    bool is_rgb_565,
    bool is_transparent );

template < typename _type_ >
void read( _type_ &value, std::istream &stream )
{
    stream.read( ( char * )&value, sizeof( value ) );
}

int main( int argc, char *argv[] )
{
    if ( argc != 3 )
    {
        std::cerr << "Not enough arguments, 2 expected" << std::endl;

        return -1;
    }

    std::fstream input_file;
    std::string input_file_name{argv[1]};
    std::string output_file_name{argv[2]};

    input_file.open( input_file_name, std::fstream::in | std::fstream::binary );

    if ( !input_file.is_open() )
    {
        std::cerr << "Unable to open output file " << argv[1] << std::endl;

        return -1;
    }

    bool is_c16 = input_file_name.find( ".c16" ) == input_file_name.size() - 4;
    bool is_blk = input_file_name.find( ".blk" ) == input_file_name.size() - 4;

    uint32_t pixel_format;
    uint16_t bitmap_count;

    uint16_t tile_width = 1;
    uint16_t tile_height = 1;

    read( pixel_format, input_file );

    if ( is_blk )
    {
        read( tile_width, input_file );
        read( tile_height, input_file );
    }

    read( bitmap_count, input_file );

    if ( bitmap_count > 1 && output_file_name.find( "%i" ) == std::string::npos )
    {
        std::cerr << "Multiple images need an output filename with a %i where the image number is located" << std::endl;

        return -1;
    }

    if ( is_c16 && !( pixel_format & C16_16bit ) )
    {
        std::cerr << "Compressed 8bit input file, not supported yet" << std::endl;

        return -1;
    }

    std::vector< Bitmap > bitmaps;

    bitmaps.resize( bitmap_count );

    for ( uint16_t i = 0; i < bitmap_count; i++ )
    {
        read( bitmaps[i].offset, input_file );
        read( bitmaps[i].width, input_file );
        read( bitmaps[i].height, input_file );

        if ( is_c16 )
        {
            bitmaps[i].offsets.resize( bitmaps[i].height );

            bitmaps[i].offsets[0] = bitmaps[i].offset;

            // Skip scanline offsets
            for ( int j = 1; j < bitmaps[i].height; ++j )
            {
                read( bitmaps[i].offsets[j], input_file );
            }
        }
    }

    auto test = input_file.tellg();

    std::vector< uint32_t > data_32bit;
    std::vector< uint16_t > data_16bit;

    bool is_rgb565 = pixel_format & C16_565_format;

    for ( int image = 0; image < bitmap_count; ++image )
    {
        auto &bitmap = bitmaps[image];

        data_32bit.resize( bitmap.width * bitmap.height );
        data_16bit.resize( bitmap.width * bitmap.height );
        input_file.seekg( bitmap.offset, std::fstream::beg );

        if ( is_c16 )
        {
            uint16_t tag;

            auto pixels = data_32bit.data();

            for ( uint32_t i = 0; i < bitmap.height; i++ )
            {
                int pos = input_file.tellg();
                input_file.seekg( bitmap.offsets[i], std::fstream::beg );
                read( tag, input_file );
                while ( tag )
                {
                    auto count = tag >> 1;

                    input_file.seekg( bitmap.offsets[i] + 1, std::fstream::beg );

                    uint16_t tag2;
                    read( tag2, input_file );

                    // Color block
                    if ( tag & 0x01 )
                    {
                        input_file.read( ( char * )data_16bit.data(), count * sizeof( uint16_t ) );

                        convert_s16_to_rgba8( data_16bit.data(), pixels, count, is_rgb565, false );
                    }
                    // Transparency block
                    else
                    {
                        memset( pixels, 0, count * sizeof( *pixels ) );
                    }

                    pixels += count;

                    read( tag, input_file );
                }
            }

            auto pixel_count = pixels - data_32bit.data();
            if ( ( pixels - data_32bit.data() ) != ( bitmap.width * bitmap.height ) )
            {
                std::cerr << "Something is wrong";
                return -1;
            }
        }
        else
        {
            input_file.read( reinterpret_cast< char * >( data_16bit.data() ), bitmap.width * bitmap.height * 2 );

            convert_s16_to_rgba8( data_16bit.data(), data_32bit.data(), data_32bit.size(), is_rgb565, !is_blk );
        }

        char filename[256];

        snprintf( filename, 255, output_file_name.c_str(), image );

        if ( stbi_write_png( filename, bitmap.width, bitmap.height, 4, data_32bit.data(), bitmap.width * 4 ) == 0 )
        {
            std::cerr << "Unable to write output file " << argv[2] << std::endl;

            return -1;
        }
    }

    std::cout << "Done" << std::endl;

    return 0;
}

void convert_s16_to_rgba8( const uint16_t *input_image,
    uint32_t *output_image,
    size_t pixel_count,
    bool is_rgb_565,
    bool is_transparent )
{
    uint32_t black_or_transparent = is_transparent ? 0x00 : 0xFF000000;

    if ( is_rgb_565 )
    {
        for ( size_t i = 0; i < pixel_count; ++i )
        {
            auto pixel = input_image[i];

            if ( pixel == 0 )
            {
                output_image[i] = black_or_transparent;
            }
            else
            {
                auto r = ( pixel >> 8 ) & 0xF8;
                auto g = ( pixel >> 3 ) & 0xFC;
                auto b = ( pixel << 3 ) & 0xF8;
                output_image[i] = ( b << 16 ) | ( g << 8 ) | ( r << 0 ) | ( 0xFF << 24 );
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < pixel_count; ++i )
        {
            auto pixel = input_image[i];

            if ( pixel == 0 )
            {
                output_image[i] = black_or_transparent;
            }
            else
            {
                auto r = ( pixel >> 7 ) & 0xF8;
                auto g = ( pixel >> 2 ) & 0xF8;
                auto b = ( pixel << 3 ) & 0xF8;
                output_image[i] = ( b << 16 ) | ( g << 8 ) | ( r << 0 ) | ( 0xFF << 24 );
            }
        }
    }
}
