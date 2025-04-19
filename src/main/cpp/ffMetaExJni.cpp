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
#include <vector>

jobject toJstring(JNIEnv *pEnv, const char *album);

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}


extern "C" JNIEXPORT jobject JNICALL
Java_wah_mikooomich_ffMetadataEx_FFMpegWrapper_getFullAudioMetadata(JNIEnv *env, jobject obj,
                                                                    jstring filePath) {

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
    const char *file_path = env->GetStringUTFChars(filePath, nullptr);
    if (!file_path) {
        fid = env->GetFieldID(metadataClass, "status", "I");
        env->SetIntField(ret, fid, 1001);
        return ret;
    }

    AVFormatContext *format_context = nullptr;
    if (avformat_open_input(&format_context, file_path, nullptr, nullptr) != 0) {
        env->ReleaseStringUTFChars(filePath, file_path);
        fid = env->GetFieldID(metadataClass, "status", "I");
        env->SetIntField(ret, fid, 1002);
        return ret;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        avformat_close_input(&format_context);
        env->ReleaseStringUTFChars(filePath, file_path);
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
    env->ReleaseStringUTFChars(filePath, file_path);

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
