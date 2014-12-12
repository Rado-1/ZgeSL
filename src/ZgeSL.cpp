/*
ZgeSL Library
Copyright (c) 2014 Radovan Cervenka

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
*/

// The main file used to compile Android shared library

// Definitions

#ifdef LOGGING
#define LOG_DEBUG(str) __android_log_write(ANDROID_LOG_DEBUG, "ZgeSL", str);
#else
#define LOG_DEBUG(str)
#endif

#define export extern "C"

#define DONE SL_RESULT_SUCCESS
#define ERROR -1

#define TRY(cmd) result = cmd; \
if(result != SL_RESULT_SUCCESS) { \
LOG_DEBUG("OpenSLES Error: " + result); \
return result; }

// Includes

#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <android/log.h>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// Globals

static AAssetManager* gAssetManager;
static bool gIsLooping = false;

static SLresult result;

// engine interfaces
static SLObjectItf gEngineObject = NULL;
static SLEngineItf gEngineEngine;

// output mix interfaces
static SLObjectItf gOutputMixObject = NULL;

// player interfaces
static SLObjectItf gPlayerObject = NULL;
static SLPlayItf gPlayerPlay;
static SLVolumeItf gPlayerVolume;
static SLSeekItf gPlayerSeek;

// Callbacks

// stop the finished player if not looping
static void playCallback(SLPlayItf caller, void *pContext, SLuint32 playevent){
LOG_DEBUG("end of playing reached");
	if(playevent == SL_PLAYEVENT_HEADATEND && !gIsLooping){
		// stop playing
		(*caller)->SetPlayState(caller, SL_PLAYSTATE_STOPPED);
		// unregister this callback
		(*caller)->RegisterCallback(caller, NULL, NULL);
	}
}

// Private functions

// destroys player and invalidate all associated interfaces
void destroyPlayer(){
	if(gPlayerObject != NULL) {
		(*gPlayerObject)->Destroy(gPlayerObject);
		gPlayerObject = NULL;
		gPlayerPlay = NULL;
		gPlayerVolume = NULL;
	}
}

// Public functions

// Used to obtain AssetManager instance
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOG_DEBUG("JNI OnLoad called");

    // get JNI environment
    JNIEnv *env;
    vm->GetEnv((void**) &env, JNI_VERSION_1_6);

    // get Zge class and its instance
    jclass zgeActivityCls = env->FindClass("org/zgameeditor/ZgeActivity");
    jfieldID zgeActivityID = env->GetStaticFieldID(zgeActivityCls, "zgeActivity",
											"Lorg/zgameeditor/ZgeActivity;");
	jobject zgeActivity = env->GetStaticObjectField(zgeActivityCls,
													zgeActivityID);
    jmethodID getAssetsID = env->GetMethodID(zgeActivityCls, "getAssets",
									"()Landroid/content/res/AssetManager;");
    jobject am = env->CallObjectMethod(zgeActivity, getAssetsID);

    // use asset manager to open asset by filename
    gAssetManager = AAssetManager_fromJava(env, am);

    return JNI_VERSION_1_6;
}

export int zslInit() {
    LOG_DEBUG("zslInit called");

    // create engine
    TRY(slCreateEngine(&gEngineObject, 0, NULL, 0, NULL, NULL));

    // realize the engine
    TRY((*gEngineObject)->Realize(gEngineObject, SL_BOOLEAN_FALSE));

    // get the engine interface in order to create other objects
    TRY((*gEngineObject)->GetInterface(gEngineObject, SL_IID_ENGINE,
                                      &gEngineEngine));

    // create output mix with volume control
    // as a non-required interface
    const SLInterfaceID ids[] = {};
    const SLboolean req[] = {};

    TRY((*gEngineEngine)->CreateOutputMix(gEngineEngine, &gOutputMixObject, 0,
                                         ids, req));

    // realize the output mix
    TRY((*gOutputMixObject)->Realize(gOutputMixObject, SL_BOOLEAN_FALSE));

	return DONE;
}

export void zslExit() {
    LOG_DEBUG("zslExit called");

	destroyPlayer();

	// destroy output mix object and invalidate all associated interfaces
	if(gOutputMixObject != NULL) {
		(*gOutputMixObject)->Destroy(gOutputMixObject);
		gOutputMixObject = NULL;
	}

	// destroy engine object and invalidate all associated interfaces
	if(gEngineObject != NULL) {
		(*gEngineObject)->Destroy(gEngineObject);
		gEngineObject = NULL;
		gEngineEngine = NULL;
	}
}

