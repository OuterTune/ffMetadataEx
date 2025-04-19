package wah.mikooomich.ffMetadataEx

class AudioMetadata {
    var title: String? = null
    var artist: String? = null
    var album: String? = null
    var genre: String? = null
    var codec: String? = null
    var codecType: String? = null
    var bitrate: Long = 0L
    var sampleRate: Int = 0
    var channels: Int = 0
    var duration: Long = 0L
    var extrasRaw: Array<String> = emptyArray<String>()

    /**
     * 0        ok
     *
     * 1001     file not found
     * 1002     error opening file
     * 1002     Error finding stream information
     */
    var status: Int = 0
}