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

package wah.mikooomich.ffMetadataEx

import android.app.Activity
import android.content.Intent

import android.widget.Toast

class Extractor : Activity() {
	// load advanced scanner libs
	init {
		System.loadLibrary("avcodec")
		System.loadLibrary("avdevice")
		System.loadLibrary("ffmetaexjni")
		System.loadLibrary("avfilter")
		System.loadLibrary("avformat")
		System.loadLibrary("avutil")
		System.loadLibrary("swresample")
		System.loadLibrary("swscale")
	}

	override fun onStart() {
		super.onStart()

		val path = intent.getStringExtra("filePath")
		if (path.isNullOrEmpty()) {
			Toast.makeText(this, "Invalid file path provided", Toast.LENGTH_SHORT).show()
			setResult(RESULT_CANCELED)
			finish()
			return
		}

		try {
			val result = FFMpegWrapper().getFullAudioMetadata(path)
			val intent = Intent().apply {
				putExtra("id", path)
				putExtra("rawExtractorData", result) // Pass raw string metadata
			}
			setResult(RESULT_OK, intent)
		} catch (e: Exception) {
			e.printStackTrace()
			setResult(RESULT_CANCELED)
		}

		finish()
	}
}
