/*
 * ffMetadataEx
 * Copyright (C) 2025 OuterTune Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a breakdown of attribution, please refer to the git commit history.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jni.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <android/log.h>

jobject toJstring(JNIEnv *pEnv, const char *album);

char *getRealPathFromFd(const int fd);

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
//#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}


extern "C" JNIEXPORT jobject JNICALL
Java_wah_mikooomich_ffMetadataEx_FFmpegWrapper_getFullAudioMetadata(JNIEnv *env, jobject obj, jint fd) {

    // create jobject
    jclass metadataClass = env->FindClass("wah/mikooomich/ffMetadataEx/AudioMetadata");
    if (metadataClass == nullptr) {
        return nullptr;
    }
    jobject ret = env->NewObject(metadataClass, env->GetMethodID(metadataClass, "<init>", "()V"));
    if (ret == nullptr) {
        return nullptr;
    }
    jfieldID fid;

    // extract from file
    const char *file_path = getRealPathFromFd(fd);
    if (!file_path) {
        fid = env->GetFieldID(metadataClass, "status", "I");
        env->SetIntField(ret, fid, 1001);
        return ret;
    }

    AVFormatContext *format_context = nullptr;
    if (avformat_open_input(&format_context, file_path, nullptr, nullptr) != 0) {
        fid = env->GetFieldID(metadataClass, "status", "I");
        env->SetIntField(ret, fid, 1002);
        return ret;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        avformat_close_input(&format_context);
        fid = env->GetFieldID(metadataClass, "status", "I");
        env->SetIntField(ret, fid, 1003);
        return ret;
    }

    int audio_stream_index = -1;
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    // audio file metadata
    const char *title = nullptr;
    const char *artist = nullptr;
    const char *album = nullptr;
    const char *genre = nullptr;
    std::vector<std::string> extraRaw;

    // audio stream metadata
    const char *codec_name = nullptr;
    const char *codec_type = nullptr;
    int64_t bitrate = format_context->bit_rate;
    int sample_rate = 0;
    int channels = 0;
    int64_t duration = format_context->duration;

    // container level tags (audio formats e.g. flac, mp3)
    AVDictionaryEntry *tag = nullptr;
    while ((tag = av_dict_get(format_context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (strcasecmp(tag->key, "title") == 0) {
            title = tag->value;
        } else if (strcasecmp(tag->key, "artist") == 0 || strcasecmp(tag->key, "artists") == 0) {
            artist = tag->value;
        } else if (strcasecmp(tag->key, "album") == 0) {
            album = tag->value;
        } else if (strcasecmp(tag->key, "genre") == 0) {
            genre = tag->value;
        } else {
            std::string entry = std::string(tag->key) + ": " + std::string(tag->value);
            extraRaw.push_back(entry);
        }
    }

    // audio stream tags (for mixed containers e.g. ogg)
    if (audio_stream_index >= 0) {
        AVStream *audio_stream = format_context->streams[audio_stream_index];
        AVCodecParameters *codecpar = audio_stream->codecpar;

        // add codec information
        sample_rate = codecpar->sample_rate;
        channels = codecpar->ch_layout.nb_channels;

        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        if (codec != nullptr) {
            codec_name = codec->long_name;
        }
        const char *type = av_get_media_type_string(codecpar->codec_type);
        if (type != nullptr) {
            codec_type = type;
        }

        // add audio stream tags (ID3 result)
        tag = nullptr;
        while ((tag = av_dict_get(audio_stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            std::string entry = std::string(tag->key) + ": " + std::string(tag->value);
            extraRaw.push_back(entry);
        }
    }

    avformat_close_input(&format_context);

    fid = env->GetFieldID(metadataClass, "bitrate", "J");
    env->SetLongField(ret, fid, bitrate);

    fid = env->GetFieldID(metadataClass, "sampleRate", "I");
    env->SetIntField(ret, fid, sample_rate);

    fid = env->GetFieldID(metadataClass, "channels", "I");
    env->SetIntField(ret, fid, channels);

    fid = env->GetFieldID(metadataClass, "duration", "J");
    env->SetLongField(ret, fid, duration);

    fid = env->GetFieldID(metadataClass, "status", "I");
    env->SetIntField(ret, fid, 0);

    if (codec_name) {
        fid = env->GetFieldID(metadataClass, "codec", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, env->NewStringUTF(codec_name));
    }
    if (codec_type) {
        fid = env->GetFieldID(metadataClass, "codecType", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, env->NewStringUTF(codec_type));
    }
    if (title) {
        fid = env->GetFieldID(metadataClass, "title", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, toJstring(env, title));
    }
    if (artist) {
        fid = env->GetFieldID(metadataClass, "artist", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, toJstring(env, artist));
    }
    if (album) {
        fid = env->GetFieldID(metadataClass, "album", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, toJstring(env, album));
    }
    if (genre) {
        fid = env->GetFieldID(metadataClass, "genre", "Ljava/lang/String;");
        env->SetObjectField(ret, fid, toJstring(env, genre));
    }

    jfieldID extrasField = env->GetFieldID(metadataClass, "extrasRaw", "[Ljava/lang/String;");
    if (extrasField) {
        jclass stringClass = env->FindClass("java/lang/String");
        jobjectArray jExtras = env->NewObjectArray(static_cast<jsize>(extraRaw.size()), stringClass,
                                                   nullptr);
        for (jsize i = 0; i < extraRaw.size(); ++i) {
            jstring jstr = env->NewStringUTF(extraRaw[i].c_str());
            env->SetObjectArrayElement(jExtras, i, jstr);
        }
        env->SetObjectField(ret, extrasField, jExtras);
    }

    return ret;
}


// Structure for the values used in ReplayGain normalization (modern EBU R128 style).
// ReplayGain gain can be computed as e.g. target_lufs - integrated_lufs (common targets: -18 or -23).
// True peak is used to avoid clipping (normalize peak to <= 1.0).
struct LoudnessValues {
    double integrated_lufs = 0.0;   // "I" value in LUFS (the key value for gain calculation)
    double loudness_range = 0.0;    // LRA in LU
    double true_peak = 0.0;         // True peak in dBTP (most important for normalization safety)
    double sample_peak = 0.0;       // Sample peak in dBFS (fallback)
    std::string debug;
};

extern "C" JNIEXPORT jobject JNICALL
Java_wah_mikooomich_ffMetadataEx_FFmpegWrapper_getVibebur128(JNIEnv *env, jobject obj, jint fd) {
    // create jobject
    jclass vibebur128Class = env->FindClass("wah/mikooomich/ffMetadataEx/Vibebur128");
    if (vibebur128Class == nullptr) {
        __android_log_print(ANDROID_LOG_DEBUG, "ffMetaDataEx", "%s", "Cannot find jclass");
        return nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "you ");
    jobject ret = env->NewObject(vibebur128Class, env->GetMethodID(vibebur128Class, "<init>", "()V"));
    if (ret == nullptr) {
        __android_log_print(ANDROID_LOG_DEBUG, "ffMetaDataEx", "%s", "Cannot construct jobject");
        return nullptr;
    }


    jclass doubleClass = env->FindClass("java/lang/Double");
    jmethodID doubleCtor = env->GetMethodID(doubleClass, "<init>", "(D)V");
    if (!doubleCtor) {
        __android_log_print(ANDROID_LOG_DEBUG, "ffMetaDataEx", "%s", "Cannot construct double jobject");
        return nullptr;
    }
    jfieldID fid;

    LoudnessValues result;

    try {
        // open file and audio stream
        const char *file_path = getRealPathFromFd(fd);
        if (!file_path) {
            result.debug = "Error getting file path from fd";
            throw -1;
        }
        AVFormatContext *fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, file_path, nullptr, nullptr) < 0) {
            result.debug = "Error opening file";
            throw -2;
        }
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            result.debug = "Error finding stream info";
            avformat_close_input(&fmt_ctx);
            throw -11;
        }

        int audio_stream_idx = -1;
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_idx = i;
                break;
            }
        }
        if (audio_stream_idx == -1) {
            result.debug = "No audio stream found";
            avformat_close_input(&fmt_ctx);
            throw -12;
        }

        AVCodecParameters *codec_par = fmt_ctx->streams[audio_stream_idx]->codecpar;
        const AVCodec *decoder = avcodec_find_decoder(codec_par->codec_id);
        if (!decoder) {
            result.debug = "Decoder not found";
            avformat_close_input(&fmt_ctx);
            throw -13;
        }

        AVCodecContext *dec_ctx = avcodec_alloc_context3(decoder);
        if (!dec_ctx) {
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to allocate codec context";
            throw -14;
        }
        if (avcodec_parameters_to_context(dec_ctx, codec_par) < 0) {
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to copy codec parameters";
            throw -15;
        }
        if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to open decoder";
            throw -16;
        }

        // ebur setup part
        AVFilterGraph *filter_graph = avfilter_graph_alloc();
        if (!filter_graph) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to allocate filter graph";
            throw 1001;
        }

        const AVFilter *abuffer = avfilter_get_by_name("abuffer");
        const AVFilter *ebur128 = avfilter_get_by_name("ebur128");
        const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

        AVFilterContext *src_ctx = nullptr;
        AVFilterContext *ebur_ctx = nullptr;
        AVFilterContext *sink_ctx = nullptr;

        char args[512];
        char ch_layout_buf[64] = {0};
        av_channel_layout_describe(&dec_ctx->ch_layout, ch_layout_buf, sizeof(ch_layout_buf));

        snprintf(args, sizeof(args),
                 "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                 fmt_ctx->streams[audio_stream_idx]->time_base.num,
                 fmt_ctx->streams[audio_stream_idx]->time_base.den,
                 dec_ctx->sample_rate,
                 av_get_sample_fmt_name(dec_ctx->sample_fmt),
                 ch_layout_buf);

        if (avfilter_graph_create_filter(&src_ctx, abuffer, "in", args, nullptr, filter_graph) < 0) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to create abuffer filter";
            throw 1002;
        }
        if (avfilter_graph_create_filter(&ebur_ctx, ebur128, "ebur128", "metadata=1:peak=true", nullptr, filter_graph) <
            0) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to create ebur128 filter";
            throw 1003;
        }
        if (avfilter_graph_create_filter(&sink_ctx, abuffersink, "out", nullptr, nullptr, filter_graph) < 0) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to create abuffersink filter";
            throw 1004;
        }

        // Link filters
        if (avfilter_link(src_ctx, 0, ebur_ctx, 0) < 0 ||
            avfilter_link(ebur_ctx, 0, sink_ctx, 0) < 0) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to link filters";
            throw 1005;
        }

        if (avfilter_graph_config(filter_graph, nullptr) < 0) {
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
            result.debug = "Failed to configure filter graph";
            throw 1006;
        }

        // do the entire file
        AVPacket *pkt = av_packet_alloc();
        AVFrame *dec_frame = av_frame_alloc();
        AVFrame *filt_frame = av_frame_alloc();

        int value;
        while ((value = av_read_frame(fmt_ctx, pkt)) >= 0) {
            if (pkt->stream_index == audio_stream_idx) {
                if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
                    while (avcodec_receive_frame(dec_ctx, dec_frame) >= 0) {
                        if (av_buffersrc_add_frame_flags(src_ctx, dec_frame, AV_BUFFERSRC_FLAG_PUSH) >= 0) {
                            while (av_buffersink_get_frame(sink_ctx, filt_frame) >= 0) {
                                // Extract metadata injected by ebur128 (updated on every output frame)
                                AVDictionaryEntry *entry = nullptr;

                                if ((entry = av_dict_get(filt_frame->metadata, "lavfi.r128.I", nullptr, 0))) {
                                    result.integrated_lufs = std::atof(entry->value);
                                }
                                if ((entry = av_dict_get(filt_frame->metadata, "lavfi.r128.LRA", nullptr, 0))) {
                                    result.loudness_range = std::atof(entry->value);
                                }
                                if ((entry = av_dict_get(filt_frame->metadata, "lavfi.r128.true_peak", nullptr, 0))) {
                                    result.true_peak = std::atof(entry->value);
                                }
                                if ((entry = av_dict_get(filt_frame->metadata, "lavfi.r128.sample_peak", nullptr, 0))) {
                                    result.sample_peak = std::atof(entry->value);
                                }

                                av_frame_unref(filt_frame);
                            }
                        }
                        av_frame_unref(dec_frame);
                    }
                }
            }
            av_packet_unref(pkt);
        }

        // Flush decoder and filter
        avcodec_send_packet(dec_ctx, nullptr);
        while (avcodec_receive_frame(dec_ctx, dec_frame) >= 0) {
            // same processing as above (omitted for brevity - copy the block if needed)
            av_frame_unref(dec_frame);
        }
        av_buffersrc_add_frame_flags(src_ctx, nullptr, AV_BUFFERSRC_FLAG_PUSH); // flush
        while (av_buffersink_get_frame(sink_ctx, filt_frame) >= 0) {
            // final metadata read (same as above)
            // ... copy the metadata extraction block here if you want the absolute final values
            av_frame_unref(filt_frame);
        }


        av_frame_free(&filt_frame);
        av_frame_free(&dec_frame);
        av_packet_free(&pkt);
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
    } catch (int err) {
        fid = env->GetFieldID(vibebur128Class, "status", "I");
        env->SetIntField(ret, fid, err);
        return ret;
    }

    // add data to jobject with data
    jfieldID f1 = env->GetFieldID(vibebur128Class, "truePeak", "Ljava/lang/Double;");
    jfieldID f2 = env->GetFieldID(vibebur128Class, "samplePeak", "Ljava/lang/Double;");
    jfieldID f3 = env->GetFieldID(vibebur128Class, "integratedLufs", "Ljava/lang/Double;");
    jfieldID f4 = env->GetFieldID(vibebur128Class, "loudnessRange", "Ljava/lang/Double;");
    if (!f1 || !f2 || !f3 || !f4) {
        fid = env->GetFieldID(vibebur128Class, "status", "I");
        env->SetIntField(ret, fid, (!f1 << 0) | (!f2 << 1) | (!f3 << 2) | (!f4 << 3));
        return ret;
    }

    jobject truePeakObj = env->NewObject(doubleClass, doubleCtor, result.true_peak);
    jobject samplepeakObj = env->NewObject(doubleClass, doubleCtor, result.sample_peak);
    jobject integratedLufsObj = env->NewObject(doubleClass, doubleCtor, result.integrated_lufs);
    jobject loudnessRangeObj = env->NewObject(doubleClass, doubleCtor, result.loudness_range);

    env->SetObjectField(ret, f1, truePeakObj);
    env->SetObjectField(ret, f2, samplepeakObj);
    env->SetObjectField(ret, f3, integratedLufsObj);
    env->SetObjectField(ret, f4, loudnessRangeObj);

    fid = env->GetFieldID(vibebur128Class, "status", "I");
    env->SetIntField(ret, fid, 0);
    return ret;
}


jobject toJstring(JNIEnv *env, const char *str) {
    if (str == nullptr) {
        return nullptr;
    }

    size_t len = std::strlen(str);
    jbyteArray byteArray = env->NewByteArray(static_cast<jsize>(len));
    env->SetByteArrayRegion(byteArray, 0, static_cast<jsize>(len), reinterpret_cast<const jbyte *>(str));

    jclass stringClass = env->FindClass("java/lang/String");
    jmethodID ctor = env->GetMethodID(stringClass, "<init>", "([BLjava/lang/String;)V");

    jstring charsetName = env->NewStringUTF("UTF-8");
    jstring result = static_cast<jstring>(env->NewObject(stringClass, ctor, byteArray, charsetName));

    env->DeleteLocalRef(byteArray);
    env->DeleteLocalRef(charsetName);
    env->DeleteLocalRef(stringClass);

    return result;
}

// from taglib https://github.com/Kyant0/taglib/blob/57d6fe6effdf759618a50d5da0b32a0f52bef1bc/src/main/cpp/utils.h
char *getRealPathFromFd(const int fd) {
    char path[22];
    if (snprintf(path, sizeof(path), "/proc/self/fd/%d", fd) < 0) {
        return nullptr;
    }

    size_t size = 128;
    char *link = reinterpret_cast<char *>(malloc(size));

    ssize_t bytesRead;
    while ((bytesRead = readlink(path, link, size)) == static_cast<ssize_t>(size)) {
        size *= 2;
        char *temp = reinterpret_cast<char *>(realloc(link, size));
        if (temp == nullptr) {
            free(link);
            return nullptr;
        }
        link = temp;
    }

    link[bytesRead] = '\0';

    return link;
}
