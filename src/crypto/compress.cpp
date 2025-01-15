
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#include "./compress.hpp"

#include <lzma.h>

#include "crypto/safe_buffer.hpp"

namespace
{
auto init_encoder(lzma_stream* strm) -> bool
{
  // Use the default preset (6) for LZMA2.
  //
  // The lzma_options_lzma structure and the lzma_lzma_preset() function
  // are declared in lzma/lzma12.h (src/liblzma/api/lzma/lzma12.h in the
  // source package or e.g. /usr/include/lzma/lzma12.h depending on
  // the install prefix).
  lzma_options_lzma opt_lzma2;
  if (lzma_lzma_preset(&opt_lzma2, LZMA_PRESET_DEFAULT) != 0U) {
    // It should never fail because the default preset
    // (and presets 0-9 optionally with LZMA_PRESET_EXTREME)
    // are supported by all stable liblzma versions.
    //
    // (The encoder initialization later in this function may
    // still fail due to unsupported preset *if* the features
    // required by the preset have been disabled at build time,
    // but no-one does such things except on embedded systems.)
    std::println(stderr, "Unsupported preset, possibly a bug\n");
    return false;
  }
  // Now we could customize the LZMA2 options if we wanted. For example,
  // we could set the dictionary size (opt_lzma2.dict_size) to
  // something else than the default (8 MiB) of the default preset.
  // See lzma/lzma12.h for details of all LZMA2 options.
  //
  // The x86 BCJ filter will try to modify the x86 instruction stream so
  // that LZMA2 can compress it better. The x86 BCJ filter doesn't need
  // any options so it will be set to NULL below.
  //
  // Construct the filter chain. The uncompressed data goes first to
  // the first filter in the array, in this case the x86 BCJ filter.
  // The array is always terminated by setting .id = LZMA_VLI_UNKNOWN.
  //
  // See lzma/filter.h for more information about the lzma_filter
  // structure.
  std::array filters = {
      // {.id = LZMA_FILTER_X86, .options = NULL},
      lzma_filter {.id = LZMA_FILTER_LZMA2, .options = &opt_lzma2},
      lzma_filter {.id = LZMA_VLI_UNKNOWN, .options = NULL},
  };

  // Initialize the encoder using the custom filter chain.
  lzma_ret ret = lzma_stream_encoder(strm, filters.data(), LZMA_CHECK_CRC64);

  if (ret == LZMA_OK) {
    return true;
  }

  std::string_view error_message = [&ret]()
  {
    switch (ret) {
      case LZMA_MEM_ERROR:
        return std::string_view {"Memory allocation failed"};
      case LZMA_OPTIONS_ERROR:
        // We are no longer using a plain preset so this error
        // message has been edited accordingly compared to
        // 01_compress_easy.c.
        return std::string_view {"Specified filter chain is not supported"};
      case LZMA_UNSUPPORTED_CHECK:
        return std::string_view {"Specified integrity check is not supported"};
      default:
        return std::string_view {"Unknown error, possibly a bug"};
    }
  }();

  std::println("Error initializing the encoder: {} (error code {})",
               error_message,
               static_cast<int>(ret));
  return false;
}

// This function is identical to the one in 01_compress_easy.c.
auto compress_lzma(lzma_stream* strm, std::span<const unsigned char> input)
    -> safe_vector<unsigned char>
{
  const lzma_action action = LZMA_FINISH;

  safe_array<unsigned char, BUFSIZ> outbuf {};
  safe_vector<unsigned char> result {};

  strm->next_in = input.data();
  strm->avail_in = input.size();
  strm->next_out = outbuf.data();
  strm->avail_out = outbuf.size();

  while (true) {
    lzma_ret ret = lzma_code(strm, action);

    if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
      const size_t write_size = outbuf.size() - strm->avail_out;

      result.insert(result.end(),
                    outbuf.begin(),
                    outbuf.begin() + static_cast<int64_t>(write_size));

      strm->next_out = outbuf.data();
      strm->avail_out = outbuf.size();
    }

    if (ret == LZMA_STREAM_END) {
      return result;
    }
    if (ret != LZMA_OK) {
      std::println(stderr,
                   "Encoder error (error code {})",
                   static_cast<std::underlying_type_t<lzma_ret>>(ret));
      throw std::runtime_error("Could not compress");
    }
  }
}

