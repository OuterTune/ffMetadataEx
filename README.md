# ffMetadataEx

Module that contains the FFmpeg libraries required for [OuterTune](https://github.com/OuterTune/OuterTune) FFmpeg tag
extractor

## Usage

1. Install an OuterTune version with ffMetadataEx

2. Select this scanner implementation. Open the OuterTune app and navigate to
   `Settings --> Library & Content -> Local media -> Metadata extractor`, and select FFmpeg.

## Developer use

Metadata is accessible through the AudioMetadata class

### Code example

Extract metadata

```kotlin
val ffmpeg = FFMpegWrapper()
val data: AudioMetadata? = ffmpeg.getFullAudioMetadata("file path of audio file")

// get metadata tags
var title: String? = data.title
var artists: String? = data.artist

// get audio information
var codec: String? = data.codec
var type: String? = data.codecType
var sampleRate: Int = data.sampleRate
var duration: Long = data.duration // duration is in milliseconds
```

For any additional tag information that does not have an associated field, they can be accessible via `extrasRaw` field

```kotlin
// Array of strings, in format of: <key>:<value>
data.extrasRaw.forEach {
    val tag = it.substringBefore(':')
    when (tag) {
        // read date
        "DATE", "date" -> date = it.substringAfter(':')
    }
}
```

## Building

1. First you will need to setup the [Android NDK](https://developer.android.com/studio/projects/install-ndk)

2. Import this module into your app project:

    - Building for OuterTune: Additional OuterTune patches are needed for scanner functionality, see this branch for
      more details, and as reference: https://github.com/OuterTune/OuterTune/tree/feature/ffMetadataEx. This configures
      ffMetadataEx as a git submodule. This branch will be updated from time to time.
    - For other projects: Import the module and its code into your project manually, then setup Proguard correctly.

3. FFmpeg is used to extract metadata from local files. Those binaries must be resolved in one of two ways:

    - a) Build libraries yourself. Clone [ffmpeg-android-maker](https://github.com/Javernaut/ffmpeg-android-maker) into
      `<project root>/ffmpeg-android-maker`, run the build script. Note: It may be helpful to modify the
      FFmpeg build script disable unneeded FFmpeg features to reduce app size,
      see [here](https://github.com/mikooomich/ffmpeg-android-maker/blob/master/scripts/ffmpeg/build.sh) for an example.

    - b) Use prebuilt FFmpeg libraries.
      Clone [prebuilt ffmpeg-android-maker](https://github.com/mikooomich/ffmpeg-android-maker-prebuilt) into
      `<project root>/ffmpeg-android-maker`.

4. Gradle sync, then start the build as you normally would.
