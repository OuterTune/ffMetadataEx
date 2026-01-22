package wah.mikooomich.ffMetadataEx

class Ebur128 {
    var truePeak: Double? = null
    var loudnessIntegrated: Double? = null
    var loudnessRange: Double? = null

    /**
     * 0        ok
     *
     * 1001     file not found
     * 1002     error opening file
     * 1002     Error finding stream information
     */
    var status: Int = -1
}