auto init_decoder(lzma_stream* strm) -> bool
{
  // Initialize a .xz decoder. The decoder supports a memory usage limit
  // and a set of flags.
  //
  // The memory usage of the decompressor depends on the settings used
  // to compress a .xz file. It can vary from less than a megabyte to
  // a few gigabytes, but in practice (at least for now) it rarely
  // exceeds 65 MiB because that's how much memory is required to
  // decompress files created with "xz -9". Settings requiring more
  // memory take extra effort to use and don't (at least for now)
  // provide significantly better compression in most cases.
  //
  // Memory usage limit is useful if it is important that the
  // decompressor won't consume gigabytes of memory. The need
  // for limiting depends on the application. In this example,
  // no memory usage limiting is used. This is done by setting
  // the limit to UINT64_MAX.
  //
  // The .xz format allows concatenating compressed files as is:
  //
  //     echo foo | xz > foobar.xz
  //     echo bar | xz >> foobar.xz
  //
  // When decompressing normal standalone .xz files, LZMA_CONCATENATED
  // should always be used to support decompression of concatenated
  // .xz files. If LZMA_CONCATENATED isn't used, the decoder will stop
  // after the first .xz stream. This can be useful when .xz data has
  // been embedded inside another file format.
  //
  // Flags other than LZMA_CONCATENATED are supported too, and can
  // be combined with bitwise-or. See lzma/container.h
  // (src/liblzma/api/lzma/container.h in the source package or e.g.
  // /usr/include/lzma/container.h depending on the install prefix)
  // for details.
  lzma_ret ret = lzma_stream_decoder(strm, UINT64_MAX, 0);

  // Return successfully if the initialization went fine.
  if (ret == LZMA_OK) {
    return true;
  }

  // Something went wrong. The possible errors are documented in
  // lzma/container.h (src/liblzma/api/lzma/container.h in the source
  // package or e.g. /usr/include/lzma/container.h depending on the
  // install prefix).
  //
  // Note that LZMA_MEMLIMIT_ERROR is never possible here. If you
  // specify a very tiny limit, the error will be delayed until
  // the first headers have been parsed by a call to lzma_code().
  std::string_view msg = [&]()
  {
    switch (ret) {
      case LZMA_MEM_ERROR:
        return std::string_view {"Memory allocation failed"};

      case LZMA_OPTIONS_ERROR:
        return std::string_view {"Unsupported decompressor flags"};

      default:
        // This is most likely LZMA_PROG_ERROR indicating a bug in
        // this program or in liblzma. It is inconvenient to have a
        // separate error message for errors that should be impossible
        // to occur, but knowing the error code is important for
        // debugging. That's why it is good to print the error code
        // at least when there is no good error message to show.
        return std::string_view {"Unknown error, possibly a bug"};
    }
  }();

  std::println(stderr,
               "Error initializing the decoder: {} (error code {})",
               msg,
               static_cast<std::underlying_type_t<lzma_ret>>(ret));
  return false;
}

auto decompress_error(lzma_ret return_code)
{
  // It is important to check for LZMA_STREAM_END. Do not
  // assume that getting ret != LZMA_OK would mean that
  // everything has gone well or that when you aren't
  // getting more output it must have successfully
  // decoded everything.

  // It's not LZMA_OK nor LZMA_STREAM_END,
  // so it must be an error code. See lzma/base.h
  // (src/liblzma/api/lzma/base.h in the source package
  // or e.g. /usr/include/lzma/base.h depending on the
  // install prefix) for the list and documentation of
  // possible values. Many values listen in lzma_ret
  // enumeration aren't possible in this example, but
  // can be made possible by enabling memory usage limit
  // or adding flags to the decoder initialization.
  const char* msg {};
  switch (return_code) {
    case LZMA_MEM_ERROR:
      msg = "Memory allocation failed";
      break;

    case LZMA_FORMAT_ERROR:
      // .xz magic bytes weren't found.
      msg = "The input is not in the .xz format";
      break;

    case LZMA_OPTIONS_ERROR:
      // For example, the headers specify a filter
      // that isn't supported by this liblzma
      // version (or it hasn't been enabled when
      // building liblzma, but no-one sane does
      // that unless building liblzma for an
      // embedded system). Upgrading to a newer
      // liblzma might help.
      //
      // Note that it is unlikely that the file has
      // accidentally became corrupt if you get this
      // error. The integrity of the .xz headers is
      // always verified with a CRC32, so
      // unintentionally corrupt files can be
      // distinguished from unsupported files.
      msg = "Unsupported compression options";
      break;

    case LZMA_DATA_ERROR:
      msg = "Compressed file is corrupt";
      break;

    case LZMA_BUF_ERROR:
      // Typically this error means that a valid
      // file has got truncated, but it might also
      // be a damaged part in the file that makes
      // the decoder think the file is truncated.
      // If you prefer, you can use the same error
      // message for this as for LZMA_DATA_ERROR.
      msg =
          "Compressed file is truncated or "
          "otherwise corrupt";
      break;

    default:
      // This is most likely LZMA_PROG_ERROR.
      msg = "Unknown error, possibly a bug";
      break;
  }

  throw std::runtime_error(
      std::format("Could not decompress:\n"
                  "Decoder error: "
                  "{} (error code {})",
                  msg,
                  static_cast<std::underlying_type_t<lzma_ret>>(return_code)));
}

