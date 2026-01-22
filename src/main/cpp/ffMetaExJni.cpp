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
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>


#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

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

// helper: parse leading double from a string like "-14.6 LUFS" or "-0.75 dB"
static double parse_leading_double(const char* s) {
    if (!s) return 0.0;
    // strtod will parse prefix double and stop at first non-number char
    char *endptr = nullptr;
    double v = strtod(s, &endptr);
    return v;
}

// helper: starts-with
static bool starts_with(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

extern "C" JNIEXPORT jobject JNICALL
Java_wah_mikooomich_ffMetadataEx_FFmpegWrapper_getEbur128(JNIEnv *env, jobject obj, jint fd) {
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "bro ");

    // create jobject

    jclass ebur128Class = env->FindClass("wah/mikooomich/ffMetadataEx/Ebur128");
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "can ");
    if (ebur128Class == nullptr) {
        return nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "you ");
    jobject ret = env->NewObject(ebur128Class, env->GetMethodID(ebur128Class, "<init>", "()V"));
    if (ret == nullptr) {
        return nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "fucking ");
    jfieldID fid;

    // extract from file
    const char *file_path = getRealPathFromFd(fd);
    if (!file_path) {
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 1001);
        return ret;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "do ");
    // Initialize FFmpeg (safe to call multiple times)
