package wah.mikooomich.ffMetadataEx

class Vibebur128 {
    var truePeak: Double? = null
    var samplePeak: Double? = null
    var integratedLufs: Double? = null
    var loudnessRange: Double? = null

    /**
     * 0            ok
     * negative 1s  file issues
     * negative 10s tragic ffmpeg issues
     * 1000s        ebur / filter issues
     */
    var status: Int = -1
    var debug: String? = null
}