auto decompress_lzma(lzma_stream* strm, std::span<const unsigned char> input)
    -> safe_vector<unsigned char>
{
  // When LZMA_CONCATENATED flag was used when initializing the decoder,
  // we need to tell lzma_code() when there will be no more input.
  // This is done by setting action to LZMA_FINISH instead of LZMA_RUN
  // in the same way as it is done when encoding.
  //
  // When LZMA_CONCATENATED isn't used, there is no need to use
  // LZMA_FINISH to tell when all the input has been read, but it
  // is still OK to use it if you want. When LZMA_CONCATENATED isn't
  // used, the decoder will stop after the first .xz stream. In that
  // case some unused data may be left in strm->next_in.
  const lzma_action action = LZMA_FINISH;

  safe_array<unsigned char, BUFSIZ> outbuf {};
  safe_vector<unsigned char> result {};

  strm->next_in = input.data();
  strm->avail_in = input.size();
  strm->next_out = outbuf.data();
  strm->avail_out = outbuf.size();

  while (true) {
    lzma_ret const ret = lzma_code(strm, action);

    if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
      const size_t write_size = outbuf.size() - strm->avail_out;

      result.insert(result.end(),
                    outbuf.begin(),
                    outbuf.begin() + static_cast<int64_t>(write_size));

      strm->next_out = outbuf.data();
      strm->avail_out = outbuf.size();
    }

    if (ret == LZMA_STREAM_END) {
      // Once everything has been decoded successfully, the
      // return value of lzma_code() will be LZMA_STREAM_END.
      return result;
    }
    if (ret != LZMA_OK) {
      decompress_error(ret);
    }
  }
}
}  // namespace

struct owned_lzma_decode_stream
{
  lzma_stream strm {};
  owned_lzma_decode_stream()
  {
    strm = LZMA_STREAM_INIT;
    if (!init_decoder(&strm)) {
      throw std::runtime_error("Could not initialize compression stream");
    }
  }
  owned_lzma_decode_stream(const owned_lzma_decode_stream&) = delete;
  owned_lzma_decode_stream(owned_lzma_decode_stream&&) = delete;
  auto operator=(const owned_lzma_decode_stream&)
      -> owned_lzma_decode_stream& = delete;
  auto operator=(owned_lzma_decode_stream&&)
      -> owned_lzma_decode_stream& = delete;
  ~owned_lzma_decode_stream() { lzma_end(&strm); }
};
struct owned_lzma_encode_stream
{
  lzma_stream strm {};
  owned_lzma_encode_stream()
  {
    strm = LZMA_STREAM_INIT;
    if (!init_encoder(&strm)) {
      throw std::runtime_error("Could not initialize compression stream");
    }
  }
  owned_lzma_encode_stream(const owned_lzma_encode_stream&) = delete;
  owned_lzma_encode_stream(owned_lzma_encode_stream&&) = delete;
  auto operator=(const owned_lzma_encode_stream&)
      -> owned_lzma_encode_stream& = delete;
  auto operator=(owned_lzma_encode_stream&&)
      -> owned_lzma_encode_stream& = delete;
  ~owned_lzma_encode_stream() { lzma_end(&strm); }
};

auto decompress(std::span<const unsigned char> input)
    -> safe_vector<unsigned char>
{
  owned_lzma_decode_stream strm {};
  auto result = decompress_lzma(&strm.strm, input);
  return result;
}

auto compress(std::span<const unsigned char> input)
    -> safe_vector<unsigned char>
{
  owned_lzma_encode_stream strm {};

  auto compressed = compress_lzma(&strm.strm, input);

  return compressed;
}