//    av_register_all();
//    avfilter_register_all();
//    avcodec_register_all();

    AVFormatContext *format_context = nullptr;
    if (avformat_open_input(&format_context, file_path, nullptr, nullptr) != 0) {
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 2);
        return ret;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 3);
        return ret;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "someting ");
    int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 4);
        return ret;
    }

    AVStream *audio_stream = format_context->streams[audio_stream_index];
    const AVCodec *decoder = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!decoder) {
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 5);
        return ret;
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(decoder);
    if (!dec_ctx) {
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 6);
        return ret;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "cock ");
    if (avcodec_parameters_to_context(dec_ctx, audio_stream->codecpar) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 7);
        return ret;
    }

    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 8);
        return ret;
    }

    // Setup filter graph: abuffer -> ebur128 -> abuffersink
    char args[1024];
    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 9);
        return ret;
    }

    const AVFilter *abuffer = avfilter_get_by_name("abuffer");
    const AVFilter *ebur128 = avfilter_get_by_name("ebur128");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffer || !ebur128 || !abuffersink) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 10);
        return ret;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "why ");
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterContext *ebur128_ctx = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;

    // Prepare abuffer arguments: sample rate, sample format, channel layout
    // get sample fmt name
    const char *sample_fmt_name = av_get_sample_fmt_name(dec_ctx->sample_fmt);
    if (!sample_fmt_name) {
        // fallback to flt
        sample_fmt_name = "flt";
    }
    uint64_t ch_layout = dec_ctx->ch_layout.nb_channels;

    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%d",
             dec_ctx->time_base.num, dec_ctx->time_base.den,
             dec_ctx->sample_rate,
             sample_fmt_name,
             dec_ctx->ch_layout.nb_channels);

    if (avfilter_graph_create_filter(&buffersrc_ctx, abuffer, "in", args, nullptr, filter_graph) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 11);
        return ret;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "aaaaaaaaaaaaa ");
    // Create ebur128 filter; no special options needed normally.
    // Some builds accept "peak=true" but default ebur128 already computes true peaks; leave options empty.
    if (avfilter_graph_create_filter(&ebur128_ctx, ebur128, "ebur", "metadata=1:peak=true", nullptr, filter_graph) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 12);
        return ret;
    }

    // Create buffersink
    if (avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", nullptr, nullptr, filter_graph) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 13);
        return ret;
    }

    // Link filters: buffersrc -> ebur128 -> buffersink
    if (avfilter_link(buffersrc_ctx, 0, ebur128_ctx, 0) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 14);
        return ret;
    }
    if (avfilter_link(ebur128_ctx, 0, buffersink_ctx, 0) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 15);
        return ret;
    }

    // Configure graph
    if (avfilter_graph_config(filter_graph, nullptr) < 0) {
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 16);
        return ret;
    }

    // Prepare decoding loop
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "huh ");
    if (!pkt || !frame || !filt_frame) {
        av_packet_free(&pkt);
        av_frame_free(&frame);
        av_frame_free(&filt_frame);
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&format_context);
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 17);
        return ret;
    }

    // Variables to store results (set defaults)
    double integrated = 0.0;
    double lra = 0.0;
    double max_true_peak = 0.0;
    bool got_integrated = false;
    bool got_lra = false;
    bool got_true_peak = false;
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "so when ");
    // Read packets and decode
    while (av_read_frame(format_context, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_index) {
            if (avcodec_send_packet(dec_ctx, pkt) < 0) {
                av_packet_unref(pkt);
                continue;
            }
            while (avcodec_receive_frame(dec_ctx, frame) == 0) {
                // push decoded frame into filtergraph
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    // push failed, continue
                }

                // Pull available filtered frames (ebur128 produces metadata frames intermittently)
                while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0) {
                    // Iterate metadata keys
                    AVDictionary *m = filt_frame->metadata;
                    AVDictionaryEntry *t = nullptr;
                    while ((t = av_dict_get(m, "", t, AV_DICT_IGNORE_SUFFIX))) {
                        const char* key = t->key;
                        const char* val = t->value;
                        if (!key || !val) continue;

                        // Integrated loudness
                        if (strcmp(key, "lavfi.r128.I") == 0) {
                            integrated = parse_leading_double(val); // usually in LUFS (negative)
                            got_integrated = true;
                        }
                            // Loudness range
                        else if (strcmp(key, "lavfi.r128.LRA") == 0) {
                            // some implementations give one value, others give two (low/high). We'll parse the first number.
                            lra = parse_leading_double(val);
                            got_lra = true;
                        }
                            // True peaks per channel: keys may be lavfi.r128.true_peaks_ch0 or lavfi.r128.true_peaks_ch_0
                        else if (starts_with(key, "lavfi.r128.true_peaks_ch")
                                 || starts_with(key, "lavfi.r128.true_peaks_ch_")) {
                            // value is like "-0.32 dBTP"
                            double tp = parse_leading_double(val);
                            if (!got_true_peak || tp > max_true_peak) {
                                max_true_peak = tp;
                            }
                            got_true_peak = true;
                        }
                    } // end iter metadata

                    av_frame_unref(filt_frame);
                } // end while get filtered frames

                av_frame_unref(frame);
            } // end receive_frame
        } // end if audio packet

        av_packet_unref(pkt);
    } // end read_frame
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "do you die ");
    // signal EOF to filters
    av_buffersrc_add_frame_flags(buffersrc_ctx, nullptr, 0);

    // drain filtered frames after EOF
    while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0) {
        AVDictionary *m = filt_frame->metadata;
        AVDictionaryEntry *t = nullptr;
        while ((t = av_dict_get(m, "", t, AV_DICT_IGNORE_SUFFIX))) {
            const char* key = t->key;
            const char* val = t->value;
            if (!key || !val) continue;

            if (strcmp(key, "lavfi.r128.I") == 0) {
                integrated = parse_leading_double(val);
                got_integrated = true;
            } else if (strcmp(key, "lavfi.r128.LRA") == 0) {
                lra = parse_leading_double(val);
                got_lra = true;
            } else if (starts_with(key, "lavfi.r128.true_peaks_ch")
                       || starts_with(key, "lavfi.r128.true_peaks_ch_")) {
                double tp = parse_leading_double(val);
                if (!got_true_peak || tp > max_true_peak) {
                    max_true_peak = tp;
                }
                got_true_peak = true;
            }
        }
        av_frame_unref(filt_frame);
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "asss ");
    // cleanup ffmpeg resources
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&format_context);

    // If values not present, set NaN or null equivalents. We'll put 0.0 if missing.
    double truePeak = got_true_peak ? max_true_peak : 0.0;
    double integratedVal = got_integrated ? integrated : 0.0;
    double lraVal = got_lra ? lra : 0.0;

    // --- Build Java Double wrappers and set fields ---
    jclass doubleClass = env->FindClass("java/lang/Double");
    jmethodID doubleCtor = env->GetMethodID(doubleClass, "<init>", "(D)V");
    if (!doubleCtor) {
        return ret; // constructor missing, return empty object
    }
    __android_log_print(ANDROID_LOG_DEBUG, "WTFBRO", "%s", "cock ");
    jfieldID f1 = env->GetFieldID(ebur128Class, "truePeak", "Ljava/lang/Double;");
    jfieldID f2 = env->GetFieldID(ebur128Class, "loudnessIntegrated", "Ljava/lang/Double;");
    jfieldID f3 = env->GetFieldID(ebur128Class, "loudnessRange", "Ljava/lang/Double;");
    if (!f1 || !f2 || !f3) {
        fid = env->GetFieldID(ebur128Class, "status", "I");
        env->SetIntField(ret, fid, 20);
        return ret;
    }

    jobject tpObj = env->NewObject(doubleClass, doubleCtor, truePeak);
    jobject liObj = env->NewObject(doubleClass, doubleCtor, integratedVal);
    jobject lraObj = env->NewObject(doubleClass, doubleCtor, lraVal);

    env->SetObjectField(ret, f1, tpObj);
    env->SetObjectField(ret, f2, liObj);
    env->SetObjectField(ret, f3, lraObj);

    fid = env->GetFieldID(ebur128Class, "status", "I");
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