export int zslPlayFile(const char* filename, bool isAsset) {
    LOG_DEBUG("zslPlayFile called");

    destroyPlayer();

    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource audioSrc;

	if(isAsset){

		// open file from assets

		// open asset by filename
		AAsset* asset = AAssetManager_open(gAssetManager, filename, AASSET_MODE_UNKNOWN);
		if(asset == NULL) return ERROR;

		// open asset as file descriptor
		off_t start, length;
		int fd = AAsset_openFileDescriptor(asset, &start, &length);
		AAsset_close(asset);
		if(fd < 0) return ERROR;

		// configure audio source
		SLDataLocator_AndroidFD loc_fd = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
		audioSrc = {&loc_fd, &format_mime};

	} else {

		// open file from URI, including SD card

		// configure audio source
		// (requires the INTERNET permission depending on the uri parameter)
		SLDataLocator_URI loc_uri = {SL_DATALOCATOR_URI, (SLchar *)filename};
		audioSrc = {&loc_uri, &format_mime};
	}

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, gOutputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[] = {SL_IID_SEEK, SL_IID_VOLUME};
    const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    TRY((*gEngineEngine)->CreateAudioPlayer(gEngineEngine, &gPlayerObject,
											&audioSrc, &audioSnk, 2, ids, req));

    // realize the player
    TRY((*gPlayerObject)->Realize(gPlayerObject, SL_BOOLEAN_FALSE));

	// get the play interface
	TRY((*gPlayerObject)->GetInterface(gPlayerObject, SL_IID_PLAY,
										&gPlayerPlay));

	// register play callback for SL_PLAYEVENT_HEADATEND
	TRY((*gPlayerPlay)->RegisterCallback(gPlayerPlay, playCallback, NULL));
	TRY((*gPlayerPlay)->SetCallbackEventsMask(gPlayerPlay, SL_PLAYEVENT_HEADATEND));

	// get the volume interface
	TRY((*gPlayerObject)->GetInterface(gPlayerObject, SL_IID_VOLUME,
										&gPlayerVolume));

	// enable setting stereo position (panning)
	TRY((*gPlayerVolume)->EnableStereoPosition(gPlayerVolume, SL_BOOLEAN_TRUE));

	// get the seek interface
	TRY((*gPlayerObject)->GetInterface(gPlayerObject, SL_IID_SEEK,
										&gPlayerSeek));

	// set the player's state to playing
	TRY((*gPlayerPlay)->SetPlayState(gPlayerPlay, SL_PLAYSTATE_PLAYING));

	gIsLooping = false;
	return DONE;
}

export int zslPause() {
    LOG_DEBUG("zslPause called");

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->SetPlayState(gPlayerPlay, SL_PLAYSTATE_PAUSED));
		return DONE;
	} else
		return ERROR;
}

export int zslResume() {
    LOG_DEBUG("zslResume called");

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->SetPlayState(gPlayerPlay, SL_PLAYSTATE_PLAYING));
		return DONE;
	} else
		return ERROR;
}

export int zslStop() {
    LOG_DEBUG("zslStop called");

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->SetPlayState(gPlayerPlay, SL_PLAYSTATE_STOPPED));
		return DONE;
	} else
		return ERROR;
}

export int zslSetLooping(bool isLooping) {
    LOG_DEBUG("zslSetLooping called");

	if(gPlayerSeek != NULL) {
		TRY((*gPlayerSeek)->SetLoop(gPlayerSeek, isLooping ?
									SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE,
									0, SL_TIME_UNKNOWN));
		gIsLooping = isLooping;
		return DONE;
	} else
		return ERROR;
}

export int zslGetPlayState() {
	LOG_DEBUG("zslGetPlayState called");

	SLuint32 pst;

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->GetPlayState(gPlayerPlay, &pst));
		return pst;
	} else
		return ERROR;
}

export int zslGetDuration() {
	LOG_DEBUG("zslGetDuration called");

	SLmillisecond dur;

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->GetDuration(gPlayerPlay, &dur));
		return dur;
	} else
		return ERROR;
}

export int zslSetPosition(int position, int mode) {
    LOG_DEBUG("zslSetPosition called");

	if(gPlayerSeek != NULL) {
		TRY((*gPlayerSeek)->SetPosition(gPlayerSeek, position, mode));
		return DONE;
	} else
		return ERROR;
}

export int zslGetPosition() {
	LOG_DEBUG("zslGetPosition called");

	SLmicrosecond pos;

	if(gPlayerPlay != NULL) {
		TRY((*gPlayerPlay)->GetPosition(gPlayerPlay, &pos));
		return pos;
	} else
		return ERROR;
}

export int zslSetVolume(int volume) {
	LOG_DEBUG("zslSetVolume called");

	if(gPlayerVolume != NULL) {

		TRY((*gPlayerVolume)->SetVolumeLevel(gPlayerVolume,
			volume < 10 ? SL_MILLIBEL_MIN : 2000 * log10( volume / 1000.0 )));
		return DONE;
	} else
		return ERROR;
}

export int zslSetMute(bool isMuted) {
	LOG_DEBUG("zslSetMute called");

	if(gPlayerVolume != NULL) {
		TRY((*gPlayerVolume)->SetMute(gPlayerVolume, isMuted));
		return DONE;
	} else
		return ERROR;
}

export int zslSetPanning(int panning) {
	LOG_DEBUG("zslSetPanning called");

	if(gPlayerVolume != NULL) {
		TRY((*gPlayerVolume)->EnableStereoPosition(gPlayerVolume, SL_BOOLEAN_TRUE));
		return DONE;
	} else
		return ERROR;
}