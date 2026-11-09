# Keep pinned upstream submodule unchanged; patch its wrapper in the build tree.
file(READ "${PROJECT_SOURCE_DIR}/modules/CPLUG/src/cplug_auv2.c" CUSTOM_AU_SOURCE)
string(REPLACE "cplug_getLatencyInSamples(auv2->userPlugin);" "cplug_getLatencyInSamples(auv2->userPlugin) / auv2->sampleRate;" CUSTOM_AU_SOURCE "${CUSTOM_AU_SOURCE}")
string(REPLACE "cplug_getTailInSamples(auv2->userPlugin);" "cplug_getTailInSamples(auv2->userPlugin) / auv2->sampleRate;" CUSTOM_AU_SOURCE "${CUSTOM_AU_SOURCE}")
string(REPLACE "// TODO: support this?" "extern void scream_custom_reset(void*);\n    scream_custom_reset(auv2->userPlugin);" CUSTOM_AU_SOURCE "${CUSTOM_AU_SOURCE}")
# Ensure the default sample rate is prepared even if host doesn't change it.
string(REPLACE "cplug_log(\"AUMethodInitializeProcessing\");" "cplug_log(\"AUMethodInitializeProcessing\");\n    cplug_setSampleRateAndBlockSize(auv2->userPlugin, auv2->sampleRate, auv2->mMaxFramesPerSlice);" CUSTOM_AU_SOURCE "${CUSTOM_AU_SOURCE}")
set(CUSTOM_AU_PATH "${CMAKE_CURRENT_BINARY_DIR}/cplug_auv2_custom.c")
file(WRITE "${CUSTOM_AU_PATH}" "${CUSTOM_AU_SOURCE}")
# GetProperty must return bytes actually written, not caller capacity.
string(REPLACE "paramList[i] = cplug_getParameterID(auv2->userPlugin, i);" "paramList[i] = cplug_getParameterID(auv2->userPlugin, i);\n        *ioDataSize = numParams * sizeof(AudioUnitParameterID);" CUSTOM_AU_SOURCE "${CUSTOM_AU_SOURCE}")
file(WRITE "${CUSTOM_AU_PATH}" "${CUSTOM_AU_SOURCE}")
