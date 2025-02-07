# ffMetadataEx

Standalone extra app plugin for [OuterTune](https://github.com/DD3Boh/OuterTune)

## Usage

ffMetadataEx is an optional standalone plugin that contains the FFmpeg libraries required for OuterTine's FFmpeg tag
extractor.

1. Install ffMetadataEx. [Download](https://github.com/mikooomich/ffMetadataEx/releases) one that is compatible with
   your OuterTune version.

2. Grant ffMetadataEx either permission: storage or "Music and audio". Both OuterTune AND ffMetadataEx require this
   permission.

3. Select this scanner implementation. To configure which scanner implementation OuterTune will use, open the OuterTune
   app and navigate to "settings --> Local media -> Metadata extractor". Then select the desired extractor.

## Developer use

At it's core, this extracts all the (text) tag data from audio files, and returns it in a plain text string, all in a
somewhat hacky way. You are probably better using a more elegant solution. While this is built for OuterTune in mind,
any app should be able to use this plugin.

## Building

1. First you will need to setup the [Android NDK](https://developer.android.com/studio/projects/install-ndk)

2. We use FFmpeg to extract metadata from local files. The FFmpeg binaries must be resolved in one of two ways:

	- a) Build libraries yourself. Clone [ffmpeg-android-maker](https://github.com/Javernaut/ffmpeg-android-maker) into
	  `/app/src/main/cpp/ffmpeg-android-maker`, run the build script. Note: It may be helpful to modify the
	  FFmpeg build script disable unneeded FFmpeg features to reduce app size,
	  see [here](https://github.com/mikooomich/ffmpeg-android-maker/blob/master/scripts/ffmpeg/build.sh) for an example.

	- b) Use prebuilt FFmpeg libraries.
	  Clone [prebuilt ffmpeg-android-maker](https://github.com/mikooomich/ffmpeg-android-maker-prebuilt) into
	  `/app/src/main/cpp/ffmpeg-android-maker`.

3. Start the build are you normally would.
